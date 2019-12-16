#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <semaphore.h>
#include <shared_mutex>
#include <sstream>
#include <sys/syscall.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#include "buffer/RingQueue.hpp"
#include "chrono/Chrono.hpp"
#include "misc/Exceptions.hpp"
#include "misc/MiscTools.hpp"
#include "leonlog.hpp"
#include "Version.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;
using namespace std::this_thread;
using namespace leon_ext;

using std::atomic_bool;
using std::make_unique;
using std::map;
using std::ofstream;
using std::ostringstream;
using std::shared_lock;
using std::shared_mutex;
using std::string;
using std::thread;
using std::unique_lock;
using std::unique_ptr;

namespace leon_log {

LogLevel_e g_ellLogLevel = LogLevel_e::ellDebug;

// 日志时间,用于日志时戳
using LogTimePoint_T = time_point<system_clock, microseconds>;

// LogEntry: 定义一条日志记录所具有的基本内容
struct LogEntry_T {
   // 日志产生时间
   SysTimeNS_t m_tpStamp;

   // 日志内容
   string      m_strBody;

   // 日志级别
   LogLevel_e  m_enmLevel;
};

// 返回16进制串表达的线程Id
inline string thrdIdInHex(
   thread::id thiMyId = std::this_thread::get_id() ) {

   ostringstream oss;
   oss << std::hex << thiMyId;
   return oss.str();
};

extern "C" const char* getVersion() {
   return PROJECT_VERSION;
};

/* FileLogger_T:
 * 本对象其实是一个std::map<ThdId, LogQue>的Wrapper, 主要解决线程安全问题.
 * 即:
 *    1.当有线程向本容器内插入新队列时, 其它线程不能自本对象检索;
 *    2.同理, 当有线程自本对象检索时, 其它线程不能向本对象插入；
 *    3.任意多个线程可并发地自本对象检索.
 */
class LogQue_t;   // 日志队列，此处只是前向引用
class FileLogger_t {
//=======   对外接口    =======
public:
   FileLogger_t();
   ~FileLogger_t();
   // 启动日志系统, 指定日志文件名
   void open( const string&   crp_strLogFile,
              LogLevel_e      p_enmLogLevel,
              size_t          p_uiStampPrecesion,
              size_t          p_uiLogQueCapa );
   // 关闭日志, 并Flush所有日志到磁盘
   void close();
   // 轮转日志文件名
   void rotateLog( const string& crp_strInfix );
   // 可选函数, 登记一个线程名, 此后遇到该线程的日志, 输出中会包含此名，而非线程Id
   void registThrdName( const string& crp_strName );

   // 添加日志的主函数, 其实此函数只是把日志加入队列, 等待日志线程来写入文件
   void append( LogLevel_e p_enmLevel, string&& rrp_strBody );

   // 设置每个日志队列的容量
   const size_t& LogQueCapa = m_uiLogQueCapa;
   inline void setLogQueCapa( const size_t& crp_uiCapa ) {
      m_uiLogQueCapa = crp_uiCapa;
   };

private:
//=======   内部实现    =======
   void addLogQue( LogQue_t* p_pLogQue,
                   const thread::id& crp_thdId,
                   pid_t p_pidOsThreadId );
   void delLogQue( const thread::id& crp_thdId,
                   const string& crp_strThreadName );

   // 从所有线程的日志队列中取出最早的那条日志
   bool pickOneLog( LogEntry_T& rp_log,
                    const string*& rp_pstrThdName );
   // 输出一条日志
   void writeLog( ofstream&         p_out,
                  const LogEntry_T& crp_log,
                  const string&     crp_strThreadName );

   // logger线程体
   void threadBody();
   // semaphore事件循环
   void semLoop();

//=======   内部变量    =======
   // 写日志的线程
   thread   m_thdLogger;
   // 用于其它线程通知日志线程"新日志已入队"的信号量
   sem_t    m_semNewLog;

   // 所有日志队列的容器(用线程Id找出一个队列)
   map<thread::id, LogQue_t*> m_mapLogQues;
   // map<string, pid_t> m_mapOsThreadIds;
   // m_mtxToken用作协调对 m_mapLogQues 对象的访问
   shared_mutex m_mtxToken;

   // 单个日志队列的容量
   size_t   m_uiLogQueCapa = DEFAULT_LOG_QUE_SIZE;

   // 时戳精度(0~9代表精确到秒的几位小数)
   size_t   m_uiStampPrecision = 6;
   // 每次
   uint64_t m_uiStampUnitBase;

   // 日志文件名, 包含全路径
   string   m_strLogFile;
   // 日志文件名中缀, 用于日志文件轮转
   string   m_strLogInfix;
   // 是否正在进行日志文件轮转
   bool     m_bRotating = false;

   // (主线程)指示logger线程是否还应继续运行的标志
   // 若将其置false, 日志线程将清空日志队列后退出
   atomic_bool m_abShouldRun = { false };

   // 日志系统正在运行标志, 避免重复启动
   atomic_bool m_abRunning = { false };

   // 允许输出的最低日志级别
// LogLevel_e m_enmLogLevel;

   friend class LogQue_t;
};

// 全系统应该只有一个logger
FileLogger_t s_fileLogger;

/* LogQue_t: 单个日志队列的类型(每个队列服务一个用户线程)
 * 本对象其实是个Wrapper, 包装了一个 RingQueue<LogEntry_T>, 加上几个附属属性.
 * 即: 线程Id、线程名称.
 * 因为每个日志队列只服务一个用户线程, 本对象依然无须考虑线程安全的问题
 */
class LogQue_t : public leon_ext::RingQueue_t<LogEntry_T> {
//=======   对外接口    =======
public:
   // 只能显式构造
   explicit LogQue_t( const size_t           p_uiSize,
                      const thread::id& crp_thdId,
                      const string&     crp_strName,
                      pid_t p_pidThreadIdInOs )
      : RingQueue_t<LogEntry_T>( p_uiSize )
      , m_thiProductorId( crp_thdId )
      , m_strProductorName( crp_strName ) {

      s_fileLogger.addLogQue( this, m_thiProductorId, p_pidThreadIdInOs );
   };
   ~LogQue_t() {
      // 必须要等日志写完, 也就是本队列已空再释放, 才不会丢日志, 也不会导致Seg Fault
      uint64_t uiLoops = 0;
      while ( hasData() ) {
         ++ uiLoops;
         sem_post( &( s_fileLogger.m_semNewLog ) );
         std::this_thread::sleep_for( 1us );
      }

#ifdef DEBUG
      if ( uiLoops > 0 )
         std::cerr << m_strProductorName
                   << " 等待日志队列清空" << uiLoops << "次"
                   << std::endl;
#endif

      s_fileLogger.delLogQue( m_thiProductorId, m_strProductorName );
   };

   // 设置我们所属(所服务)的线程名
   inline void setProductorName( const string& crp_strName ) {
      m_strProductorName = crp_strName;
   };
   const string& ProductorName = m_strProductorName;

//=======   内部变量    =======
private:
   // 本队列服务于哪个线程(生产者)
   thread::id m_thiProductorId;
   // 那个线程(生产者)的名称
   string m_strProductorName;
};

map<string, pid_t> g_mapOsThreadIds;

thread_local unique_ptr<LogQue_t> tl_upqMyLogQue = nullptr;

extern "C" void startLogging( const string& crp_strLogFile,
                              LogLevel_e    p_enmLogLevel,
                              uint64_t      p_uiStampPrecision,
                              size_t        p_uiLogQueSize ) {
//    LogLevel_e* pLL = (LogLevel_e*)&g_ellLogLevel;
//    std::cout << "leon_log::pLL:" << pLL << std::endl;
//    *pLL = p_enmLogLevel;
//    const_cast<LogLevel_e&>( g_ellLogLevel ) = p_enmLogLevel;
   s_fileLogger.open( crp_strLogFile,
                      p_enmLogLevel, p_uiStampPrecision, p_uiLogQueSize );
};

extern "C" void stopLogging() {
   s_fileLogger.close();
};

extern "C" void rotateLog( const string& crp_strInfix ) {
   s_fileLogger.rotateLog( crp_strInfix );
};

extern "C" void registThrdName( const string& crp_strName ) {
   s_fileLogger.registThrdName( crp_strName );
};

extern "C" bool appendLog( LogLevel_e p_enmLevel, string&& rrp_strBody ) {
   // 只有不低于门限值的日志才能得到输出
   if ( p_enmLevel < g_ellLogLevel )
      return false;

   s_fileLogger.append( p_enmLevel, std::move( rrp_strBody ) );
   return true;
};

FileLogger_t::FileLogger_t() {
   if ( sem_init( &m_semNewLog, 0, 0 ) )
      throw std::runtime_error( "信号量创建失败, 不能启动日志系统!" );
};

FileLogger_t::~FileLogger_t() {
   if ( sem_destroy( &m_semNewLog ) )
      // 析构函数内不能抛出异常，只能输出到StdErr
      std::cerr << "信号量销毁失败!" << std::endl;
};

void FileLogger_t::open( const string& crp_strLogFile,
                         LogLevel_e    p_enmLogLevel,
                         size_t        p_uiStampPrecesion,
                         size_t        p_uiLogQueCapa ) {
   if ( m_abRunning )
      throw bad_usage( "日志系统已启动, 不能重复初始化!" );

   m_uiLogQueCapa = p_uiLogQueCapa;
   m_uiStampPrecision =
      std::min<decltype( m_uiStampPrecision )>( p_uiStampPrecesion, 9 );
   m_uiStampUnitBase  = std::pow( 10.0, 9 - m_uiStampPrecision );

   const_cast<LogLevel_e&>( g_ellLogLevel ) = std::min(
            LogLevel_e::ellFatal,
            std::max( LogLevel_e::ellDebug, p_enmLogLevel ) );

   m_strLogFile = crp_strLogFile;
   registThrdName( "主线程" );
   m_abShouldRun = true;
   asm volatile( "mfence" ::: "memory" );
   m_thdLogger = std::thread( &FileLogger_t::threadBody, this );

   // 走慢点, 确保日志线程已经开始监听信号量
   // 简单等待一段时间, 省得又要 条件变量 + 锁机制
   steady_clock::time_point tpTimeout = steady_clock::now() + 2s;
   while ( !m_abRunning && steady_clock::now() < tpTimeout )
      std::this_thread::sleep_for( 1ns );

   // 如果日志线程启动成功, m_abRunning应该已置位了
   asm volatile( "mfence" ::: "memory" );
   if ( !m_abRunning ) {
      m_abShouldRun = false;
      m_thdLogger.detach();
      throw std::runtime_error( "日志系统启动失败" );
   }
};

// 关闭日志, 并Flush所有日志到磁盘
void FileLogger_t::close() {
   if ( ! m_abRunning )
      throw bad_usage( "日志系统尚未启动, 怎么关闭?" );

   LOG_NOTIF( "将要停止日志系统......" );
   m_abShouldRun = false;
   asm volatile( "mfence" ::: "memory" );

   sem_post( & m_semNewLog );
   steady_clock::time_point tpTimeout = steady_clock::now() + 10s;
   while ( m_abRunning && steady_clock::now() < tpTimeout )
      std::this_thread::sleep_for( 1ns );

   asm volatile( "mfence" ::: "memory" );
   if ( m_abRunning ) {
      m_thdLogger.detach();
      throw std::runtime_error( "日志系统停止失败" );
   } else {
      m_thdLogger.join();
   }
};

void FileLogger_t::rotateLog( const string& crp_strInfix ) {
   // 先确保日志线程真的进入事件循环,否则它首次进入事件循环就会去轮转日志
//    std::this_thread::sleep_for( 1ms );
   m_strLogInfix = crp_strInfix;
   asm volatile( "mfence" ::: "memory" );
   m_bRotating = true;
   asm volatile( "mfence" ::: "memory" );
   sem_post( & m_semNewLog );
   asm volatile( "mfence" ::: "memory" );
   steady_clock::time_point tpTimeout = steady_clock::now() + 10s;
   while ( m_bRotating && steady_clock::now() < tpTimeout )
      std::this_thread::sleep_for( 1ns );

   if ( m_bRotating ) {
      m_thdLogger.detach();
      throw std::runtime_error( "日志系统轮转超时!" );
   }
};

// 可选函数, 登记一个线程名, 此后遇到该线程的日志, 输出中会包含此名，而非线程Id
void FileLogger_t::registThrdName( const string& crp_strName ) {
   // 如果还没有本线程日志队列， 就新建一个
   if ( tl_upqMyLogQue == nullptr )
      tl_upqMyLogQue = make_unique<LogQue_t>(
                          m_uiLogQueCapa,
                          std::this_thread::get_id(), crp_strName,
                          syscall( SYS_gettid ) );
   else //已经存在，就只更新名称
      tl_upqMyLogQue->setProductorName( crp_strName );
};

// 添加日志的主函数, 此处是实现。其实此函数只是把日志加入队列, 等待日志线程来写入文件
void FileLogger_t::append( LogLevel_e p_enmLevel,
                              string&& rrp_strBody ) {
   // 日志系统必须已经启动
   if ( ! m_abRunning ) {
      // throw bad_usage( "日志系统尚未启动, 不能添加日志:" + rrp_strBody );
      std::cerr << LOG_LEVEL_NAMES[( int )p_enmLevel ]
                << ",早期日志," << rrp_strBody << "\n";
      return;
   }

   // 如果还没有本线程日志队列， 就新建一个
   if ( tl_upqMyLogQue == nullptr ) {
      thread::id thisThread = std::this_thread::get_id();
      tl_upqMyLogQue = make_unique<LogQue_t>(
                          m_uiLogQueCapa,
                          thisThread, thrdIdInHex( thisThread ),
                          syscall( SYS_gettid ) );
   }

   LogEntry_T * pLog = tl_upqMyLogQue->tail();
   if ( ! tl_upqMyLogQue->isFull() ) {
      // 正常添加本条日志,逐一赋值是为了move string对象
      pLog->m_enmLevel = p_enmLevel;
      pLog->m_tpStamp = SysTimeNS_t::clock::now();
      pLog->m_strBody = std::move( rrp_strBody );
      tl_upqMyLogQue->enque();
      sem_post( & m_semNewLog );
      return;
   }

   sem_post( & m_semNewLog );
   rrp_strBody = "日志队列已满, 抛弃日志:\'" + rrp_strBody + "\"";
   std::cerr << rrp_strBody << std::endl;

   // 日志队列已满, 目前简单方案是:"等待1us, 如果还满, 抛弃日志".
   std::this_thread::sleep_for( 1us );
   if ( tl_upqMyLogQue->isFull() ) {
      sem_post( & m_semNewLog );
      return;
   }
   pLog->m_enmLevel = LogLevel_e::ellError;
   pLog->m_tpStamp = SysTimeNS_t::clock::now();
   pLog->m_strBody = std::move( rrp_strBody );
   tl_upqMyLogQue->enque();
   sem_post( & m_semNewLog );
};

void FileLogger_t::addLogQue( LogQue_t * p_pLogQue,
                              const thread::id & crp_thdId,
                              pid_t p_pidOsThreadId ) {
   // 将队列加入到队列容器中
   // 插入应使用独占锁
   unique_lock<shared_mutex> uniquLock( m_mtxToken );
// shared_lock<shared_mutex> shareLock( m_mtxToken );
   m_mapLogQues[ crp_thdId ] = p_pLogQue;

   // 保存OS层面的线程id, 供输出status之用
   g_mapOsThreadIds[ p_pLogQue->ProductorName ] = p_pidOsThreadId;
};

void FileLogger_t::delLogQue( const thread::id & crp_thdId,
                              const string& crp_strThreadName ) {
   // 将队列从容器中移除
   // 删除应使用独占锁
   unique_lock<shared_mutex> uniquLock( m_mtxToken );
// shared_lock<shared_mutex> shareLock( m_mtxToken );
   m_mapLogQues.erase( crp_thdId );
   g_mapOsThreadIds.erase( crp_strThreadName );
};

// 从所有线程的日志队列中取出一条最早的日志
inline bool FileLogger_t::pickOneLog( LogEntry_T & rp_log,
                                      const string*& rp_pstrThdName ) {
   // 请注意, 完全有可能所有队列都是空的!所以用bFound作为标记
   bool bFound = false;    // 确实找到
   const LogEntry_T * oldestLog = nullptr;
   LogQue_t * oldestQue = nullptr;

   /* 所有操作 m_mapLogQues 的代码都应持锁完成, 因为有如下几种行为可能
    * 并发地执行:
    *  1.一个或多个线程要求插入元素
    *  2.一个或多个线程要求移除元素
    *  3.日志线程在遍历它
    */
   // 检索应持共享锁
   shared_lock<shared_mutex> shareLock( m_mtxToken );
   /* 暂时使用"死等"方案, 相关互斥量只有一个, 应该不会出现死锁.所以注释掉如下方案
   shared_lock<shared_mutex> shareLock( m_mtxToken, std::defer_lock );
   if( ! shareLock.try_lock_for( 100us ) )
       return bFound;
   */
   // 共享锁定成功, 不会有人能够修改 m_mapLogQues 了
   for ( const auto & pair : m_mapLogQues ) {
      LogQue_t * pQue = pair.second;
      if ( pQue->hasData() ) {
         const LogEntry_T * pLog = pQue->head();
         if ( !bFound || oldestLog->m_tpStamp > pLog->m_tpStamp ) {
            oldestLog = pLog;
            oldestQue = pQue;
         }
         bFound = true;
      }
   }
   // 临界资源是容器, 不是队列本身, 应尽快释放锁. 出队操作无需持锁
   shareLock.unlock();

   if ( bFound ) {
      // 为了move string对象, 只好逐个赋值!
      rp_log.m_strBody = std::move( oldestLog->m_strBody );
      rp_log.m_enmLevel = oldestLog->m_enmLevel;
      rp_log.m_tpStamp = oldestLog->m_tpStamp;
      oldestQue->deque();
      rp_pstrThdName = & oldestQue->ProductorName;
   }
   return bFound;
};

const char * const LOG_STAMP_FORMAT = "%m/%d %H:%M:%S";

inline void FileLogger_t::writeLog( ofstream & p_out,
                                    const LogEntry_T & crp_log,
                                    const string& crp_strThreadName ) {
   SysTimeNS_t tpSecPart =
      time_point_cast<SysTimeNS_t::duration>(
         std::chrono::floor<seconds>( crp_log.m_tpStamp ) );
   // 输出时戳
   p_out << formatTimeP( tpSecPart, LOG_STAMP_FORMAT );

   // 是否精确到秒以下
   if ( m_uiStampPrecision > 0 ) {
      uint64_t uiUnderSec =
         duration_cast<nanoseconds>( crp_log.m_tpStamp - tpSecPart ).count();
      uiUnderSec /= m_uiStampUnitBase;
      p_out << '.'
            << formatNumber( uiUnderSec, m_uiStampPrecision, 0, 0, '0' );
   }

   p_out
         << ',' << LOG_LEVEL_NAMES[( int )crp_log.m_enmLevel ]
         << ',' << crp_strThreadName
         << ',' << crp_log.m_strBody << "\n";

   std::cerr
         << LOG_LEVEL_NAMES[( int )crp_log.m_enmLevel ]
         << ',' << crp_strThreadName
         << ',' << crp_log.m_strBody << "\n";
};

void FileLogger_t::threadBody() {
   // 日志线程自己也有日志队列,也可以添加日志,当然就也可以注册有意义的线程名称
   registThrdName( "Logger" );
   asm volatile( "mfence" ::: "memory" );

   while ( m_abShouldRun ) {
      semLoop();
      if ( m_bRotating ) {
         std::filesystem::path   oldPath( m_strLogFile );
         std::filesystem::path   newPath =
            oldPath.parent_path() /
            ( oldPath.stem().string() + '-' + m_strLogInfix );
         newPath += oldPath.extension();
         char cSuf = 'a';
         while ( std::filesystem::exists( newPath ) ) {
            newPath = oldPath.parent_path() /
                      ( oldPath.stem().string() + '-'
                        + m_strLogInfix + cSuf++ );
            newPath += oldPath.extension();
         }
         std::filesystem::rename( oldPath, newPath );
      }
   }

   asm volatile( "mfence" ::: "memory" );
   m_abRunning = false;
};

void FileLogger_t::semLoop() {
   ofstream ofsLogFile( m_strLogFile,
                        std::ios_base::out | std::ios_base::app );
   ofsLogFile.imbue( std::locale( "C" ) );
   ofsLogFile.clear();

   LogEntry_T aLog;
   const string* thrdName;
   // 每过1秒Flush一下, 所以需要记录时间
   timespec tsNextFlush;
   timespec_get( &tsNextFlush, TIME_UTC );

   aLog.m_enmLevel = LogLevel_e::ellNotif;
   aLog.m_tpStamp = SysTimeNS_t::clock::now();
   if ( m_bRotating )
      aLog.m_strBody = "---------- 日志文件已轮转 ----------";
   else
      aLog.m_strBody = "====== leonlog-" + string( PROJECT_VERSION )
                       + " 日志已启动("
                       + string( LOG_LEVEL_NAMES[( int )g_ellLogLevel] )
                       + ") ======";
   writeLog( ofsLogFile, aLog, "Logger" );
// std::cerr << aLog.m_strBody << std::endl;
   asm volatile( "mfence" ::: "memory" );
   m_bRotating = false;

   // 主循环, 等待日志->写日志->判断是否需要轮转或退出, 周而复始...
   // m_abShouldRun 为 true 表示本线程应该继续工作, 否则就退出
   while ( m_abShouldRun && !m_bRotating ) {
      // 要真正进入主循环,才置标识位
      m_abRunning = true;

      ++( tsNextFlush.tv_sec );
      sem_timedwait( &m_semNewLog, &tsNextFlush );

      // 有日志就写日志
      while ( pickOneLog( aLog, thrdName ) )
         writeLog( ofsLogFile, aLog, *thrdName );

      // Flush一下
      ofsLogFile.flush();
   }

   if ( !m_abShouldRun ) {
      // 开始清盘. 当然, 如果日志还在源源不断地入队, 有可能导致我们停不下来哦!
      // 所以主线程应该确保所有其它线程均已停止或者至少不会再添加日志, 而后才停止我们.
      while ( pickOneLog( aLog, thrdName ) )
         writeLog( ofsLogFile, aLog, *thrdName );

      aLog.m_enmLevel = LogLevel_e::ellNotif;
      aLog.m_tpStamp = SysTimeNS_t::clock::now();
      aLog.m_strBody = "================ 日志已停止 =================";
      writeLog( ofsLogFile, aLog, "Logger" );
//    std::cerr << aLog.m_strBody << std::endl;

   } else if ( m_bRotating ) {
      aLog.m_enmLevel = LogLevel_e::ellNotif;
      aLog.m_tpStamp = SysTimeNS_t::clock::now();
      aLog.m_strBody = "---------- 日志文件将轮转 ----------";
      writeLog( ofsLogFile, aLog, "Logger" );
//    std::cerr << aLog.m_strBody << std::endl;
   }

   ofsLogFile.close();
};

}; // namespace leon_log

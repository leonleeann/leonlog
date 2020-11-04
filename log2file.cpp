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

#include "Version.hpp"
#include "buffer/CraflinRQ.tpp"
#include "chrono/Chrono.hpp"
#include "leonlog.hpp"
#include "misc/Exceptions.hpp"

using namespace leon_ext;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace std::filesystem;
using namespace std::this_thread;

using std::atomic_bool;
using std::cerr;
using std::endl;
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

//###### 各种类型 ###############################################################

// 日志时间,用于日志时戳
// using LogTimePoint_T = time_point<system_clock, microseconds>;

// LogEntry: 定义一条日志记录所具有的基本内容
struct LogEntry_t {
   // 日志产生时间
   SysTimeNS_t stamp;

   // 产生日志的线程
   string      thrd;

   // 日志内容
   string      body;

   // 日志级别
   LogLevel_e  level;
};

// LogQue_t: 日志队列(一个队列服务整个APP)
using LogQue_t = leon_ext::CraflinRQ_t<LogEntry_t>;

//###### 各种常量 ###############################################################

// constexpr auto mo_acq_rel = std::memory_order_acq_rel;
constexpr auto mo_acquire = std::memory_order_acquire;
// constexpr auto mo_consume = std::memory_order_consume;
// constexpr auto mo_relaxed = std::memory_order_relaxed;
constexpr auto mo_release = std::memory_order_release;
// constexpr auto mo_seq_cst = std::memory_order_seq_cst;

// 日志入队重试次数
constexpr unsigned int ENQUE_RETRIES = 10;
// constexpr unsigned int CHECK_INTERVL = 128;

//###### 各种函数前置申明 #########################################################

// 返回16进制串表达的线程Id
string thrdId2Hex( thread::id my_id = std::this_thread::get_id() );

// 写日志的线程体
void writerThreadBody();

// 日志线程的核心工作：出队日志，写日志
void processLogs();

// 写一条日志
void writeLog( ofstream&, const LogEntry_t& );

// 完成一次日志轮转(将当前日志文件保存、关闭、改名)
void renameLogFile();

//###### 各种变量 ###############################################################

// 日志级别
LogLevel_e  g_log_level = LogLevel_e::Debug;

// 写盘间隔(每隔多少秒确保保存一次)
decltype( timespec::tv_sec ) s_flush_secs = 3;  // 单位:秒
// 干掉日志线程之前等待多少秒
unsigned int s_exit_secs = 1;  // 单位:秒
// 时戳精度(0~9代表精确到秒的几位小数)
size_t      s_stamp_pre = 6;
// 时戳单位(为了截断时戳到指定精度,每次要用的除数)
uint64_t    s_time_unit;   //多少纳秒
// 日志文件名, 包含全路径
string      s_log_file;
// 日志文件名中缀, 用于日志文件轮转
string      s_log_infix;

// 给每个线程起个名字,输出的日志内能够看出每条日志都是由谁产生的
thread_local string  tl_thd_name = thrdId2Hex();

// 缓存日志的队列,整个系统产生的所有日志都存放于此,等待writer线程来消费
unique_ptr<LogQue_t> s_log_que = nullptr;

// 写日志的线程
thread      s_writer;

// 用于其它线程通知日志线程"新日志已入队"的信号量
sem_t       s_new_log;
// 是否正在进行日志文件轮转
atomic_bool s_is_rolling = { false };
// 指示writer线程是否还应继续运行的标志. 若将其置false, 日志线程将清空日志队列后退出
atomic_bool s_should_run = { false };
// 日志系统正在运行标志, 避免重复启动
atomic_bool s_is_running = { false };

//###### 各种函数实现 ############################################################

inline string thrdId2Hex( thread::id thread_id ) {
   ostringstream oss;
   oss << std::hex << thread_id;
   return oss.str();
};

//==== 实现API的几个函数 ==============

extern "C" const char* getVersion() {
   return PROJECT_VERSION;
};

extern "C" void startLogging( const string& log_file,
                              LogLevel_e    log_level,
                              size_t        t_precesion,
                              size_t        q_capacity ) {
   if( s_is_running.load( mo_acquire ) )
      throw bad_usage( "日志系统已启动, 不能重复初始化!" );

   const_cast<LogLevel_e&>( g_log_level ) =
      std::min( LogLevel_e::Fatal, std::max( LogLevel_e::Debug, log_level ) );
   s_stamp_pre = std::min<decltype( s_stamp_pre )>( t_precesion, 9 );
   s_time_unit = std::pow( 10.0, 9 - s_stamp_pre );
   s_log_file = log_file;
   s_log_que = make_unique<LogQue_t>( q_capacity );
   if( sem_init( &s_new_log, 0, 0 ) )
      throw std::runtime_error( "信号量创建失败, 不能启动日志系统!" );

   registThrdName( "主线程" );
   s_should_run.store( true, mo_release );
   s_writer = std::thread( writerThreadBody );
   steady_clock::time_point time_out = steady_clock::now() + 1s;
   while( !s_is_running.load( mo_acquire ) && steady_clock::now() < time_out )
      std::this_thread::sleep_for( 1ns );
   // 如果日志线程启动成功, s_is_running 应该已置位了
   if( !s_is_running.load( mo_acquire ) ) {
      s_should_run.store( false, mo_release );
      s_writer.detach();
      throw std::runtime_error( "日志系统启动失败" );
   }
};

extern "C" void stopLogging() {
   if( !s_is_running.load( mo_acquire ) )
      throw bad_usage( "日志系统尚未启动, 怎么关闭?" );

   LOG_NOTIF( "将要停止日志系统......" );
   s_should_run.store( false, mo_release );
   steady_clock::time_point time_out =
      steady_clock::now() + seconds( s_exit_secs );
   while( s_is_running.load( mo_acquire ) && steady_clock::now() < time_out ) {
      // 为避免 s_writer 苦等信号量而不能退出, 多给它发点
      sem_post( &s_new_log );
      std::this_thread::sleep_for( 1us );
   }

// 如果日志线程还在运行,就强制把它杀了
   if( s_is_running.load( mo_acquire ) ) {
      cerr << "====队内日志太多(" << s_log_que->size()
           << "),写不完了.将要杀掉日志线程...====" << endl;
      pthread_cancel( s_writer.native_handle() );
      //s_writer.detach();
      cerr << "====日志线程已杀!!!====" << endl;
   }
   
#ifdef DEBUG
   cerr << "joinning writer..." << endl;
#endif
   s_writer.join();

   if( sem_destroy( &s_new_log ) )
      throw std::runtime_error( "信号量销毁失败!" );
};

// 添加日志的主函数, 此处是实现。此函数只是把日志加入队列, 等待日志线程来写入文件
extern "C" bool appendLog( LogLevel_e level, const string& body ) {

   // 只有不低于门限值的日志才能得到输出
   if( level < g_log_level )
      return false;

   // 日志系统必须已经启动
   if( !s_is_running.load( mo_acquire ) ) {
      cerr << LOG_LEVEL_NAMES[static_cast<int>( level )]
           << ",早期日志," << body << "\n";
      return true;
   }

   // 日志入队
   LogEntry_t le { SysTimeNS_t::clock::now(), tl_thd_name, body, level, };
   auto tries = ENQUE_RETRIES;
   while( !s_log_que->enque( le ) ) {
      --tries;
      if( tries == 0 ) {
         cerr << LOG_LEVEL_NAMES[static_cast<int>( LogLevel_e::Error )]
              << "," << tl_thd_name << ",日志入队失败,抛弃日志:"
              << body << endl;
         return false;
      }
   };

   // 发信号
   sem_post( &s_new_log );
   return true;
};

extern "C" void registThrdName( const string& thd_name ) {
// 登记一个线程名, 此后输出该线程的日志时, 会包含此名，而非线程Id
   tl_thd_name = thd_name;
};

// 设置写盘间隔(每隔多少秒确保保存一次,默认3s)
extern "C" void setFlushSeconds( unsigned int secs ) {
   s_flush_secs = secs;
};

// 设置退出等待时长(给日志线程多少时间清盘,默认1s)
extern "C" void setExitSeconds( unsigned int secs ) {
   s_exit_secs = secs;
};

extern "C" void rotateLog( const string& infix ) {
// 本函数不是直接改名日志文件,只是置位全局变量,由日志线程完成真正的改名

// 先确保日志线程真的进入事件循环,否则它首次进入事件循环就会去轮转日志
   steady_clock::time_point time_out = steady_clock::now() + 100ms;
   while( !s_is_running.load( mo_acquire ) && steady_clock::now() < time_out )
      std::this_thread::sleep_for( 1ms );
   if( !s_is_running.load( mo_acquire ) )
      throw bad_usage( "日志系统尚未启动, 怎么轮转?" );

   s_log_infix = infix;
   s_is_rolling.store( true, mo_release );
   sem_post( &s_new_log );
   time_out = steady_clock::now() + 10s;
   while( s_is_rolling.load( mo_acquire ) && steady_clock::now() < time_out )
      std::this_thread::sleep_for( 1ns );

   if( s_is_rolling.load( mo_acquire ) ) {
      s_writer.detach();
      throw std::runtime_error( "日志系统轮转超时!" );
   }
};

void writerThreadBody() {
   /* 如果本系统已被海量的日志淹没,日志线程会需要很久才能写完退出(尤其是日志队列用得很大时),
    * 导致本系统停止失败。 所以日志线程设置为可以立即终止。
   int old_state = 0, old_type = 0;
   pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, &old_type );
   pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, &old_state );
    */

   // 日志线程自己也可以添加日志,当然就也可以注册有意义的线程名称
   registThrdName( "Logger" );

   while( s_should_run.load( mo_acquire ) ) {
      processLogs();

      // 如果退出信号量循环,有可能就是为了轮转日志
      if( s_is_rolling.load( mo_acquire ) )
         renameLogFile();
   }

   s_is_running.store( false, mo_release );
};

void processLogs() {
   ofstream ofsLogFile( s_log_file, std::ios_base::out | std::ios_base::app );
   ofsLogFile.imbue( std::locale( "C" ) );
   ofsLogFile.clear();

   // 每过1秒Flush一下, 所以需要记录时间
   timespec tsNextFlush, tsNow;
   timespec_get( &tsNextFlush, TIME_UTC );
   tsNextFlush.tv_sec += s_flush_secs;

   LogEntry_t aLog { SysTimeNS_t::clock::now(), "Logger", {}, LogLevel_e::Notif, };
   // LogEntry_t aLog; aLog.stamp  = SysTimeNS_t::clock::now(); aLog.thrd   = "Logger"; aLog.level  = LogLevel_e::Notif;
   if( s_is_rolling.load( mo_acquire ) )
      aLog.body = "---------- 日志文件已轮转 ----------";
   else
      aLog.body = "====== leonlog-" + string( PROJECT_VERSION ) + " 日志已启动("
                  + string( LOG_LEVEL_NAMES[static_cast<int>( g_log_level )] )
                  + ") ======";
   writeLog( ofsLogFile, aLog );
   s_is_rolling.store( false, mo_release );
   s_is_running.store( true, mo_release );

   // 主循环, 等待日志->写日志->判断是否需要轮转或退出, 周而复始...
   while( s_should_run.load( mo_acquire ) && !s_is_rolling.load( mo_acquire ) ) {
      // 等一个信号
      sem_timedwait( &s_new_log, &tsNextFlush );

      while( s_log_que->deque( aLog ) )
         writeLog( ofsLogFile, aLog );

      // 每1秒Flush一下
      timespec_get( &tsNow, TIME_UTC );
      if( tsNow > tsNextFlush ) {
         ofsLogFile.flush();
         tsNextFlush = tsNow;
         tsNextFlush.tv_sec += s_flush_secs;
      }
   }

   if( s_is_rolling.load( mo_acquire ) ) {
      // 这是需要轮转日志
      aLog.level = LogLevel_e::Notif;
      aLog.thrd = "Logger";
      aLog.stamp = SysTimeNS_t::clock::now();
      aLog.body = "---------- 日志文件将轮转 ----------";
      writeLog( ofsLogFile, aLog );
   } else {
      // 开始清盘, 如果此时日志还在源源不断地入队, 就会导致我们停不下来!
      // 所以在 stopLogging 函数内会杀掉本线程!
      while( s_log_que->size() > 0 )
         if( s_log_que->deque( aLog ) )
            writeLog( ofsLogFile, aLog );

      aLog.level = LogLevel_e::Notif;
      aLog.thrd = "Logger";
      aLog.stamp = SysTimeNS_t::clock::now();
      aLog.body = "================ 日志已停止 =================";
      writeLog( ofsLogFile, aLog );
   }

   ofsLogFile.close();
};

const char* const LOG_STAMP_FORMAT = "%m/%d %H:%M:%S";

inline void writeLog( ofstream& p_out, const LogEntry_t& log ) {
   SysTimeNS_t tpSecPart =
      time_point_cast<SysTimeNS_t::duration>(
         std::chrono::floor<seconds>( log.stamp ) );
   // 输出时戳
   p_out << formatTime( tpSecPart, LOG_STAMP_FORMAT );
   // 是否精确到秒以下
   if( s_stamp_pre > 0 ) {
      uint64_t sub_sec =
         duration_cast<nanoseconds>( log.stamp - tpSecPart ).count();
      sub_sec /= s_time_unit;
      p_out << '.' << formatNumber( sub_sec, s_stamp_pre, 0, 0, '0' );
   }

   p_out << ',' << LOG_LEVEL_NAMES[static_cast<int>( log.level )]
         << ',' << log.thrd << ',' << log.body << "\n";
   cerr
         << LOG_LEVEL_NAMES[static_cast<int>( log.level )]
         << ',' << log.thrd << ',' << log.body << "\n";
};

void renameLogFile() {
   path  oldPath( s_log_file );
   path  newPath = oldPath.parent_path() /
                   ( oldPath.stem().string() + '-' + s_log_infix );
   newPath += oldPath.extension();
   char cSuf = 'a';
   // 如果新起的文件名已被占用,就另想一个名字
   while( std::filesystem::exists( newPath ) ) {
      newPath = oldPath.parent_path() /
                ( oldPath.stem().string() + '-' + s_log_infix + cSuf++ );
      newPath += oldPath.extension();
   }
   std::filesystem::rename( oldPath, newPath );
};

}; // namespace leon_log

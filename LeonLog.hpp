#pragma once
#include <chrono>
#include <sstream>      // ostringstream
#include <string>

namespace leon_log {

enum class LogLevel_e {
   // 供跟踪、调试时使用的日志, Release编译时不会生成相应代码
   Debug = 0,

   // 程序运行期间非常详尽的细节报告, 供运维期间排查问题使用
   // 如: "本次写入DB数据为...."
   Infor,

   // 程序运行期间较简洁的状态报告, "通知性"、"状态转移性"报告
   // 如: "通信连接成功"、"数据库连接成功"
   Notif,

   // "警示性"报告, 通常用于指出某些尚能容忍的异常情况, 程序尚能运行
   // 如: "通信中收到异常数据"、"函数收到异常参数等"
   Warnn,

   // 错误报告, 指出程序已出现明显错误且无法自行恢复的状态, 但尚能"优雅地"退出
   // 如: "服务器拒绝登录"、"没有文件访问权限，无法继续"
   Error,

   // 失败报告, 指出程序已出现"崩溃性"错误, 将会立即崩溃
   // 如: "SegFalt"、"Abort"
   Fatal,

   // 取值计数
   VALUES_COUNT
};

constexpr std::string_view LOG_LEVEL_NAMES[] = {
   "DEBUG", // Debug
   "INFOR", // Infor
   "NOTIF", // Notif
   "WARNN", // Warnn
   "ERROR", // Error
   "FATAL"  // Fatal
};

using LogTimestamp_t = std::chrono::system_clock::time_point;

// 全系统日志级别
extern LogLevel_e g_log_level;
/* static 会导致多重"影子"变量,下面这些都不行!
   static LogLevel_e g_log_level = LogLevel_e::Debug;
   static const LogLevel_e g_log_level = LogLevel_e::Debug; */

// 常量定义: 单个日志队列的容量(整个系统中每个线程对应一个日志队列, 只要它添加过日志)
// 队列再大也只是缓冲突发的日志，如果产生日志持续比消费日志快, 再大的队列也会爆...
constexpr size_t DEFAULT_LOG_QUE_SIZE = 256;

extern "C" const char* getVersion();

// 指定日志文件名, 启动日志系统
extern "C" void startLogging(
   const std::string&   log_file,                           // 日志文件路径及名称
   LogLevel_e           log_levl = LogLevel_e::Debug,       // 日志级别
   size_t               stamp_precision = 6,                // 时戳精度
   size_t               que_size = DEFAULT_LOG_QUE_SIZE );  // 队列容量

// 关闭日志, 并Flush所有日志到磁盘
extern "C" void stopLogging();
// 在fork后的子进程内关闭日志,因为此时没有writer线程,只能静默释放资源
extern "C" void _stopLogging();

// 为当前线程登记一个名字,此后输出该线程的日志会包含此名，而非线程Id
extern "C" void registThreadName( const std::string& );

// 添加日志的主函数
extern "C" bool appendLog( LogLevel_e level, const std::string& body );

// 设置写盘间隔(每隔多少秒确保保存一次,默认3s)
extern "C" void setFlushSeconds( unsigned int secs );  // 单位:秒

// 设置退出等待时长(给日志线程多少时间清盘,默认1s)
extern "C" void setExitSeconds( unsigned int secs );  // 单位:秒

// 设置一个存放时间戳的指针,之后输出日志时都会去那个地址找时戳
extern "C" void setTimestampPtr( const LogTimestamp_t* );

// 轮转日志文件
extern "C" void rotateLog( const std::string& infix /*中缀*/ );

#ifdef DEBUG

#define LOG_DEBUG( log_body ) ( g_log_level <= LogLevel_e::Debug && appendLog( LogLevel_e::Debug, std::string( __func__ ) + "()," + ( log_body ) ) )
#define LOG_INFOR( log_body ) ( g_log_level <= LogLevel_e::Infor && appendLog( LogLevel_e::Infor, std::string( __func__ ) + "()," + ( log_body ) ) )
#define LOG_NOTIF( log_body ) ( g_log_level <= LogLevel_e::Notif && appendLog( LogLevel_e::Notif, std::string( __func__ ) + "()," + ( log_body ) ) )
#define LOG_WARNN( log_body ) ( g_log_level <= LogLevel_e::Warnn && appendLog( LogLevel_e::Warnn, std::string( __func__ ) + "()," + ( log_body ) ) )
#define LOG_ERROR( log_body ) ( g_log_level <= LogLevel_e::Error && appendLog( LogLevel_e::Error, std::string( __func__ ) + "()," + ( log_body ) ) )
#define LOG_FATAL( log_body ) ( g_log_level <= LogLevel_e::Fatal && appendLog( LogLevel_e::Fatal, std::string( __func__ ) + "()," + ( log_body ) ) )

#else

#define LOG_DEBUG( log_body ) ( g_log_level <= LogLevel_e::Debug && appendLog( LogLevel_e::Debug, ( log_body ) ) )
#define LOG_INFOR( log_body ) ( g_log_level <= LogLevel_e::Infor && appendLog( LogLevel_e::Infor, ( log_body ) ) )
#define LOG_NOTIF( log_body ) ( g_log_level <= LogLevel_e::Notif && appendLog( LogLevel_e::Notif, ( log_body ) ) )
#define LOG_WARNN( log_body ) ( g_log_level <= LogLevel_e::Warnn && appendLog( LogLevel_e::Warnn, ( log_body ) ) )
#define LOG_ERROR( log_body ) ( g_log_level <= LogLevel_e::Error && appendLog( LogLevel_e::Error, ( log_body ) ) )
#define LOG_FATAL( log_body ) ( g_log_level <= LogLevel_e::Fatal && appendLog( LogLevel_e::Fatal, ( log_body ) ) )

#endif

class Logger_t : public std::ostringstream {
public:
   explicit Logger_t( LogLevel_e l ) : m_LogLevel( l ) {};

   // 释放本对象时一并输出,且本类可派生
   ~Logger_t() override {
      appendLog( m_LogLevel, str() );
   };

   inline operator bool() {
      return m_LogLevel >= g_log_level;
   };

   // 把属性公开之后,就不需要后面那一堆友元函数了.关键是,用户自定义类型也可流式输出了!
   LogLevel_e m_LogLevel;
};

template <typename T>
inline Logger_t& operator<<( Logger_t& logger, const T& body ) {

   if( logger.m_LogLevel >= g_log_level )
      static_cast<std::ostringstream&>( logger ) << ( body );

// 很多自定义类(包括STL内的)会重载(overloading) << 运算符,以便输出自己,所以不可能直接
// 调用 ostream::operator<<() 的方法(以下都不好):
//    static_cast<std::ostringstream&>( logger ).operator<<( body );
//    dynamic_cast<std::ostringstream&>( logger ).operator<<( body );
//    dynamic_cast<std::ostringstream&>( logger ) << body;
//    return std::ostringstream::operator<<( body );

   return logger;
};

#ifdef DEBUG

#define lg_debg g_log_level <= LogLevel_e::Debug && Logger_t(LogLevel_e::Debug) << __func__ << "(),"
#define lg_info g_log_level <= LogLevel_e::Infor && Logger_t(LogLevel_e::Infor) << __func__ << "(),"
#define lg_note g_log_level <= LogLevel_e::Notif && Logger_t(LogLevel_e::Notif) << __func__ << "(),"
#define lg_warn g_log_level <= LogLevel_e::Warnn && Logger_t(LogLevel_e::Warnn) << __func__ << "(),"
#define lg_erro g_log_level <= LogLevel_e::Error && Logger_t(LogLevel_e::Error) << __func__ << "(),"
#define lg_fatl g_log_level <= LogLevel_e::Fatal && Logger_t(LogLevel_e::Fatal) << __func__ << "(),"

#else

#define lg_debg g_log_level <= LogLevel_e::Debug && Logger_t(LogLevel_e::Debug)
#define lg_info g_log_level <= LogLevel_e::Infor && Logger_t(LogLevel_e::Infor)
#define lg_note g_log_level <= LogLevel_e::Notif && Logger_t(LogLevel_e::Notif)
#define lg_warn g_log_level <= LogLevel_e::Warnn && Logger_t(LogLevel_e::Warnn)
#define lg_erro g_log_level <= LogLevel_e::Error && Logger_t(LogLevel_e::Error)
#define lg_fatl g_log_level <= LogLevel_e::Fatal && Logger_t(LogLevel_e::Fatal)

#endif

};  // namespace leon_log

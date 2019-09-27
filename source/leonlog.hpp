#pragma once
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>

namespace leon_log {

enum class LogLevel_e {
   // 供跟踪、调试时使用的日志, Release编译时不会生成相应代码
   ellDebug = 0,

   // 程序运行期间非常详尽的细节报告, 供运维期间排查问题使用
   // 如: "本次写入DB数据为...."
   ellInfor,

   // 程序运行期间较简洁的状态报告, "通知性"、"状态转移性"报告
   // 如: "通信连接成功"、"数据库连接成功"
   ellNotif,

   // "警示性"报告, 通常用于指出某些尚能容忍的异常情况, 程序尚能运行
   // 如: "通信中收到异常数据"、"函数收到异常参数等"
   ellWarnn,

   // 错误报告, 指出程序已出现明显错误且无法自行恢复的状态, 但尚能"优雅地"退出
   // 如: "服务器拒绝登录"、"没有文件访问权限，无法继续"
   ellError,

   // 失败报告, 指出程序已出现"崩溃性"错误, 将会立即崩溃
   // 如: "SegFalt"、"Abort"
   ellFatal,

   // 取值计数
   VALUES_COUNT
};

constexpr std::string_view LOG_LEVEL_NAMES[] = {
   "DEBUG", // ellDebug
   "INFOR", // ellInfor
   "NOTIF", // ellNotif
   "WARNN", // ellWarnn
   "ERROR", // ellError
   "FATAL"  // ellFatal
};

// static const LogLevel_e g_ellLogLevel = ellDebug;
static LogLevel_e g_ellLogLevel;

/* thread::get_id()返回的不是操作系统内真正的线程id,我们要通过OS的工具(top)监测优化
 * 本系统内的各个线程,就必须要知道各线程在OS层面的id.所以每个线程来调用registThrdName
 * 函数时,就会获取其真正id并写入下面这个map,供监控所用.
 */
extern std::map<std::string, pid_t> g_mapOsThreadIds;

// 常量定义: 单个日志队列的容量(整个系统中每个线程对应一个日志队列, 只要它添加过日志)
// 队列再大也只是缓冲突发的日志，如果产生日志持续比消费日志快, 再大的队列也会爆...
constexpr size_t DEFAULT_LOG_QUE_SIZE = 128;

extern "C" const char* getVersion();

// 指定日志文件名, 启动日志系统
extern "C" void startLogging(
   const std::string&   logFile,
   LogLevel_e           logLevel = LogLevel_e::ellDebug,
   uint64_t             timeStampPrecision = 6,
   size_t               logQueSize = DEFAULT_LOG_QUE_SIZE );

// 关闭日志, 并Flush所有日志到磁盘
extern "C" void stopLogging();

// 轮转日志文件
extern "C" void rotateLog( const std::string& crp_strInfix );

// 可选函数, 登记一个线程名, 此后输出该线程的日志会包含此名，而非线程Id
extern "C" void registThrdName( const std::string& crp_strName );

// 添加日志的主函数
extern "C" void appendLog0( LogLevel_e logLevel, std::string&& logBody );
// extern "C" void appendLog1( LogLevel_e logLevel, const std::string& logBody );

/* Linux 动态库不能支持C++函数的 overloading (参见 symbol name mangle),
 * 因此动态库能够导出的函数只能是C语言的普通函数(extern "C"),
 * 所以弄出如下的函数Wrapper */
inline void appendLog( LogLevel_e logLevel, std::string&& logBody ) {
   if ( logLevel >= g_ellLogLevel )
      appendLog0( logLevel, std::move( logBody ) );
};
inline void appendLog( LogLevel_e logLevel, const std::string& logBody ) {
   if ( logLevel >= g_ellLogLevel )
      appendLog0( logLevel, std::string( logBody ) );
};

#ifdef DEBUG
#define LOG_DEBUG( strLogBody ) \
   appendLog( leon_log::LogLevel_e::ellDebug, \
              std::string( __func__ ) + "()," + ( strLogBody ) )
#define LOG_INFOR( strLogBody ) \
   appendLog( leon_log::LogLevel_e::ellInfor, \
              std::string( __func__ ) + "()," + ( strLogBody ) )
#define LOG_NOTIF( strLogBody ) \
   appendLog( leon_log::LogLevel_e::ellNotif, \
              std::string( __func__ ) + "()," + ( strLogBody ) )
#define LOG_WARNN( strLogBody ) \
   appendLog( leon_log::LogLevel_e::ellWarnn, \
              std::string( __func__ ) + "()," + ( strLogBody ) )
#define LOG_ERROR( strLogBody ) \
   appendLog( leon_log::LogLevel_e::ellError, \
              std::string( __func__ ) + "()," + ( strLogBody ) )
#define LOG_FATAL( strLogBody ) \
   appendLog( leon_log::LogLevel_e::ellFatal, \
              std::string( __func__ ) + "()," + ( strLogBody ) )
#else
// 如果不是DEBUG编译, 就根本不会为LOG_DEBUG产生代码
#define LOG_DEBUG( strLogBody )
#define LOG_INFOR( strLogBody ) \
   appendLog( leon_log::LogLevel_e::ellInfor, ( strLogBody ) )
#define LOG_NOTIF( strLogBody ) \
   appendLog( leon_log::LogLevel_e::ellNotif, ( strLogBody ) )
#define LOG_WARNN( strLogBody ) \
   appendLog( leon_log::LogLevel_e::ellWarnn, ( strLogBody ) )
#define LOG_ERROR( strLogBody ) \
   appendLog( leon_log::LogLevel_e::ellError, ( strLogBody ) )
#define LOG_FATAL( strLogBody ) \
   appendLog( leon_log::LogLevel_e::ellFatal, ( strLogBody ) )
#endif

class Logger_t : public std::ostringstream {
public:
   explicit Logger_t( LogLevel_e p_enmLevel ) : m_LogLevel( p_enmLevel ) {};

   // 释放本对象时一并输出,且本类可派生
   virtual ~Logger_t() {
      appendLog( m_LogLevel, str() );
   };

   // 把属性公开之后,就不需要后面那一堆友元函数了.关键是,用户自定义类型也可流式输出了!
   LogLevel_e m_LogLevel;
};

template <typename T>
inline Logger_t& operator<<( Logger_t& lgr, const T& body ) {
   if ( lgr.m_LogLevel >= g_ellLogLevel )
      dynamic_cast<std::ostringstream&>( lgr ) << body;

   return lgr;
};

#ifdef DEBUG

#define log_debug ( Logger_t( LogLevel_e::ellDebug ) << __func__ << "()," )
#define log_infor ( Logger_t( LogLevel_e::ellInfor ) << __func__ << "()," )
#define log_notif ( Logger_t( LogLevel_e::ellNotif ) << __func__ << "()," )
#define log_warnn ( Logger_t( LogLevel_e::ellWarnn ) << __func__ << "()," )
#define log_error ( Logger_t( LogLevel_e::ellError ) << __func__ << "()," )
#define log_fatal ( Logger_t( LogLevel_e::ellFatal ) << __func__ << "()," )

#else

#define log_debug ( Logger_t( LogLevel_e::ellDebug ) )
#define log_infor ( Logger_t( LogLevel_e::ellInfor ) )
#define log_notif ( Logger_t( LogLevel_e::ellNotif ) )
#define log_warnn ( Logger_t( LogLevel_e::ellWarnn ) )
#define log_error ( Logger_t( LogLevel_e::ellError ) )
#define log_fatal ( Logger_t( LogLevel_e::ellFatal ) )

#endif

};  // namespace leon_log

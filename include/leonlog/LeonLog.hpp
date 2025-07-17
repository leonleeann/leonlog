#pragma once
#include <chrono>
#include <functional>
#include <leonutils/Chrono.hpp>
#include <leonutils/UnionTypes.hpp>
#include <sstream>
#include <string>
#include <type_traits>

using oss_t = std::ostringstream;
using ost_t = std::ostream;
using str_t = std::string;

// 指针概念
template<typename T>
concept AnyPtr =
	( std::is_pointer_v<std::remove_cvref_t<T>> ||
	  std::is_null_pointer_v<std::remove_cvref_t<T>> ) &&
	!( std::is_same_v<std::remove_pointer_t<std::remove_cvref_t<T>>, char> ||
	   std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, char> ) &&
	!( std::is_same_v<std::remove_pointer_t<std::remove_cvref_t<T>>, void> ||
	   std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, void> );

// 非指针概念
template<typename T>
concept NonPtr =
	std::is_same_v<std::remove_pointer_t<std::remove_cvref_t<T>>, T>&&
	!std::is_same_v<std::remove_pointer_t<std::remove_cvref_t<T>>, char>&&
	!std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, char>&&
	!std::is_same_v<std::remove_pointer_t<std::remove_cvref_t<T>>, void>&&
	!std::is_same_v<std::remove_cvref_t<std::remove_pointer_t<T>>, void>;

//==============================================================================
namespace leon_log {

enum LogLevel_e : int {
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

using LogStamp_t = std::chrono::system_clock::time_point;

// 全系统日志级别
extern LogLevel_e g_log_level;
/* static 会导致多重"影子"变量,下面这些都不行!
   static LogLevel_e g_log_level = LogLevel_e::Debug;
   static const LogLevel_e g_log_level = LogLevel_e::Debug; */

// 常量定义: 单个日志队列的容量(整个系统中每个线程对应一个日志队列, 只要它添加过日志)
// 队列再大也只是缓冲突发的日志，如果产生日志持续比消费日志快, 再大的队列也会爆...
constexpr size_t DEFAULT_LOG_QUE_SIZE = 256;

extern "C" const char* Version();
extern "C" const char* NameOf( LogLevel_e );

// 指定日志文件名, 启动日志系统
extern "C" void StartLog(
	const str_t&	log_file,							// 日志文件路径及名称
	LogLevel_e		log_levl = LogLevel_e::Debug,		// 日志级别
	size_t			stamp_precision = 6,				// 时戳精度
	size_t			que_size = DEFAULT_LOG_QUE_SIZE,	// 日志队列容量
	bool			header = true,						// 启动时输出header
	bool			stdout = true,						// 同时输出至stdout
	bool			tstamp = false						// 输出至stdout的带不带时戳
);  // 队列容量

// 关闭日志, 并Flush所有日志到磁盘
extern "C" void StopLog(
	bool			footer = true,	// 是否在日志尾部输出 footer
	bool			rename = true,	// 关闭日志文件后是否改名
	const str_t&	infix = ""		// 改名的中缀
);
// 日志系统正在运行
extern "C" bool IsLogging();
// 显示错误信息,关闭日志并退出
extern "C" void ExitWithLog( const str_t& );

// 在fork后的子进程内关闭日志,因为此时没有writer线程,只能静默释放资源
// extern "C" void _stopLogging();

// 为当前线程登记一个名字,此后输出该线程的日志会包含此名，而非线程Id
extern "C" void RegistThread( const str_t& );

// 添加日志的主函数
extern "C" bool AppendLog( LogLevel_e, const str_t& body );

// 设置写盘间隔(每隔多少秒确保保存一次,默认1s)
extern "C" void SetFlushIntrvl( leon_utl::SysDura_t interval );

// 设置退出等待时长(给日志线程多少时间清盘,默认3s)
extern "C" void SetExitSeconds( unsigned int secs );

// 设置一个存放时间戳的指针,之后输出日志时都会去那个地址找时戳
extern "C" void SetLogStampPtr( const LogStamp_t* );

// 轮转日志文件
extern "C" void RotateLogFile( const str_t& infix /*中缀*/ );

// 绑核到哪个 CPU. 其实绑核不如"nice值"和"调度优先级"更节约cpu
extern "C" bool AffineCpu( int cpu );

#ifdef DEBUG

#define LOG_DEBUG( log_body ) ( g_log_level <= LogLevel_e::Debug && AppendLog( LogLevel_e::Debug, str_t( __func__ ) + "()," + ( log_body ) ) )
#define LOG_INFOR( log_body ) ( g_log_level <= LogLevel_e::Infor && AppendLog( LogLevel_e::Infor, str_t( __func__ ) + "()," + ( log_body ) ) )
#define LOG_NOTIF( log_body ) ( g_log_level <= LogLevel_e::Notif && AppendLog( LogLevel_e::Notif, str_t( __func__ ) + "()," + ( log_body ) ) )
#define LOG_WARNN( log_body ) ( g_log_level <= LogLevel_e::Warnn && AppendLog( LogLevel_e::Warnn, str_t( __func__ ) + "()," + ( log_body ) ) )
#define LOG_ERROR( log_body ) ( g_log_level <= LogLevel_e::Error && AppendLog( LogLevel_e::Error, str_t( __func__ ) + "()," + ( log_body ) ) )
#define LOG_FATAL( log_body ) ( g_log_level <= LogLevel_e::Fatal && AppendLog( LogLevel_e::Fatal, str_t( __func__ ) + "()," + ( log_body ) ) )

#else

#define LOG_DEBUG( log_body ) ( g_log_level <= LogLevel_e::Debug && AppendLog( LogLevel_e::Debug, ( log_body ) ) )
#define LOG_INFOR( log_body ) ( g_log_level <= LogLevel_e::Infor && AppendLog( LogLevel_e::Infor, ( log_body ) ) )
#define LOG_NOTIF( log_body ) ( g_log_level <= LogLevel_e::Notif && AppendLog( LogLevel_e::Notif, ( log_body ) ) )
#define LOG_WARNN( log_body ) ( g_log_level <= LogLevel_e::Warnn && AppendLog( LogLevel_e::Warnn, ( log_body ) ) )
#define LOG_ERROR( log_body ) ( g_log_level <= LogLevel_e::Error && AppendLog( LogLevel_e::Error, ( log_body ) ) )
#define LOG_FATAL( log_body ) ( g_log_level <= LogLevel_e::Fatal && AppendLog( LogLevel_e::Fatal, ( log_body ) ) )

#endif

class Log_t: public oss_t {
public:
	explicit Log_t( LogLevel_e l ) : _level( l ) {};

	// 释放本对象时一并输出,且本类可派生
	~Log_t() override { AppendLog( _level, str() ); };

	operator bool() { return _level >= g_log_level; };

	// 把属性公开之后,就不需要后面那一堆友元函数了.关键是,用户自定义类型也可流式输出了!
	LogLevel_e _level;
};

/*-------------------------------------
	为了特殊处理指针, 当遭遇空指针时, 能够输出诸如 "{null-char*}", "{null-void*}"...
	这样的玩意, 而非直接崩溃, 所以试试针对特定类型做 overload. */
inline Log_t& operator<<( Log_t& log_, const char* ptr_ ) {
	if( log_._level < leon_log::g_log_level )
		return log_;

	if( ptr_ == nullptr )
		static_cast<oss_t&>( log_ ) << "{null-char*}";
	else
		static_cast<oss_t&>( log_ ) << ptr_;
	return log_;
};

inline Log_t& operator<<( Log_t& log_, const void* ptr_ ) {
	if( log_._level < g_log_level )
		return log_;

	if( ptr_ == nullptr )
		static_cast<oss_t&>( log_ ) << "{null-void*}";
	else
		static_cast<oss_t&>( log_ ) << std::hex << ptr_;
	return log_;
};

inline ost_t& operator<<( ost_t& os_, leon_utl::U64_u u_ ) {
	return os_ << u_.view();
};

template <typename C, typename D>
inline ost_t& operator<<( ost_t& os_, time_point<C, D> tp_ ) {
	return os_ << leon_utl::fmt( tp_, 6, leon_utl::NEAT_FORMAT );
};

//-------------------------------------

};	// namespace leon_log ======================================================

/* operator<<函数的overload, 在不同的命名空间内似乎不一定能被用上, 试试在地板上也放一个
inline leon_log::Log_t& operator<<( leon_log::Log_t& log_, const char* ptr_ ) {
	return leon_log::operator<<( log_, ptr_ );
};
inline leon_log::Log_t& operator<<( leon_log::Log_t& log_, const void* ptr_ ) {
	return leon_log::operator<<( log_, ptr_ );
};
*/

// template<typename T> T& unmove( T&& r_ ) { return static_cast<T&>( r_ ); }

template <NonPtr T>
inline leon_log::Log_t& operator<<( leon_log::Log_t& log_, const T& body_ ) {

	if( log_._level >= leon_log::g_log_level )
		static_cast<oss_t&>( log_ ) << body_;

	return log_;
};

template <AnyPtr T>
inline leon_log::Log_t& operator<<( leon_log::Log_t& log_, T body_ ) {

	if( log_._level < leon_log::g_log_level )
		return log_;

	if( body_ == nullptr )
		static_cast<oss_t&>( log_ ) << "{nullptr}";
	else
		static_cast<oss_t&>( log_ ) << *body_;

	return log_;
};

#ifdef DEBUG

#define lg_debg (g_log_level <= LogLevel_e::Debug) && Log_t(LogLevel_e::Debug) << __func__ << "(),"
#define lg_info (g_log_level <= LogLevel_e::Infor) && Log_t(LogLevel_e::Infor) << __func__ << "(),"
#define lg_note (g_log_level <= LogLevel_e::Notif) && Log_t(LogLevel_e::Notif) << __func__ << "(),"
#define lg_warn (g_log_level <= LogLevel_e::Warnn) && Log_t(LogLevel_e::Warnn) << __func__ << "(),"
#define lg_erro (g_log_level <= LogLevel_e::Error) && Log_t(LogLevel_e::Error) << __func__ << "(),"
#define lg_fatl (g_log_level <= LogLevel_e::Fatal) && Log_t(LogLevel_e::Fatal) << __func__ << "(),"

#else

#define lg_debg (g_log_level <= LogLevel_e::Debug) && Log_t(LogLevel_e::Debug)
#define lg_info (g_log_level <= LogLevel_e::Infor) && Log_t(LogLevel_e::Infor)
#define lg_note (g_log_level <= LogLevel_e::Notif) && Log_t(LogLevel_e::Notif)
#define lg_warn (g_log_level <= LogLevel_e::Warnn) && Log_t(LogLevel_e::Warnn)
#define lg_erro (g_log_level <= LogLevel_e::Error) && Log_t(LogLevel_e::Error)
#define lg_fatl (g_log_level <= LogLevel_e::Fatal) && Log_t(LogLevel_e::Fatal)

#endif

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;

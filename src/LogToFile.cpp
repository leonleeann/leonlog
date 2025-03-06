#include <algorithm>    // for_each, max, min, sort, swap
#include <atomic>
#include <cerrno>		// errno
#include <chrono>
#include <cmath>		// abs, ceil, floor, isnan, log, log10, pow, round, sqrt
#include <cstring>		// strlen, strncmp, strncpy, memset, memcpy, memmove, strerror
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <leonutils/Chrono.hpp>
#include <leonutils/CraflinRQ.tpp>
#include <leonutils/Exceptions.hpp>
#include <leonutils/MemoryOrder.hpp>
#include <memory>
#include <mutex>
#include <sched.h>		// sched_setaffinity
#include <semaphore.h>
#include <shared_mutex>
#include <sys/syscall.h>	// SYS_gettid
#include <sys/sysinfo.h>	// get_nprocs
#include <thread>
#include <unistd.h>		// syscall

#include "Version.hpp"
#include "leonlog/LeonLog.hpp"
#include "leonlog/StatusFile.hpp"
#include "leonlog/ThreadName.hpp"

using namespace leon_utl;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace std::filesystem;

using abool_t = std::atomic_bool;
using aptid_t = std::atomic<pthread_t>;
using ofs_t = std::ofstream;
using std::cerr;
using std::endl;
using std::make_unique;
using std::max;
using std::min;
using std::shared_lock;
using std::shared_mutex;
using std::thread;
using std::unique_lock;
using std::unique_ptr;

namespace leon_log {

//###### 各种类型 ###############################################################

// LogEntry: 定义一条日志记录所具有的基本内容
struct LogEntry_t {
	LogStamp_t	stamp;	// 日志产生时间
	str_t		tname;	// 产生日志的线程
	str_t		body;	// 日志内容
	LogLevel_e	level;	// 日志级别
};

// LogQue_t: 日志队列(一个队列服务整个APP)
using LogQue_t = CraflinRQ_t<LogEntry_t>;

//###### 各种常量 ###############################################################

const str_t LOG_LEVEL_NAMES[] = {
	"DEBUG", // Debug
	"INFOR", // Infor
	"NOTIF", // Notif
	"WARNN", // Warnn
	"ERROR", // Error
	"FATAL"  // Fatal
};

//###### 各种函数前置申明 #########################################################

// 返回16进制串表达的线程Id
str_t ThreadId2Hex( thread::id my_id = std::this_thread::get_id() );

// 写日志的线程体
void WriterThreadBody();

// 日志线程的核心工作：出队日志，写日志
void ProcessLogs();

// 写一条日志
void Write1Log( ofs_t&, const LogEntry_t& );

// 完成一次日志轮转(将当前日志文件保存、关闭、改名)
void RenameLogFile();

//###### 各种变量 ###############################################################

// 日志级别
LogLevel_e	g_log_level = LogLevel_e::Debug;

// 写盘间隔(每隔多少秒确保保存一次)
decltype( timespec::tv_nsec )	s_flush_ns = 1000000000;	// 单位:纳秒
// 干掉日志线程之前等待多少秒
unsigned int					s_exit_secs = 3;	// 单位:秒
// 时戳精度(0~9代表精确到秒的几位小数)
size_t							s_stamp_pre = 6;
// 时戳单位(为了截断时戳到指定精度,每次要用的除数)
uint64_t						s_time_unit = 1;		//多少纳秒
// 日志文件名, 包含全路径
str_t							s_log_file;
// 日志文件名中缀, 用于日志文件轮转
str_t							s_log_infix;
// 状态文件名, 包含全路径
str_t							s_status_file;
// 状态输出周期
LogStamp_t::duration			s_status_interval;
// 下次输出状态的时点
LogStamp_t						s_next_status;

// 给每个线程起个名字,输出的日志内能够看出每条日志都是由谁产生的
thread_local str_t				tl_t_name = ThreadId2Hex();
Names2LinuxTId_t				s_t_ids;		// 线程名到t_id的映射
shared_mutex					s_mtx4nids;		// 更新s_t_ids时的同步控制

// 日志时戳,为空就用当前时间
thread_local const LogStamp_t*	tl_stamp = nullptr;

// 缓存日志的队列,整个系统产生的所有日志都存放于此,等待writer线程来消费
unique_ptr<LogQue_t>			s_log_que = nullptr;
unique_ptr<ofs_t>				s_log_ofs = nullptr;

// 写日志的线程
thread	s_writer;

// 生成"状态文本"的回调函数
WriteStatus_f	s_wr_status;

// 用于其它线程通知日志线程"新日志已入队"的信号量
sem_t	s_new_log;
// 是否正在进行日志文件轮转
abool_t	s_is_rolling { false };
// 指示writer线程是否还应继续运行的标志. 若将其置false, 日志线程将清空日志队列后退出
abool_t	s_should_run { false };
// 日志系统正在运行标志, 避免重复启动
abool_t	s_is_running { false };
// 要不要在启停时输出header/footer
abool_t	s_headr_foot { true };
// 是否同时输出至stdout
bool	s_to_stdout { false };
// 输出至stdout的内容是否也带时戳
bool	s_sto_stamp { false };
// logger 线程的 pthread_id
aptid_t	s_log_tid {};

//###### 各种函数实现 ############################################################

inline str_t ThreadId2Hex( thread::id thread_id ) {
	oss_t oss;
	oss << std::hex << thread_id;
	return oss.str();
};

//==== 实现API的几个函数 ==============

extern "C" const char* Version() {
	return PROJECT_VERSION;
};

extern "C" const char* NameOf( LogLevel_e ll ) {
	return LOG_LEVEL_NAMES[ll].c_str();
};

extern "C" void StartLog( const str_t&	file_,
						  LogLevel_e	levl_,
						  size_t		prec_,
						  size_t		capa_,
						  bool			head_,
						  bool			stdo_,
						  bool			stot_ ) {
	if( s_is_running.load( mo_acquire ) )
		throw bad_usage( "日志系统已启动, 不能重复初始化!" );

//	const_cast<LogLevel_e&>( g_log_level ) =
	g_log_level = min( LogLevel_e::Fatal, max( LogLevel_e::Debug, levl_ ) );
	s_stamp_pre = min<decltype( s_stamp_pre )>( prec_, 9 );
	s_time_unit = std::pow( 10.0, 9 - s_stamp_pre );
	s_log_file = file_;
	s_log_que = make_unique<LogQue_t>( capa_ );
	s_headr_foot.store( head_ );
	s_to_stdout = stdo_;
	s_sto_stamp = stot_;
	if( sem_init( &s_new_log, 0, 0 ) )
		throw std::runtime_error( "信号量创建失败, 不能启动日志系统!" );

	RegistThread( "MainThread" );
	s_should_run.store( true, mo_release );
	s_writer = std::thread( WriterThreadBody );
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

extern "C" void StopLog( bool ft_, bool rn_, const str_t& infix_ ) {
	if( !s_is_running.load( mo_acquire ) )
// 		throw bad_usage( "日志系统尚未启动, 怎么关闭?" );
		return;

	if( ft_ )
		LOG_DEBUG( "将要停止日志系统......" );

// 本函数不会直接改名日志文件,只是置位全局变量,由日志线程完成真正的改名
	s_headr_foot.store( ft_, mo_release );
	s_log_infix = infix_;
	s_is_rolling.store( rn_, mo_release );
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
		cerr << "====队内日志太多(" << s_log_que->size() << '/' << s_log_que->capa()
			 << "),写不完了.将要杀掉日志线程...====" << endl;
		pthread_cancel( s_writer.native_handle() );
		s_writer.detach();
		s_is_running.store( false, mo_release );
		cerr << "====日志线程已杀!!!====" << endl;
	}

#ifdef DEBUG
	cerr << "joinning writer..." << endl;
#endif
	if( s_writer.joinable() )
		s_writer.join();

	if( sem_destroy( &s_new_log ) )
		throw std::runtime_error( "信号量销毁失败!" );
};

extern "C" bool IsLogging() {
	return s_is_running.load( mo_acquire );
};

extern "C" void ExitWithLog( const str_t& err ) {
	std::cerr << err << std::endl;

	if( s_is_running.load( mo_acquire ) ) {
		AppendLog( LogLevel_e::Fatal, err );
		StopLog();
	}
	exit( EXIT_FAILURE );
};

/* extern "C" void _stopLogging() {
	try {
		s_is_running.store( false, mo_release );
		s_should_run.store( false, mo_release );

		LogEntry_t aLog;
		while( s_log_que->deque( aLog ) )
			;

		s_t_ids.clear();
		s_log_que = nullptr;
		s_log_ofs = nullptr;
		if( sem_destroy( &s_new_log ) )
			throw std::runtime_error( "信号量销毁失败!" );
	} catch( ... ) {
		cerr << "出错";
	};
}; */

extern Names2LinuxTId_t LinuxThreadIds() {
	shared_lock<shared_mutex> sh_lk( s_mtx4nids );
	Names2LinuxTId_t result { s_t_ids };
	return result;
};

// 登记一个线程名, 此后输出该线程的日志时, 会包含此名，而非线程Id
extern "C" void RegistThread( const str_t& my_name ) {
	tl_t_name = my_name;
	unique_lock<shared_mutex> ex_lk( s_mtx4nids );
	s_t_ids[my_name] = syscall( SYS_gettid );
};

// 添加日志的主函数, 此处是实现。此函数只是把日志加入队列, 等待日志线程来写入文件
extern "C" bool AppendLog( LogLevel_e level, const str_t& body ) {
	// 只有不低于门限值的日志才能得到输出
	if( level < g_log_level )
		return false;

	// 日志系统必须已经启动
	if( ! s_is_running.load( mo_acquire ) ) {
		cerr << LOG_LEVEL_NAMES[level]
			 << ",早期日志," << tl_t_name << ',' << body << "\n";
		return true;
	}

	// 日志入队
	LogEntry_t le { tl_stamp != nullptr ? *tl_stamp : system_clock::now(),
					tl_t_name, body, level, };

	// 日志入队重试次数
	constexpr int ENQUE_RETRIES = 10;
	auto tries = ENQUE_RETRIES;
	while( ! s_log_que->enque( le ) ) {
		sem_post( &s_new_log );
		--tries;
		if( tries <= 0 ) {
			cerr << LOG_LEVEL_NAMES[LogLevel_e::Error]
				 << "," << tl_t_name << ",日志入队失败,抛弃日志:"
				 << body << endl;
			return false;
		}
	};

	// 发信号
	sem_post( &s_new_log );
	return true;
};

// 设置写盘间隔(每隔多少秒确保保存一次,默认3s)
extern "C" void SetFlushSeconds( SysDura_t int_ ) {
	s_flush_ns = int_.count();
};

// 设置退出等待时长(给日志线程多少时间清盘,默认1s)
extern "C" void SetExitSeconds( unsigned int secs ) {
	s_exit_secs = secs;
};

extern "C" void SetLogStampPtr( const LogStamp_t* tstamp ) {
	tl_stamp = tstamp;
};

extern "C" void RotateLogFile( const str_t& infix ) {
// 本函数不会直接改名日志文件,只是置位全局变量,由日志线程完成真正的改名
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

extern "C" void SetStatus( const str_t& file_, WriteStatus_f writer_, int secs_ ) {
	s_status_file = file_;
	s_wr_status = writer_;
	s_status_interval = seconds( secs_ );
	s_next_status = system_clock::now() + s_status_interval;
};

// 输出一次状态信息
void WriteStatus() {
	if( s_status_file.empty() )
		return;

	auto now_tp = system_clock::now();
	if( now_tp < s_next_status )
		return;

	ofs_t f { s_status_file, std::ios_base::out | std::ios_base::trunc };
	s_wr_status( f );
};

void WriterThreadBody() {
	/* 如果本系统已被海量的日志淹没,日志线程会需要很久才能写完退出(尤其是日志队列用得很大时),
	 * 导致本系统停止失败。 所以日志线程设置为可以立即终止。
	int old_state = 0, old_type = 0;
	pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, &old_type );
	pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, &old_state );
	 */

	s_log_tid = pthread_self();
	nice( 19 );	// 注意nice接受的参数是增量,有积累效应, 但最大也就 19

	// 日志线程自己也可以添加日志,当然就也可以注册有意义的线程名称
	RegistThread( "Logger" );

	while( s_should_run.load( mo_acquire ) ) {
		ProcessLogs();

		path tmp = path( s_log_file );
		if( s_log_file != "/dev/null" && exists( tmp ) ) {
			if( file_size( tmp ) == 0 )
				try { remove( tmp ); } catch( const std::exception& e ) {
					cerr << "删除空文件(" << tmp << ")甩出异常:" << e.what();
				}

			else if( s_is_rolling.load( mo_acquire ) )
				RenameLogFile();
		}
	}

	s_is_running.store( false, mo_release );
};

void ProcessLogs() {
	s_log_ofs = make_unique<ofs_t>( s_log_file,
									   std::ios_base::out | std::ios_base::app );
	s_log_ofs->imbue( std::locale( "C" ) );
	s_log_ofs->clear();

	// 每过1秒Flush一下, 所以需要记录时间
	timespec tsNextFlush, tsNow;
	timespec_get( &tsNextFlush, TIME_UTC );
	tsNextFlush += s_flush_ns;

	LogEntry_t aLog { system_clock::now(), "Logger", {}, LogLevel_e::Infor, };
	if( s_is_rolling.load( mo_acquire ) ) {
		aLog.body = "---------- 日志文件已轮转 ----------";
		Write1Log( *s_log_ofs, aLog );
	} else if( s_headr_foot.load( mo_acquire ) ) {
		aLog.body = "====== leonlog-" + str_t( PROJECT_VERSION ) + " 日志已启动("
					+ LOG_LEVEL_NAMES[g_log_level] + ") ======";
		Write1Log( *s_log_ofs, aLog );
	}
	s_is_rolling.store( false, mo_release );
	s_is_running.store( true, mo_release );

	// 主循环, 等待日志->写日志->判断是否需要轮转或退出, 周而复始...
	while( s_should_run.load( mo_acquire ) && !s_is_rolling.load( mo_acquire ) ) {
		// 等一个信号
		sem_timedwait( &s_new_log, &tsNextFlush );

		while( s_log_que->deque( aLog ) )
			Write1Log( *s_log_ofs, aLog );

		// 每1秒Flush一下
		timespec_get( &tsNow, TIME_UTC );
		if( tsNow > tsNextFlush ) {
			s_log_ofs->flush();
			tsNextFlush = tsNow;
			tsNextFlush += s_flush_ns;
			WriteStatus();
		}
	}

	if( s_should_run.load( mo_acquire ) ) {
		// 这是需要轮转日志
		aLog.level = LogLevel_e::Notif;
		aLog.tname = "Logger";
		aLog.stamp = system_clock::now();
		aLog.body = "---------- 日志文件将轮转 ----------";
		Write1Log( *s_log_ofs, aLog );
	} else {
		// 开始清盘, 如果此时日志还在源源不断地入队, 就会导致我们停不下来!
		// 所以在 stopLogging 函数内会杀掉本线程!
		while( s_log_que->size() > 0 )
			if( s_log_que->deque( aLog ) )
				Write1Log( *s_log_ofs, aLog );

		if( s_headr_foot.load( mo_acquire ) ) {
			aLog.level = LogLevel_e::Infor;
			aLog.tname = "Logger";
			aLog.stamp = system_clock::now();
			aLog.body = "================ 日志已停止 =================";
			Write1Log( *s_log_ofs, aLog );
		}
	}

	s_log_ofs->close();
	s_log_ofs = nullptr;
};

const char* const LOG_STAMP_FORMAT = "%y/%m/%d %H:%M:%S";

inline void Write1Log( ofs_t& p_out, const LogEntry_t& log ) {

	// 构造时戳
	LogStamp_t tpSecPart =
		time_point_cast<LogStamp_t::duration>(
			std::chrono::floor<seconds>( log.stamp ) );
	str_t time_str = format_time( tpSecPart, LOG_STAMP_FORMAT );

	// 输出时戳(尽量不拼接字符串,应该快点?)
	p_out << time_str;

	// 是否精确到秒以下
	str_t nsec_str;
	if( s_stamp_pre > 0 ) {
		uint64_t sub_sec =
			duration_cast<nanoseconds>( log.stamp - tpSecPart ).count();
		sub_sec /= s_time_unit;
		nsec_str = fmt( sub_sec, s_stamp_pre, 0, 0, '0' );
		p_out << '.' << nsec_str;
	}

	p_out << ',' << LOG_LEVEL_NAMES[log.level]
		  << ',' << log.tname << ',' << log.body << "\n";

	// 要否也输出至stdout
	if( !s_to_stdout )
		return;
	// 输出至stdout时还要不要时戳
	if( s_sto_stamp ) {
		std::cout << time_str;
		if( s_stamp_pre > 0 )
			std::cout << '.' << nsec_str;
		std::cout << ',';
	}
	std::cout << LOG_LEVEL_NAMES[log.level]
			  << ',' << log.tname << ',' << log.body << endl;
};

void RenameLogFile() {
	path	old_path( s_log_file );
	path	new_path = old_path.parent_path() /
					   ( old_path.stem().string() + '-' + s_log_infix );
	new_path += old_path.extension();

	// 如果新起的文件名已被占用,就另想一个名字
	char	suf_chr = 'a' - 1;
	while( exists( new_path ) ) {
		if( ++suf_chr > 'z' ) {
			cerr << "改名日志文件(" << old_path
				 << ")失败,期望文件名(" << new_path << ")已存在!";
			return;
		}
		new_path = old_path.parent_path() /
				   ( old_path.stem().string() + '-' + s_log_infix + suf_chr );
		new_path += old_path.extension();
	}
	rename( old_path, new_path );
};

extern "C" bool AffineCpu( int cpu_ ) {
	cpu_set_t mask;
	CPU_ZERO( &mask );
	CPU_SET( cpu_, &mask );
	return ( pthread_setaffinity_np( s_log_tid, sizeof( mask ), &mask ) == 0 );
};

}; // namespace leon_log

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;

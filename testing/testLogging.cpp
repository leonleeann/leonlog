// #include <LeonLog>
#include <ThreadName>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <string_view>
#include <thread>
#include <unistd.h>
#include <vector>

#include "LeonLog.hpp"
#include "misc/Converts.hpp"

using namespace leon_ext;
using namespace leon_log;
using namespace std::chrono;
using namespace std;

// 解析命令行参数
void parseAppOptions( int arg_count, const char* const* const args );
// 显示用法后退出
void showUsageAndExit();

string   g_app_name;
uint64_t g_stamp_p = 6;
uint64_t g_threads = 2;
uint64_t g_intervl = 100000;
uint64_t g_lasting = 3;
uint64_t g_quesize = 1024;
atomic_bool g_should_run = { true };
std::vector<thread> makers;

void threadBody( uint64_t p_uiMyId ) {
// LogTimestamp_t tstamp;
// setTimestampPtr( &tstamp );
	string myName = string( "线程" ) + to_string( p_uiMyId );
	RegistThread( myName );
	auto t_id = leon_log::getLinuxThreadIds()[myName];
	lg_debg << "[" << t_id << "]:开始了!";
	string_view sv { myName.c_str() };

	uint64_t j = 0;
	try {
		while( g_should_run.load( std::memory_order::acquire ) ) {
//       Logger_t ll( LogLevel_e::Debug );
//       ll << j;

//       ( Logger_t().setlevel( ellInfor ) << __func__ << "()," ) << j;
			lg_erro << sv << ":" << j++;

//       LOG_INFOR( "子线程日志..." + to_string( j ) );
			this_thread::sleep_for( nanoseconds( g_intervl ) );
		}

	} catch( const std::exception& e ) {
//    } catch ( ... ) {
		cerr << myName << "遭遇异常:" << e.what();
	}
//   LOG_INFOR( myName + "总共循环:" + to_string( j ) );
	lg_erro << "[" << t_id << "]:总共循环:" << j;
};

int main( int argc, char** argv ) {
	g_app_name = argv[0];
	parseAppOptions( argc, argv );
	cout << "pid:" << getpid()
		 << "\n日志库版本:" << leon_log::Version()
		 << "\n日志级别:" << leon_log::LevelName( g_log_level )
		 << "\n时戳精度:" << g_stamp_p << "位"
		 << "\n线程数量:" << g_threads
		 << "\n生产间隔:" << g_intervl << "ns"
		 << "\n持续时间:" << g_lasting << "s"
		 << "\n队列长度:" << g_quesize << endl;

	StartLogging( g_app_name + ".log", LogLevel_e::Debug, g_stamp_p, g_quesize,
				  true, true, true );

	for( uint64_t k = 0; k < g_threads; ++k )
		makers.emplace_back( thread( threadBody, k ) );

	std::this_thread::sleep_for( seconds( g_lasting ) );

	g_should_run.store( false, std::memory_order::release );
	for( auto& mkr : makers )
		mkr.join();

	/*
	   LifeCycleWatcher_t lcw { "lcw000" };
	   lg_debg << "一条调试日志" + lcw;
	   LOG_DEBUG( "一条调试日志" + lcw );
	   LOG_INFOR( "一条信息日志" );
	   log_notif << "一条通告日志" << lcw;
	   log_warnn << "一条警告日志" << lcw;
	   lg_erro << "一条错误日志" << lcw;
	   log_fatal << "一条失败日志" << lcw;

	   // 故意中途轮转日志,看看谁跑得快
	   rotateLog( "1234" );

	   k = 0;
	   for ( ; k < g_uiThrds; ++k ) {
	      thrds[k].join();
	   }

	   lg_debg << str1;
	   log_infor << str1;
	   log_notif << str1;
	   log_warnn << str1;
	   lg_erro << str1;
	   log_fatal << str1;
	   LOG_DEBUG( str1 );
	   LOG_INFOR( str1 );
	   LOG_NOTIF( str1 );
	   LOG_WARNN( str1 );
	   LOG_ERROR( str1 );
	   LOG_FATAL( str1 );
	*/

//    cout << mss;
//    lg_debg << &mss;
//    oss << mss;
//    Logger_t lg { LogLevel_e::Fatal };
//    dynamic_cast<ostream&>( lg ) << mss;
//    dynamic_cast<Logger_t&>( lg ) << mss;
//    lg << &mss;

//    cerr << "==============准备停止日志系统==============" << endl;
//    cout << "==============准备停止日志系统==============" << endl;
//    cerr << "==============准备停止日志系统==============" << endl;
//    cout << "==============准备停止日志系统==============" << endl;
//    cerr << "==============准备停止日志系统==============" << endl;
	for( const auto& [n, i] : leon_log::getLinuxThreadIds() )
		lg_erro << n << ":[" << i << "]." << endl;

	StopLogging();
	cout << "系统正常退出." << endl;
	return EXIT_SUCCESS;
};

void parseAppOptions( int argc, const char* const* const args ) {
	for( int i = 1; i < argc; ++i ) {
		string val = trim( args[i] );
//================ 一般选项 =====================================================
		if( val == "-H" || val == "--help" ) {
			showUsageAndExit();

//================ 本应用选项 ===================================================
		} else if( val == "-P" || val == "--stamp-p" ) {
			if( ++i >= argc ) {
				cerr << "-P(--stamp-p)选项后面需要数量,无法继续!" << endl;
				showUsageAndExit();
			}
			g_stamp_p = atoi( args[i] );
		} else if( val == "-T" || val == "--threads" ) {
			if( ++i >= argc ) {
				cerr << "-T(--threads)选项后面需要数量,无法继续!" << endl;
				showUsageAndExit();
			}
			g_threads = atoi( args[i] );
		} else if( val == "-I" || val == "--intervl" ) {
			if( ++i >= argc ) {
				cerr << "-I(--intervl)选项后面需要数量,无法继续!" << endl;
				showUsageAndExit();
			}
			g_intervl = atoi( args[i] );
		} else if( val == "-L" || val == "--lasting" ) {
			if( ++i >= argc ) {
				cerr << "-L(--lasting)选项后面需要数量,无法继续!" << endl;
				showUsageAndExit();
			}
			g_lasting = atoi( args[i] );
		} else if( val == "-Q" || val == "--quesize" ) {
			if( ++i >= argc ) {
				cerr << "-Q(--quesize)选项后面需要数量,无法继续!" << endl;
				showUsageAndExit();
			}
			g_quesize = atoi( args[i] );

//================= 未知选项 ====================================================
		} else {
			cerr << '"' << val << "\" 是无法识别的选项,无法继续!" << endl;
			showUsageAndExit();
		}
	};
};

void showUsageAndExit() {
	cerr << "目的: 测试 leonlog 日志库"
		 << "\n用法0: " << g_app_name
		 << "\n\t-H(--help)  : 显示用法后退出"
		 << "\n用法1: " << g_app_name
		 << "\n\t-P (--stamp-p) <时戳精度,6>"
		 << "\n\t-Q (--quesize) <日志队列长度,1024>"
		 << "\n\t-T (--threads) <并发产生日志的线程数量,2>"
		 << "\n\t-I (--intervl) <产生日志的间隔纳秒数,100000ns>"
		 << "\n\t-L (--lasting) <测试持续秒数,10s>"
		 << endl;
	exit( EXIT_FAILURE );
};

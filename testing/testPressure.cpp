#include <chrono>
#include <filesystem>
#include <forward_list>
#include <iomanip>
#include <iostream>
#include <thread>
#include <unistd.h>

#include "LeonLog.hpp"
#include "misc/Converts.hpp"

using namespace leon_ext;
using namespace leon_log;
using namespace std::chrono;
using namespace std;

void threadBody( int thread_id );

// 解析命令行参数
void parseCmdLineOpts( int, const char* const* const );

int				s_thread_count = 1;
LogLevel_e		s_log_level = LogLevel_e::Debug;
int				s_run_seconds = 3;
int				s_sleep_nanos = 1;
nanoseconds					s_sleep_ns = nanoseconds( s_sleep_nanos );
steady_clock::time_point	s_end_time;

forward_list<thread> runners;

int main( int argc, char** argv ) {
	string app_name;
	std::filesystem::path full_name { argv[0] };
	app_name = full_name.stem();
	parseCmdLineOpts( argc, argv );
	s_end_time = steady_clock::now() + seconds( s_run_seconds );
	s_sleep_ns = nanoseconds( s_sleep_nanos );

	cout << "pid:" << getpid()
		 << "\n日志库版本:" << leon_log::Version()
		 << "\n日志级别:" << leon_log::LevelName( s_log_level )
		 << endl;

	startLogging( app_name + ".log", LogLevel_e::Debug, 9, 1024 * 1024, true, false );
	lg_note << "主日志开始...";

	for( int i = 0; i < s_thread_count; ++i )
		runners.emplace_front( threadBody, i );
	for( auto& rnr : runners )
		rnr.join();

	lg_note << "主进程退出";
	stopLogging();
	return EXIT_SUCCESS;
};

void threadBody( int my_id ) {
	registThreadName( "runner" + formatNumber(my_id, 2, 0, 0, '0' ) );

	int count = 0;
	auto start_time = steady_clock::now();
	while( steady_clock::now() < s_end_time ) {
		if( ! appendLog( LogLevel_e::Debug, std::to_string( count ) ) )
			break;

		++count;
		this_thread::sleep_for( s_sleep_ns );
	}

	auto lasted = steady_clock::now() - start_time;
	double write_rate = count * 1000000000.0 / lasted.count();
	this_thread::sleep_for( milliseconds( my_id * 10 ) );
	cerr << my_id << "我已退出:"
		 << formatNumber( write_rate, 0, 3, 4, ' ', '\'' )
		 << endl;
};

void parseCmdLineOpts( int argc, const char* const* const args ) {
	bool opt_err = false;

	for( int i = 1; i < argc; ++i ) {
		string argv = trim( args[i] );
		if( argv == "-TC" || argv == "--thread-count" ) {
			if( !( opt_err = ++i >= argc ) )
				s_thread_count = atoi( args[i] );
		} else if( argv == "-L" || argv == "--log-level" ) {
			if( !( opt_err = ++i >= argc ) )
				s_log_level = static_cast<LogLevel_e>( atoi( args[i] ) );
		} else if( argv == "-R" || argv == "--run-seconds" ) {
			if( !( opt_err = ++i >= argc ) )
				s_run_seconds = atoi( args[i] );
		} else if( argv == "-S" || argv == "--sleep-ns" ) {
			if( !( opt_err = ++i >= argc ) )
				s_sleep_nanos = atoi( args[i] );
		} else
			exitWithLog( '"' + argv + "\"是无法识别的选项,无法继续!" );
	};

	if( opt_err )
		exitWithLog( "选项错误,无法继续!" );
};

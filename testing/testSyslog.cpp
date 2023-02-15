#include <chrono>
#include <filesystem>
#include <forward_list>
#include <iomanip>
#include <iostream>
#include <syslog.h>
#include <thread>
#include <unistd.h>

#include "misc/Converts.hpp"

using namespace leon_ext;
using namespace std::chrono;
using namespace std;

void threadBody( int thread_id );

// 解析命令行参数
void parseCmdLineOpts( int, const char* const* const );

string			s_app_name;
int				s_thread_count = 1;
int				s_run_seconds = 3;
int				s_sleep_nanos = 1;
nanoseconds					s_sleep_ns = nanoseconds( s_sleep_nanos );
steady_clock::time_point	s_end_time;

forward_list<thread> runners;

int main( int argc, char** argv ) {
	std::filesystem::path full_path { argv[0] };
	s_app_name = full_path.stem();
	parseCmdLineOpts( argc, argv );
	s_end_time = steady_clock::now() + seconds( s_run_seconds );
	s_sleep_ns = nanoseconds( s_sleep_nanos );

// 	cout << "pid:" << getpid() << endl;
	openlog( s_app_name.c_str(), LOG_CONS | LOG_NDELAY | LOG_PID, LOG_USER );
	syslog( LOG_NOTICE, "主日志开始..." );

	for( int i = 0; i < s_thread_count; ++i )
		runners.emplace_front( threadBody, i );
	for( auto& rnr : runners )
		rnr.join();

	syslog( LOG_NOTICE, "主进程退出" );
	closelog();
	return EXIT_SUCCESS;
};

void threadBody( int my_id ) {
	string my_name = "runner" + formatNumber( my_id, 2, 0, 0, '0' );
	auto module_id = LOG_LOCAL0 + my_id;
	openlog( my_name.c_str(), LOG_CONS | LOG_NDELAY | LOG_PID, module_id );

	int count = 0;
	auto start_time = steady_clock::now();
	while( steady_clock::now() < s_end_time ) {
		syslog( module_id | LOG_DEBUG, "%d", count );
		++count;
		this_thread::sleep_for( s_sleep_ns );
	}

	auto lasted = steady_clock::now() - start_time;
	double write_rate = count * 1000000000.0 / lasted.count();
	this_thread::sleep_for( milliseconds( my_id * 10 ) );
	syslog( module_id | LOG_NOTICE, "我已退出:%f", write_rate );
	closelog();
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
		} else if( argv == "-R" || argv == "--run-seconds" ) {
			if( !( opt_err = ++i >= argc ) )
				s_run_seconds = atoi( args[i] );
		} else if( argv == "-S" || argv == "--sleep-ns" ) {
			if( !( opt_err = ++i >= argc ) )
				s_sleep_nanos = atoi( args[i] );
		} else {
			cerr << '"' << argv << "\"是无法识别的选项,无法继续!" << endl;
			exit( EXIT_FAILURE );
		};
	};

	if( opt_err ) {
		cerr << "选项错误,无法继续!" << endl;
		exit( EXIT_FAILURE );
	};
};

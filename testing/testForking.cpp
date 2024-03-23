#include <LeonLog>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

#include "LeonLog.hpp"
#include "misc/Converts.hpp"

using namespace leon_ext;
using namespace leon_log;
using namespace std::chrono;
using namespace std;

int main( int argc, char** argv ) {
	string app_name;
	std::filesystem::path full_name { argv[0] };
	app_name = full_name.stem();

	cout << "pid:" << getpid()
		 << "\n日志库版本:" << leon_log::Version()
		 << "\n日志级别:" << leon_log::LevelName( g_log_level )
		 << endl;

	StartLogging( app_name + ".log", LogLevel_e::Debug, 9, 128, true, true, true );
	lg_note << "主日志开始...";
// 	std::this_thread::sleep_for( 1s );
// 	lg_note << "主进程退出";
// 	stopLogging();
// 	return EXIT_SUCCESS;

	stopLogging( false );
	pid_t child = fork();
	if( child < 0 ) {
		cerr << "fork失败!" << endl;
		exit( EXIT_FAILURE );
	}

	if( child > 0 ) {
		StartLogging( app_name + ".log", LogLevel_e::Debug, 9, 128, false, true, true );
// 		lg_note << "主日志重启...";
		lg_info << child << "子进程forked";
// 		std::this_thread::sleep_for( 5s );
		lg_note << "父进程退出";
		stopLogging( true );
		std::cerr << "父进程已退出" << getpid() << endl;
		return EXIT_SUCCESS;
	};

	std::this_thread::sleep_for( 5s );
	std::cerr << "子进程将要退出:" << getpid();


// 		_stopLogging();
// 		cerr << "==子进程:" << getpid();
// 		cerr << "===子进程:" << getpid();
// 		child = getpid();
// 		startLogging( to_string( child ) + ".log", LogLevel_e::Debug, 3, 128 );
// 		lg_note << child << "子进程将要退出";
// 		exit( EXIT_SUCCESS );
};

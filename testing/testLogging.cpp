#include <chrono>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <thread>
#include <vector>

#include <leonlog>

using namespace std;
using namespace std::chrono;
using namespace leon_log;

uint64_t       g_uiThrds = 8;
uint64_t       g_uiInterval = 1024;
uint64_t       g_uiLasting = 32;

void threadBody( uint64_t p_uiMyId ) {
   string myName = string( "线程" ) + to_string( p_uiMyId );
   registThrdName( myName );
   string_view sv { myName.c_str() };

   uint64_t j = 0;
   for ( ; j < g_uiInterval; ++j ) {
//    Logger_t ll( LogLevel_e::ellDebug );
//       ll << j;

//    ( Logger_t().setlevel( ellInfor ) << __func__ << "()," ) << j;
      log_fatal << sv << ":" << j;

//    LOG_INFOR( "子线程日志..." + to_string( j ) );
      this_thread::sleep_for( nanoseconds( g_uiLasting ) );
   }

//   LOG_INFOR( myName + "总共循环:" + to_string( j ) );
   log_infor << myName << "总共循环:" << j;
};

int main( int argc, char ** argv ) {
   cout << "日志库版本:" << leon_log::getVersion() << endl;

   size_t      uiLogQueSize = 1024;
   // 日志级别
   if ( argc > 1 )
      g_ellLogLevel = ( LogLevel_e ) atoi( argv[1] );
   g_ellLogLevel = std::min(
                      std::max( g_ellLogLevel, LogLevel_e::ellDebug ),
                      LogLevel_e::ellFatal );

   // 并发线程数
   if ( argc > 2 )
      g_uiThrds = atoi( argv[2] );

   // 每线程产生多少条日志
   if ( argc > 3 )
      g_uiInterval = atoi( argv[3] );

   // 日志队列容量
   if ( argc > 4 )
      uiLogQueSize = atoi( argv[4] );

   // 同一线程内产生两条日志间,间隔的纳秒数
   if ( argc > 5 )
      g_uiLasting = atoi( argv[5] );

   cout << "日志级别:" << LOG_LEVEL_NAMES[( int )g_ellLogLevel] << endl;

   startLogging( "testLogging.log", g_ellLogLevel, 3, uiLogQueSize );

   std::vector<thread> thrds;
   uint64_t k = 0;
   for ( ; k < g_uiThrds; ++k ) {
      thrds.push_back( thread( threadBody, k ) );
   }

   log_debug << "一条调试日志";
   log_infor << "一条信息日志";
   log_notif << "一条通告日志";
   log_warnn << "一条警告日志";
   log_error << "一条错误日志";
   log_fatal << "一条失败日志";

   // 故意中途轮转日志,看看谁跑得快
   rotateLog( "1234" );

   k = 0;
   for ( ; k < g_uiThrds; ++k ) {
      thrds[k].join();
   }

   stopLogging();
   return EXIT_SUCCESS;
};

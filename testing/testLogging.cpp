#include <chrono>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <thread>
#include <vector>

#include <leonlog.hpp>

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
      log_debug << sv << ":" << j;

//    LOG_INFOR( "子线程日志..." + to_string( j ) );
      this_thread::sleep_for( nanoseconds( g_uiLasting ) );
   }

//   LOG_INFOR( myName + "总共循环:" + to_string( j ) );
   log_infor << myName << "总共循环:" << j;
};

int main( int argc, char ** argv ) {
   cout << "日志库版本:" << leon_log::getVersion() << endl;
//    cout << "g_LogLevel[" << &g_ellLogLevel << "]:" << (int)g_ellLogLevel << endl;

   size_t      uiLogQueSize = 1024;
   if ( argc > 1 )   // 线程数
      g_uiThrds = atoi( argv[1] );
   if ( argc > 2 )   // 每线程产生多少条日志
      g_uiInterval = atoi( argv[2] );
   if ( argc > 3 )   // 日志队列容量
      uiLogQueSize = atoi( argv[3] );
   if ( argc > 4 )   // 同一线程内产生两条日志间,间隔的纳秒数
      g_uiLasting = atoi( argv[4] );

   startLogging( "testLogging.log", LogLevel_e::ellDebug, 6, uiLogQueSize );

   std::vector<thread> thrds;
   uint64_t k = 0;
   for ( ; k < g_uiThrds; ++k ) {
      thrds.push_back( thread( threadBody, k ) );
   }

   // 故意中途轮转日志,看看谁跑得快
   rotateLog( "1234" );

   k = 0;
   for ( ; k < g_uiThrds; ++k ) {
      thrds[k].join();
   }

   stopLogging();
   return EXIT_SUCCESS;
};

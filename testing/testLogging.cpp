#include <chrono>
#include <iomanip>
#include <iostream>
#include <leonlog>
#include <map>
#include <string_view>
#include <thread>
#include <vector>

#include "misc/LifeCycleWatcher.hpp"

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

using StrMap_t = map<string, string>;
StrMap_t mss { { "k0", "v0" }, { "k1", "v1" } };

ostream& operator<<( ostream& os, const StrMap_t& mss ) {
//    cout << "ostream& operator<<()被调用" << endl;
   for ( const auto& [k, v] : mss )
      os << '{' << k << ':' << v << '}';

   return os;
};
ostringstream& operator<<( ostringstream& os, const StrMap_t& mss ) {
   cout << "ostringstream& operator<<()被调用" << endl;
   return os;
};
Logger_t& operator<<( Logger_t& os, const StrMap_t* mss ) {
   cout << "Logger_t& operator<<()被调用" << endl;
   return os;
};

int main( int argc, char ** argv ) {
   cout << "日志库版本:" << leon_log::getVersion() << endl;

   size_t     uiLogQueSize = 1024;
   LogLevel_e ll = LogLevel_e::ellDebug;
   // 日志级别
   if ( argc > 1 )
      ll = static_cast<LogLevel_e>( atoi( argv[1] ) );
   const_cast<LogLevel_e&>( g_ellLogLevel ) = std::min(
            std::max( ll, LogLevel_e::ellDebug ),
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
   string str1 { "str1" };
   ostringstream oss;

   /*
      std::vector<thread> thrds;
      uint64_t k = 0;
      for ( ; k < g_uiThrds; ++k ) {
         thrds.push_back( thread( threadBody, k ) );
      }

      LifeCycleWatcher_t lcw { "lcw000" };
      log_debug << "一条调试日志" + lcw;
      LOG_DEBUG( "一条调试日志" + lcw );
      LOG_INFOR( "一条信息日志" );
      log_notif << "一条通告日志" << lcw;
      log_warnn << "一条警告日志" << lcw;
      log_error << "一条错误日志" << lcw;
      log_fatal << "一条失败日志" << lcw;

      // 故意中途轮转日志,看看谁跑得快
      rotateLog( "1234" );

      k = 0;
      for ( ; k < g_uiThrds; ++k ) {
         thrds[k].join();
      }

      log_debug << str1;
      log_infor << str1;
      log_notif << str1;
      log_warnn << str1;
      log_error << str1;
      log_fatal << str1;
      LOG_DEBUG( str1 );
      LOG_INFOR( str1 );
      LOG_NOTIF( str1 );
      LOG_WARNN( str1 );
      LOG_ERROR( str1 );
      LOG_FATAL( str1 );
   */

//    cout << mss;
//    log_debug << &mss;
//    oss << mss;
   Logger_t lg { LogLevel_e::ellFatal };
//    dynamic_cast<ostream&>( lg ) << mss;
//    dynamic_cast<Logger_t&>( lg ) << mss;
   lg << &mss;

   stopLogging();
   return EXIT_SUCCESS;
};

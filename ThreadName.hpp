#pragma once
#include <map>
#include <pthread.h>
#include <string>

namespace leon_log {

using Names2PThread_t = std::map<std::string, pthread_t>;

// 通过登记过的线程名获取 pthread_id
// extern "C" pthread_t threadNativeIdOf( const std::string& );

// 获取所有已登记过的映射,注意不能返回引用或指针,必须是对象的副本(多线程环境)!!!
extern Names2PThread_t  pthreadIdOfNames();

};  // namespace leon_log

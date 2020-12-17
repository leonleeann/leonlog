#pragma once
#include <map>
#include <string>
// #include <sys/types.h>

namespace leon_log {

using Names2LinuxTId_t = std::map<std::string, pid_t>;

// 通过登记过的线程名获取 pthread_id
// extern "C" pthread_t threadNativeIdOf( const std::string& );

// 获取所有已登记过的映射,注意不能返回引用或指针,必须是对象的副本(多线程环境)!!!
extern Names2LinuxTId_t getLinuxThreadIds();

};  // namespace leon_log

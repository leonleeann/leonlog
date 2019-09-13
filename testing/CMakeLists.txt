cmake_minimum_required(VERSION 3.0.0)

project( test-log LANGUAGES CXX )

# 静态库版
add_executable( test-log testLogging.cpp
   ${LEON_CODE_BASE}/chrono/Chrono.cpp
   ${LEON_CODE_BASE}/misc/MiscTools.cpp
)
target_link_libraries( test-log
   leonlog
   "stdc++fs"
   ${CMAKE_THREAD_LIBS_INIT}
)
install( TARGETS test-log RUNTIME DESTINATION testing )

cmake_minimum_required(VERSION 3.0.0)

project( test-log LANGUAGES CXX )

add_library( leonlog_s STATIC IMPORTED )
add_library( leonlog_d SHARED IMPORTED )
set_property( TARGET leonlog_s PROPERTY IMPORTED_LOCATION "/usr/local/lib/libleonlog.a" )
set_property( TARGET leonlog_d PROPERTY IMPORTED_LOCATION "/usr/local/lib/libleonlog.so" )

#======== obj libs =====================
add_library( objlib_testLogging OBJECT
   testLogging.cpp 
   ${OCTOPUS_CODE_BASE}/common/chrono/Chrono.cpp
   ${OCTOPUS_CODE_BASE}/common/misc/Converts.cpp
)

#======== 静态版 =====================
add_executable( s-log $<TARGET_OBJECTS:objlib_testLogging> )
target_link_libraries( s-log
   Threads::Threads
   "stdc++fs"
   leonlog_s
#    leonlog_static
)
install( TARGETS s-log RUNTIME DESTINATION testing )

#======== 动态版 =====================
add_executable( d-log $<TARGET_OBJECTS:objlib_testLogging> )
target_link_libraries( d-log
   Threads::Threads
   "stdc++fs"
   leonlog_d
)
install( TARGETS d-log RUNTIME DESTINATION testing )

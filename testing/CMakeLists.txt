cmake_minimum_required(VERSION 3.0.0)

project( test-log LANGUAGES CXX )

#======== obj libs =====================
add_library( objTestLog OBJECT
   testLogging.cpp
   ${OCTOPUS_CODE_BASE}/common/chrono/Chrono.cpp
   ${OCTOPUS_CODE_BASE}/common/misc/Converts.cpp
)

#======== 静态版 =====================
add_executable( s-log $<TARGET_OBJECTS:objTestLog> )
target_link_libraries( s-log
   Threads::Threads
   "stdc++fs"
   target_static
)
install( TARGETS s-log RUNTIME DESTINATION testing )

#======== 动态版 =====================
add_executable( d-log $<TARGET_OBJECTS:objTestLog> )
target_link_libraries( d-log
   Threads::Threads
   "stdc++fs"
   target_dynmic
)
install( TARGETS d-log RUNTIME DESTINATION testing )

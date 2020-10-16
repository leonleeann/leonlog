cmake_minimum_required(VERSION 3.0.0)

project( test-log LANGUAGES CXX )

#======== 静态版 =====================
add_executable( test-log testLogging.cpp
   ${OCTOPUS_CODE_BASE}/common/chrono/Chrono.cpp
   ${OCTOPUS_CODE_BASE}/common/misc/Converts.cpp
)
target_link_libraries( test-log
   ${CMAKE_THREAD_LIBS_INIT}
   "stdc++fs"
   leonlog
)
install( TARGETS test-log RUNTIME DESTINATION testing )

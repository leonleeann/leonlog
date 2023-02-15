cmake_minimum_required( VERSION 3.16.0 )

project( test-log LANGUAGES CXX )

add_library( objTestLog OBJECT testLogging.cpp )

#[[======== 静态版 =====================
add_executable( s-log )
target_link_libraries( s-log
# 	objCommon
	objLeonExt
	objTestLog
	Threads::Threads
	"stdc++fs"
	target_static
)
install( TARGETS s-log RUNTIME DESTINATION testing )

#======== 动态版 =====================
add_executable( d-log )
target_link_libraries( d-log
# 	objCommon
	objLeonExt
	objTestLog
	Threads::Threads
	"stdc++fs"
	target_dynmic
)
install( TARGETS d-log RUNTIME DESTINATION testing )

#======== test forking =====================
add_executable( testForking testForking.cpp )
target_link_libraries( testForking
# 	objCommon
	objLeonExt
	Threads::Threads
	"stdc++fs"
	target_static
)
install( TARGETS testForking RUNTIME DESTINATION testing )

#======== test pressure =====================
add_executable( testPressure testPressure.cpp )
target_link_libraries( testPressure
# 	objCommon
	objLeonExt
	Threads::Threads
	"stdc++fs"
	target_static
)
install( TARGETS testPressure RUNTIME DESTINATION testing )
]]

#======== test syslog =====================
add_executable( testSyslog testSyslog.cpp )
target_link_libraries( testSyslog
# 	objCommon
	objLeonExt
	Threads::Threads
	"stdc++fs"
)
install( TARGETS testSyslog RUNTIME DESTINATION testing )

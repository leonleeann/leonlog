cmake_minimum_required( VERSION 3.16.0 )

project( test-log LANGUAGES CXX )

#======== 单元测试 ===================
add_executable( ut-leonlog UnitTestMain.cpp )
target_link_libraries( ut-leonlog
	${GTEST_BOTH_LIBRARIES} ${GMOCK_BOTH_LIBRARIES}
)
install( TARGETS ut-leonlog RUNTIME DESTINATION testing )

#======== 静态版 =====================
add_library( objTestLog OBJECT testLogging.cpp )
add_executable( s-log )
target_link_libraries( s-log
	objTestLog

	leonlog_static
	Threads::Threads
)
install( TARGETS s-log RUNTIME DESTINATION testing )

#======== 动态版 =====================
add_executable( d-log )
target_link_libraries( d-log
	objTestLog

	leonlog_dynmic
	Threads::Threads
)
install( TARGETS d-log RUNTIME DESTINATION testing )

#[[======== test forking =====================
add_executable( testForking testForking.cpp )
target_link_libraries( testForking
	leonlog_static
	Threads::Threads
)
install( TARGETS testForking RUNTIME DESTINATION testing )

#======== test pressure =====================
add_executable( testPressure testPressure.cpp )
target_link_libraries( testPressure
	leonlog_static
	Threads::Threads
)
install( TARGETS testPressure RUNTIME DESTINATION testing )
]]

#======== test syslog =====================
add_executable( testSyslog testSyslog.cpp )
target_link_libraries( testSyslog
	LeonUtils
	Threads::Threads
)
install( TARGETS testSyslog RUNTIME DESTINATION testing )

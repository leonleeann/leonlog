cmake_minimum_required( VERSION 3.16.0 )

project( test-log LANGUAGES CXX )

#======== obj libs =====================
add_library( objCommon OBJECT
	${OCTOPUS_CODE_BASE}/common/chrono/Chrono.cpp
	${OCTOPUS_CODE_BASE}/common/misc/Converts.cpp
)

add_library( objTestLog OBJECT
	testLogging.cpp
)

#======== 静态版 =====================
add_executable( s-log )
target_link_libraries( s-log
	objCommon
	objTestLog
	Threads::Threads
	"stdc++fs"
	target_static
)
install( TARGETS s-log RUNTIME DESTINATION testing )

#======== 动态版 =====================
add_executable( d-log )
target_link_libraries( d-log
	objCommon
	objTestLog
	Threads::Threads
	"stdc++fs"
	target_dynmic
)
install( TARGETS d-log RUNTIME DESTINATION testing )

#======== test forking =====================
add_executable( testForking testForking.cpp )
target_link_libraries( testForking
	objCommon
	Threads::Threads
	"stdc++fs"
	target_static
)
install( TARGETS testForking RUNTIME DESTINATION testing )

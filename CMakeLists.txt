cmake_minimum_required( VERSION 3.16.0 )

set( PROJECT_VERSION_MAJOR 1 )   # 主版本
set( PROJECT_VERSION_MINOR 7 )   # 次版本
set( PROJECT_VERSION_PATCH 2 )   # 修订号(Bug fix)

# 版本全串
set( PROJECT_VERSION
	${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

# 版本说明
set( PROJECT_VERSION_NOTES "关闭的同时可以改名日志文件" )

# 项目描述
set( PROJECT_DESCRIPTION   "线程安全的异步文件日志库" )

project( LeonLog
	VERSION     ${PROJECT_VERSION}
	DESCRIPTION ${PROJECT_DESCRIPTION}
	LANGUAGES CXX
)
set( OCTOPUS_CODE_BASE $ENV{OCTOPUS_CODE_BASE} )
include( ${OCTOPUS_CODE_BASE}/common/OctopusCommon.cmake )
configure_file( ${OCTOPUS_CODE_BASE}/common/version/Version.hpp.in Version.hpp )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

#======== obj libs =====================
add_library( objLeonExt			OBJECT
	${OCTOPUS_CODE_BASE}/common/chrono/Chrono.cpp
#	${OCTOPUS_CODE_BASE}/common/misc/Algorithms.cpp
	${OCTOPUS_CODE_BASE}/common/misc/Converts.cpp
# 	${OCTOPUS_CODE_BASE}/common/misc/Exceptions.cpp
# 	${OCTOPUS_CODE_BASE}/common/misc/FileSystem.cpp
# 	${OCTOPUS_CODE_BASE}/common/misc/Signals.cpp
)

######## 主要程序 ###############################################################
#======== 静态版 =====================
add_library( target_static STATIC LogToFile.cpp )
set_target_properties(  target_static PROPERTIES
	OUTPUT_NAME_RELEASE  LeonLog
	OUTPUT_NAME_DEBUG    LeonLog
	PUBLIC_HEADER        LeonLog.hpp
	VERSION              ${PROJECT_VERSION}
	SUFFIX               ".a.${PROJECT_VERSION}"
#	ARCHIVE_OUTPUT_NAME_RELEASE  "ARCHIVE_OUTPUT_NAME_.${PROJECT_VERSION}"
#	LIBRARY_OUTPUT_NAME_RELEASE  "LIBRARY_OUTPUT_NAME_.${PROJECT_VERSION}"
)
target_link_libraries( target_static objLeonExt Threads::Threads )
install( TARGETS target_static
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

#======== 动态版 =====================
add_library( target_dynmic SHARED LogToFile.cpp )
set_target_properties( target_dynmic PROPERTIES
	OUTPUT_NAME_RELEASE    LeonLog
	OUTPUT_NAME_DEBUG      LeonLog
	PUBLIC_HEADER          ThreadName.hpp
	VERSION                ${PROJECT_VERSION}
	SOVERSION              ${PROJECT_VERSION_MAJOR}
)
target_link_libraries( target_dynmic objLeonExt Threads::Threads )
install( TARGETS target_dynmic
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

######## 单元测试 ###############################################################
add_subdirectory( testing )

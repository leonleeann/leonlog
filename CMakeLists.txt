cmake_minimum_required( VERSION 3.16.0 )

set( PROJECT_VERSION_MAJOR 1 )   # 主版本
set( PROJECT_VERSION_MINOR 7 )   # 次版本
set( PROJECT_VERSION_PATCH 5 )   # 修订号(Bug fix)

# 版本全串
set( PROJECT_VERSION
	${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

# 版本说明
set( PROJECT_VERSION_NOTES "Bugfix_避免删除/dev/null" )

# 项目描述
set( PROJECT_DESCRIPTION   "线程安全的异步文件日志库" )

project( LeonLog
	VERSION     ${PROJECT_VERSION}
	DESCRIPTION ${PROJECT_DESCRIPTION}
	LANGUAGES CXX
)

# 用环境变量指定octopus源码树不好,因为会影响全系统所有项目(包括new_arch)。
# set( OCTOPUS_CODE_BASE $ENV{OCTOPUS_CODE_BASE} )
set( OCTOPUS_CODE_BASE "/data/projects/producing" )
message( STATUS "要用生产环境的源码树!!!:\"${OCTOPUS_CODE_BASE}\"" )
configure_file( ${OCTOPUS_CODE_BASE}/common/version/Version.hpp.in Version.hpp )
include( ${OCTOPUS_CODE_BASE}/common/OctopusCommon.cmake )
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
set_target_properties( target_static PROPERTIES
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

#[[
get_target_property( MY_NAME target_static ARCHIVE_OUTPUT_NAME_RELEASE )
message( STATUS "ARCHIVE_OUTPUT_NAME_RELEASE:\"${MY_NAME}\"" )
get_target_property( MY_NAME target_static LIBRARY_OUTPUT_NAME_RELEASE )
message( STATUS "LIBRARY_OUTPUT_NAME_RELEASE:\"${MY_NAME}\"" )
get_property( MY_NAME TARGET target_static PROPERTY OUTPUT_NAME )
message( STATUS "OUTPUT_NAME:\"${MY_NAME}\"" )
]]
get_target_property( MY_NAME target_static OUTPUT_NAME_RELEASE )
set( MY_NAME "lib${MY_NAME}.a" )
message( STATUS "MY_NAME:\"${MY_NAME}\"" )
install( CODE "execute_process(
	COMMAND ln -fs ${MY_NAME}.${PROJECT_VERSION} ${MY_NAME}
	WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR} )" )
# get_target_property( MY_PATH target_static ARCHIVE_OUTPUT_DIRECTORY_RELEASE )
# message( STATUS "MY_PATH:\"${CMAKE_INSTALL_LIBDIR}\"" )

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

cmake_minimum_required( VERSION 3.16.0 )

set( PROJECT_VERSION_MAJOR 1 )   # 主版本
set( PROJECT_VERSION_MINOR 8 )   # 次版本
set( PROJECT_VERSION_PATCH 1 )   # 修订号(Bug fix)

# 版本全串
set( PROJECT_VERSION
	${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

# 版本说明
set( PROJECT_VERSION_NOTES "适配leonutils" )

# 项目描述
set( PROJECT_DESCRIPTION   "线程安全的异步文件日志库" )

project( LeonLog
	VERSION     ${PROJECT_VERSION}
	DESCRIPTION ${PROJECT_DESCRIPTION}
	LANGUAGES CXX
)

include( CheckCXXCompilerFlag )
check_cxx_compiler_flag( -Wall HAVE_WALL )
if ( HAVE_WALL )
	# gcc 警告全开
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )
endif()

# 采用C++标准:20
set( CMAKE_CXX_STANDARD 20 )

set( CMAKE_POSITION_INDEPENDENT_CODE ON )
# 遵循GNU文件分布约定(如:库文件输出至/usr/lib之下)
include( GNUInstallDirs )

set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )
find_package( GTest REQUIRED )

configure_file( Version.hpp.in Version.hpp )
set( CMAKE_INCLUDE_CURRENT_DIR ON )
include_directories( "." )

######## 主要程序 ###############################################################
#======== obj libs =====================
add_library( objCommon OBJECT LogToFile.cpp )

#======== 静态版 =====================
add_library( leonlog_static STATIC )
set_target_properties( leonlog_static PROPERTIES
	OUTPUT_NAME_RELEASE  LeonLog
	OUTPUT_NAME_DEBUG    LeonLog
	PUBLIC_HEADER        LeonLog.hpp
	VERSION              ${PROJECT_VERSION}
	SUFFIX               ".a.${PROJECT_VERSION}"
#	ARCHIVE_OUTPUT_NAME_RELEASE  "ARCHIVE_OUTPUT_NAME_.${PROJECT_VERSION}"
#	LIBRARY_OUTPUT_NAME_RELEASE  "LIBRARY_OUTPUT_NAME_.${PROJECT_VERSION}"
)
target_link_libraries( leonlog_static objCommon LeonUtils Threads::Threads )
install( TARGETS leonlog_static
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

#[[
get_target_property( MY_NAME leonlog_static ARCHIVE_OUTPUT_NAME_RELEASE )
message( STATUS "ARCHIVE_OUTPUT_NAME_RELEASE:\"${MY_NAME}\"" )
get_target_property( MY_NAME leonlog_static LIBRARY_OUTPUT_NAME_RELEASE )
message( STATUS "LIBRARY_OUTPUT_NAME_RELEASE:\"${MY_NAME}\"" )
get_property( MY_NAME TARGET leonlog_static PROPERTY OUTPUT_NAME )
message( STATUS "OUTPUT_NAME:\"${MY_NAME}\"" )
]]
get_target_property( MY_NAME leonlog_static OUTPUT_NAME_RELEASE )
set( MY_NAME "lib${MY_NAME}.a" )
message( STATUS "MY_NAME:\"${MY_NAME}\"" )
install( CODE "execute_process(
	COMMAND ln -fs ${MY_NAME}.${PROJECT_VERSION} ${MY_NAME}
	WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR} )" )
# get_target_property( MY_PATH leonlog_static ARCHIVE_OUTPUT_DIRECTORY_RELEASE )
# message( STATUS "MY_PATH:\"${CMAKE_INSTALL_LIBDIR}\"" )

#======== 动态版 =====================
add_library( leonlog_dynmic SHARED )
set_target_properties( leonlog_dynmic PROPERTIES
	OUTPUT_NAME_RELEASE    LeonLog
	OUTPUT_NAME_DEBUG      LeonLog
	PUBLIC_HEADER          ThreadName.hpp
	VERSION                ${PROJECT_VERSION}
	SOVERSION              ${PROJECT_VERSION_MAJOR}
)
target_link_libraries( leonlog_dynmic objCommon LeonUtils Threads::Threads )
install( TARGETS leonlog_dynmic
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

######## 单元测试 ###############################################################
add_subdirectory( tests )

cmake_minimum_required( VERSION 3.16.0 )

set( PROJECT_VERSION_MAJOR 1 )   # 主版本
set( PROJECT_VERSION_MINOR 9 )   # 次版本
set( PROJECT_VERSION_PATCH 3 )   # 修订号(Bug fix)

# 版本全串
set( PROJECT_VERSION
	${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

# 版本说明
set( PROJECT_VERSION_NOTES "适配leonUtils_0_3_4" )

# 项目描述
set( PROJECT_DESCRIPTION   "线程安全的异步文件日志库" )

project( LeonLog
	VERSION     ${PROJECT_VERSION}
	DESCRIPTION ${PROJECT_DESCRIPTION}
	LANGUAGES CXX
)
configure_file( Version.hpp.in Version.hpp )

######## 全局配置 ###############################################################
# 采用C++标准:20
set( CMAKE_CXX_STANDARD 20 )

# 遵循GNU文件分布约定(如:库文件输出至/usr/lib之下)
include( GNUInstallDirs )

include( CheckCXXCompilerFlag )
check_cxx_compiler_flag( -Wall HAVE_WALL )
if ( HAVE_WALL )
	# gcc 警告全开
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )
endif()

set( CMAKE_POSITION_INDEPENDENT_CODE ON )
set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )
find_package( GTest REQUIRED )

# 对任何头文件的搜索都可 以本目录为根开始
set( CMAKE_INCLUDE_CURRENT_DIR ON )
include_directories( "." )
include_directories( "include" )

######## 主要模块 ###############################################################
add_library( leonutl_static STATIC IMPORTED )
set_property( TARGET leonutl_static PROPERTY
	IMPORTED_LOCATION "/usr/local/lib/libLeonUtils.a.0.3.4" )
add_library( objCommon OBJECT src/LogToFile.cpp )

######## 主要产出 ###############################################################
#======== 静态版 =====================
add_library( leonlog_static STATIC )
set_target_properties( leonlog_static PROPERTIES
	OUTPUT_NAME_RELEASE  LeonLog
	OUTPUT_NAME_DEBUG    LeonLog
#	PUBLIC_HEADER        LeonLog.hpp
	VERSION              ${PROJECT_VERSION}
	SUFFIX               ".a.${PROJECT_VERSION}"
#	ARCHIVE_OUTPUT_NAME_RELEASE  "ARCHIVE_OUTPUT_NAME_.${PROJECT_VERSION}"
#	LIBRARY_OUTPUT_NAME_RELEASE  "LIBRARY_OUTPUT_NAME_.${PROJECT_VERSION}"
)
target_sources( leonlog_static PUBLIC FILE_SET HEADERS BASE_DIRS "include"
FILES
	include/leonlog/LeonLog.hpp
	include/leonlog/StatusFile.hpp
	include/leonlog/ThreadName.hpp
)
target_link_libraries( leonlog_static objCommon leonutl_static Threads::Threads )
install( TARGETS leonlog_static
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILE_SET HEADERS
)

#======== 动态版 =====================
add_library( leonlog_dynmic SHARED )
set_target_properties( leonlog_dynmic PROPERTIES
	OUTPUT_NAME_RELEASE    LeonLog
	OUTPUT_NAME_DEBUG      LeonLog
#	PUBLIC_HEADER          ThreadName.hpp
	VERSION                ${PROJECT_VERSION}
	SOVERSION              ${PROJECT_VERSION_MAJOR}
)
target_link_libraries( leonlog_dynmic objCommon leonutl_static Threads::Threads )
install( TARGETS leonlog_dynmic
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

######## 单元测试 ###############################################################
add_subdirectory( tests )

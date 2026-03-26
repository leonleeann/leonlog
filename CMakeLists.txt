cmake_minimum_required( VERSION 3.25.0 )

set( PROJECT_VERSION_MAJOR 1 )   # 主版本
set( PROJECT_VERSION_MINOR 10 )  # 次版本
set( PROJECT_VERSION_PATCH 10 )   # 修订号(Bug fix)

# 版本全串
set( PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} )

# 版本说明
set( PROJECT_VERSION_NOTES	"适配leonutils_0_3_28" )

# 项目描述
set( PROJECT_DESCRIPTION	"线程安全的异步文件日志库" )

project( LeonLog
	VERSION		${PROJECT_VERSION}
	DESCRIPTION	${PROJECT_DESCRIPTION}
	LANGUAGES	CXX
)

# 版本头文件应输出回源码目录(而非构建目录), 因为本项目有可能会被纳入其它项目统一编译
set( VERSION_H "LeonLogVer.hpp" )
# message( STATUS "VER_FILE_IN_BUILD:\"${CMAKE_BINARY_DIR}/${VERSION_H}.in\"" )
# message( STATUS "VER_FILE_IN_SOURC:\"${CMAKE_SOURCE_DIR}/include/leonlog/${VERSION_H}\"" )
configure_file( "${VERSION_H}.in" ${CMAKE_SOURCE_DIR}/include/leonlog/${VERSION_H} )

######## 全局配置 ###############################################################
# 采用C++标准:20
set( CMAKE_CXX_STANDARD 20 )
# LTO 优化
set( CMAKE_POSITION_INDEPENDENT_CODE ON )
set( CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE )

# 遵循GNU文件分布约定(如:库文件输出至/usr/lib之下)
include( GNUInstallDirs )

include( CheckCXXCompilerFlag )
check_cxx_compiler_flag( -Wall HAVE_WALL )
if ( HAVE_WALL )
	# gcc 警告全开
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )
endif()

set( THREADS_PREFER_PTHREAD_FLAG ON )
find_package( Threads REQUIRED )
find_package( GTest REQUIRED )

# 对任何头文件的搜索都可 以本目录为根开始
set( CMAKE_INCLUDE_CURRENT_DIR ON )
include_directories( "${CMAKE_CURRENT_BINARY_DIR}" )
# 不知道下面这句与上面一句有何区别
include_directories( "." )	# 这不是本文件的目录！是包含者文件的目录！！！
include_directories( "include" )
get_filename_component( LEONUTL_CODE_BASE "../leonutils" ABSOLUTE )
include_directories( "${LEONUTL_CODE_BASE}/include" )

######## 主要模块 ###############################################################
add_library( objCommon OBJECT src/LogToFile.cpp )

######## 主要产出 ###############################################################
#[[======== 静态版 ==============================================================
add_library( leonlog_static STATIC )
set_target_properties( leonlog_static PROPERTIES
	OUTPUT_NAME_RELEASE		LeonLog
	OUTPUT_NAME_DEBUG		LeonLog
	VERSION					${PROJECT_VERSION}
#	SUFFIX					".a.${PROJECT_VERSION}"
#	ARCHIVE_OUTPUT_NAME_RELEASE  "ARCHIVE_OUTPUT_NAME_.${PROJECT_VERSION}"
#	LIBRARY_OUTPUT_NAME_RELEASE  "LIBRARY_OUTPUT_NAME_.${PROJECT_VERSION}"
	ENABLE_EXPORTS			TRUE	# CMake 3.4 之后必须显示指定, 确保策略需要调用的函数存在
)
target_link_libraries( leonlog_static PUBLIC objCommon LeonUtils PRIVATE Threads::Threads )
install( TARGETS leonlog_static
	ARCHIVE			DESTINATION	${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER	DESTINATION	${CMAKE_INSTALL_INCLUDEDIR}
)
# 自己创建软链(CMake不会帮 静态库 做这个, 只会为 *.so 做)
install( CODE "execute_process(
	COMMAND ${CMAKE_COMMAND} -E create_symlink
		$<TARGET_FILE_NAME:leonlog_static>
		$<TARGET_FILE_PREFIX:leonlog_static>$<TARGET_FILE_BASE_NAME:leonlog_static>.a
	WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/lib )" )]]

#======== 动态版 ================================================================
add_library( leonlog_dynmic SHARED )
set_target_properties( leonlog_dynmic PROPERTIES
	OUTPUT_NAME_RELEASE		LeonLog
	OUTPUT_NAME_DEBUG		LeonLog
	VERSION					${PROJECT_VERSION}
	SOVERSION				${PROJECT_VERSION_MAJOR}
)
#[[target_sources( leonlog_dynmic PUBLIC FILE_SET HEADERS BASE_DIRS "include" FILES
	include/leonlog/LeonLog.hpp
	include/leonlog/LeonLogVer.hpp
	include/leonlog/LogSet.hpp
	include/leonlog/StatusFile.hpp
	include/leonlog/ThreadName.hpp
)]]
target_link_libraries( leonlog_dynmic PUBLIC objCommon LeonUtils Threads::Threads )
install( TARGETS leonlog_dynmic
	ARCHIVE			DESTINATION	${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER	DESTINATION	${CMAKE_INSTALL_INCLUDEDIR}
	FILE_SET		HEADERS )

######## 单元测试 ###############################################################
add_subdirectory( tests )

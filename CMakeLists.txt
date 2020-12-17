cmake_minimum_required( VERSION 3.0.0 )

set( PROJECT_VERSION_MAJOR 1 )   # 主版本
set( PROJECT_VERSION_MINOR 5 )   # 次版本
set( PROJECT_VERSION_PATCH 0 )   # 修订号(Bug fix)

# 版本全串
set( PROJECT_VERSION
   ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

# 版本说明
set( PROJECT_VERSION_NOTES "重新加上“线程名到pthread_id”的映射" )

# 项目描述
set( PROJECT_DESCRIPTION   "线程安全的异步文件日志库" )

project( LeonLog
   VERSION     ${PROJECT_VERSION}
   DESCRIPTION ${PROJECT_DESCRIPTION}
   LANGUAGES CXX
)
set( OCTOPUS_CODE_BASE $ENV{OCTOPUS_CODE_BASE} )
include( ${OCTOPUS_CODE_BASE}/common/OctopusCommon.cmake )
configure_file( ${OCTOPUS_CODE_BASE}/common/misc/Version.hpp.in Version.hpp )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

######## 主要程序 ###############################################################
#======== 静态版 =====================
add_library( leonlog_static STATIC LogToFile.cpp )
set_target_properties(  leonlog_static PROPERTIES
   OUTPUT_NAME_RELEASE  LeonLog
   OUTPUT_NAME_DEBUG    LeonLog
   PUBLIC_HEADER        LeonLog.hpp
   VERSION              ${PROJECT_VERSION}
   SUFFIX               ".a.${PROJECT_VERSION}"
#  ARCHIVE_OUTPUT_NAME_RELEASE  "ARCHIVE_OUTPUT_NAME_.${PROJECT_VERSION}"
#  LIBRARY_OUTPUT_NAME_RELEASE  "LIBRARY_OUTPUT_NAME_.${PROJECT_VERSION}"
)
target_link_libraries( leonlog_static Threads::Threads )
install( TARGETS leonlog_static
   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
   PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

#======== 动态版 =====================
add_library( leonlog_shared SHARED LogToFile.cpp )
set_target_properties(  leonlog_shared PROPERTIES
   OUTPUT_NAME_RELEASE  LeonLog
   OUTPUT_NAME_DEBUG    LeonLog
   PUBLIC_HEADER        ThreadName.hpp
   VERSION              ${PROJECT_VERSION}
   SOVERSION            ${PROJECT_VERSION_MAJOR}
)
target_link_libraries( leonlog_shared Threads::Threads )
install( TARGETS leonlog_shared
   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
   PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

######## 单元测试 ###############################################################
# add_subdirectory( testing )

cmake_minimum_required( VERSION 3.0.0 )

set( PROJECT_VERSION_MAJOR 1 )   # 主版本
set( PROJECT_VERSION_MINOR 4 )   # 次版本
set( PROJECT_VERSION_PATCH 4 )   # 修订号(Bug fix)

# 版本全串
set( PROJECT_VERSION
   ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

# 版本说明
set( PROJECT_VERSION_NOTES "新的处理pthread的cmake配置及选项" )

# 项目描述
set( PROJECT_DESCRIPTION   "线程安全的异步文件日志库" )

project( leonlog
   VERSION     ${PROJECT_VERSION}
   DESCRIPTION ${PROJECT_DESCRIPTION}
   LANGUAGES CXX
)
set( OCTOPUS_CODE_BASE $ENV{OCTOPUS_CODE_BASE} )
include( ${OCTOPUS_CODE_BASE}/common/OctopusCommon.cmake )
configure_file( ${OCTOPUS_CODE_BASE}/common/misc/Version.hpp.in Version.hpp )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

######## 各种库 #################################################################
#======== obj libs =====================
# add_library( objlib_log2file OBJECT log2file.cpp )

######## 主要程序 ###############################################################
#[[======== 静态版 =====================
add_library( leonlog_static STATIC log2file.cpp )
set_target_properties(  leonlog_static PROPERTIES
   OUTPUT_NAME_RELEASE  leonlog
   OUTPUT_NAME_DEBUG    leonlog
   PUBLIC_HEADER        leonlog.hpp
   VERSION              ${PROJECT_VERSION}
   SUFFIX               ".a.${PROJECT_VERSION}"
#    ARCHIVE_OUTPUT_NAME_RELEASE  "ARCHIVE_OUTPUT_NAME_.${PROJECT_VERSION}"
#    LIBRARY_OUTPUT_NAME_RELEASE  "LIBRARY_OUTPUT_NAME_.${PROJECT_VERSION}"
)
target_link_libraries( leonlog_static Threads::Threads )
install( TARGETS leonlog_static
   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
   PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

#======== 动态版 =====================
add_library( leonlog_shared SHARED log2file.cpp )
set_target_properties(  leonlog_shared PROPERTIES
   OUTPUT_NAME_RELEASE  leonlog
   OUTPUT_NAME_DEBUG    leonlog
   PUBLIC_HEADER        leonlog.hpp
   VERSION              ${PROJECT_VERSION}
   SOVERSION            ${PROJECT_VERSION_MAJOR}
)
target_link_libraries( leonlog_shared Threads::Threads )
install( TARGETS leonlog_shared
   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
   PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
]]

######## 单元测试 ###############################################################
add_subdirectory( testing )

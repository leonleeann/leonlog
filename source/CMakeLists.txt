cmake_minimum_required(VERSION 3.0.0)

set( PROJECT_VERSION_MAJOR 1 )   # 主版本
set( PROJECT_VERSION_MINOR 3 )   # 次版本
set( PROJECT_VERSION_PATCH 1 )   # 修订号(Bug fix)

# 完整版本串
set( PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} )

# 版本说明
set( PROJECT_VERSION_NOTES "纠正头文件内wrapper函数名字笔误" )

# 项目描述
set( PROJECT_DESCRIPTION   "线程安全的异步文件日志库" )

project( leonlog
   VERSION     ${PROJECT_VERSION}
   DESCRIPTION ${PROJECT_DESCRIPTION}
   LANGUAGES CXX
)
configure_file( ${LEON_CODE_BASE}/misc/Version.hpp.in Version.hpp )

OPTION( BUILD_SHARED_LIBS "默认输出静态库" OFF )

set( CMAKE_INCLUDE_CURRENT_DIR ON )

##### libleonlog.a ###########################################################
add_library( leonlog STATIC log2file.cpp )

set_target_properties( leonlog PROPERTIES
   VERSION        ${PROJECT_VERSION}
   SOVERSION      ${PROJECT_VERSION_MAJOR}
   PUBLIC_HEADER  leonlog.hpp
)

message( STATUS "CMAKE_INSTALL_LIBDIR:\"${CMAKE_INSTALL_LIBDIR}\"" )
message( STATUS "CMAKE_INSTALL_INCLUDEDIR:\"${CMAKE_INSTALL_INCLUDEDIR}\"" )
install( TARGETS leonlog
   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
   PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

##### libleonlog.so ##########################################################

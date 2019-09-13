cmake_minimum_required(VERSION 3.0.0)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag( -Wall HAVE_WALL )
if ( HAVE_WALL )
# gcc 警告全开
   set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )
endif()
# 采用C++标准:20
set( CMAKE_CXX_STANDARD 20 )

# OPTION( BUILD_SHARED_LIBS "默认输出静态库" OFF )

# 遵循GNU文件分布约定(如:库文件输出至/usr/lib之下)
include( GNUInstallDirs )

# find_package( GTest REQUIRED )
# message( STATUS "GTEST_INCLUDE_DIRS:\"${GTEST_INCLUDE_DIRS}\"" )
# message( STATUS "GTEST_BOTH_LIBRARIES:\"${GTEST_BOTH_LIBRARIES}\"" )
set( GTEST_INCLUDE_DIRS "/usr/local/include/gtest" )
set( GMOCK_INCLUDE_DIRS "/usr/local/include/gmock" )
set( GTEST_BOTH_LIBRARIES "/usr/local/lib/libgtest.a;/usr/local/lib/libgtest_main.a" )
set( GMOCK_BOTH_LIBRARIES "/usr/local/lib/libgmock.a;/usr/local/lib/libgmock_main.a" )

set( LEON_CODE_BASE "/home/leon/projects/octopus/common" )
include_directories( ${LEON_CODE_BASE} )

# 线程库基本上每个子项目都会用到,这里统一找了吧
find_package( Threads REQUIRED )

# config++将采用静态链接, 此处不必找了
# find_package( libconfig++ REQUIRED )

# 根据CMAKE_BUILD_TYPE重新定义DEBUG及NDEBUG
string( TOUPPER ${CMAKE_BUILD_TYPE} build_type_str )
if( ( build_type_str STREQUAL "DEBUG" )
      OR ( build_type_str STREQUAL "RELWITHDEBINFO" ) )
   add_definitions( -DDEBUG )
   remove_definitions( -DNDEBUG )	# 不起作用
else( )
   add_definitions( -DNDEBUG )
   remove_definitions( -DDEBUG )
endif( )

# add_subdirectory( source )
add_subdirectory( testing )

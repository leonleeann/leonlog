// 项目全局配置，各项值由CMake每次build时自动填入，请勿手动修改本文件！

#pragma once

#define PROJECT_VERSION_MAJOR (1)
#define PROJECT_VERSION_MINOR (10)
#define PROJECT_VERSION_PATCH (9)
#define PROJECT_RELEASE_NOTES ("")
#define PROJECT_VERSION       ("1.10.9")
#define PROJECT_DESCRIPTION   ("线程安全的异步文件日志库")

#define APP_NAME ("")
#define APP_FILE ("")

#define CMAKE_BUILD_TYPE ( "Release" )
#define CMAKE_CXX_FLAGS ( "" )

#ifdef DEBUG
#undef NDEBUG
#else
#ifdef NDEBUG
#undef DEBUG
#endif
#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 

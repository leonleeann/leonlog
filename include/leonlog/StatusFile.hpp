#pragma once
#include <functional>
#include <iosfwd>

namespace leon_log {

using str_t = std::string;

using WriteStatus_f = std::function<void( std::ostream& )>;

// 每隔 n 秒输出一次状态
extern "C" void SetStatus( const str_t&		to_file,	// 输出文件全路径及全名
						   WriteStatus_f	writer,		// 输出状态内容的回调函数
						   int				intrvl		// 输出时间间隔
						 );

};	// namespace leon_log

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;

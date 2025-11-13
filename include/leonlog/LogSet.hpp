#pragma once
#include <leonlog/LeonLog.hpp>
#include <set>

template<typename K, typename C = std::less<K>, typename A = std::allocator<K> >
using set_t = std::set<K, C, A>;

// 好像运算符重载要放入 leon_log::Log_t 所在的命名空间才能匹配到
namespace leon_log {

template<typename K, typename C = std::less<K>, typename A = std::allocator<K> >
Log_t& operator<<( Log_t& lg_, const set_t<K, C, A>& set_ ) {
	lg_ << '{';

	for( auto& v : set_ )
		lg_ << v << ',';

	lg_ << '}';
	return lg_;
};

}; // namespace leon_log

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;

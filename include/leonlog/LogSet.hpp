#pragma once
#include <leonlog/LeonLog.hpp>
#include <set>
#include <string>

template<typename K, typename C = std::less<K>, typename A = std::allocator<K> >
using set_t = std::set<K, C, A>;

using str_t = std::string;
using StrSet_t = set_t<str_t>;
using IntSet_t = set_t<int>;

namespace leon_log {

template<typename K, typename C = std::less<K>, typename A = std::allocator<K> >
Log_t& operator<<( Log_t& lg_, const set_t<K, C, A>& ss_ ) {

	lg_ << '{';

	for( auto& s : ss_ )
		lg_ << s << ',';

	lg_ << '}';
	return lg_;
};

}; // namespace leon_log

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;

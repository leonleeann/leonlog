#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <leonlog/LeonLog.hpp>
#include <sstream>

using namespace leon_log;
using namespace std;
using oss_t = std::ostringstream;
using ost_t = std::ostream;
using stv_t = std::string_view;

struct Custom_t {
	str_t _data;
};
ost_t& operator<<( ost_t& os_, const Custom_t& cd_ ) {
	return os_ << cd_._data;
};

namespace leon_log {

LogLevel_e	g_log_level = LogLevel_e::Debug;
str_t		s_log_buf;

// 这是 AppendLog 的 fake
extern "C" bool AppendLog( LogLevel_e, const str_t& body_ ) {

	s_log_buf = body_;
	return true;
};

/*
// 试试 U64_u 能不能输出
ost_t& operator<<( ost_t& os_, leon_utl::U64_u u_ ) {
	return os_ << u_.view();
};

Log_t& operator<<( Log_t& log_, leon_utl::U64_u u_ ) {
	if( log_._level < leon_log::g_log_level )
		return log_;

	stv_t svw = u_.view();
	log_ << svw;
	return log_;
}; */

TEST( TestLog, withU64View ) {
	s_log_buf.clear();
	leon_utl::U64_u u64 { "UStrView" };
	lg_debg << u64;
	ASSERT_EQ( s_log_buf, u64.str() );
};

TEST( TestLog, withString ) {
	s_log_buf.clear();
	lg_debg << "str123";
	ASSERT_EQ( s_log_buf, "str123" );

	str_t str456 { "str456" };
	s_log_buf.clear();
	lg_debg << str456;
	ASSERT_EQ( s_log_buf, str456 );

	char* str_null { nullptr };
	s_log_buf.clear();
	lg_debg << str_null;
	// 如果把 operator<<函数 放到 leon_log 空间内, 这里就能得到期望输出 "{null-char*}"
	ASSERT_EQ( s_log_buf, "{null-char*}" );

	s_log_buf.clear();
	{ Log_t lg{Debug}; lg << str_null; }
	ASSERT_EQ( s_log_buf, "{null-char*}" );
};

TEST( TestLog, withCustomizedType ) {
	s_log_buf.clear();
	Custom_t cd { ._data = "cust567" };
	lg_debg << cd;
	ASSERT_EQ( s_log_buf, "cust567" );

	cd._data = "cust678";
	Custom_t* pcd = &cd;
	s_log_buf.clear();
	lg_debg << *pcd;
	ASSERT_EQ( s_log_buf, "cust678" );
};

TEST( TestLog, withPointers ) {
	Custom_t cd { ._data = "cust8888" };
	Custom_t* pcd = &cd;

	s_log_buf.clear();
	lg_debg << pcd;
	ASSERT_EQ( s_log_buf, "cust8888" );

	s_log_buf.clear();
	pcd = nullptr;
	lg_debg << pcd;
	ASSERT_EQ( s_log_buf, "{nullptr}" );

	s_log_buf.clear();
	lg_debg << nullptr;
	ASSERT_EQ( s_log_buf, "nullptr" );

	s_log_buf.clear();
	void* vptr = nullptr;
	lg_debg << vptr;
	// 如果把 operator<<函数 放到 leon_log 空间内, 这里就能得到期望输出 "{null-void*}"
	ASSERT_EQ( s_log_buf, "{null-void*}" );

	s_log_buf.clear();
	{ Log_t lg{Debug}; lg << vptr; }
	ASSERT_EQ( s_log_buf, "{null-void*}" );
};

}; // namespace leon_log

// 在命名空间之外,再试试
TEST( TestLogOutside, overloadingCustoms ) {
	char* str_null { nullptr };
	s_log_buf.clear();
	lg_debg << str_null;
	ASSERT_EQ( s_log_buf, "{null-char*}" );

	s_log_buf.clear();
	{ Log_t lg{Debug}; lg << str_null; }
	ASSERT_EQ( s_log_buf, "{null-char*}" );

	s_log_buf.clear();
	void* vptr = nullptr;
	lg_debg << vptr;
	ASSERT_EQ( s_log_buf, "{null-void*}" );

	s_log_buf.clear();
	{ Log_t lg{Debug}; lg << vptr; }
	ASSERT_EQ( s_log_buf, "{null-void*}" );

	s_log_buf.clear();
	leon_utl::U64_u u64 { "UStrView" };
	lg_debg << u64;
	ASSERT_EQ( s_log_buf, u64.str() );
};

GTEST_API_ int main( int argc, char** argv ) {

	testing::InitGoogleTest( &argc, argv );

	return RUN_ALL_TESTS();
};

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;

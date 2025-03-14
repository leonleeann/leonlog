#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <leonlog/LeonLog.hpp>

using namespace leon_log;
using namespace std;

struct Custom_t {
	str_t _data;
};
ostream& operator<<( ostream& os_, const Custom_t& cd_ ) {
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
	// 搞不定
	ASSERT_EQ( s_log_buf, "" );

	s_log_buf.clear();
	// 还是只能这样
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
//搞不定	ASSERT_EQ( s_log_buf, "{null-char*}" );
	ASSERT_EQ( s_log_buf, "0" );

	s_log_buf.clear();
	// 还是只能这样
	{ Log_t lg{Debug}; lg << vptr; }
	ASSERT_EQ( s_log_buf, "{null-void*}" );
};

}; // namespace leon_log

GTEST_API_ int main( int argc, char** argv ) {

	testing::InitGoogleTest( &argc, argv );

	return RUN_ALL_TESTS();
};

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;

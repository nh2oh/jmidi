#include "gtest/gtest.h"
#include "mthd_t.h"
#include "mthd_test_data.h"
#include <array>
#include <cstdint>



//
// Tests for:
// mthd_t();
//
// The default ctor should create a format 1 file w/ 0 trks and
// a division field => 120 tpq
//
TEST(mthd_tests, defaultCtor) {
	std::array<unsigned char,14> ans {
		0x4Du,0x54u,0x68u,0x64u, 0x00u,0x00u,0x00u,0x06u,
		0x00u,0x01u, 0x00u,0x00u, 0x00u,0x78u
	};
	auto tdiv_ans = jmid::time_division_t(120);

	auto mthd = jmid::mthd_t();
	EXPECT_EQ(mthd.size(), ans.size());
	EXPECT_EQ(mthd.nbytes(), ans.size());
	
	int i=0;
	for (const auto& b : mthd) {
		EXPECT_EQ(b,ans[i++]);
	}

	EXPECT_EQ(mthd.length(),6);
	
	EXPECT_EQ(mthd.format(),1);
	EXPECT_EQ(mthd.ntrks(),0);
	EXPECT_EQ(mthd.division(),tdiv_ans);
	EXPECT_EQ(mthd.division().get_tpq(),tdiv_ans.get_tpq());
	EXPECT_EQ(mthd.division().get_raw_value(),tdiv_ans.get_raw_value());	
}


//
// mthd_t::set_length()
// The smallest value allowed is 6.  Call the method on the default ctor'd
// object.  
TEST(mthd_tests, setLength) {
	struct test_t {
		int new_length;
		int length_ans;
	};
	std::array<test_t,11> tests {{
		{-1,6},{0,6},{1,6},{2,6},{3,6},{5,6},  // Invalid
		{6,6},{7,7},{16,16},{24,24},{1000,1000}  // Valid
	}};
	auto tdiv_ans = jmid::time_division_t(120);
	int ntrks_ans = 0;
	int format_ans = 1;
	
	for (const auto& tc : tests) {
		auto mthd = jmid::mthd_t();
		auto result_length = mthd.set_length(tc.new_length);
		EXPECT_EQ(result_length,tc.length_ans);
		EXPECT_EQ(tc.length_ans,mthd.length());
		EXPECT_EQ(mthd.size(), tc.length_ans+8);
		EXPECT_EQ(mthd.nbytes(), tc.length_ans+8);

		EXPECT_EQ(mthd.end()-mthd.begin(),mthd.size());

		// The other fields are not disturbed
		EXPECT_EQ(mthd.format(),format_ans);
		EXPECT_EQ(mthd.ntrks(),ntrks_ans);
		EXPECT_EQ(mthd.division(),tdiv_ans);
	}
}


//
// mthd_t::set_length()
// The smallest value allowed is 6.  Repeatedly change the length, forcing
// allocation, deallocation etc.  
TEST(mthd_tests, repeatedlyChangeLength) {
	struct test_t {
		int new_length;
		int length_ans;
	};
	std::array<test_t,14> tests {{
		{-1,6},{1000,1000},{0,6},{16,16},{1,6},{2,6},{5,6},{3,6},
		{6,6},{71,71},{16,16},{24,24},{241,241},{-1,6}
	}};
	auto tdiv_ans = jmid::time_division_t(150);
	int ntrks_ans = 73;
	int format_ans = 2;
	
	auto mthd = jmid::mthd_t(format_ans,ntrks_ans,tdiv_ans);
	for (const auto& tc : tests) {
		auto result_length = mthd.set_length(tc.new_length);
		EXPECT_EQ(result_length,tc.length_ans);
		EXPECT_EQ(tc.length_ans,mthd.length());
		EXPECT_EQ(mthd.size(), tc.length_ans+8);
		EXPECT_EQ(mthd.nbytes(), tc.length_ans+8);

		EXPECT_EQ(mthd.end()-mthd.begin(),mthd.size());

		// The other fields are not disturbed
		EXPECT_EQ(mthd.format(),format_ans);
		EXPECT_EQ(mthd.ntrks(),ntrks_ans);
		EXPECT_EQ(mthd.division(),tdiv_ans);
	}
}


//
// Tests for:
// explicit mthd_t(int32_t fmt, int32_t ntrks, jmid::time_division_t div);
//
// With valid input combinations.  
//
TEST(mthd_tests, fieldValueCtorWithValidValues) {
	for (const auto& tcase : mthd_test::byfieldvalue_valid) {
		jmid::mthd_t mthd = jmid::mthd_t(tcase.format, tcase.ntrks, tcase.division);
		EXPECT_EQ(mthd.size(), 14);
		EXPECT_EQ(mthd.nbytes(), 14);
		EXPECT_EQ(mthd.length(), 6);
		EXPECT_EQ(mthd.format(), tcase.format);
		EXPECT_EQ(mthd.ntrks(), tcase.ntrks);
		auto d = mthd.division();
		EXPECT_TRUE(is_tpq(d));
		EXPECT_FALSE(is_smpte(d));
		EXPECT_EQ(d.get_tpq(), tcase.division);
	}
}

//
// Tests for:
// explicit mthd_t(int32_t fmt, int32_t ntrks, jmid::time_division_t div);
//
// With _invalid_ input combinations.  
//
TEST(mthd_tests, fieldValueCtorWithInvalidValues) {
	for (const auto& tcase : mthd_test::byfieldvalue_invalid) {
		jmid::mthd_t mthd = jmid::mthd_t(tcase.format, tcase.ntrks, tcase.division);
		EXPECT_EQ(mthd.size(), 14);
		EXPECT_EQ(mthd.nbytes(), 14);
		EXPECT_EQ(mthd.length(), 6);
		auto f = mthd.format();
		EXPECT_EQ(mthd.format(), tcase.ans_format);
		EXPECT_EQ(mthd.ntrks(), tcase.ans_ntrks);
		auto d = mthd.division();
		EXPECT_TRUE(is_tpq(d));
		EXPECT_FALSE(is_smpte(d));
		EXPECT_EQ(d.get_tpq(), tcase.ans_division);
	}
}

//
// Tests for:
// maybe_mthd_t make_mthd(const unsigned char *end, const unsigned char *end);
//
// With invalid input data.  
//
TEST(mthd_tests, MakeMthdInvalidInput) {
	for (const auto& tcase : mthd_test::invalid_set_a) {
		auto beg = tcase.data.data();
		auto end = beg + tcase.offset_to_data_end;
		
		{
		jmid::mthd_error_t mthd2_err;
		auto mthd = jmid::make_mthd2(beg,end,&mthd2_err);
		EXPECT_NE(mthd2_err.code,jmid::mthd_error_t::errc::no_error);
		}

		{
		jmid::mthd_t mthd2_result;
		jmid::mthd_error_t mthd2_err;
		auto it_end = jmid::make_mthd2(beg,end,&mthd2_result,&mthd2_err);
		EXPECT_NE(mthd2_err.code,jmid::mthd_error_t::errc::no_error);
		}
	}
}


//
// Tests for:
// maybe_mthd_t make_mthd(const unsigned char *end, const unsigned char *end);
//
// With _valid_ input data
//
TEST(mthd_tests, MakeMthdValidInput) {
	int i=0;
	for (const auto& tcase : mthd_test::valid_set_a) {
		auto beg = tcase.data.data();
		auto end = beg + tcase.offset_to_data_end;
		{
		jmid::mthd_error_t mthd2_err;
		auto mthd = jmid::make_mthd2(beg,end,&mthd2_err);
		EXPECT_EQ(mthd2_err.code,jmid::mthd_error_t::errc::no_error);
		}

		/*auto mthd = jmid::make_mthd(beg,end,nullptr,tcase.offset_to_data_end);
		EXPECT_TRUE(mthd) 
			<< "test case i==" << i 
			<< "; offset_to_data_end==" << tcase.offset_to_data_end;*/

		{
		jmid::mthd_t mthd2_result;
		jmid::mthd_error_t mthd2_err;
		auto it_end = jmid::make_mthd2(beg,end,&mthd2_result,&mthd2_err);
		EXPECT_EQ(mthd2_err.code,jmid::mthd_error_t::errc::no_error);
		}

		/*jmid::mthd_t mthd2_result;
		jmid::mthd_error_t mthd2_err;
		auto it_end = jmid::make_mthd2(beg,end,&mthd2_result,&mthd2_err);
		EXPECT_EQ(mthd2_err.code,jmid::mthd_error_t::errc::no_error) 
			<< "test case i==" << i;*/

		++i;
	}
}


//
// Tests for:
// maybe_mthd_t make_mthd(const unsigned char *end, const unsigned char *end);
//
// With valid, but "unusual" input data observed in the wild
//
TEST(mthd_tests, MakeMthdValidUnusualInput) {
	for (const auto& tcase : mthd_test::valid_unusual_a) {
		auto beg = tcase.data.data();
		auto end = tcase.data.data() + tcase.data.size();
		{
		jmid::mthd_error_t mthd_err;
		auto mthd = jmid::make_mthd2(beg,end,&mthd_err);
		EXPECT_EQ(mthd_err.code,jmid::mthd_error_t::errc::no_error);
		EXPECT_EQ(mthd.length(),tcase.ans_length);
		EXPECT_EQ(mthd.format(),tcase.ans_format);
		EXPECT_EQ(mthd.ntrks(),tcase.ans_ntrks);
		EXPECT_EQ(mthd.division().get_tpq(),tcase.ans_division);
		}

		/*auto mthd = jmid::make_mthd(beg,end,nullptr,tcase.data.size());
		EXPECT_TRUE(mthd);
		EXPECT_EQ(mthd.mthd.length(),tcase.ans_length);
		EXPECT_EQ(mthd.mthd.format(),tcase.ans_format);
		EXPECT_EQ(mthd.mthd.ntrks(),tcase.ans_ntrks);
		EXPECT_EQ(mthd.mthd.division().get_tpq(),tcase.ans_division);*/
		{
		jmid::mthd_t mthd_result;
		jmid::mthd_error_t mthd_err;
		auto mthd = jmid::make_mthd2(beg,end,&mthd_result,&mthd_err);
		EXPECT_EQ(mthd_err.code,jmid::mthd_error_t::errc::no_error);
		EXPECT_EQ(mthd_result.length(),tcase.ans_length);
		EXPECT_EQ(mthd_result.format(),tcase.ans_format);
		EXPECT_EQ(mthd_result.ntrks(),tcase.ans_ntrks);
		EXPECT_EQ(mthd_result.division().get_tpq(),tcase.ans_division);
		}
	}
}


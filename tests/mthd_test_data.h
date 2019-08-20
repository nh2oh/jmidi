#pragma once
#include <array>
#include <vector>
#include <cstdint>

namespace mthd_test {

//
// For a:
// bytes_range_t testcase,
// auto beg = testcase.data.data();
// auto end = beg + testcase.offset_to_data_end;
// Meant to be used as: make_mtrk(beg,end);
//
struct bytes_range_t {
	std::array<unsigned char,14> data;
	uint32_t offset_to_data_end {0};
};
extern std::vector<bytes_range_t> valid_set_a;
extern std::vector<bytes_range_t> invalid_set_a;


//
// Tests set a:  MThd chunks found in the wild w/ "unusual" values.  
// All are valid MThd chunks.  
// 
struct mthd_testset_a_t {
	std::array<unsigned char,14> data;
	int ans_length;
	int ans_format;
	int ans_ntrks;
	int ans_division;
};
extern std::vector<mthd_testset_a_t> valid_unusual_a;


//
// Create by manually specifying format, ntrks, division
// 
struct byfieldvalue_valid_t {
	int format;
	int ntrks;
	int division;
};
extern std::vector<byfieldvalue_valid_t> byfieldvalue_valid;


struct byfieldvalue_invalid_t {
	int format;
	int ntrks;
	int division;
	int ans_format;
	int ans_ntrks;
	int ans_division;
};
extern std::vector<byfieldvalue_invalid_t> byfieldvalue_invalid;

};  // namespace mthd_test


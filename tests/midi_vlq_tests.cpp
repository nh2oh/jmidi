#include "gtest/gtest.h"
#include "midi_vlq.h"
#include "midi_vlq_deprecated.h"
#include <array>
#include <cstdint>
#include <limits>


TEST(midi_vlq_tests, toBEByteOrder) {
	struct test_t {
		uint32_t input;
		uint32_t ans;
	};
	std::array<test_t,6> tests {{
		{0,0},
		{1,16777216},  // 0x00,00,00,01  =>  0x01,00,00,00
		{0x01000000u,0x00000001u},
		{0xFFFFFFFFu,0xFFFFFFFFu},
		{0x000C0000u, 0x00000C00u},
		{0x12345678u, 0x78563412u}
	}};

	for (const auto& tc : tests) {
		auto res = to_be_byte_order(tc.input);
		EXPECT_EQ(res,tc.ans) << "Failed for e.ans==" << tc.ans << "\n";
	}
}

// read_be<uint8_t>(...)
// These test cases are the same as for the ui32 test set (directly below).  
// The full range (4 bytes) of the input field is passed to read_be().  The 
// expectation is that only the first byte will be read and contribute to 
// the value returned.  
TEST(midi_vlq_tests, readBigEndianUi8) {
	struct test_t {
		std::array<unsigned char,4> field;
		uint8_t ans;
	};

	std::array<test_t,15> ui8_tests {{
		{{0x00,0x00,0x00,0x00},0x00u},  // min value
		{{0x40,0x00,0x00,0x00},0x40u},
		{{0x00,0x00,0x00,0x40},0x00u},
		{{0x7F,0x00,0x00,0x00},0x7Fu},
		{{0xFF,0x00,0x00,0x00},0xFFu},
		{{0x00,0x00,0x00,0x7F},0x00u},

		{{0x71,0x00,0x00,0x00},0x71u},
		{{0x70,0x00,0x00,0x00},0x70u},
		{{0x0F,0x7F,0x00,0x00},0x0Fu},
		{{0x7F,0xFF,0x00,0x00},0x7Fu},

		{{0x71,0x80,0x00,0x00},0x71u},
		{{0x00,0xC0,0x80,0x00},0x00u},
		{{0x3F,0xFF,0x7F,0x00},0x3Fu},
		{{0x00,0x7F,0xFF,0x3F},0x00u},
		{{0xFF,0xFF,0xFF,0xFF},0xFFu}  // max value
	}};
	for (const auto& tc : ui8_tests) {
		auto res = read_be<uint8_t>(tc.field.begin(),tc.field.end());
		EXPECT_EQ(res,tc.ans) << "Failed for e.ans==" << tc.ans << "\n";
	}
}

// read_be<uint32_t>(...)
// Big-endian encoded multi-byte ints
TEST(midi_vlq_tests, readBigEndianUi32) {
	struct test_t {
		std::array<unsigned char,4> field;
		uint32_t ans;
	};

	std::array<test_t,14> ui32_tests {{
		{{0x00,0x00,0x00,0x00},0x00u},  // min value
		{{0x40,0x00,0x00,0x00},0x40000000u},
		{{0x00,0x00,0x00,0x40},0x00000040u},
		{{0x7F,0x00,0x00,0x00},0x7F000000u},
		{{0x00,0x00,0x00,0x7F},0x0000007Fu},

		{{0x71,0x00,0x00,0x00},0x71000000u},
		{{0x70,0x00,0x00,0x00},0x70000000u},
		{{0x0F,0x7F,0x00,0x00},0x0F7F0000u},
		{{0x7F,0xFF,0x00,0x00},0x7FFF0000u},

		{{0x71,0x80,0x00,0x00},0x71800000u},
		{{0x00,0xC0,0x80,0x00},0x00C08000u},
		{{0x3F,0xFF,0x7F,0x00},0x3FFF7F00u},
		{{0x00,0x7F,0xFF,0x3F},0x007FFF3Fu},
		{{0xFF,0xFF,0xFF,0xFF},0xFFFFFFFFu}  // max value
	}};
	for (const auto& tc : ui32_tests) {
		auto res = read_be<uint32_t>(tc.field.begin(),tc.field.end());
		EXPECT_EQ(res,tc.ans) << "Failed for e.ans==" << tc.ans << "\n";
	}
}

// read_be<uint64_t>(...)
TEST(midi_vlq_tests, readBigEndianUi64) {
	struct test_t {
		std::array<unsigned char,8> field;;
		uint64_t ans;
	};
	std::array<test_t,9> ui64_tests {{
		{{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},0x0000000000000000u},  // min val
		{{0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00},0x4000000000000000u},
		{{0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00},0x0000004000000000u},
		{{0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00},0x7F00000000000000u},
		{{0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00},0xFF00000000000000u},
		{{0x00,0x00,0x00,0x7F,0x00,0x00,0x00,0x00},0x0000007F00000000u},

		{{0x00,0x7F,0xFF,0x3F,0x00,0x00,0x00,0x00},0x007FFF3F00000000u},
		{{0x00,0x00,0x00,0x00,0x00,0x7F,0xFF,0x3F},0x00000000007FFF3Fu},
		{{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},0xFFFFFFFFFFFFFFFFu}   // max val
	}};
	for (const auto& tc : ui64_tests) {
		auto res = read_be<uint64_t>(tc.field.begin(),tc.field.end());
		EXPECT_EQ(res,tc.ans) << "Failed for e.ans==" << tc.ans << "\n";
	}
}


// read_be<uint32_t>(...)
// These test cases are the same as for the ui32 test set (above), however,
// the iterator range passed in to read_be<uint32_t>(...) only spans 3 bytes
// (24 bits).  The output should be the correct value for a 24-bit 
// BE-encoded int.  
TEST(midi_vlq_tests, read24BitBigEndianIntoUi32) {
	struct test_t {
		std::array<unsigned char,4> field;
		uint32_t ans;
	};

	std::array<test_t,15> ui24_tests {{
		{{0x00,0x00,0x00,0x00},0x00u},  // min value
		{{0x40,0x00,0x00,0x00},0x00'40'00'00u},
		{{0x00,0x00,0x00,0x40},0x00'00'00'00u},
		{{0x7F,0x00,0x00,0x00},0x00'7F'00'00u},
		{{0x00,0x00,0x00,0x7F},0x00'00'00'00u},

		{{0x71,0x00,0x00,0x00},0x00'71'00'00u},
		{{0x70,0x00,0x00,0x00},0x00'70'00'00u},
		{{0x0F,0x7F,0x00,0x00},0x00'0F'7F'00u},
		{{0x7F,0xFF,0x00,0x00},0x00'7F'FF'00u},

		{{0x71,0x80,0x00,0x00},0x00'71'80'00u},
		{{0x00,0xC0,0x80,0x00},0x00'00'C0'80u},
		{{0x3F,0xFF,0x7F,0x00},0x00'3F'FF'7Fu},
		{{0x00,0x7F,0xFF,0x3F},0x00'00'7F'FFu},
		{{0xFF,0xFF,0xFF,0x00},0x00'FF'FF'FFu},  // max value
		{{0xFF,0xFF,0xFF,0xFF},0x00'FF'FF'FFu}  // max value
	}};
	for (const auto& tc : ui24_tests) {
		auto res = read_be<uint32_t>(tc.field.begin(),tc.field.begin()+3);
		EXPECT_EQ(res,tc.ans) << "Failed for e.ans==" << tc.ans << "\n";
	}
}


// All the examples from p.131 of the MIDI std.  
TEST(midi_vlq_tests, VlqFieldSizeP131Examples) {
	std::array<uint32_t,3> onebyte_ui32t {0x00u,0x40u,0x7Fu};
	for (const auto& tc : onebyte_ui32t) {
		auto res = vlq_field_size(tc);
		EXPECT_EQ(res,1);
	}
	std::array<uint8_t,3> onebyte_ui8t {0x00u,0x40u,0x7Fu};
	for (const auto& tc : onebyte_ui8t) {
		auto res = vlq_field_size(tc);
		EXPECT_EQ(res,1);
	}
	std::array<uint32_t,3> twobyte_ui32t {0x80u,0x2000u,0x3FFFu};
	for (const auto& tc : twobyte_ui32t) {
		auto res = vlq_field_size(tc);
		EXPECT_EQ(res,2);
	}
	std::array<uint32_t,3> threebyte_ui32t {0x4000u,0x100000u,0x1FFFFFu};
	for (const auto& tc : threebyte_ui32t) {
		auto res = vlq_field_size(tc);
		EXPECT_EQ(res,3);
	}
	std::array<uint32_t,3> fourbyte_ui32t {0x00200000u,0x08000000u,0x0FFFFFFFu};
	for (const auto& tc : fourbyte_ui32t) {
		auto res = vlq_field_size(tc);
		EXPECT_EQ(res,4);
	}
}

// vlq_field_size() w/ values invalid for MIDI VLQ quantities:
// Input values are either < 0 or > 0x0FFFFFFF.  
// Values < 0 should be clamped to 0 and occupy 1 byte.  
// Values > 0x0FFFFFFF should be clamped to 0x0FFFFFFF and occupy 4 bytes.  
TEST(midi_vlq_tests, VlqFieldSizeInvalidValues) {
	// Negative values
	std::array<int16_t,4> i16t_negative {-1,-2,-127,-32768};
	for (const auto& tc : i16t_negative) {
		ASSERT_TRUE(tc >= std::numeric_limits<int16_t>::min());
		auto res = vlq_field_size(tc);
		EXPECT_EQ(res,1);
	}
	std::array<int32_t,8> i32t_negative {-1,-2,-127,-32767,-32768,-32769,
		-147483647,-147483648};
	for (const auto& tc : i32t_negative) {
		ASSERT_TRUE(tc >= std::numeric_limits<int32_t>::min());
		auto res = vlq_field_size(tc);
		EXPECT_EQ(res,1);
	}
	
	// Positive values > 0x0FFFFFFF (268,435,455)
	std::array<int32_t,5> i32t_positive {
		268435456,  // 0x0FFFFFFF + 1
		268435457,  // 0x0FFFFFFF + 2
		268435458,  // 0x0FFFFFFF + 3
		2147483646,  // == numeric_limits<int32_t>::max() - 1
		2147483647 // 0x7FFFFFFF == numeric_limits<int32_t>::max()
	};
	for (const auto& tc : i32t_positive) {
		auto res = vlq_field_size(tc);
		EXPECT_EQ(res,4);
	}
	std::array<uint32_t,8> ui32t_positive {
		268435456u,  // 0x0FFFFFFF + 1
		268435457u,  // 0x0FFFFFFF + 2
		268435458u,  // 0x0FFFFFFF + 3
		2147483646u,  // == numeric_limits<int32_t>::max() - 1
		2147483647u, // 0x7FFFFFFF == numeric_limits<int32_t>::max()
		std::numeric_limits<uint32_t>::max()-2,
		std::numeric_limits<uint32_t>::max()-1,
		std::numeric_limits<uint32_t>::max()
	};
	for (const auto& tc : i32t_positive) {
		auto res = vlq_field_size(tc);
		EXPECT_EQ(res,4);
	}
	std::array<uint64_t,8> ui64t_positive {
		268435456u,  // 0x0FFFFFFF + 1
		268435457u,  // 0x0FFFFFFF + 2
		268435458u,  // 0x0FFFFFFF + 3
		2147483646u,  // == numeric_limits<int32_t>::max() - 1
		2147483647u, // 0x7FFFFFFF == numeric_limits<int32_t>::max()
		std::numeric_limits<uint64_t>::max()-2,
		std::numeric_limits<uint64_t>::max()-1,
		std::numeric_limits<uint64_t>::max()
	};
	for (const auto& tc : ui64t_positive) {
		auto res = vlq_field_size(tc);
		EXPECT_EQ(res,4);
	}
}


TEST(midi_vlq_tests, interpretVLFieldValidFields) {
	struct test_t {
		std::array<unsigned char,6> field {0x00,0x00,0x00,0x00,0x00,0x00};
		vlq_field_interpreted ans {};
	};

	// Examples from p131 of the MIDI std
	std::array<test_t,12> p131_tests {{
		{{0x00,0x00,0x00,0x00,0x00,0x00},{0x00000000,1,true}},
		{{0x40,0x00,0x00,0x00,0x00,0x00},{0x00000040,1,true}},
		{{0x7F,0x00,0x00,0x00,0x00,0x00},{0x0000007F,1,true}},
		{{0x81,0x00,0x00,0x00,0x00,0x00},{0x00000080,2,true}},
		{{0xC0,0x00,0x00,0x00,0x00,0x00},{0x00002000,2,true}},
		{{0xFF,0x7F,0x00,0x00,0x00,0x00},{0x00003FFF,2,true}},
		{{0x81,0x80,0x00,0x00,0x00,0x00},{0x00004000,3,true}},
		{{0xC0,0x80,0x00,0x00,0x00,0x00},{0x00100000,3,true}},
		{{0xFF,0xFF,0x7F,0x00,0x00,0x00},{0x001FFFFF,3,true}},
		{{0x81,0x80,0x80,0x00,0x00,0x00},{0x00200000,4,true}},
		{{0xC0,0x80,0x80,0x00,0x00,0x00},{0x08000000,4,true}},
		{{0xFF,0xFF,0xFF,0x7F,0x00,0x00},{0x0FFFFFFF,4,true}}
	}};
	for (const auto& tc : p131_tests) {
		auto res = read_vlq(tc.field.begin(),tc.field.end());
		EXPECT_EQ(res.val,tc.ans.val);
		EXPECT_EQ(res.N,tc.ans.N);
		EXPECT_EQ(res.is_valid,tc.ans.is_valid);
	}

	// A few addnl assorted examples
	std::array<test_t,5> other_tests {{
		// Does not read past a byte w/a leading 0
		{{0x00,0x80,0x80,0x80,0x00,0x00},{0x00000000,1,true}},
		// 0x80 bytes => 0
		{{0x80,0x80,0x00,0x00,0x00,0x00},{0x00000000,3,true}},
		{{0x80,0x80,0x80,0x00,0x00,0x00},{0x00000000,4,true}},
		{{0x80,0x80,0x80,0x70,0x00,0x00},{0x00000070,4,true}},
		// Should stop reading after 4 bytes, even though the iterator
		// range being passed in spans 6 bytes
		{{0x80,0x80,0x80,0x70,0x80,0x80},{0x00000070,4,true}}
	}};
	for (const auto& tc : other_tests) {
		auto res = read_vlq(tc.field.begin(),tc.field.end());
		EXPECT_EQ(res.val,tc.ans.val);
		EXPECT_EQ(res.N,tc.ans.N);
		EXPECT_EQ(res.is_valid,tc.ans.is_valid);
	}
}


TEST(midi_vlq_tests, interpretVLFieldInvalidFields) {
	struct test_t {
		std::array<unsigned char,6> field {0x00,0x00,0x00,0x00,0x00,0x00};
		vlq_field_interpreted ans {};
	};
	std::array<test_t,4> tests {{
		// Should stop reading after 4 bytes, even though the iterator
		// range being passed in spans 6 bytes
		{{0x80,0x80,0x80,0x80,0x80,0x80},{0x00000000,4,false}},
		// Last byte has a leading 1 => !valid
		{{0xC0,0x80,0x80,0x80,0x00,0x00},{0x08000000,4,false}},
		{{0x80,0x80,0x80,0x80,0x00,0x00},{0x00000000,4,false}},
		{{0xFF,0xFF,0xFF,0xFF,0x00,0x00},{0x0FFFFFFF,4,false}}
	}};

	for (const auto& tc : tests) {
		auto res = read_vlq(tc.field.begin(),tc.field.end());
		EXPECT_EQ(res.val,tc.ans.val);
		EXPECT_EQ(res.N,tc.ans.N);
		EXPECT_EQ(res.is_valid,tc.ans.is_valid);
	}
}



TEST(midi_vlq_tests, advanceToVlqEndValidFields) {
	struct test_t {
		std::array<unsigned char,6> field {0x00,0x00,0x00,0x00,0x00,0x00};
		int distance;
	};

	// Examples from p131 of the MIDI std
	std::array<test_t,12> p131_tests {{
		{{0x00,0x00,0x00,0x00,0x00,0x00},1},
		{{0x40,0x00,0x00,0x00,0x00,0x00},1},
		{{0x7F,0x00,0x00,0x00,0x00,0x00},1},
		{{0x81,0x00,0x00,0x00,0x00,0x00},2},
		{{0xC0,0x00,0x00,0x00,0x00,0x00},2},
		{{0xFF,0x7F,0x00,0x00,0x00,0x00},2},
		{{0x81,0x80,0x00,0x00,0x00,0x00},3},
		{{0xC0,0x80,0x00,0x00,0x00,0x00},3},
		{{0xFF,0xFF,0x7F,0x00,0x00,0x00},3},
		{{0x81,0x80,0x80,0x00,0x00,0x00},4},
		{{0xC0,0x80,0x80,0x00,0x00,0x00},4},
		{{0xFF,0xFF,0xFF,0x7F,0x00,0x00},4}
	}};
	for (const auto& tc : p131_tests) {
		auto it = advance_to_vlq_end(tc.field.begin(),tc.field.end());
		EXPECT_EQ(it-tc.field.begin(),tc.distance);
	}

	// A few addnl assorted examples
	std::array<test_t,5> other_tests {{
		// Does not read past a byte w/a leading 0
		{{0x00,0x80,0x80,0x80,0x00,0x00},1},
		// 0x80 bytes => 0
		{{0x80,0x80,0x00,0x00,0x00,0x00},3},
		{{0x80,0x80,0x80,0x00,0x00,0x00},4},
		{{0x80,0x80,0x80,0x70,0x00,0x00},4},
		// Should stop reading after 4 bytes, even though the iterator
		// range being passed in spans 6 bytes
		{{0x80,0x80,0x80,0x70,0x80,0x80},4}
	}};
	for (const auto& tc : other_tests) {
		auto it = advance_to_vlq_end(tc.field.begin(),tc.field.end());
		EXPECT_EQ(it-tc.field.begin(),tc.distance);
	}
}


TEST(midi_vlq_tests, advanceToVlqEndInvalidFields) {
	struct test_t {
		std::array<unsigned char,6> field {0x00,0x00,0x00,0x00,0x00,0x00};
		int distance;
	};
	std::array<test_t,4> tests {{
		// Should stop reading after 4 bytes, even though the iterator
		// range being passed in spans 6 bytes
		{{0x80,0x80,0x80,0x80,0x80,0x80},4},
		// Last byte has a leading 1 => !valid
		{{0xC0,0x80,0x80,0x80,0x00,0x00},4},
		{{0x80,0x80,0x80,0x80,0x00,0x00},4},
		{{0xFF,0xFF,0xFF,0xFF,0x00,0x00},4}
	}};

	for (const auto& tc : tests) {
		auto it = advance_to_vlq_end(tc.field.begin(),tc.field.end());
		EXPECT_EQ(it-tc.field.begin(),tc.distance);
	}
}


// midi_write_vl_field() and all the examples from p.131 of the MIDI std.  
TEST(midi_vlq_tests, WriteVLFieldP131Examples) {
	struct test_t {
		std::array<unsigned char,4> ans;
		uint32_t val;
		int32_t N;
	};
	std::array<test_t,12> tests {{
		{{0x00,0x00,0x00,0x00},0x00,1},
		{{0x40,0x00,0x00,0x00},0x40,1},
		{{0x7F,0x00,0x00,0x00},0x7F,1},

		{{0x81,0x00,0x00,0x00},0x80,2},
		{{0xC0,0x00,0x00,0x00},0x2000,2},
		{{0xFF,0x7F,0x00,0x00},0x3FFF,2},

		{{0x81,0x80,0x00,0x00},0x4000,3},
		{{0xC0,0x80,0x00,0x00},0x100000,3},
		{{0xFF,0xFF,0x7F,0x00},0x1FFFFF,3},

		{{0x81,0x80,0x80,0x00},0x00200000,4},
		{{0xC0,0x80,0x80,0x00},0x08000000,4},
		{{0xFF,0xFF,0xFF,0x7F},0x0FFFFFFF,4}
	}};
	for (const auto& tc : tests) {
		std::array<unsigned char,4> curr_result {0x00u,0x00u,0x00u,0x00u};

		auto it = write_vlq(tc.val,curr_result.begin());
		auto nbytes_written = it-curr_result.begin();
		EXPECT_EQ(nbytes_written,tc.N);

		ASSERT_TRUE(tc.N<=tc.ans.size());
		ASSERT_TRUE(tc.N<=curr_result.size());
		for (int i=0; i<tc.N; ++i) {
			EXPECT_EQ(curr_result[i],tc.ans[i]);
		}

		EXPECT_EQ(tc.val,
			read_vlq(curr_result.begin(),curr_result.end()).val);
	}
}


// midi_write_vl_field() w/ values invalid for MIDI VLQ quantities:
// Input values are either < 0 or > 0x0FFFFFFF.  
// Values < 0 should be clamped to 0 and occupy 1 byte.  
// Values > 0x0FFFFFFF should be clamped to 0x0FFFFFFF and occupy 4 bytes.  
TEST(midi_vlq_tests, WriteVLFieldInvalidInput) {
	std::array<unsigned char,4> field_input_too_big {0xFFu,0xFFu,0xFFu,0x7Fu};
	uint32_t val_input_too_big = 0x0FFFFFFFu;
	int32_t Nbytes_field_too_big = 4;
	std::array<unsigned char,4> field_input_too_small {0x00u,0x00u,0x00u,0x00u};
	uint32_t val_input_too_small = 0u;
	int32_t Nbytes_field_too_small = 1;

	// Values too large
	std::array<uint32_t,8> ui32t_positive {
		268435456u,  // 0x0FFFFFFF + 1
		268435457u,  // 0x0FFFFFFF + 2
		268435458u,  // 0x0FFFFFFF + 3
		2147483646u,  // == numeric_limits<int32_t>::max() - 1
		2147483647u, // 0x7FFFFFFF == numeric_limits<int32_t>::max()
		std::numeric_limits<uint32_t>::max()-2,
		std::numeric_limits<uint32_t>::max()-1,
		std::numeric_limits<uint32_t>::max()
	};
	for (const auto& tc : ui32t_positive) {
		std::array<unsigned char,4> curr_result {0x00u,0x00u,0x00u,0x00u};
		auto it = write_vlq(tc,curr_result.begin());
		auto nbytes_written = it-curr_result.begin();
		EXPECT_EQ(nbytes_written,Nbytes_field_too_big);
		ASSERT_TRUE(field_input_too_big.size()==curr_result.size());
		for (int i=0; i<field_input_too_big.size(); ++i) {
			EXPECT_EQ(curr_result[i],field_input_too_big[i]);
		}
		EXPECT_EQ(val_input_too_big,
			read_vlq(curr_result.begin(),curr_result.end()).val);
	}
	std::array<uint64_t,8> ui64t_positive {
		268435456u,  // 0x0FFFFFFF + 1
		268435457u,  // 0x0FFFFFFF + 2
		268435458u,  // 0x0FFFFFFF + 3
		2147483646u,  // == numeric_limits<int32_t>::max() - 1
		2147483647u, // 0x7FFFFFFF == numeric_limits<int32_t>::max()
		std::numeric_limits<uint64_t>::max()-2,
		std::numeric_limits<uint64_t>::max()-1,
		std::numeric_limits<uint64_t>::max()
	};
	for (const auto& tc : ui64t_positive) {
		std::array<unsigned char,4> curr_result {0x00u,0x00u,0x00u,0x00u};
		auto it = write_vlq(tc,curr_result.begin());
		auto nbytes_written = it-curr_result.begin();
		EXPECT_EQ(nbytes_written,Nbytes_field_too_big);
		ASSERT_TRUE(field_input_too_big.size()==curr_result.size());
		for (int i=0; i<field_input_too_big.size(); ++i) {
			EXPECT_EQ(curr_result[i],field_input_too_big[i]);
		}
		EXPECT_EQ(val_input_too_big,
			read_vlq(curr_result.begin(),curr_result.end()).val);
	}

	// Values too small (negative)
	std::array<int16_t,4> i16t_negative {-1,-2,-127,-32768};
	for (const auto& tc : i16t_negative) {
		ASSERT_TRUE(tc >= std::numeric_limits<int16_t>::min());
		std::array<unsigned char,4> curr_result {0x00u,0x00u,0x00u,0x00u};
		auto it = write_vlq(tc,curr_result.begin());
		auto nbytes_written = it-curr_result.begin();
		EXPECT_EQ(nbytes_written,Nbytes_field_too_small);
		ASSERT_TRUE(field_input_too_small.size()==curr_result.size());
		for (int i=0; i<field_input_too_small.size(); ++i) {
			EXPECT_EQ(curr_result[i],field_input_too_small[i]);
		}
		EXPECT_EQ(val_input_too_small,
			read_vlq(curr_result.begin(),curr_result.end()).val);
	}
	std::array<int32_t,8> i32t_negative {-1,-2,-127,-32767,-32768,-32769,
		-147483647,-147483648};
	for (const auto& tc : i32t_negative) {
		ASSERT_TRUE(tc >= std::numeric_limits<int32_t>::min());
		std::array<unsigned char,4> curr_result {0x00u,0x00u,0x00u,0x00u};
		auto it = write_vlq(tc,curr_result.begin());
		auto nbytes_written = it-curr_result.begin();
		EXPECT_EQ(nbytes_written,Nbytes_field_too_small);
		ASSERT_TRUE(field_input_too_small.size()==curr_result.size());
		for (int i=0; i<field_input_too_small.size(); ++i) {
			EXPECT_EQ(curr_result[i],field_input_too_small[i]);
		}
		EXPECT_EQ(val_input_too_small,
			read_vlq(curr_result.begin(),curr_result.end()).val);
	}

}






// vlq_field_literal_value() is in midi_vlq_deprecated.h
TEST(midi_vlq_tests, VlqLiteralValue) {
	uint32_t a {137};
	uint32_t a_ans = 33033;
	EXPECT_EQ(a_ans,vlq_field_literal_value(a));

	uint32_t b {106903};
	uint32_t b_ans = 8831767;
	EXPECT_EQ(b_ans,vlq_field_literal_value(b));

	uint32_t c {268435455};  // 0x0F'FF'FF'FF 
	uint32_t c_ans = 4294967167;  // 0xFF'FF'FF'7F 
	EXPECT_EQ(c_ans,vlq_field_literal_value(c));

	uint32_t d = 255;
	uint32_t d_ans = 0x817F; 
	EXPECT_EQ(d_ans,vlq_field_literal_value(d));
}




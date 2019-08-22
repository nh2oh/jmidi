#include "gtest/gtest.h"
#include "generic_chunk_low_level.h"
#include <vector>
#include <array>
#include <cstdint>


//
// Tests for:
// maybe_header_t read_chunk_header(const unsigned char *beg, 
//									const unsigned char *end);
//
// With inputs not valid as chunk headers
//
TEST(midi_chunk_low_level, ReadChunkHeaderInvalidChunkHeaders) {
	struct test_invalid_t {
		std::array<unsigned char,8> data {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
		uint32_t offset_to_data_end {0};
	};
	std::vector<test_invalid_t> tests_invalid_headers {
		// Invalid: Non-ASCII in ID field
		{{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},100},
		{{0x4D,0x7F,0x72,0x6B,0x00,0x00,0x00,0x00},100},  // 0x7F
		{{0x4D,0x80,0x72,0x6B,0x00,0x00,0x00,0x00},100},  // 0x80
		{{0x4D,0x1F,0x72,0x6B,0x00,0x00,0x00,0x00},100},  // 0x1F==31
		// Invalid: valid id, but length > max
		// max_length == 0x7FFFFFF7 == <int32_t>::max()-8
		// id == MTrk
		{{0x4D,0x54,0x72,0x6B,0x7F,0xFF,0xFF,0xF8},100},
		{{0x4D,0x54,0x72,0x6B,0xFF,0xFF,0xFF,0xFF},100},
		// Id == MThd
		{{0x4D,0x54,0x68,0x64,0x7F,0xFF,0xFF,0xF8},100},
		{{0x4D,0x54,0x68,0x64,0xFF,0xFF,0xFF,0xFF},100},
		// Invalid: valid id, but range < 8
		// Id == MTrk
		{{0x4D,0x54,0x72,0x6B,0x00,0x00,0x00,0x06},0},
		{{0x4D,0x54,0x72,0x6B,0x00,0x00,0x00,0x06},7},
		{{0x4D,0x54,0x72,0x6B,0x00,0x00,0x00,0x06},4},
		// Id == MThd
		{{0x4D,0x54,0x68,0x64,0x00,0x00,0x00,0x06},0},
		{{0x4D,0x54,0x68,0x64,0x00,0x00,0x00,0x06},7},
		{{0x4D,0x54,0x68,0x64,0x00,0x00,0x00,0x06},4}
	};

	for (const auto& tcase : tests_invalid_headers) {
		auto beg = tcase.data.data();
		auto end = beg + tcase.offset_to_data_end;
		auto header = read_chunk_header(beg,end);
		EXPECT_FALSE(header);
	}
}

//
// Tests for:
// maybe_header_t read_chunk_header(const unsigned char *beg, 
//									const unsigned char *end);
//
// With inputs valid as chunk headers
//
TEST(midi_chunk_low_level, ReadChunkHeaderValidChunkHeaders) {
	struct test_valid_t {
		std::array<unsigned char,8> data {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
		uint32_t offset_to_data_end {0};
		chunk_id ans_type {chunk_id::unknown};
		int32_t ans_length {0};
	};
	std::vector<test_valid_t> tests_valid_headers {
		// valid, MThd, MTrk
		// Length  == 0
		{{0x4D,0x54,0x68,0x64,0x00,0x00,0x00,0x00},100, chunk_id::mthd,0},
		{{0x4D,0x54,0x72,0x6B,0x00,0x00,0x00,0x00},100, chunk_id::mtrk,0},
		// Length == 6
		{{0x4D,0x54,0x68,0x64,0x00,0x00,0x00,0x06},100, chunk_id::mthd,6},
		{{0x4D,0x54,0x72,0x6B,0x00,0x00,0x00,0x06},100, chunk_id::mtrk,6},
		// max_length == 0x7FFFFFF7 == <int32_t>::max()-8
		// Note that:
		// data.begin()+8+length is > data.begin()+offset_to_data_end
		// But that's ok
		{{0x4D,0x54,0x68,0x64,0x7F,0xFF,0xFF,0xF7},100, chunk_id::mthd,0x7FFFFFF7},
		{{0x4D,0x54,0x72,0x6B,0x7F,0xFF,0xFF,0xF7},100, chunk_id::mtrk,0x7FFFFFF7},
		
		// id == unknown
		{{0x4D,0x54,0x4D,0x54,0x00,0x00,0x00,0x00},100, chunk_id::unknown,0},
		{{0x4D,0x54,0x68,0x65,0x00,0x00,0x00,0x06},100, chunk_id::unknown,6},
		{{0x4D,0x54,0x70,0x6B,0x00,0x00,0x00,0x00},100, chunk_id::unknown,0},
		{{0x4E,0x53,0x72,0x6B,0x00,0x00,0x00,0x06},100, chunk_id::unknown,6},

		// From random MIDI Files i have laying around
		{{0x4D,0x54,0x68,0x64,0x00,0x00,0x00,0x06},5584, chunk_id::mthd,6},
		{{0x4D,0x54,0x72,0x6B,0x00,0x00,0x00,0x1C},5570, chunk_id::mtrk,28},
		{{0x4D,0x54,0x72,0x6B,0x00,0x00,0x0F,0x13},5534, chunk_id::mtrk,3859},
		{{0x4D,0x54,0x72,0x6B,0x00,0x00,0x06,0x7B},1667, chunk_id::mtrk,1659},
	};
	for (const auto& tcase : tests_valid_headers) {
		auto beg = tcase.data.data();
		auto end = beg + tcase.offset_to_data_end;
		auto header = read_chunk_header(beg,end);
		EXPECT_TRUE(header);
		EXPECT_EQ(header.id,tcase.ans_type);
		EXPECT_EQ(header.length,tcase.ans_length);
	}
}





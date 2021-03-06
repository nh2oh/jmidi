#include "gtest/gtest.h"
#include "delta_time_test_data.h"
#include "midi_vlq.h"
#include "midi_delta_time.h"
#include "aux_types.h"
#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include "midi_status_byte.h"  // channel_status_byte_n_data_bytes()
#include <vector>
#include <cstdint>
#include <array>


// 
// Test of the default-constructed value, a middle C (note-num==60)
// Note-on event on channel "1" w/ velocity 60 and delta-time == 0.  
//
TEST(mtrk_event_ctor_tests, defaultCtor) {
	const auto d = jmid::mtrk_event_t();

	EXPECT_EQ(d.size(),0);
	EXPECT_TRUE(d.is_empty());
	EXPECT_TRUE(d.size()<=d.capacity());
	EXPECT_EQ(d.end()-d.begin(),d.size());
}


//
// Tests of the mtrk_event_t(uint32_t, const ch_event_data_t&) ctor
// with valid data in the ch_event_data_t struct.  
//
TEST(mtrk_event_ctor_tests, MidiChEventStructCtorValidInputData) {
	struct test_t {
		uint32_t dt_input {0};
		jmid::ch_event_data_t md_input {};
		uint32_t data_size {0};
	};
	// ch_event_data_t {status, ch, p1, p2}
	std::vector<test_t> tests {
		// Events w/ 2 data bytes:
		{0, {0x90,0,57,32}, 3},
		{23, {0x80,1,57,32}, 3},
		{12354, {0xA0u,0,57,32}, 3},
		{0, {0xB0u,15,72,100}, 3},
		{45541, {0xE0u,0,127,127}, 3},
		// Events w/ 1 data byte:
		{785, {0xC0u,14,127,0x00u}, 2},
		{2, {0xD0u,2,0,0x00u}, 2}
	};
	
	for (const auto& tc : tests) {
		unsigned char curr_s = (tc.md_input.status_nybble + tc.md_input.ch);
		int curr_dt_size = jmid::vlq_field_size(tc.dt_input);
		int curr_size = curr_dt_size+tc.data_size;
		const jmid::mtrk_event_t ev(tc.dt_input,tc.md_input);

		EXPECT_TRUE(jmid::is_channel(ev));
		EXPECT_EQ(ev.delta_time(),tc.dt_input);
		EXPECT_EQ(ev.size(),curr_size);
		EXPECT_EQ(ev.data_size(),tc.data_size);
		EXPECT_EQ(ev.status_byte(),curr_s);
		EXPECT_EQ(ev.running_status(),curr_s);

		auto it_beg = ev.event_begin();
		auto it_end = ev.end();
		EXPECT_EQ((it_end-ev.begin()),curr_size);
		EXPECT_EQ((it_end-it_beg),curr_size-curr_dt_size);
		EXPECT_EQ(it_beg,ev.payload_begin());
		EXPECT_EQ(*it_beg++,curr_s);
		EXPECT_EQ(*it_beg++,tc.md_input.p1);
		if (it_beg < it_end) {
			EXPECT_TRUE((it_end-it_beg)==1);
			EXPECT_EQ(*it_beg++,tc.md_input.p2);
		}
		EXPECT_TRUE(it_beg==it_end);
		EXPECT_TRUE(it_beg>=it_end);
		EXPECT_TRUE(it_beg<=it_end);
		EXPECT_FALSE(it_beg>it_end);
		EXPECT_FALSE(it_beg<it_end);
	}
}


//
// Tests of the mtrk_event_t(uint32_t, const ch_event_data_t&) ctor
// with _invalid_ data in the ch_event_data_t struct.  
//
// TODO:  This is better used as a test for normalize(ch_event_data_t)...
TEST(mtrk_event_ctor_tests, MidiChEventStructCtorInvalidInputData) {
	struct test_t {
		uint32_t dt_input;
		jmid::ch_event_data_t md_input;
	};
	// ch_event_data_t {status, ch, p1, p2}
	std::vector<test_t> tests {
		{0, {0x90,16,57,32}},  // Invalid channel (>15)
		{1, {0x90,127,57,32}},  // Invalid channel (>15)
		{128, {0x90,14,128,32}},  // Invalid p1
		{256, {0x90,14,129,32}},  // Invalid p1
		{512, {0x90,14,7,130}},  // Invalid p2
		{1024, {0x90,14,57,255}},  // Invalid p2

		// Exactly the same as the set above, but w/a 1-data-byte msg type
		{0, {0xC0u,16,57,32}},  // Invalid channel
		{1, {0xC0u,127,57,32}},  // Invalid channel
		{128, {0xC0u,14,128,32}},  // Invalid p1
		{256, {0xC0u,14,129,32}},  // Invalid p1
		{512, {0xC0u,14,7,130}},  // Invalid p2
		{1024, {0xC0u,14,57,255}},  // Invalid p2

		// Exactly the same as the set above, but w/an invalid status-nybble
		{0, {0x90u&0x7Fu,16,57,32}},  // Invalid channel
		{1, {0x90u&0x7Fu,127,57,32}},  // Invalid channel
		{128, {0x90u&0x7Fu,14,128,32}},  // Invalid p1
		{256, {0x90u&0x7Fu,14,129,32}},  // Invalid p1
		{512, {0x90u&0x7Fu,14,7,130}},  // Invalid p2
		{1024, {0x90u&0x7Fu,14,57,255}}  // Invalid p2
	};
	
	for (const auto& tc : tests) {
		auto expect_ans = jmid::normalize(tc.md_input);
		auto expect_s = expect_ans.status_nybble|expect_ans.ch;
		auto expect_n_data = jmid::channel_status_byte_n_data_bytes(expect_s);
		int curr_dt_size = jmid::vlq_field_size(tc.dt_input);
		int expect_size = curr_dt_size + 1 
			+ expect_n_data;
		const jmid::mtrk_event_t ev(tc.dt_input,tc.md_input);

		EXPECT_EQ(ev.delta_time(),tc.dt_input);
		EXPECT_EQ(ev.status_byte(),expect_s);
		EXPECT_EQ(ev.size(),expect_size);
		EXPECT_EQ(ev.end()-ev.begin(), ev.size());

		auto it = ev.event_begin();
		EXPECT_EQ(*it++,expect_s);
		EXPECT_EQ(*it++,expect_ans.p1);
		if (expect_n_data==2) {
			EXPECT_EQ(*it++,expect_ans.p2);
		}
		auto it_beg = ev.begin();
		auto it_evbeg = ev.event_begin();
		auto it_end = ev.end();
	}
}


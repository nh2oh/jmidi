#include "gtest/gtest.h"
#include "delta_time_test_data.h"
#include "midi_raw.h"
#include "midi_vlq.h"
#include "midi_delta_time.h"
#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include <vector>
#include <cstdint>
#include <array>

std::array<unsigned char,4> default_ctord_data {0x00u,0x90u,0x3Cu,0x3Fu};

// 
// Test of the default-constructed value, a middle C (note-num==60)
// Note-on event on channel "1" w/ velocity 60 and delta-time == 0.  
//
TEST(mtrk_event_ctor_tests, defaultCtor) {
	const auto d = mtrk_event_t();

	EXPECT_EQ(d.size(),4);
	EXPECT_TRUE(d.size()<=d.capacity());
	EXPECT_EQ(d.dt_end()-d.dt_begin(),1);
	EXPECT_EQ(d.end()-d.begin(),d.size());

	EXPECT_TRUE(is_channel(d));
	EXPECT_EQ(d.delta_time(),0);
	EXPECT_EQ(d.status_byte(),0x90u);
	EXPECT_EQ(d.running_status(),0x90u);
	EXPECT_EQ(d.data_size(),3);
	
	for (int i=0; i<d.size(); ++i) {
		EXPECT_EQ(d[i],default_ctord_data[i]);
	}
}

// 
// Test of the mtrk_event_t(uint32_t dt) ctor, which constructs a middle C
// (note-num==60) note-on event on channel "1" w/ velocity 60 and as
// specified.  For values of delta_time>max allowed, the value written is the
// max allowed.  
//
TEST(mtrk_event_ctor_tests, dtOnlyCtor) {
	std::array<unsigned char,6> ans_dt_encoded;
	auto ans_data_size = 3;  // For a default-ctor'd mtrk_event
	for (const auto& tc : dt_test_set_a) {
		ans_dt_encoded.fill(0x00u);
		write_delta_time(tc.ans_value,ans_dt_encoded.begin());
		auto ans_size = ans_data_size + tc.ans_n_bytes;

		const mtrk_event_t ev(tc.dt_input);

		EXPECT_EQ(ev.size(),ans_size);
		EXPECT_TRUE(ev.size()<=ev.capacity());
		EXPECT_EQ(ev.dt_end()-ev.dt_begin(),tc.ans_n_bytes);
		EXPECT_EQ(ev.end()-ev.begin(),ev.size());

		EXPECT_TRUE(is_channel(ev));
		EXPECT_EQ(ev.delta_time(),tc.ans_value);
		EXPECT_EQ(ev.status_byte(),0x90u);
		EXPECT_EQ(ev.running_status(),0x90u);
		auto ds=ev.data_size();
		EXPECT_EQ(ev.data_size(),ans_data_size);
	
		for (int i=0; i<tc.ans_n_bytes; ++i) {
			EXPECT_EQ(ev[i],ans_dt_encoded[i]);
		}
		for (int i=tc.ans_n_bytes; i<ev.size(); ++i) {
			EXPECT_EQ(ev[i],default_ctord_data[i-(tc.ans_n_bytes-1)]);
		}
	}
}


//
// Tests of the mtrk_event_t(uint32_t, const ch_event_data_t&) ctor
// with valid data in the ch_event_data_t struct.  
//
TEST(mtrk_event_ctor_tests, MidiChEventStructCtorValidInputData) {
	struct test_t {
		uint32_t dt_input {0};
		ch_event_data_t md_input {};
		uint32_t data_size {0};
	};
	// ch_event_data_t {status, ch, p1, p2}
	std::vector<test_t> tests {
		// Events w/ 2 data bytes:
		{0, {note_on,0,57,32}, 3},
		{23, {note_off,1,57,32}, 3},
		{12354, {key_pressure,0,57,32}, 3},
		{0, {ctrl_change,15,72,100}, 3},
		{45541, {pitch_bend,0,127,127}, 3},
		// Events w/ 1 data byte:
		{785, {prog_change,14,127,0x00u}, 2},
		{2, {ch_pressure,2,0,0x00u}, 2}
	};
	
	for (const auto& tc : tests) {
		unsigned char curr_s = (tc.md_input.status_nybble + tc.md_input.ch);
		int curr_dt_size = vlq_field_size(tc.dt_input);
		int curr_size = curr_dt_size+tc.data_size;
		const mtrk_event_t ev(tc.dt_input,tc.md_input);

		EXPECT_TRUE(is_channel(ev));
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
		ch_event_data_t md_input;
	};
	// ch_event_data_t {status, ch, p1, p2}
	std::vector<test_t> tests {
		{0, {note_on,16,57,32}},  // Invalid channel (>15)
		{1, {note_on,127,57,32}},  // Invalid channel (>15)
		{128, {note_on,14,128,32}},  // Invalid p1
		{256, {note_on,14,129,32}},  // Invalid p1
		{512, {note_on,14,7,130}},  // Invalid p2
		{1024, {note_on,14,57,255}},  // Invalid p2

		// Exactly the same as the set above, but w/a 1-data-byte msg type
		{0, {prog_change,16,57,32}},  // Invalid channel
		{1, {prog_change,127,57,32}},  // Invalid channel
		{128, {prog_change,14,128,32}},  // Invalid p1
		{256, {prog_change,14,129,32}},  // Invalid p1
		{512, {prog_change,14,7,130}},  // Invalid p2
		{1024, {prog_change,14,57,255}},  // Invalid p2

		// Exactly the same as the set above, but w/an invalid status-nybble
		{0, {note_on&0x7Fu,16,57,32}},  // Invalid channel
		{1, {note_on&0x7Fu,127,57,32}},  // Invalid channel
		{128, {note_on&0x7Fu,14,128,32}},  // Invalid p1
		{256, {note_on&0x7Fu,14,129,32}},  // Invalid p1
		{512, {note_on&0x7Fu,14,7,130}},  // Invalid p2
		{1024, {note_on&0x7Fu,14,57,255}}  // Invalid p2
	};
	
	for (const auto& tc : tests) {
		auto expect_ans = normalize(tc.md_input);
		auto expect_s = expect_ans.status_nybble|expect_ans.ch;
		auto expect_n_data = channel_status_byte_n_data_bytes(expect_s);
		int curr_dt_size = vlq_field_size(tc.dt_input);
		int expect_size = curr_dt_size + 1 
			+ expect_n_data;
		const mtrk_event_t ev(tc.dt_input,tc.md_input);

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


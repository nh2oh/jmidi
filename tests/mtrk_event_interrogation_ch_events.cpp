#include "gtest/gtest.h"
#include "midi_raw.h"  // Declares smf_event_type
#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include "midi_raw_test_data.h"
#include <vector>
#include <cstdint>


// Test set C data (all cases are valid mtrk channel events)
TEST(mtrk_event_channel_interrogators, isChannelVoiceModeTestSetCEvents) {
	for (auto& tc : set_c_midi_events_valid) {
		auto maybe_ev = make_mtrk_event(tc.data.data(),
			tc.data.data()+tc.data.size(),tc.midisb_prev_event,nullptr);
		EXPECT_TRUE(maybe_ev);
		auto ev = maybe_ev.event;
		
		EXPECT_TRUE(is_channel(ev));
		if (tc.is_ch_mode) {
			EXPECT_TRUE(is_channel_mode(ev));
			EXPECT_FALSE(is_channel_voice(ev));
		} else {
			EXPECT_FALSE(is_channel_mode(ev));
			EXPECT_TRUE(is_channel_voice(ev));
		}
	}
}


// Some assorted events
TEST(mtrk_event_channel_interrogators, isChannelVoiceModeAssortedEvents) {
	struct test_t {
		int32_t dt_input {0};
		ch_event_data_t md_input {};  // ch_event_data_t {status, ch, p1, p2}
		int32_t data_size {0};
	};	
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
		const mtrk_event_t ev(tc.dt_input,tc.md_input);
		EXPECT_TRUE(is_channel(ev));
		EXPECT_TRUE(is_channel_voice(ev));
		EXPECT_FALSE(is_channel_mode(ev));
	}
}



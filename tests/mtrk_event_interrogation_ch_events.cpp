#include "gtest/gtest.h"
#include "aux_types.h"
#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include "midi_raw_test_data.h"
#include <vector>
#include <cstdint>


// Test set C data (all cases are valid mtrk channel events)
TEST(mtrk_event_channel_interrogators, isChannelVoiceModeTestSetCEvents) {
	for (auto& tc : set_c_midi_events_valid) {
		auto maybe_ev = jmid::make_mtrk_event(tc.data.data(),
			tc.data.data()+tc.data.size(),tc.midisb_prev_event,nullptr,
			tc.data.size());
		EXPECT_TRUE(maybe_ev);
		auto ev = maybe_ev.event;
		
		EXPECT_TRUE(jmid::is_channel(ev));
		if (tc.is_ch_mode) {
			EXPECT_TRUE(jmid::is_channel_mode(ev));
			EXPECT_FALSE(jmid::is_channel_voice(ev));
		} else {
			EXPECT_FALSE(jmid::is_channel_mode(ev));
			EXPECT_TRUE(jmid::is_channel_voice(ev));
		}
	}
}


// Some assorted events
TEST(mtrk_event_channel_interrogators, isChannelVoiceModeAssortedEvents) {
	struct test_t {
		int32_t dt_input {0};
		jmid::ch_event_data_t md_input {};  // ch_event_data_t {status, ch, p1, p2}
		int32_t data_size {0};
	};	
	std::vector<test_t> tests {
		// Events w/ 2 data bytes:
		{0, {0x90,0,57,32}, 3},
		{23, {0x80,1,57,32}, 3},
		{12354, {0xA0,0,57,32}, 3},
		{0, {0xB0,15,72,100}, 3},
		{45541, {0xE0,0,127,127}, 3},
		// Events w/ 1 data byte:
		{785, {0xC0,14,127,0x00u}, 2},
		{2, {0xD0,2,0,0x00u}, 2}
	};
	
	for (const auto& tc : tests) {
		const jmid::mtrk_event_t ev(tc.dt_input,tc.md_input);
		EXPECT_TRUE(jmid::is_channel(ev));
		EXPECT_TRUE(jmid::is_channel_voice(ev));
		EXPECT_FALSE(jmid::is_channel_mode(ev));
	}
}



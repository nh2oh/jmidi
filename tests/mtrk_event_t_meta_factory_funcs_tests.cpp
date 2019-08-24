#include "gtest/gtest.h"
#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include "midi_delta_time.h"
#include "aux_types.h"
#include <vector>
#include <cstdint>
#include <string>

// 
// mtrk_event_t make_tempo(const uint32_t& dt, const uint32_t& uspqn);
//
TEST(mtrk_event_t_meta_factories, makeTempo) {
	struct test_t {
		int32_t dt_in {0};
		uint32_t tempo_in {0};
		int32_t dt_ans {0};
		uint32_t tempo_ans {0};
	};

	std::vector<test_t> tests {
		{0, 0, 0, 0},
		{1, 1, 1, 1},
		{128, 1523, 128, 1523},
		{125428, 1523, 125428, 1523},
		// 16777215 == 0x00'FF'FF'FF is the largest possible 24-bit int
		{1,16777215,1,16777215}, 
		// In the next two, the value for tempo_in exceeds the max value, so
		// will be truncated to the max value of 16777215.  
		{1,16777216,1,16777215}, 
		{1,0xFFFFFFFF,1,16777215},
		
	};
	for (const auto& e : tests) {
		const auto ev = jmid::make_tempo(e.dt_in,e.tempo_in);
		auto dt_size = delta_time_field_size(e.dt_ans);
		auto pyld_size = 3;
		auto dat_size = 3+pyld_size;
		auto tot_size = dt_size+dat_size;
		EXPECT_EQ(jmid::classify_meta_event(ev),jmid::meta_event_t::tempo);
		EXPECT_TRUE(jmid::is_tempo(ev));
		EXPECT_EQ(ev.delta_time(),e.dt_ans);
		EXPECT_EQ(ev.size(),tot_size);
		EXPECT_EQ(ev.data_size(),dat_size);
		EXPECT_EQ(ev.running_status(),0x00u);
		EXPECT_EQ(ev.status_byte(),0xFFu);

		EXPECT_EQ(jmid::get_tempo(ev),e.tempo_ans);

		EXPECT_EQ(ev.end()-ev.begin(),tot_size);
		EXPECT_EQ(ev.end()-ev.event_begin(),dat_size);
		EXPECT_EQ(ev.end()-ev.payload_begin(),pyld_size);

		EXPECT_EQ(*(ev.event_begin()),0xFFu);
	}
}


// 
// mtrk_event_t make_eot(const uint32_t& dt)
//
TEST(mtrk_event_t_meta_factories, makeEOT) {
	std::vector<int32_t> tests {0,1,128,125428};
	for (const auto& e : tests) {
		const auto ev = jmid::make_eot(e);
		auto dt_size = delta_time_field_size(e);
		auto pyld_size = 0;
		auto dat_size = 3+pyld_size;
		auto tot_size = dt_size+dat_size;
		EXPECT_EQ(jmid::classify_meta_event(ev),jmid::meta_event_t::eot);
		EXPECT_TRUE(jmid::is_eot(ev));
		EXPECT_EQ(ev.delta_time(),e);
		EXPECT_EQ(ev.size(),tot_size);
		EXPECT_EQ(ev.data_size(),dat_size);
		EXPECT_EQ(ev.running_status(),0x00u);
		EXPECT_EQ(ev.status_byte(),0xFFu);

		EXPECT_EQ(ev.end()-ev.begin(),tot_size);
		EXPECT_EQ(ev.end()-ev.event_begin(),dat_size);
		EXPECT_EQ(ev.end()-ev.payload_begin(),pyld_size);

		EXPECT_EQ(*(ev.event_begin()),0xFFu);
	}
}


// 
// mtrk_event_t make_timesig(const uint32_t& dt, const midi_timesig_t& ts)
//
TEST(mtrk_event_t_meta_factories, makeTimesig) {
	struct test_t {
		int32_t dt {0};
		jmid::midi_timesig_t ts {0,0,0,0};
	};

	std::vector<test_t> tests {
		{0, {0, 0, 0, 0}},
		{0, {6, 3, 36, 8}},  // From the midi std p. 10
		{1, {1, 1, 1, 1}},

		{128, {0, 0, 0, 0}},
		{128, {6, 3, 36, 8}},  // From the midi std p. 10
		{128, {1, 1, 1, 1}},

		{125428, {0, 0, 0, 0}},
		{125428, {6, 3, 36, 8}},  // From the midi std p. 10
		{125428, {1, 1, 1, 1}},
	};
	for (const auto& e : tests) {
		const auto ev = make_timesig(e.dt,e.ts);
		auto dt_size = delta_time_field_size(e.dt);
		auto pyld_size = 4;
		auto dat_size = 3+pyld_size;
		auto tot_size = dt_size+dat_size;
		EXPECT_EQ(jmid::classify_meta_event(ev),jmid::meta_event_t::timesig);
		EXPECT_TRUE(jmid::is_timesig(ev));
		EXPECT_EQ(ev.delta_time(),e.dt);
		EXPECT_EQ(ev.size(),tot_size);
		EXPECT_EQ(ev.data_size(),dat_size);
		EXPECT_EQ(ev.running_status(),0x00u);
		EXPECT_EQ(ev.status_byte(),0xFFu);

		EXPECT_EQ(jmid::get_timesig(ev),e.ts);

		EXPECT_EQ(ev.end()-ev.begin(),tot_size);
		EXPECT_EQ(ev.end()-ev.event_begin(),dat_size);
		EXPECT_EQ(ev.end()-ev.payload_begin(),pyld_size);

		EXPECT_EQ(*(ev.event_begin()),0xFFu);
	}
}


// 
// mtrk_event_t make_{instname,lyric,marker,cuepoint,text,
//						copyright}(const uint32_t& dt, const std::string& s)
// ... All maker-functions for meta events with text payloads
//
TEST(mtrk_event_t_meta_factories, makeEventsWithTextPayloads) {
	struct testset_t {
		mtrk_event_t (*fp_make)(const int32_t&, const std::string&);
		bool (*fp_is)(const mtrk_event_t&);
		jmid::meta_event_t ans_evtype;
	};
	std::vector<testset_t> testsets {
		{jmid::make_text,jmid::is_text,jmid::meta_event_t::text},
		{jmid::make_copyright,jmid::is_copyright,jmid::meta_event_t::copyright},
		{jmid::make_trackname,jmid::is_trackname,jmid::meta_event_t::trackname},
		{jmid::make_instname,jmid::is_instname,jmid::meta_event_t::instname},
		{jmid::make_lyric,jmid::is_lyric,jmid::meta_event_t::lyric},
		{jmid::make_marker,jmid::is_marker,jmid::meta_event_t::marker},
		{jmid::make_cuepoint,jmid::is_cuepoint,jmid::meta_event_t::cuepoint}		
	};

	struct test_t {
		int32_t dt {0};
		std::string s {};
		bool issmall {false};
	};
	std::vector<test_t> tests {
		{0, "Acoustic Grand", true},
		{0, "", true},
		{1, " ", true},
		{9, "     ", true},
		{0, "This string exceeds the size of the small buffer in mtrk_event_t.  ", false},
		{125428, "This string exceeds the size of the small buffer in mtrk_event_t.  ", false},
		{125428, "", true},
		{125428, "", true},
		// Maximum allowed dt
		{0x0FFFFFFF, "", true},
		{0x0FFFFFFF, "Acoustic Grand", true},  // From the midi std p. 10
		{0x0FFFFFFF, "This string exceeds the size of the small buffer in mtrk_event_t.  ", false}
	};
	for (const auto curr_testset : testsets) {
		for (const auto& e : tests) {
			auto ev = (*curr_testset.fp_make)(e.dt,e.s);
			EXPECT_EQ(jmid::classify_meta_event(ev),curr_testset.ans_evtype);
			EXPECT_TRUE((*curr_testset.fp_is)(ev));
			EXPECT_TRUE(jmid::meta_has_text(ev));
			EXPECT_EQ(ev.delta_time(),e.dt);

			auto txt = jmid::meta_generic_gettext(ev);

			EXPECT_EQ(jmid::meta_generic_gettext(ev),e.s);
		}
	}
}


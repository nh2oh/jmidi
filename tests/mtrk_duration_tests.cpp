#include "gtest/gtest.h"
#include "midi_time.h"
#include "smf_t.h"
#include <cstdint>
#include <array>
#include <filesystem>
#include <cmath>

// 
// Tests of:
// double duration(const mtrk_t&, const jmid::time_division_t&, int32_t=500000);
//
// test_data\tempo_duration\...
// Each file has 12 notes.  For each on/off pair, the off-event is 
// seperated from the on-event by 50 ticks (all off-events have delta-time
// == 50 tks).  For all other events, delta-times are 0.  
// The timesig is {FF 58 04 04 02 18 08} 
// => value = 4/4, 24 MIDI clocks/click, 8 notated 32'nd nts / MIDI q nt;
TEST(mtrk_duration_tests, cchromSingleTempoEvent) {
	std::filesystem::path basepath = "..\\..\\..\\tests\\test_data\\tempo_duration\\";
	struct test_t {
		jmid::time_division_t tdiv;
		int32_t tempo;  // us/q-nt
		std::filesystem::path file;
	};

	std::array<test_t,12> tests {
		test_t {jmid::time_division_t(25), 0, "tdiv25_tempo0.midi"},
		test_t {jmid::time_division_t(25), 1, "tdiv25_tempo1.midi"},
		test_t {jmid::time_division_t(25), 250000, "tdiv25_tempo250k.midi"},
		test_t {jmid::time_division_t(25), 500000, "tdiv25_tempo500k.midi"},
		test_t {jmid::time_division_t(25), 750000, "tdiv25_tempo750k.midi"},
		test_t {jmid::time_division_t(25), 0xFFFFFF, "tdiv25_tempo0x0FFFFFFF.midi"},
		// Minimum permitted time-div value
		test_t {jmid::time_division_t(1), 1, "tdiv1_tempo1.midi"},
		test_t {jmid::time_division_t(1), 1234567, "tdiv1_tempo1234567.midi"},

		test_t {jmid::time_division_t(152), 500000, "tdiv152_tempo500k.midi"},
		// Max permitted time-div value
		test_t {jmid::time_division_t(0x7FFF), 1, "tdiv0x7FFF_tempo1.midi"},
		test_t {jmid::time_division_t(0x7FFF), 500000, "tdiv0x7FFF_tempo500k.midi"},
		test_t {jmid::time_division_t(0x7FFF), 0x00FFFFFF, "tdiv0x7FFF_tempo0x00FFFFFF.midi"}
	};
	
	double permissible_err_s = 1.0/1000000000.0;  // 1 ns
	
	int num_files_tested = 0;
	for (const auto& tc : tests) {
		auto p = basepath/tc.file;
		if (!jmid::has_midifile_extension(p)) {
			continue;
		}

		jmid::maybe_smf_t maybe_smf = jmid::read_smf(p,nullptr);
		ASSERT_TRUE(maybe_smf);
		EXPECT_EQ(maybe_smf.smf.mthd().division(),tc.tdiv);

		double curr_tpq = static_cast<double>(tc.tdiv.get_tpq());  // tks/q-nt
		double curr_tempo = static_cast<double>(tc.tempo)/1000000.0;  // s/q-nt
		double curr_sptk = curr_tempo/curr_tpq;  // "seconds-per-tick"
		
		//auto cum_sec = ticks2sec(cum_num_tks,tc.tdiv,tc.tempo);
		//auto cum_tk = sec2ticks(cum_sec,tc.tdiv,tc.tempo);
		for (const auto& trk : maybe_smf.smf) {
			auto curr_duration_tks = trk.nticks();
			auto expect_duration_s = curr_sptk*curr_duration_tks;
			
			auto curr_duration_s = jmid::duration(trk,maybe_smf.smf.mthd().division());
			EXPECT_TRUE(std::abs(curr_duration_s-expect_duration_s)<=permissible_err_s);
		}
		++num_files_tested;
	}
	// A bug in has_midifile_extension() could cause all the to be 
	// skipped.  This will also trigger if any of the files go missing from
	// the test dir.  
	EXPECT_EQ(num_files_tested,tests.size());
}




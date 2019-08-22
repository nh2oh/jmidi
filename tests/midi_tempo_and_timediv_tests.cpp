#include "gtest/gtest.h"
#include "midi_time.h"
#include <cstdint>
#include <cmath>
#include <vector>
#include <array>
#include <limits>



TEST(midi_tempo_and_timediv_tests, interpretSMPTEField) {
	struct test_t {
		uint16_t input {0};
		int8_t ans_tcf {0};
		uint8_t ans_upf {0};
	};
	std::array<test_t,3> tests {
		test_t {0xE250u,-30,80},  // p.133 of the midi std
		test_t {0xE728u,-25,40},   // 25fr/sec * 40tk/fr => 1000tk/sec => ms resolution
		test_t {0xE350u,-29,80}
	};

	for (const auto& e : tests) {
		auto curr_maybe = jmid::make_time_division_from_raw(e.input);
		jmid::time_division_t curr_tdf = curr_maybe.value;
		EXPECT_EQ(curr_tdf.get_type(),jmid::time_division_t::type::smpte);

		auto curr_tcf = curr_tdf.get_smpte().time_code; //get_time_code_fmt(curr_tdf);
		EXPECT_EQ(curr_tcf,e.ans_tcf);
		auto curr_upf = curr_tdf.get_smpte().subframes; //get_units_per_frame(curr_tdf);
		EXPECT_EQ(curr_upf,e.ans_upf);
	}
}


// 
// Tests of ticks2sec() and sec2ticks()
// 
TEST(midi_tempo_and_timediv_tests, maxErrorTickSecondInterconversion) {
	std::vector<jmid::time_division_t> tdivs {
		// Small values
		jmid::time_division_t(1),jmid::time_division_t(2),jmid::time_division_t(3),
		// "Normal" values
		jmid::time_division_t(24),jmid::time_division_t(25),jmid::time_division_t(48),
		jmid::time_division_t(96),jmid::time_division_t(152),
		// Max allowed value for ticks-per-qnt
		jmid::time_division_t(0x7FFF)
	};
	std::vector<int32_t> tempos {0,1,2,3,250000,500000,750000,0xFFFFFF};
	std::vector<int32_t> tks {0,1,2,3,154,4287,24861,251111,52467,754714,
		0x7FFFFFFF};

	for (const auto& tdiv: tdivs) {
		for (const auto& tempo: tempos) {
			for (const auto& tk : tks) {
				auto s_from_tk = jmid::ticks2sec(tk,tdiv,tempo);
				if (tempo==0) {
					// If the tempo is 0, sec obviously == 0 no matter the
					// value of tk, however, you can't go back to tk...
					continue;
				}
				// ticks2sec() returns a value within 1 ulp of the exact value
				auto max_err_computed_sec = 
					std::numeric_limits<double>::epsilon()*s_from_tk;
				auto s_from_tk_min = s_from_tk - max_err_computed_sec;
				auto s_from_tk_max = s_from_tk + max_err_computed_sec;

				auto tk_from_s = jmid::sec2ticks(s_from_tk,tdiv,tempo);
				auto tk_from_s_min = jmid::sec2ticks(s_from_tk_min,tdiv,tempo);
				auto tk_from_s_max = jmid::sec2ticks(s_from_tk_max,tdiv,tempo);

				double curr_tk = static_cast<double>(tk);
				EXPECT_GE(curr_tk,tk_from_s_min);
				EXPECT_LE(curr_tk,tk_from_s_max);
			}
		}
	}
}


// 
// Tests of note2ticks(...)
// 
TEST(midi_tempo_and_timediv_tests, noteValueToNumTicks) {
	std::vector<jmid::time_division_t> tdivs {
		// Small values
		//jmid::time_division_t(1),jmid::time_division_t(2),jmid::time_division_t(3),
		//jmid::time_division_t(20),jmid::time_division_t(24),jmid::time_division_t(25),
		// "Normal" values
		jmid::time_division_t(48),jmid::time_division_t(96),jmid::time_division_t(120),
		jmid::time_division_t(152),jmid::time_division_t(960),
		// Max allowed value for ticks-per-qnt
		jmid::time_division_t(0x7FFF)
	};
	std::vector<int32_t> base_exp {
		-3,-2,-1,  // -3 => 8 whole notes, -1 => double-whole
		0,  // whole note
		1,2,3,4,5  // 1 => half note, 5=>32'nd note
	};
	std::vector<int32_t> ndots {
		0,1,2,3
	};

	for (const auto& tdiv: tdivs) {
		for (const auto& exp: base_exp) {
			for (const auto& ndot : ndots) {
				auto nticks = jmid::note2ticks(exp,ndot,tdiv);
				auto note_backcalc = jmid::ticks2note(nticks,tdiv);

				bool bexp = (note_backcalc.exp==exp);
				bool bdot = (note_backcalc.ndot==ndot);

				EXPECT_EQ(note_backcalc.exp,exp);
				EXPECT_EQ(note_backcalc.ndot,ndot);
			}  // to next ndot
		}  // to next pow2
	}  // to next tdiv
}

#include "gtest/gtest.h"
#include "delta_time_test_data.h"
#include "mtrk_event_t.h"
#include <vector>
#include <cstdint>
#include <random>

// 
// Call set_delta_time() on a default-constructed object mtrk_event
// Delta-time test set A contains (+) values, some of which exceed the max
// value of 0x0FFFFFFF; these should be clamped to 0x0FFFFFFF.  
TEST(mtrk_event_class_method_tests, setDtPositiveValues) {
	const auto default_ev = jmid::mtrk_event_t();
	for (const auto& tc : dt_test_set_a) {
		auto ev = jmid::mtrk_event_t();
		ev.set_delta_time(tc.dt_input);
		EXPECT_EQ(ev.delta_time(),tc.ans_value);

		auto it_evbeg = ev.event_begin();
		auto it_end = ev.cend();
		auto it_def_evbeg = default_ev.event_begin();
		auto it_def_end = default_ev.cend();
		ASSERT_EQ((it_def_end-it_def_evbeg),(it_end-it_evbeg));
		while (it_evbeg!=it_end) {
			EXPECT_EQ(*it_evbeg++,*it_def_evbeg++);
		}
	}
}


// Call set_delta_time() on a default-constructed object mtrk_event
// Delta-time test set B contains - values, all of which should be clamped
// to 0.  
TEST(mtrk_event_class_method_tests, setDtNegativeValues) {
	const auto default_ev = jmid::mtrk_event_t();
	for (const auto& tc : dt_test_set_b) {
		auto ev = jmid::mtrk_event_t();
		ev.set_delta_time(tc.dt_input);
		EXPECT_EQ(ev.delta_time(),tc.ans_value);

		auto it_evbeg = ev.event_begin();
		auto it_end = ev.cend();
		auto it_def_evbeg = default_ev.event_begin();
		auto it_def_end = default_ev.cend();
		ASSERT_EQ((it_def_end-it_def_evbeg),(it_end-it_evbeg));
		while (it_evbeg!=it_end) {
			EXPECT_EQ(*it_evbeg++,*it_def_evbeg++);
		}
	}
}


// Call set_delta_time() repeatedly on the same mtrk_event (initially default-
// constructed).  Delta-time test set C contains all values from set A and 
// some from set B.  
TEST(mtrk_event_class_method_tests, CallSetDtRepeatedly) {
	const auto default_ev = jmid::mtrk_event_t();
	auto ev = jmid::mtrk_event_t();
	for (const auto& tc : dt_test_set_c) {
		ev.set_delta_time(tc.dt_input);
		EXPECT_EQ(ev.delta_time(),tc.ans_value);

		auto it_evbeg = ev.event_begin();
		auto it_end = ev.cend();
		auto it_def_evbeg = default_ev.event_begin();
		auto it_def_end = default_ev.cend();
		ASSERT_EQ((it_def_end-it_def_evbeg),(it_end-it_evbeg));
		while (it_evbeg!=it_end) {
			EXPECT_EQ(*it_evbeg++,*it_def_evbeg++);
		}
	}
}


// Call set_delta_time() repeatedly on the same mtrk_event (initially 
// default-constructed).  Delta-times are random.  
TEST(mtrk_event_class_method_tests, CallSetDtRepeatedlyRandomValues) {
	std::random_device rdev;
	std::default_random_engine re(rdev());
	//std::uniform_int_distribution rd(0,0x0FFFFFFF);
	std::geometric_distribution rd(0.5);

	const auto default_ev = jmid::mtrk_event_t();
	auto ev = jmid::mtrk_event_t();
	int32_t dt = 0;
	int32_t prev_dt = 0;
	for (int i=0; i<10000; ++i) {
		prev_dt = dt;
		dt = rd(re);

		ev.set_delta_time(dt);
		EXPECT_EQ(ev.delta_time(),dt) 
			<< "Previous dt==" << prev_dt << ";  dt==" << dt;

		auto it_evbeg = ev.event_begin();
		auto it_end = ev.cend();
		auto it_def_evbeg = default_ev.event_begin();
		auto it_def_end = default_ev.cend();
		ASSERT_EQ((it_def_end-it_def_evbeg),(it_end-it_evbeg));
		while (it_evbeg!=it_end) {
			EXPECT_EQ(*it_evbeg++,*it_def_evbeg++);
		}
	}
}



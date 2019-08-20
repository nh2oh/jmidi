#include "gtest/gtest.h"
#include "midi_vlq.h"
#include "midi_delta_time.h"
#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include "sysex_factory_test_data.h"
#include <vector>
#include <cstdint>


// 
// mtrk_event_t make_sysex_f0(const uint32_t& dt, 
//								std::vector<unsigned char> payload);
//
// Input payloads that lack a terminating 0xF7u; Expect that the factory
// func will add the 0xF7u.  
//
TEST(mtrk_event_sysex_factories, makeSysexF0PayloadsLackTerminalF7) {
	for (const auto& e : f0f7_tests_no_terminating_f7_on_pyld) {
		auto curr_dtN = delta_time_field_size(e.ans_dt);
		auto ans_event_size = 1 + vlq_field_size(e.ans_pyld_len) 
			+ e.ans_pyld_len;
		auto ans_tot_size = curr_dtN + ans_event_size;
		bool ans_is_small = (ans_tot_size<=23);
		auto ans_payload = e.payload_in;  ans_payload.push_back(0xF7u);

		const auto ev = make_sysex_f0(e.dt_in,e.payload_in);

		EXPECT_TRUE(is_sysex(ev));
		EXPECT_TRUE(is_sysex_f0(ev));
		EXPECT_FALSE(is_sysex_f7(ev));

		EXPECT_EQ(ev.delta_time(),e.ans_dt);
		EXPECT_EQ(ev.size(),ans_tot_size);
		EXPECT_EQ(ev.data_size(),ans_event_size);
		EXPECT_TRUE(ev.capacity() >= ev.size());
		EXPECT_EQ(ev.running_status(),0x00u);
		EXPECT_EQ(ev.status_byte(),0xF0u);

		EXPECT_EQ(ev.begin(),ev.dt_begin());
		EXPECT_EQ(ev.event_begin(),ev.dt_end());
		EXPECT_EQ((ev.end()-ev.begin()),ev.size());
		EXPECT_EQ((ev.end()-ev.dt_begin()),ev.size());
		EXPECT_EQ((ev.end()-ev.event_begin()),ans_event_size);
		EXPECT_EQ((ev.end()-ev.payload_begin()),e.ans_pyld_len);
		
		ASSERT_EQ(ans_payload.size(), (ev.end()-ev.payload_begin()));
		auto it = ev.payload_begin();
		for (int i=0; i<ans_payload.size(); ++i) {
			EXPECT_EQ(*it++,ans_payload[i]);
		}
	}
}


// 
// mtrk_event_t make_sysex_f0(const uint32_t& dt, 
//								std::vector<unsigned char> payload);
//
// Input payloads that have one or more terminating 0xF7u elements; Expect 
// that the factory func will _not_ add an 0xF7u to the payload as 
// provided.  
//
TEST(mtrk_event_sysex_factories, makeSysexF0PayloadsWithTerminalF7) {
	for (const auto& e : f0f7_tests_terminating_f7_on_pyld) {
		auto curr_dtN = delta_time_field_size(e.ans_dt);
		auto ans_event_size = 1 + vlq_field_size(e.ans_pyld_len) 
			+ e.ans_pyld_len;
		auto ans_tot_size = curr_dtN + ans_event_size;
		bool ans_is_small = (ans_tot_size<=23);
		auto ans_payload = e.payload_in;

		const auto ev = make_sysex_f0(e.dt_in,e.payload_in);

		EXPECT_TRUE(is_sysex(ev));
		EXPECT_TRUE(is_sysex_f0(ev));
		EXPECT_FALSE(is_sysex_f7(ev));

		EXPECT_EQ(ev.delta_time(),e.ans_dt);
		EXPECT_EQ(ev.size(),ans_tot_size);
		EXPECT_EQ(ev.data_size(),ans_event_size);
		EXPECT_TRUE(ev.capacity() >= ev.size());
		EXPECT_EQ(ev.running_status(),0x00u);
		EXPECT_EQ(ev.status_byte(),0xF0u);

		EXPECT_EQ(ev.begin(),ev.dt_begin());
		EXPECT_EQ(ev.event_begin(),ev.dt_end());
		EXPECT_EQ((ev.end()-ev.begin()),ev.size());
		EXPECT_EQ((ev.end()-ev.dt_begin()),ev.size());
		EXPECT_EQ((ev.end()-ev.event_begin()),ans_event_size);
		EXPECT_EQ((ev.end()-ev.payload_begin()),e.ans_pyld_len);
		
		ASSERT_EQ(ans_payload.size(), (ev.end()-ev.payload_begin()));
		auto it = ev.payload_begin();
		for (int i=0; i<ans_payload.size(); ++i) {
			EXPECT_EQ(*it++,ans_payload[i]);
		}
	}
}



// 
// mtrk_event_t make_sysex_f7(const uint32_t& dt, 
//								std::vector<unsigned char> payload);
//
// Input payloads that lack a terminating 0xF7u; Expect that the factory
// func will add the 0xF7u.  
//
TEST(mtrk_event_sysex_factories, makeSysexF7PayloadsLackTerminalF7) {
	for (const auto& e : f0f7_tests_no_terminating_f7_on_pyld) {
		auto curr_dtN = delta_time_field_size(e.ans_dt);
		auto ans_event_size = 1 + vlq_field_size(e.ans_pyld_len) 
			+ e.ans_pyld_len;
		auto ans_tot_size = curr_dtN + ans_event_size;
		bool ans_is_small = (ans_tot_size<=23);
		auto ans_payload = e.payload_in;  ans_payload.push_back(0xF7u);

		const auto ev = make_sysex_f7(e.dt_in,e.payload_in);

		EXPECT_TRUE(is_sysex(ev));
		EXPECT_FALSE(is_sysex_f0(ev));
		EXPECT_TRUE(is_sysex_f7(ev));

		EXPECT_EQ(ev.delta_time(),e.ans_dt);
		EXPECT_EQ(ev.size(),ans_tot_size);
		EXPECT_EQ(ev.data_size(),ans_event_size);
		EXPECT_TRUE(ev.capacity() >= ev.size());
		EXPECT_EQ(ev.running_status(),0x00u);
		EXPECT_EQ(ev.status_byte(),0xF7u);

		EXPECT_EQ(ev.begin(),ev.dt_begin());
		EXPECT_EQ(ev.event_begin(),ev.dt_end());
		EXPECT_EQ((ev.end()-ev.begin()),ev.size());
		EXPECT_EQ((ev.end()-ev.dt_begin()),ev.size());
		EXPECT_EQ((ev.end()-ev.event_begin()),ans_event_size);
		EXPECT_EQ((ev.end()-ev.payload_begin()),e.ans_pyld_len);
		
		ASSERT_EQ(ans_payload.size(), (ev.end()-ev.payload_begin()));
		auto it = ev.payload_begin();
		for (int i=0; i<ans_payload.size(); ++i) {
			EXPECT_EQ(*it++,ans_payload[i]);
		}
	}
}


// 
// mtrk_event_t make_sysex_f7(const uint32_t& dt, 
//								std::vector<unsigned char> payload);
//
// Input payloads that have one or more terminating 0xF7u elements; Expect 
// that the factory func will _not_ add an 0xF7u to the payload as 
// provided.  
//
TEST(mtrk_event_sysex_factories, makeSysexF7PayloadsWithTerminalF7) {
	for (const auto& e : f0f7_tests_terminating_f7_on_pyld) {
		auto curr_dtN = delta_time_field_size(e.ans_dt);
		auto ans_event_size = 1 + vlq_field_size(e.ans_pyld_len) 
			+ e.ans_pyld_len;
		auto ans_tot_size = curr_dtN + ans_event_size;
		bool ans_is_small = (ans_tot_size<=23);
		auto ans_payload = e.payload_in;

		const auto ev = make_sysex_f7(e.dt_in,e.payload_in);

		EXPECT_TRUE(is_sysex(ev));
		EXPECT_FALSE(is_sysex_f0(ev));
		EXPECT_TRUE(is_sysex_f7(ev));

		EXPECT_EQ(ev.delta_time(),e.ans_dt);
		EXPECT_EQ(ev.size(),ans_tot_size);
		EXPECT_EQ(ev.data_size(),ans_event_size);
		EXPECT_TRUE(ev.capacity() >= ev.size());
		EXPECT_EQ(ev.running_status(),0x00u);
		EXPECT_EQ(ev.status_byte(),0xF7u);

		EXPECT_EQ(ev.begin(),ev.dt_begin());
		EXPECT_EQ(ev.event_begin(),ev.dt_end());
		EXPECT_EQ((ev.end()-ev.begin()),ev.size());
		EXPECT_EQ((ev.end()-ev.dt_begin()),ev.size());
		EXPECT_EQ((ev.end()-ev.event_begin()),ans_event_size);
		EXPECT_EQ((ev.end()-ev.payload_begin()),e.ans_pyld_len);
		
		ASSERT_EQ(ans_payload.size(), (ev.end()-ev.payload_begin()));
		auto it = ev.payload_begin();
		for (int i=0; i<ans_payload.size(); ++i) {
			EXPECT_EQ(*it++,ans_payload[i]);
		}
	}
}



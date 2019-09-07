#include "gtest/gtest.h"
#include "make_mtrk_event.h"
#include "midi_raw_test_data.h"
#include <vector>
#include <cstdint>


//
// maybe_mtrk_event_t make_mtrk_event(const unsigned char*,
//			const unsigned char*, unsigned char, mtrk_event_error_t*);
// maybe_mtrk_event_t make_mtrk_event(int32_t, const unsigned char*,
//			const unsigned char*, unsigned char,mtrk_event_error_t*);
//
// A set of "small" (fit in the mtrk_event_t object) midi_channel,
// meta, sysex_f0/f7 events from the midi std and from files observed 
// in the wild.  
//
// All events have an event-local status byte; all tests pass 0x00u for the
// value of the running-status.  
//
TEST(make_mtrk_event3_tests, assortedMidiEventsNoRS) {
	struct test_t {
		std::vector<unsigned char> bytes {};
		int32_t delta_time {0};
		int32_t size {0};
		int32_t data_size {0};
	};

	std::vector<test_t> tests {
		// From p.142 of the midi std
		{{0x00,0xFF,0x58,0x04,0x04,0x02,0x18,0x08},  // meta, Time sig
			0x00,8,7},
		{{0x00,0xFF,0x51,0x03,0x07,0xA1,0x20},  // meta, Tempo
			0x00,7,6},
		{{0x00,0xFF,0x2F,0x00},  // meta, End of track
			0x00,4,3},

		// From p. 136 of the midi std:
		{{0x00,0xF0,0x03,0x43,0x12,0x00},  // sysex_f0
			0x00,6,5},
		{{0x81,0x48,0xF7,0x06,0x43,0x12,0x00,0x43,0x12,0x00},  // sysex_f7
			200,10,8},
		{{0x64,0xF7,0x04,0x43,0x12,0x00,0xF7},  // sysex_f7
			100,7,6},

		// From p. 141 of the midi std:
		{{0x00,0x92,0x30,0x60},  // Ch. 3 Note On #48, forte; dt=0
			0x00,4,3},
		{{0x81,0x48,0x82,0x30,0x60},  // Ch. 3 Note Off #48, standard; dt=200
			200,5,3}
	};

	for (const auto& tc : tests) {
		auto ev = jmid::make_mtrk_event3(tc.bytes.data(),
			tc.bytes.data()+tc.bytes.size(),0,nullptr);
		EXPECT_TRUE(ev.size()!=0);
		EXPECT_EQ(ev.delta_time(),tc.delta_time);

		EXPECT_EQ(ev.size(),tc.size);
		EXPECT_EQ(ev.data_size(),tc.data_size);
		ASSERT_TRUE(ev.size()<=tc.bytes.size());
		for (int i=0; i<ev.size(); ++i) {
			auto a = ev[i];
			auto b = tc.bytes[i];
			EXPECT_EQ(ev[i],tc.bytes[i]);
		}

		// Test of the caller-supplied-dt overload
		/*auto dt_end = tc.bytes.data() + (tc.size-tc.data_size);
		maybe_ev = jmid::make_mtrk_event(dt_end,
			tc.bytes.data()+tc.bytes.size(),tc.delta_time,0,
			nullptr,tc.bytes.size());
		EXPECT_TRUE(maybe_ev);
		EXPECT_EQ(maybe_ev.event.delta_time(),tc.delta_time);

		auto sz = maybe_ev.event.size();
		EXPECT_EQ(maybe_ev.event.size(),tc.size);
		EXPECT_EQ(maybe_ev.event.data_size(),tc.data_size);
		for (int i=0; i<maybe_ev.event.size(); ++i) {
			auto a = maybe_ev.event[i];
			auto b = tc.bytes[i];
			EXPECT_EQ(maybe_ev.event[i],tc.bytes[i]);
		}*/
	}
}


//
// maybe_mtrk_event_t make_mtrk_event(const unsigned char*,
//			const unsigned char*, unsigned char, mtrk_event_error_t*);
// maybe_mtrk_event_t make_mtrk_event(int32_t, const unsigned char*,
//			const unsigned char*, unsigned char,mtrk_event_error_t*);
//
// A set of "small" (fit in the mtrk_event_t object) midi_channel events
// from a random set i generated on 04/21/19.  All test cases are valid midi
// events.  Some are in running-status (do not have an event-local status 
// byte), others not.  The delta-times are random.  
// The value in tests_t.midisb_prev_event is meant to represent the status 
// byte of the prior event in a ficticious stream of events, and represents
// the running status.  
// For those events with no event-local status-byte, the running-status 
// supplied w/ the test case (tests_t.midisb_prev_event) is valid as a 
// running-status value and accurately describes the number of data bytes 
// in the event.
// For those events that _do_ have a valid event-local status byte, the 
// value in tests_t.midisb_prev_event is sometimes not valid as a 
// running-status (ex, it's == 0xFFu or something).  
//
TEST(make_mtrk_event3_tests, randomChEvntsSmallRandomRS) {
	// Fields applic_midi_status, n_data_bytes are not (yet) tested here
	struct test_t {
		std::vector<unsigned char> data {};
		unsigned char midisb_prev_event {};  // "midi status byte previous event"
		unsigned char applic_midi_status {};  // "applicable midi status byte"
		uint8_t n_data_bytes {};  // Based on value of applic_midi_status
		uint32_t data_length {};  // n_data_bytes (+ 1 if not in running status)
		uint32_t dt_value {};
		uint8_t dt_field_size {};
	};
	std::vector<test_t> tests {
		{{0xC0,0x80,0x00,0x01},0xD0,0xD0,1,1,1048576,3},
		{{0x81,0x80,0x80,0x00,0xC1,0x00},0xC0,0xC1,1,2,2097152,4},
		{{0xC0,0x00,0x09},0xC0,0xC0,1,1,8192,2},
		{{0xC0,0x80,0x80,0x00,0xC1,0x70},0x8A,0xC1,1,2,134217728,4},
		{{0x7F,0x00,0x0F},0x81,0x81,2,2,127,1},
		{{0x00,0xD1,0x0F},0xD1,0xD1,1,2,0,1},
		{{0xFF,0xFF,0xFF,0x7F,0x77,0x70},0x80,0x80,2,2,268435455,4},
		{{0x81,0x00,0x10},0xDF,0xDF,1,1,128,2},
		{{0xFF,0xFF,0x7F,0x90,0x09,0x0A},0xC0,0x90,2,3,2097151,3},
		{{0xFF,0x7F,0xC0,0x09},0xD1,0xC0,1,2,16383,2},
		{{0xFF,0xFF,0x7F,0xC0,0x0F},0x90,0xC0,1,2,2097151,3},
		{{0x81,0x48,0xE1,0x77,0x03},0xDF,0xE1,2,3,200,2},
		{{0x81,0x48,0x01,0x02},0x9F,0x9F,2,2,200,2},
		{{0x81,0x80,0x00,0x70},0xCB,0xCB,1,1,16384,3},
		{{0x81,0x80,0x00,0x01,0x01},0xB1,0xB1,2,2,16384,3},
		{{0x81,0x00,0x01,0x03},0x90,0x90,2,2,128,2},
		{{0xC0,0x80,0x00,0x9B,0x02,0x0F},0x91,0x9B,2,3,1048576,3},
		{{0x81,0x80,0x00,0x10},0xDB,0xDB,1,1,16384,3},
		{{0xC0,0x80,0x80,0x00,0x8F,0x03,0x01},0x81,0x8F,2,3,134217728,4},
		{{0xFF,0x7F,0xEB,0x77,0x0A},0x7F,0xEB,2,3,16383,2},
		{{0x7F,0x09},0xC1,0xC1,1,1,127,1},
		{{0x81,0x00,0x80,0x03,0x03},0xEB,0x80,2,3,128,2},
		{{0xC0,0x00,0x0A,0x0A},0x91,0x91,2,2,8192,2},
		{{0x81,0x48,0xCB,0x0F},0xD0,0xCB,1,2,200,2},
		{{0xFF,0x7F,0x90,0x70,0x0F},0xDF,0x90,2,3,16383,2},
		{{0x81,0x80,0x00,0xDB,0x10},0xEB,0xDB,1,2,16384,3},
		{{0x81,0x48,0x01,0x03},0xEB,0xEB,2,2,200,2},
		{{0x81,0x48,0x02},0xD1,0xD1,1,1,200,2},
		{{0xFF,0x7F,0x90,0x0A,0x10},0xDF,0x90,2,3,16383,2},
		{{0x40,0x01,0x00},0xE0,0xE0,2,2,64,1},
		{{0xC0,0x80,0x00,0x77,0x10},0x9F,0x9F,2,2,1048576,3},
		{{0x81,0x80,0x80,0x00,0x90,0x09,0x0F},0xB1,0x90,2,3,2097152,4},
		{{0xFF,0x7F,0xCB,0x0F},0xEB,0xCB,1,2,16383,2},
		{{0x81,0x80,0x80,0x00,0x02},0xDF,0xDF,1,1,2097152,4},
		{{0x81,0x48,0xDB,0x77},0xC0,0xDB,1,2,200,2},
		{{0x81,0x48,0xDB,0x00},0xBF,0xDB,1,2,200,2},
		{{0x81,0x48,0xEF,0x0F,0x0F},0x90,0xEF,2,3,200,2},
		{{0xFF,0xFF,0xFF,0x7F,0x03,0x01},0xEB,0xEB,2,2,268435455,4},
		{{0xFF,0xFF,0xFF,0x7F,0x77,0x10},0x8A,0x8A,2,2,268435455,4},
		{{0xC0,0x00,0xC1,0x00},0xE0,0xC1,1,2,8192,2},
		{{0xC0,0x80,0x00,0xC1,0x70},0x80,0xC1,1,2,1048576,3},
		{{0x7F,0x77},0xCF,0xCF,1,1,127,1},
		{{0x81,0x80,0x80,0x00,0x02,0x02},0x90,0x90,2,2,2097152,4},
		{{0x81,0x00,0xC1,0x70},0xB0,0xC1,1,2,128,2},
		{{0xC0,0x80,0x80,0x00,0xD0,0x10},0xE0,0xD0,1,2,134217728,4},
		{{0x81,0x80,0x80,0x00,0xC0,0x10},0xEB,0xC0,1,2,2097152,4},
		{{0x64,0x70,0x03},0xE1,0xE1,2,2,100,1},
		{{0x81,0x80,0x80,0x00,0x80,0x0A,0x77},0xF0,0x80,2,3,2097152,4},
		{{0x81,0x80,0x00,0x01,0x0F},0x8F,0x8F,2,2,16384,3},
		{{0x81,0x80,0x00,0x02,0x0A},0xEF,0xEF,2,2,16384,3},
		{{0x81,0x80,0x80,0x00,0x02},0xDF,0xDF,1,1,2097152,4},
		{{0x7F,0x0F,0x70},0xE0,0xE0,2,2,127,1},
		{{0x7F,0x00},0xDF,0xDF,1,1,127,1},
		{{0x7F,0x01,0x70},0x9B,0x9B,2,2,127,1},
		{{0x7F,0x01},0xDB,0xDB,1,1,127,1},
		{{0xFF,0x7F,0x0A},0xCF,0xCF,1,1,16383,2},
		{{0x81,0x48,0xD0,0x01},0x7F,0xD0,1,2,200,2},
		{{0xC0,0x00,0xCF,0x77},0x8A,0xCF,1,2,8192,2},
		{{0xFF,0xFF,0xFF,0x7F,0x0A,0x02},0xEF,0xEF,2,2,268435455,4},
		{{0x81,0x00,0xDF,0x02},0xC0,0xDF,1,2,128,2},
		{{0xC0,0x00,0x70,0x09},0xEF,0xEF,2,2,8192,2},
		{{0xC0,0x80,0x80,0x00,0xBB,0x00,0x01},0xCB,0xBB,2,3,134217728,4},
		{{0xC0,0x00,0xCB,0x01},0x9B,0xCB,1,2,8192,2},
		{{0x81,0x00,0x00,0x00},0xE0,0xE0,2,2,128,2},
		{{0xFF,0xFF,0x7F,0x8A,0x01,0x02},0xBF,0x8A,2,3,2097151,3},
		{{0xFF,0xFF,0x7F,0x10,0x77},0x90,0x90,2,2,2097151,3},
		{{0xC0,0x80,0x80,0x00,0xDB,0x03},0x9F,0xDB,1,2,134217728,4},
		{{0x40,0xCF,0x77},0x8F,0xCF,1,2,64,1},
		{{0x7F,0x01},0xD1,0xD1,1,1,127,1},
		{{0xFF,0xFF,0xFF,0x7F,0x0A,0x70},0x80,0x80,2,2,268435455,4},
		{{0xC0,0x00,0x70},0xD1,0xD1,1,1,8192,2},
		{{0x64,0x03,0x77},0xB1,0xB1,2,2,100,1},
		{{0x00,0xCB,0x70},0x81,0xCB,1,2,0,1},
		{{0x64,0xD0,0x0A},0x7F,0xD0,1,2,100,1},
		{{0xC0,0x80,0x00,0x77},0xC0,0xC0,1,1,1048576,3},
		{{0x81,0x80,0x80,0x00,0xE1,0x10,0x09},0xB0,0xE1,2,3,2097152,4},
		{{0x81,0x80,0x80,0x00,0xD0,0x01},0x8A,0xD0,1,2,2097152,4},
		{{0x81,0x80,0x00,0xCF,0x70},0x9F,0xCF,1,2,16384,3},
		{{0x7F,0x9F,0x09,0x10},0xB0,0x9F,2,3,127,1},
		{{0x00,0x90,0x0A,0x02},0x81,0x90,2,3,0,1},
		{{0xFF,0xFF,0x7F,0x0A},0xDF,0xDF,1,1,2097151,3},
		{{0xFF,0xFF,0x7F,0x91,0x00,0x0F},0xB1,0x91,2,3,2097151,3},
		{{0x81,0x48,0x02},0xD0,0xD0,1,1,200,2},
		{{0x81,0x00,0xDB,0x02},0xD0,0xDB,1,2,128,2},
		{{0x7F,0xC1,0x00},0xDF,0xC1,1,2,127,1},
		{{0xC0,0x00,0xB0,0x03,0x0A},0xCB,0xB0,2,3,8192,2},
		{{0x81,0x48,0x10},0xDB,0xDB,1,1,200,2},
		{{0x00,0xDF,0x02},0x8F,0xDF,1,2,0,1},
		{{0x81,0x48,0x70,0x10},0x9F,0x9F,2,2,200,2},
		{{0xC0,0x80,0x80,0x00,0x01},0xD0,0xD0,1,1,134217728,4},
		{{0xFF,0xFF,0x7F,0x0A},0xC1,0xC1,1,1,2097151,3},
		{{0x81,0x80,0x00,0xCB,0x09},0x91,0xCB,1,2,16384,3},
		{{0xFF,0xFF,0x7F,0xEF,0x09,0x09},0xB1,0xEF,2,3,2097151,3},
		{{0xFF,0x7F,0xB1,0x0F,0x00},0xF0,0xB1,2,3,16383,2},
		{{0xC0,0x00,0xB1,0x70,0x0A},0x8F,0xB1,2,3,8192,2},
		{{0x7F,0xB1,0x03,0x10},0x80,0xB1,2,3,127,1},
		{{0x00,0xDF,0x03},0x8F,0xDF,1,2,0,1},
		{{0x81,0x00,0x03},0xCB,0xCB,1,1,128,2},
		{{0x64,0x9B,0x0F,0x03},0xDF,0x9B,2,3,100,1},
		{{0xC0,0x80,0x80,0x00,0x90,0x03,0x0A},0xD1,0x90,2,3,134217728,4}
	};

	for (auto& tc : tests) {
		auto data_size_with_status_byte = tc.n_data_bytes + 1;
		bool input_is_in_rs = tc.data_length < data_size_with_status_byte;
		auto size_with_status_byte = tc.dt_field_size 
			+ data_size_with_status_byte;
		// NB:  size_with_status_byte is the expected final event.size()
		auto data_no_rs = tc.data;
		if (input_is_in_rs) {
			data_no_rs.insert(data_no_rs.begin()+tc.dt_field_size,
				tc.applic_midi_status);
		}

		auto ev = jmid::make_mtrk_event3(tc.data.data(),
			tc.data.data()+tc.data.size(),tc.midisb_prev_event,nullptr);
		// NB:  I could set the max event size to something 
		// > size_with_status_byte, but not smaller.  
		EXPECT_TRUE(ev.size() != 0);
		EXPECT_EQ(ev.delta_time(),tc.dt_value);

		EXPECT_EQ(ev.size(),size_with_status_byte);
		EXPECT_EQ(ev.data_size(),data_size_with_status_byte);
		ASSERT_TRUE(ev.size()<=data_no_rs.size());
		for (int i=0; i<ev.size(); ++i) {
			EXPECT_EQ(ev[i],data_no_rs[i]);
		}

		// Test of the caller-suppleid-dt overload
		/*
		auto dt_end = tc.data.data() + tc.dt_field_size;
		maybe_ev = jmid::make_mtrk_event(dt_end,
			tc.data.data()+tc.data.size(),tc.dt_value,tc.midisb_prev_event,
			nullptr, size_with_status_byte);
		// NB:  I could set the max event size to something 
		// > size_with_status_byte, but not smaller.  
		EXPECT_TRUE(maybe_ev);
		EXPECT_EQ(maybe_ev.event.delta_time(),tc.dt_value);

		EXPECT_EQ(maybe_ev.event.size(),size_with_status_byte);
		EXPECT_EQ(maybe_ev.event.data_size(),data_size_with_status_byte);
		for (int i=0; i<maybe_ev.event.size(); ++i) {
			EXPECT_EQ(maybe_ev.event[i],data_no_rs[i]);
		}*/
	}
}


//
// maybe_mtrk_event_t make_mtrk_event(const unsigned char*,
//			const unsigned char*, unsigned char, mtrk_event_error_t*);
// maybe_mtrk_event_t make_mtrk_event(int32_t, const unsigned char*,
//			const unsigned char*, unsigned char,mtrk_event_error_t*);
//
// A set of "small" (fit in the mtrk_event_t object) meta events from the 
// midi std and from files observed in the wild.  
//
// All tests pass 0x00u for the value of the running-status.  
//
TEST(make_mtrk_event3_tests, metaEventsSmallNoRS) {
	struct p142tests_t {
		std::vector<unsigned char> bytes {};
		int32_t delta_time {0};
		int32_t size {0};
		int32_t data_size {0};
	};

	std::vector<p142tests_t> tests {
		// From p.142 of the midi std
		{{0x00,0xFF,0x58,0x04,0x04,0x02,0x18,0x08},  // Time sig
			0x00,8,7},

		{{0x00,0xFF,0x51,0x03,0x07,0xA1,0x20},  // Tempo
			0x00,7,6},

		{{0x00,0xFF,0x2F,0x00},  // End of track
			0x00,4,3},

		// Not from the midi std:
		{{0x00,0xFF,0x51,0x03,0x07,0xA1,0x20},  // Tempo (CLEMENTI.MID)
			0x00,7,6},

		// Padded w/ zeros, but otherwise identical to above
		{{0x00,0xFF,0x51,0x03,0x07,0xA1,0x20,0x00,0x00,0x00,0x00},  // Tempo (CLEMENTI.MID)
			0x00,7,6},

		{{0x00,0xFF,0x01,0x10,0x48,0x61,0x72,0x70,0x73,0x69,0x63,0x68,
		0x6F,0x72,0x64,0x20,0x48,0x69,0x67,0x68},  // Text element "Harpsichord High"
			0x00,20,19}
	};
	
	for (const auto& tc : tests) {
		auto ev = jmid::make_mtrk_event3(tc.bytes.data(),
			tc.bytes.data()+tc.bytes.size(),0,nullptr);
		EXPECT_TRUE(ev.size() != 0);
		EXPECT_EQ(ev.delta_time(),tc.delta_time);

		EXPECT_EQ(ev.size(),tc.size);
		EXPECT_EQ(ev.data_size(),tc.data_size);
		ASSERT_TRUE(ev.size()<=tc.bytes.size());
		for (int i=0; i<ev.size(); ++i) {
			EXPECT_EQ(ev[i],tc.bytes[i]);
		}

		// Test of the caller-supplied-dt overload
		/*
		auto dt_end = tc.bytes.data() + (tc.size-tc.data_size);
		maybe_ev = jmid::make_mtrk_event(dt_end,
			tc.bytes.data()+tc.bytes.size(),tc.delta_time,0,
			nullptr,tc.bytes.size()+100);
		EXPECT_TRUE(maybe_ev);
		EXPECT_EQ(maybe_ev.event.delta_time(),tc.delta_time);

		EXPECT_EQ(maybe_ev.event.size(),tc.size);
		EXPECT_EQ(maybe_ev.event.data_size(),tc.data_size);
		for (int i=0; i<maybe_ev.event.size(); ++i) {
			EXPECT_EQ(maybe_ev.event[i],tc.bytes[i]);
		}*/
	}
}


//
// maybe_mtrk_event_t make_mtrk_event(const unsigned char*,
//			const unsigned char*, unsigned char, mtrk_event_error_t*);
// maybe_mtrk_event_t make_mtrk_event(int32_t, const unsigned char*,
//			const unsigned char*, unsigned char,mtrk_event_error_t*);
//
// A set of "big" (do _not_ fit in the mtrk_event_t object) meta events 
// randomly generated and observed in the wild.  
//
// All tests pass 0x00u for the value of the running-status.  
//
TEST(make_mtrk_event3_tests, metaEventsBigNoRS) {
	struct test_t {
		std::vector<unsigned char> bytes {};
		int32_t delta_time {0};
		int32_t size {0};
		int32_t data_size {0};
	};

	std::vector<test_t> tests {
		// Sequence/track name (Hallelujah.mid)
		{{0x00,0xFF,0x03,0x1D,0x48,0x61,0x6C,0x6C,0x65,0x6C,0x75,0x6A,
				0x61,0x68,0x21,0x20,0x4A,0x6F,0x79,0x20,0x74,0x6F,0x20,0x74,
				0x68,0x65,0x20,0x57,0x6F,0x72,0x6C,0x64,0x21},
			0x00,33,32  
		},

		// Text event; 127 (0x7F) chars
		{{0x00,0xFF,0x01,0x7F,
				0x6D,0x6F,0x75,0x6E,0x74,0x20,0x6F,0x66,0x20,0x74,0x65,0x78,
				0x74,0x20,0x64,0x65,0x73,0x63,0x72,0x69,0x62,0x69,0x6E,0x67,
				0x20,0x61,0x6E,0x79,0x74,0x68,0x69,0x6E,0x67,0x2E,0x20,0x49,
				0x74,0x20,0x69,0x73,0x20,0x61,0x20,0x67,0x6F,0x6F,0x64,0x20,
				0x69,0x64,0x65,0x61,0x20,0x74,0x6F,0x20,0x70,0x75,0x74,0x20,
				0x61,0x20,0x74,0x65,0x78,0x74,0x20,0x65,0x76,0x65,0x6E,0x74,
				0x20,0x72,0x69,0x67,0x68,0x74,0x20,0x61,0x74,0x20,0x74,0x68,
				0x65,0x0D,0x0A,0x62,0x65,0x67,0x69,0x6E,0x6E,0x69,0x6E,0x67,
				0x20,0x6F,0x66,0x20,0x61,0x20,0x74,0x72,0x61,0x63,0x6B,0x2C,
				0x20,0x77,0x69,0x74,0x68,0x20,0x74,0x68,0x65,0x20,0x6E,0x61,
				0x6D,0x65,0x20,0x6F,0x66,0x20,0x74},  
			0x00,131,130
		},

		// Text event; 95 (0x5F) chars
		{{0x00,0xFF,0x01,0x5F,
				0x6D,0x6F,0x75,0x6E,0x74,0x20,0x6F,0x66,0x20,0x74,0x65,0x78,
				0x74,0x20,0x64,0x65,0x73,0x63,0x72,0x69,0x62,0x69,0x6E,0x67,
				0x20,0x61,0x6E,0x79,0x74,0x68,0x69,0x6E,0x67,0x2E,0x20,0x49,
				0x74,0x20,0x69,0x73,0x20,0x61,0x20,0x67,0x6F,0x6F,0x64,0x20,
				0x69,0x64,0x65,0x61,0x20,0x74,0x6F,0x20,0x70,0x75,0x74,0x20,
				0x61,0x20,0x74,0x65,0x78,0x74,0x20,0x65,0x76,0x65,0x6E,0x74,
				0x20,0x72,0x69,0x67,0x68,0x74,0x20,0x61,0x74,0x20,0x74,0x68,
				0x6D,0x65,0x20,0x6F,0x66,0x20,0x74,0x74,0x74,0x74,0x74},  
			0x00,99,98
		}
	};

	for (const auto& tc : tests) {
		auto ev = jmid::make_mtrk_event3(tc.bytes.data(),
			tc.bytes.data()+tc.bytes.size(),0,nullptr);
		EXPECT_TRUE(ev.size() != 0);
		EXPECT_EQ(ev.delta_time(),tc.delta_time);

		EXPECT_EQ(ev.size(),tc.size);
		EXPECT_EQ(ev.data_size(),tc.data_size);
		ASSERT_TRUE(ev.size()<=tc.bytes.size());
		for (int i=0; i<ev.size(); ++i) {
			EXPECT_EQ(ev[i],tc.bytes[i]);
		}

		// Test of the caller-supplied-dt overload
		/*
		auto dt_end = tc.bytes.data() + (tc.size-tc.data_size);
		maybe_ev = jmid::make_mtrk_event(dt_end,
			tc.bytes.data()+tc.bytes.size(),tc.delta_time,0,nullptr,
			tc.bytes.size());
		EXPECT_TRUE(maybe_ev);
		EXPECT_EQ(maybe_ev.event.delta_time(),tc.delta_time);

		EXPECT_EQ(maybe_ev.event.size(),tc.size);
		EXPECT_EQ(maybe_ev.event.data_size(),tc.data_size);
		for (int i=0; i<maybe_ev.event.size(); ++i) {
			EXPECT_EQ(maybe_ev.event[i],tc.bytes[i]);
		}*/
	}
}



//
// maybe_mtrk_event_t make_mtrk_event(const unsigned char*,
//			const unsigned char*, unsigned char, mtrk_event_error_t*);
// maybe_mtrk_event_t make_mtrk_event(int32_t, const unsigned char*,
//			const unsigned char*, unsigned char,mtrk_event_error_t*);
//
// A set of "small" (fit in the mtrk_event_t object) midi_channel events.  
// All test cases are valid midi events.  Some are in running-status (do 
// not have an event-local status byte), others not.  The delta-times are
// more or less random.  
// The value in tests_t.midisb_prev_event is meant to represent the status 
// byte of the prior event in a ficticious stream of events, and represents
// the running status.  
// For those events with no event-local status-byte, the running-status 
// supplied w/ the test case (tests_t.midisb_prev_event) is valid as a 
// running-status value and accurately describes the number of data bytes 
// in the event.
// For those events that _do_ have a valid event-local status byte, the 
// value in tests_t.midisb_prev_event is sometimes not valid as a 
// running-status (ex, it's == 0xFFu or something).  
//
TEST(make_mtrk_event3_tests, assortedChEvntsSmallRandomRSTestSetC) {
	// Fields applic_midi_status, n_data_bytes are not (yet) tested here
	for (auto& tc : set_c_midi_events_valid) {
		auto data_size_with_status_byte = tc.n_data_bytes + 1;
		bool input_is_in_rs = tc.data_length < data_size_with_status_byte;
		auto size_with_status_byte = tc.dt_field_size 
			+ data_size_with_status_byte;
		auto data_no_rs = tc.data;
		if (input_is_in_rs) {
			data_no_rs.insert(data_no_rs.begin()+tc.dt_field_size,
				tc.applic_midi_status);
		}

		auto ev = jmid::make_mtrk_event3(tc.data.data(),
			tc.data.data()+tc.data.size(),tc.midisb_prev_event,
			nullptr);
		EXPECT_TRUE(ev.size() != 0);
		EXPECT_EQ(ev.delta_time(),tc.dt_value);

		EXPECT_EQ(ev.size(),size_with_status_byte);
		EXPECT_EQ(ev.data_size(),data_size_with_status_byte);
		ASSERT_TRUE(ev.size()<=data_no_rs.size());
		for (int i=0; i<ev.size(); ++i) {
			EXPECT_EQ(ev[i],data_no_rs[i]);
		}

		// Test of the caller-supplied-dt overload
		/*
		auto dt_end = tc.data.data() + tc.dt_field_size;
		maybe_ev = jmid::make_mtrk_event(dt_end,
			tc.data.data()+tc.data.size(),tc.dt_value,tc.midisb_prev_event,
			nullptr,size_with_status_byte);
		EXPECT_TRUE(maybe_ev);
		EXPECT_EQ(maybe_ev.event.delta_time(),tc.dt_value);

		EXPECT_EQ(maybe_ev.event.size(),size_with_status_byte);
		EXPECT_EQ(maybe_ev.event.data_size(),data_size_with_status_byte);
		for (int i=0; i<maybe_ev.event.size(); ++i) {
			EXPECT_EQ(maybe_ev.event[i],data_no_rs[i]);
		}*/
	}
}


//
// maybe_mtrk_event_t make_mtrk_event(const unsigned char*,
//			const unsigned char*, unsigned char, mtrk_event_error_t*);
// maybe_mtrk_event_t make_mtrk_event(int32_t, const unsigned char*,
//			const unsigned char*, unsigned char,mtrk_event_error_t*);
//
// A set of channel event data, "invalid" for lack of a valid event-local
// & running-status byte.  
//
TEST(make_mtrk_event3_tests, assortedChEvntsSmallInvalidTestSetD) {
	for (auto& tc : set_d_midi_events_nostatus_invalid) {
		jmid::mtrk_event_error_t err;
		auto ev = jmid::make_mtrk_event3(tc.data.data(),
			tc.data.data()+tc.data.size(),tc.midisb_prev_event,&err);
		EXPECT_TRUE(ev.size() == 0);
	}
}


//
// Sets of meta events, sysex_f0/f7 events, and midi events, paired with 
// "random" but valid running-status bytes.  Events also have dt fields of
// varying size.  
//
TEST(make_mtrk_event3_tests, RandomEventsAllRSBytesValid) {
	for (const auto& tc : set_a_valid_rs) {
		jmid::mtrk_event_error_t err;
		auto ev = jmid::make_mtrk_event3(tc.data.data(),
			tc.data.data()+tc.data.size(),tc.rs_pre,&err);
		EXPECT_TRUE(ev.size() != 0);
		EXPECT_EQ(ev.running_status(),tc.rs_post);
		
		auto sz_dt = ev.event_begin()-ev.begin();
		EXPECT_EQ(sz_dt,tc.dtsize);
	}
}


//
// Sets of meta events, sysex_f0/f7 events, and midi events, paired with 
// "random" but *invalid* running-status bytes.  Events also have dt fields of
// varying size.  
// 
TEST(make_mtrk_event3_tests, RandomMtrkEventsAllRSBytesInvalid) {
	for (const auto& tc : set_b_invalid_rs) {
		jmid::mtrk_event_error_t err;
		auto ev = jmid::make_mtrk_event3(tc.data.data(),
			tc.data.data()+tc.data.size(), tc.rs_pre, &err);

		// In this set, events w/o an event-local status byte are
		// uninterpretible, since all rs bytes are invalid.  
		const unsigned char *p = &(tc.data[0]);
		auto maybe_loc_sb = *(p+tc.dtsize);
		if ((maybe_loc_sb&0x80u) != 0x80) { 
			EXPECT_TRUE(ev.size() == 0);
			continue;
		}

		EXPECT_TRUE(ev.size() != 0);
		EXPECT_EQ(ev.running_status(),tc.rs_post);
		
		auto sz_dt = ev.event_begin()-ev.begin();
		EXPECT_EQ(sz_dt,tc.dtsize);
	}
}


//
// Part 3:  midi events only; running-status may or may not be valid, but all 
// composite events (data+rs byte) are valid and interpretible.  
//
// Set of midi events, paired with "random" but valid and invalid running-
// status bytes.  Those events paired w/ invalid rs bytes have a valid
// event-local status byte.  Events w/ valid rs bytes may or may not contain
// an event-local status byte; if not, the rs byte correctly describes the
// event.  Events also have dt fields of varying size.  
//
TEST(make_mtrk_event3_tests, RandomMIDIEventsRSandNonRS) {
	for (const auto& tc : set_c_midi_events_valid) {
		jmid::mtrk_event_error_t err;
		auto ev = jmid::make_mtrk_event3(tc.data.data(),
			tc.data.data()+tc.data.size(),tc.midisb_prev_event,&err);
		EXPECT_TRUE(ev.size() != 0);
		EXPECT_EQ(ev.running_status(),tc.applic_midi_status);
	}
}


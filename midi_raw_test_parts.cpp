#include "midi_raw_test_parts.h"
#include "midi_raw.h"
#include "dbklib\byte_manipulation.h"
#include <vector>
#include <cstdint>
#include <limits>
#include <random>
#include <array>
#include <string>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <cmath>  // std::log for delta-time generation

namespace testdata {
// Assorted dt fields; can be prepended to the meta, sysex, and midi events
// below.  
// dt_fields_t ~ {data,value,field_size}
// Note that most of these are absurdly large; they are meant to exercise the
// field-length determination code, not nec. be representative of real-world
// event delta-times.  
std::vector<dt_fields_t> delta_time {
	// From p131 of the MIDI std
	{{0x00},0x00000000,1},
	{{0x40},0x00000040,1},
	{{0x7F},0x0000007F,1},
	{{0xC0,0x00},0x00002000,2},
	{{0x81,0x00},0x00000080,2},
	{{0xFF,0x7F},0x00003FFF,2},
	{{0x81,0x80,0x00},0x00004000,3},
	{{0xC0,0x80,0x00},0x00100000,3},
	{{0xFF,0xFF,0x7F},0x001FFFFF,3},
	{{0x81,0x80,0x80,0x00},0x00200000,4},
	{{0xC0,0x80,0x80,0x00},0x08000000,4},
	{{0xFF,0xFF,0xFF,0x7F},0x0FFFFFFF,4},
	
	// Other assorted examples
	{{0x64u},100,1},
	{{0x81u,0x48u},200,2}
};

//
// Example events
// Each entry is devoid of a leading delta-time and must be appended to a
// delta-time to make a valid event.  
//

// meta events
std::vector<meta_events_t> meta_events {
	// text events ---------------------------------------------------
	// "creator: "
	{{0xFFu,0x01u,0x09u,
		0x63u,0x72u,0x65u,0x61u,0x74u,0x6Fu,0x72u,0x3Au,0x20u},12,9},
	// "Harpsichord High"
	{{0xFF,0x01,0x10,
		0x48,0x61,0x72,0x70,0x73,0x69,0x63,0x68,0x6F,0x72,0x64,0x20,
		0x48,0x69,0x67,0x68},19,16},
	// 128 chars, which forces the length field to occupy 2 bytes:  
	// 128 == 0x80 => 0x81,0x00
	{{0xFF,0x01,0x81,0x00,
		0x6D,0x6F,0x75,0x6E,0x74,0x20,0x6F,0x66,0x20,0x74,0x65,0x78,
		0x74,0x20,0x64,0x65,0x73,0x63,0x72,0x69,0x62,0x69,0x6E,0x67,
		0x74,0x20,0x69,0x73,0x20,0x61,0x20,0x67,0x6F,0x6F,0x64,0x20,
		0x20,0x61,0x6E,0x79,0x74,0x68,0x69,0x6E,0x67,0x2E,0x20,0x49,
		0x61,0x20,0x74,0x65,0x78,0x74,0x20,0x65,0x76,0x65,0x6E,0x74,
		0x69,0x64,0x65,0x61,0x20,0x74,0x6F,0x20,0x70,0x75,0x74,0x20,
		0x20,0x6F,0x66,0x20,0x61,0x20,0x74,0x72,0x61,0x63,0x6B,0x2C,
		0x20,0x72,0x69,0x67,0x68,0x74,0x20,0x61,0x74,0x20,0x74,0x68,
		0x65,0x0D,0x0A,0x62,0x65,0x67,0x69,0x6E,0x6E,0x69,0x6E,0x67,
		0x20,0x77,0x69,0x74,0x68,0x20,0x74,0x68,0x65,0x20,0x6E,0x61,
		0x6D,0x65,0x20,0x6F,0x66,0x20,0x74,0x77},132,128},

		// time signature -----------------------------------------------------
		{{0xFFu,0x58u,0x04u,0x04u,0x02u,0x12u,0x08u},7,4},

		// end of track -------------------------------------------------------
		{{0xFFu,0x2Fu,0x00u},3,0},
};

meta_events_t random_meta_event(uint8_t type_byte, int len) {
	std::random_device rdev;
	std::mt19937 re(rdev());
	std::uniform_int_distribution<int> rd127(0,127);
	std::uniform_int_distribution<int> rd200(0,200);
	std::uniform_int_distribution<int> rduint8t(0,std::numeric_limits<uint8_t>::max());

	// All meta-events begin with FF, then have an event type 
	// byte which is always less than 128 (midi std p. 136)
	meta_events_t result {};
	result.data.push_back(0xFFu);
	
	if (type_byte >= 128) {
		type_byte = static_cast<uint8_t>(rd127(re));
	}
	result.data.push_back(type_byte);

	if (type_byte == 0x59 || type_byte == 0x00) {
		len = 2;
	} else if (type_byte == 0x58) {
		len = 4;
	} else if (type_byte == 0x54) {
		len = 5;
	} else if (type_byte == 0x51) {
		len = 3;
	} else if (type_byte == 0x2F) {
		len=0;
	} else if (type_byte == 0x20) {
		len=1;
	}

	if (len < 0) {
		len = rd200(re);
	}
	std::array<unsigned char,4> temp_len_field {};
	auto len_field_end = midi_write_vl_field(temp_len_field.begin(),
		temp_len_field.end(),static_cast<uint8_t>(len));
	std::copy(temp_len_field.begin(),len_field_end,std::back_inserter(result.data));
	result.payload_size = len;

	for (int i=0; i<len; ++i) {
		result.data.push_back(rduint8t(re));
	}
	result.data_length = result.data.size();

	return result;
}

// Sysex_f0/f7 events
std::vector<sysex_events_t> sysex_events {
	// sysex_f0 from p. 136 of the midi std:
	{{0xF0u,0x03u,0x43u,0x12u,0x00u},3,5},
	// sysex_f7 from p. 136 of the midi std:
	{{0xF7u,0x06u,0x43u,0x12u,0x00u,0x43u,0x12u,0x00u},8,6},
	{{0xF7u,0x04u,0x43u,0x12u,0x00u,0xF7u},6,4}
};

// midi events & parts for midi events

std::vector<unsigned char> invalid_midisbs {
	0xFFu,0x7Fu,0xF0u,
	0x00u,0x01u,0x02u,0x03u,0x09u,0x0Au,0x0Fu,0x10u,0x70u,0x77u
};
std::vector<unsigned char> valid_midisbs_1db {
	0xC0,0xC1,0xCB,0xCF,
	0xD0,0xD1,0xDB,0xDF
};
std::vector<unsigned char> valid_midisbs_2db {
	0x80,0x81,0x8A,0x8F,
	0x90,0x91,0x9B,0x9F,
	0xB0,0xB1,0xBB,0xBF,
	0xE0,0xE1,0xEB,0xEF
};
std::vector<unsigned char> valid_mididbs {
	0x00u,0x01u,0x02u,0x03u,0x09u,0x0Au,0x0Fu,0x10u,0x70u,0x77u
};
std::vector<unsigned char> invalid_mididbs {
	0xFFu,0x7Fu,0xF0u,
	0xC0,0xC1,0xCB,0xCF,0xD0,0xD1,0xDB,0xDF,0x80,0x81,0x8A,0x8F
};
std::vector<unsigned char> union_valid_invalid_midisbs {
	0xFFu,0x7Fu,0xF0u,
	0xC0,0xC1,0xCB,0xCF,0xD0,0xD1,0xDB,0xDF,0x80,0x81,0x8A,0x8F,
	0xC0,0xC1,0xCB,0xCF,
	0xD0,0xD1,0xDB,0xDF,
	0x80,0x81,0x8A,0x8F,
	0x90,0x91,0x9B,0x9F,
	0xB0,0xB1,0xBB,0xBF,
	0xE0,0xE1,0xEB,0xEF
};

// This func prepends random delta-times so "_payloads" is misnaming
std::vector<midi_tests_t> make_random_midi_tests() {
	std::random_device rdev;
	std::mt19937 re(rdev());
	std::uniform_int_distribution rd(0,1000);

	std::vector<midi_tests_t> result {};
	for (int i=0; i<100; ++i) {
		midi_tests_t curr {};
		// status byte applic to the present event
		if (rd(re)%2==0) {  // 1 data byte
			curr.applic_midi_status = valid_midisbs_1db[rd(re)%valid_midisbs_1db.size()];
			curr.data.push_back(valid_mididbs[rd(re)%valid_mididbs.size()]);
			curr.n_data_bytes = 1;
		} else {  // 2 data bytes
			curr.applic_midi_status = valid_midisbs_2db[rd(re)%valid_midisbs_2db.size()];
			curr.data.push_back(valid_mididbs[rd(re)%valid_mididbs.size()]);
			curr.data.push_back(valid_mididbs[rd(re)%valid_mididbs.size()]);
			curr.n_data_bytes = 2;
		}
		// Should the status byte applic to the present be local or 
		// inherited from the prev event?
		if (rd(re)%2==0) {  // event-local status byte
			curr.data.insert(curr.data.begin(),curr.applic_midi_status);
			// totally random running status
			curr.midisb_prev_event = union_valid_invalid_midisbs[rd(re)%union_valid_invalid_midisbs.size()];
		} else {  // non-event-local status byte
			curr.midisb_prev_event = curr.applic_midi_status;
		}
		curr.data_length = curr.data.size();
		result.push_back(curr);
	}

	// prepend random delta times
	for (auto& e : result) {
		dt_fields_t dt;
		do {
			dt = delta_time[rd(re)%delta_time.size()];
		} while (dt.value==0 && ((e.data[0]&0x80)==0x00u));
		// If the present midi payload e is in running-status, the delta-time
		// can not be 0.  

		e.data.insert(e.data.begin(),dt.data.begin(),dt.data.end());
		e.dt_field_size = dt.field_size;
		e.dt_value = dt.value;
	}

	return result;
}





std::vector<midi_tests_t> make_random_midi_tests2() {
	std::random_device rdev;
	std::mt19937 re(rdev());
	std::uniform_int_distribution rd(0);
	std::uniform_int_distribution rd_n_data_bytes(1,2);
	

	auto random_dt_val = [&re](bool running_status) -> uint32_t {
		// If in running status, can't generate a dt of 0
		std::uniform_int_distribution rd_delta_time(0,0x7FFFFFFF);  // NB: 1
		
		// I take the log to attempt to get a more uniform distribution of
		// field lengths
		int32_t dt_notscaled {0};
		int32_t dt {0};
		do {
			dt_notscaled = rd_delta_time(re); 
			dt = dt_notscaled; //static_cast<uint32_t>(std::log(dt_notscaled)/std::log(128));
		} while (running_status && dt == 0);

		return dt;
	};

	std::vector<midi_tests_t> result {};
	for (int i=0; i<100; ++i) {
		midi_tests_t curr {};

		bool running_status = (rd(re)%2==0);

		// delta-time
		curr.dt_value = random_dt_val(running_status);
		curr.dt_field_size = static_cast<uint8_t>(midi_vl_field_size(curr.dt_value));
		std::array<unsigned char,4> temp_dt_field {};
		auto dt_field_end = midi_write_vl_field(temp_dt_field.begin(),
			temp_dt_field.end(),curr.dt_value);
		std::copy(temp_dt_field.begin(),dt_field_end,std::back_inserter(curr.data));

		curr.n_data_bytes = rd_n_data_bytes(re);
		curr.data_length = curr.n_data_bytes;  // for event-local status, +=1 below
		
		// Set status byte values consistent w/ .n_data_bytes and write into
		// .data consistent w/ running_status
		curr.applic_midi_status = random_midi_status_byte(curr.n_data_bytes);
		if (!running_status) {  // event-local status byte; random running status byte
			curr.data.push_back(curr.applic_midi_status);
			curr.data_length += 1;
			curr.midisb_prev_event = random_midi_status_byte();
			curr.dt_value = random_dt_val(false);
		} else {  // non-event-local status byte (running status active)
			curr.midisb_prev_event = curr.applic_midi_status;
			curr.dt_value = random_dt_val(true);
		}

		// Generate and write data bytes, consistent w/ the applic status byte
		for (int j=0; j<curr.n_data_bytes; ++j) {
			if ((curr.applic_midi_status&0xF0)==0xB0 && j==0 && rd(re)%2==0) {
				// If the present msg is elligible to be a channel_mode msg
				// (applicible status byte ~ 0xBn) _and_ we're generating the
				// first data byte (i==0), randomly pick between the msg being a
				// 'Select Channel Mode' and a 'Control change' msg.  
				curr.data.push_back(random_midi_data_byte(true));
			} else {
				curr.data.push_back(random_midi_data_byte(false));
			}
		}

		result.push_back(curr);
	}

	return result;
}








void print_midi_test_cases() {
	auto yay = testdata::make_random_midi_tests2();
	for (const auto& tc : yay) {
		std::cout << "{{";
		for (int i=0; i<tc.data.size(); ++i) {
			std::cout << "0x" << dbk::print_hexascii(&(tc.data[i]),1);
			if (i!=(tc.data.size()-1)) {
				std::cout << ",";
			}
		}
		std::cout << "},";
		std::cout << "0x" << dbk::print_hexascii(&(tc.midisb_prev_event),1) << ",";
		std::cout << "0x" << dbk::print_hexascii(&(tc.applic_midi_status),1) << ",";
		std::cout << std::to_string(tc.n_data_bytes) << ",";
		std::cout << std::to_string(tc.data_length) << ",";
		std::cout << std::to_string(tc.dt_value) << ",";
		std::cout << std::to_string(tc.dt_field_size) << "},\n";
	}
	std::cout << std::endl;
}


uint8_t random_midi_status_byte(int require_n_data_bytes) {
	std::random_device rdev;
	std::mt19937 re(rdev());
	std::uniform_int_distribution<int> rd(128,std::numeric_limits<uint8_t>::max());
	
	int s = rd(re);
	if (require_n_data_bytes == 1) {
		while ((s&0xF0u)!=0xC0u && (s&0xF0u)!=0xD0u) {
			s = rd(re);
		}
	} else if (require_n_data_bytes == 2) {
		while ((s&0xF0u)==0xC0u || (s&0xF0u)==0xD0u) {
			s = rd(re);
		}
	}

	return static_cast<uint8_t>(s);
}

uint8_t random_midi_data_byte(bool require_byte_one_ch_mode) {
	std::random_device rdev;
	std::mt19937 re(rdev());
	std::uniform_int_distribution<int> rd(0,127);
	
	int s = rd(re);
	if (require_byte_one_ch_mode) {
		s |= 0b01111000u;
	}

	return static_cast<uint8_t>(s);
}



};  // namespace testdata





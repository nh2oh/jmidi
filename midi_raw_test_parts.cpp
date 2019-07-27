#include "midi_raw_test_parts.h"
#include "midi_raw.h"
#include "midi_vlq.h"
#include "midi_delta_time.h"
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


vlq_testcase_t make_vlq_testcase() {
	std::random_device rdev;
	std::mt19937 re(rdev());
	std::uniform_int_distribution rd(0x00u,0xFFu);

	vlq_testcase_t tc;

	auto it = tc.input_encoded.begin();
	while (it!=tc.input_encoded.end()) {
		*it = rd(re);
		*it |= 0x80u;
		if ((rd(re)/2 < 255/2) || (it==(tc.input_encoded.end()-1))) {
			*it &= 0x7Fu;
			break;  // Another digit w/ ~50% probability
		}
		++it;
	}
	tc.input_field_size = (it - tc.input_encoded.begin());
	auto vlq = read_vlq_max64(tc.input_encoded.begin());
	if (tc.input_field_size != vlq.N) {
		std::abort();
	}
	tc.input_value =  vlq.val;
	tc.ans_value = ((vlq.val)&0x0FFFFFFFu);
	write_delta_time(tc.ans_value,tc.normalized_encoded.begin());
	if (read_vlq_max64(tc.normalized_encoded.begin()).val != tc.ans_value) {
		std::abort();
	}

	return tc;
};




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

std::vector<midi_tests_t> make_random_midi_tests() {
	std::random_device rdev;
	std::mt19937 re(rdev());
	std::uniform_int_distribution rd(0);
	std::uniform_int_distribution rd_maybe_invalid_rs(0,255);
	std::uniform_int_distribution rd_n_data_bytes(1,2);

	std::vector<midi_tests_t> result {};
	for (int i=0; i<100; ++i) {
		midi_tests_t curr {};

		curr.in_running_status = (rd(re)%2==0);

		// delta-time
		auto curr_dt_field = random_dt_field();
		auto curr_dt = midi_interpret_vl_field(&(curr_dt_field[0]));
		curr.dt_value = curr_dt.val;
		curr.dt_field_size = curr_dt.N;
		std::copy(curr_dt_field.begin(),curr_dt_field.begin()+curr.dt_field_size,
			std::back_inserter(curr.data));

		curr.n_data_bytes = rd_n_data_bytes(re);
		curr.data_length = curr.n_data_bytes;  // for event-local status, +=1 below
		
		// Set status byte values consistent w/ .n_data_bytes and write into
		// .data consistent w/ running_status
		curr.applic_midi_status = random_midi_status_byte(curr.n_data_bytes);
		if (!curr.in_running_status) {  // event-local status byte; random running status byte
			curr.data.push_back(curr.applic_midi_status);
			curr.data_length += 1;
			// This will generate an invalid status byte ~1/2 the time
			curr.midisb_prev_event = rd_maybe_invalid_rs(re);  //random_midi_status_byte();
		} else {  // there is no event-local status byte (running status is active)
			curr.midisb_prev_event = curr.applic_midi_status;
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
	auto yay = testdata::make_random_midi_tests();
	std::cout << "// data, rs, applic-sb, is_rs, ndata, data_length, dt_val, dt.N\n";
	for (const auto& tc : yay) {
		std::cout << "{{";
		for (int i=0; i<tc.data.size(); ++i) {
			std::string temp_s;
			std::cout << "0x";
			print_hexascii(&(tc.data[i]),&(tc.data[i])+1,std::back_inserter(temp_s));
			std::cout << temp_s;
			if (i!=(tc.data.size()-1)) {
				std::cout << ",";
			}
		}
		std::cout << "},";
		std::cout << "0x";
		std::string temp_s;
		print_hexascii(&(tc.midisb_prev_event),&(tc.midisb_prev_event)+1,
			std::back_inserter(temp_s));
		temp_s += ",0x";
		print_hexascii(&(tc.applic_midi_status),&(tc.applic_midi_status)+1,
			std::back_inserter(temp_s));
		temp_s += ",";
		std::cout << temp_s;
		std::cout << std::to_string(tc.in_running_status) << ",";
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
	std::uniform_int_distribution<int> rd(128,239);
	
	int s = rd(re);
	while ((s&0xF0u)==0xF0u) {
		s = rd(re);
	}
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


std::array<unsigned char,4> random_dt_field(int n_digits) {
	std::random_device rdev;
	std::mt19937 re(rdev());
	std::uniform_int_distribution rd(0x00u,0xFFu);

	if (n_digits > 4) {
		n_digits = 4;
	} else if (n_digits < 0) {
		n_digits = 1;
	}

	std::array<unsigned char,4> result {0x00u,0x00u,0x00u,0x00u};
	if (n_digits==0) {  // Random number of digits
		int i=0;
		do {
			unsigned char dig = rd(re);
			result[i]=dig;
			++i;
		} while (i<4 && result[i-1]&0x80u);
		if (i==4) {
			result[3] &= ~0x80u;
		}
		// If the value is only one digit, just set it to 0 with p=0.5; 0 is
		// the most common dt value in an smf.  
		if (i==1 && result[0]%2==0) {
			result[0] = 0x00u;
		}
	} else {  // Exactly n_digits
		int i=0;
		for (i=0; i<n_digits; ++i) {
			unsigned char dig = rd(re);
			result[i]=dig;
			result[i] |= 0x80u;
		}
		result[i-1] &= ~0x80u;
	}

	return result;
};


std::vector<meta_test_t> make_random_meta_tests(int n) {
	struct type_len_pair_t {
		uint8_t type;
		int32_t len;  // a len < 0 => variable length
	};
	std::vector<type_len_pair_t> meta_db {
		{0x00,0x02},  // seq #
		{0x01,-1},  // txt event
		{0x02,-1},  // copyright
		{0x03,-1},  // seq/track name
		{0x04,-1},  // Inst. name
		{0x05,-1},  // lyric
		{0x06,-1},  // Marker
		{0x07,-1},  // cue point
		{0x20,0x01},  // midi ch prefix
		{0x2F,0x00},  // end of track
		{0x51,0x03},  // set tempo
		{0x54,0x05},  // smpte offset
		{0x58,0x04},  // time sig
		{0x59,0x02},  // key sig
		{0x7F,-1}  // sequencer-specific meta event
	};
	// A bunch of "random" type bytes that do are not part of the 
	// "known" set
	std::vector<uint8_t> unknown_meta_type {
		0x08u,0x09u,0x0Au,0x0Bu,0x0Eu,0x0Fu,
		0x10u,0x11u,
		0xA1u,0xAFu,
		0xF7u,0xFFu};
	std::string rand_txt = "Lorem ipsum sodales nunc vulputate accumsan "
		"                             maecenas vitae, elit eleifend "
		"convallis neque fames diam consequat,porta curae gravida leo "
		"faucibus cursus quis rhoncus varius vehicula taciti in accumsan."
		"  metus faucibus per.";
	
	std::random_device rdev;
	std::mt19937 re(rdev());
	std::uniform_int_distribution<uint32_t> rd(0x00u,0xFFFFFFFFu);
	
	auto make_known_evtype = [&re]()->bool {
		std::uniform_int_distribution rd_make_known_event(0,10);
		return rd_make_known_event(re)<=8;
	};
	auto make_random_size = [&re,&rand_txt]()->int {
		std::uniform_int_distribution rd_payload_size(0,static_cast<int>(rand_txt.size()));
		return rd_payload_size(re);
	};
	auto make_rand_dtval = [&re]()->uint32_t {
		std::uniform_int_distribution rd_dt_size(0,4);
		auto fld = random_dt_field(rd_dt_size(re));
		return midi_interpret_vl_field(&(fld[0])).val;

		/*
		std::uniform_int_distribution<uint32_t> rd_rand_dt(0x00u,0xFFFFFFFFu);
		uint32_t val = rd_rand_dt(re)&0x7FFFFFFF;
		if (val%2==0) {
			return 0;
		} else {
			return val;
		}*/
	};
	

	std::vector<meta_test_t> result {};
	for (int i=0; i<n; ++i) {
		meta_test_t curr;

		curr.dt_value = make_rand_dtval();
		midi_write_vl_field(std::back_inserter(curr.data),curr.dt_value);

		curr.data.push_back(0xFFu);

		//
		// Random type and (possibly) payload length
		//
		if (make_known_evtype()) {
			type_len_pair_t curr_tlp = meta_db[rd(re)%meta_db.size()];
			if (curr_tlp.len < 0) {
				curr_tlp.len = make_random_size();
			}
			curr.type_byte = curr_tlp.type;
			curr.payload_length = curr_tlp.len;
		} else {
			curr.type_byte = unknown_meta_type[rd(re)%unknown_meta_type.size()];
			curr.payload_length = make_random_size();
		}
		curr.data.push_back(curr.type_byte);
		midi_write_vl_field(std::back_inserter(curr.data),curr.payload_length);
		
		//
		// Random payload
		//
		if (curr.payload_length <= 10) {
			for (int j=0; j<curr.payload_length; ++j) {
				curr.data.push_back(rd(re));
			}
		} else {
			std::copy(rand_txt.begin(),rand_txt.begin()+curr.payload_length,
				std::back_inserter(curr.data));
		}

		curr.data_size = curr.data.size()-midi_vl_field_size(curr.dt_value);

		result.push_back(curr);
	}
	
	return result;
}


void print_meta_tests(const std::vector<meta_test_t>& tests) {
	std::cout << "// data, dt_val, type_byte, payload_len, data_size\n";
	for (const auto& tc : tests) {
		std::cout << "{{";
		for (int i=0; i<tc.data.size(); ++i) {
			std::string temp_s;
			std::cout << "0x";
			print_hexascii(&(tc.data[i]),&(tc.data[i])+1,std::back_inserter(temp_s));
			std::cout << temp_s;
			if (i!=(tc.data.size()-1)) {
				std::cout << ",";
			}
			if (i>0 && i%14==0) {
				std::cout << "\n\t";
			}
		}
		std::cout << "},\n";
		std::cout << std::to_string(tc.dt_value) << ",";
		std::cout << "0x";
		std::string temp_s;
		print_hexascii(&(tc.type_byte),&(tc.type_byte)+1,std::back_inserter(temp_s));
		std::cout << temp_s;
		std::cout << "u,";
		std::cout << std::to_string(tc.payload_length) << ",";
		std::cout << std::to_string(tc.data_size) << "},";
		std::cout << std::endl;
	}
	std::cout << std::endl;
}








};  // namespace testdata





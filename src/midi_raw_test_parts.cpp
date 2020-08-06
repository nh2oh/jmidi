#include "midi_raw_test_parts.h"
#include "midi_vlq.h"
#include "print_hexascii.h"
#include "midi_delta_time.h"
#include "smf_t.h"
#include "mtrk_t.h"
#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include "mthd_t.h"
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

namespace jmid {
namespace rand {

jmid::rand::vlq_testcase jmid::rand::make_random_vlq(std::mt19937& re, 
						jmid::rand::make_random_vlq_opts o) {
	std::uniform_int_distribution<int> rd(0x00u,0x7Fu);

	jmid::rand::vlq_testcase tc;
	tc.encoded_value = 0;
	tc.field_size = 0;  tc.canonical_field_size = 0;
	bool generated_nonzero_b = false;
	auto it=tc.field.begin();
	auto it_canonical = tc.canonical_field.begin();
	while (it<tc.field.end()) {
		tc.encoded_value <<= 7;
		
		unsigned char b = static_cast<unsigned char>(rd(re));
		if (!generated_nonzero_b && rd(re)%o.a==0) {
			b = 0;  // Causes a non-canonical field to be generated
		}
		generated_nonzero_b = (generated_nonzero_b || (b>0));
		tc.encoded_value += b;
		++(tc.field_size);
		*it++ = (b|0x80u);
		if (generated_nonzero_b) {
			++(tc.canonical_field_size);
			*it_canonical++ = (b|0x80u);
		}
		if (rd(re)%o.b == 0) {
			break;  // Terminate early w/ ~50% probability
		}
	}
	*(it-1) &= 0x7Fu;

	if (generated_nonzero_b) {
		// it_canonical is only incremented if generated_nonzero_b; for a 
		// pass where the generated_nonzero_b is never flipped, 
		// it_canonical==tc.canonical_field.begin() and it_canonical-1 is
		// invalid.  
		*(it_canonical-1) &= 0x7Fu;
	} else {
		// where the generated_nonzero_b is never flipped, 
		// canonical_field_size==0, but a field of all 0's is still considerd
		// to have a size == 1.  
		tc.canonical_field_size = 1;
	}
	tc.is_canonical = (tc.canonical_field_size == tc.field_size);
	return tc;
}


jmid::rand::random_meta_event jmid::rand::make_random_meta_valid(std::mt19937& re, 
							std::vector<unsigned char>& dest, int max_plen) {
	dest.clear();
	jmid::rand::random_meta_event result;
	result.is_valid = true;
	std::uniform_int_distribution<int> rd_type(0,127);
	std::uniform_int_distribution<int> rd_payl(0,255);

	jmid::rand::vlq_testcase rvlq;
	do {
		rvlq = jmid::rand::make_random_vlq(re,{3,3});
	} while (!jmid::is_valid_delta_time(rvlq.encoded_value) 
		|| rvlq.field_size > 4);
	// I check the field size as well as the value b/c i want to be able to
	// use the non-canonical field.  
	result.dt_val = rvlq.encoded_value;
	result.dt_field_sz = rvlq.field_size;
	result.dt_field_is_canonical = rvlq.is_canonical;
	for (int i=0; i<rvlq.field_size; ++i) {
		dest.push_back(rvlq.field[i]);
	}

	dest.push_back(0xFFu);
	result.type_byte = static_cast<unsigned char>(rd_type(re));
	dest.push_back(result.type_byte);
	
	do {
		rvlq = jmid::rand::make_random_vlq(re,{3,3});
	} while (!jmid::is_valid_vlq(rvlq.encoded_value) 
		|| rvlq.field_size > 4
		|| rvlq.encoded_value > max_plen);
	result.length_val = rvlq.encoded_value;
	result.length_field_sz = rvlq.field_size;
	result.length_field_is_canonical = rvlq.is_canonical;
	for (int i=0; i<rvlq.field_size; ++i) {
		dest.push_back(rvlq.field[i]);
	}

	for (int i=0; i<rvlq.encoded_value; ++i) {
		dest.push_back(static_cast<unsigned char>(rd_payl(re)));
	}

	return result;
}
jmid::rand::random_meta_event jmid::rand::make_random_meta(std::mt19937& re, 
							std::vector<unsigned char>& dest, int max_plen) {
	dest.clear();
	jmid::rand::random_meta_event result;
	result.is_valid = true;
	std::uniform_int_distribution<int> rd_type(0,255);
	std::uniform_int_distribution<int> rd_payl(0,255);

	jmid::rand::vlq_testcase rvlq;
	rvlq = jmid::rand::make_random_vlq(re,{3,3});
	result.dt_val = rvlq.encoded_value;
	result.dt_field_sz = rvlq.field_size;
	result.dt_field_is_canonical = rvlq.is_canonical;
	for (int i=0; i<rvlq.field_size; ++i) {
		dest.push_back(rvlq.field[i]);
	}
	if (!jmid::is_valid_delta_time(result.dt_val)
			|| result.dt_field_sz > 4) {
		result.is_valid = false;
	}

	dest.push_back(0xFFu);
	result.type_byte = static_cast<unsigned char>(rd_type(re));
	dest.push_back(result.type_byte);
	if (!jmid::is_meta_type_byte(result.type_byte)) {
		result.is_valid = false;
	}

	do {
		rvlq = jmid::rand::make_random_vlq(re,{3,3});
	} while (rvlq.encoded_value > max_plen);
	result.length_val = rvlq.encoded_value;
	result.length_field_sz = rvlq.field_size;
	result.length_field_is_canonical = rvlq.is_canonical;
	for (int i=0; i<rvlq.field_size; ++i) {
		dest.push_back(rvlq.field[i]);
	}
	if (!jmid::is_valid_vlq(result.length_val)
			|| result.length_field_sz > 4) {
		result.is_valid = false;
	}

	for (int i=0; i<rvlq.encoded_value; ++i) {
		dest.push_back(static_cast<unsigned char>(rd_payl(re)));
	}

	return result;
}

jmid::rand::random_ch_event jmid::rand::make_random_ch_valid(std::mt19937& re, 
							std::vector<unsigned char>& dest) {
	dest.clear();
	jmid::rand::random_ch_event result;
	result.is_valid = true;
	std::uniform_int_distribution<int> rd_data(0,127);
	std::uniform_int_distribution<int> rd_s(0x80,0xEF);

	jmid::rand::vlq_testcase rvlq;
	do {
		rvlq = jmid::rand::make_random_vlq(re,{3,3});
	} while (!jmid::is_valid_delta_time(rvlq.encoded_value) 
		|| rvlq.field_size > 4);
	// I check the field size as well as the value b/c i want to be able to
	// use the non-canonical field.  
	result.dt_val = rvlq.encoded_value;
	result.dt_field_sz = rvlq.field_size;
	result.dt_field_is_canonical = rvlq.is_canonical;
	for (int i=0; i<rvlq.field_size; ++i) {
		dest.push_back(rvlq.field[i]);
	}

	result.s = static_cast<unsigned char>(rd_s(re));
	result.p1 = static_cast<unsigned char>(rd_data(re));
	dest.push_back(result.s);
	dest.push_back(result.p1);
	if (jmid::channel_status_byte_n_data_bytes(result.s) == 2) {
		result.p2 = static_cast<unsigned char>(rd_data(re));
		dest.push_back(result.p2);
	}

	return result;
}
jmid::rand::random_ch_event jmid::rand::make_random_ch(std::mt19937& re, 
							std::vector<unsigned char>& dest) {
	dest.clear();
	jmid::rand::random_ch_event result;
	result.is_valid = true;
	std::uniform_int_distribution<int> rd_data(0,255);
	std::uniform_int_distribution<int> rd_s(0,0xFF);

	jmid::rand::vlq_testcase rvlq;
	rvlq = jmid::rand::make_random_vlq(re,{3,3});
	result.dt_val = rvlq.encoded_value;
	result.dt_field_sz = rvlq.field_size;
	result.dt_field_is_canonical = rvlq.is_canonical;
	for (int i=0; i<rvlq.field_size; ++i) {
		dest.push_back(rvlq.field[i]);
	}
	if (!jmid::is_valid_delta_time(result.dt_val)
			|| result.dt_field_sz > 4) {
		result.is_valid = false;
	}

	result.s = static_cast<unsigned char>(rd_s(re));
	result.p1 = static_cast<unsigned char>(rd_data(re));
	if (!jmid::is_channel_status_byte(result.s) 
			|| !jmid::is_data_byte(result.p1)) {
		result.is_valid = false;
	}
	dest.push_back(result.s);
	dest.push_back(result.p1);
	if (jmid::channel_status_byte_n_data_bytes(result.s) == 2) {
		result.p2 = static_cast<unsigned char>(rd_data(re));
		if (!jmid::is_data_byte(result.p2)) {
			result.is_valid = false;
		}
		dest.push_back(result.p2);
	}

	return result;
}


void make_random_sequence(int n_events, std::mt19937& re, 
							std::vector<unsigned char>& dest) {
	dest.clear();
	std::uniform_int_distribution<int> rd_meta_or_ch(0,100);
	std::uniform_int_distribution<int> rd_valid_or_invalid(0,1000);
	std::vector<unsigned char> curr_event;
	int n_events_generated = 0;
	bool curr_event_is_valid = true;
	while (n_events_generated < n_events) {//(seq_is_valid && n_events < 40) {
		if (rd_meta_or_ch(re) > 95) {
			auto curr_meta = jmid::rand::make_random_meta(re,curr_event,127);
			curr_event_is_valid = curr_meta.is_valid;
		} else {
			auto curr_ch = jmid::rand::make_random_ch(re,curr_event);
			curr_event_is_valid = curr_ch.is_valid;
		}

		if (!curr_event_is_valid) {
			if (rd_valid_or_invalid(re) < 999) {
				continue;
			} else {
				int idx_invalid = n_events_generated;
			}
		}

		++n_events_generated;
		std::copy(curr_event.begin(),curr_event.end(),std::back_inserter(dest));
	}
	return;
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
		auto curr_dt = jmid::read_delta_time(curr_dt_field.begin(),curr_dt_field.end());
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
	auto yay = jmid::rand::make_random_midi_tests();
	std::cout << "// data, rs, applic-sb, is_rs, ndata, data_length, dt_val, dt.N\n";
	for (const auto& tc : yay) {
		std::cout << "{{";
		for (int i=0; i<tc.data.size(); ++i) {
			std::string temp_s;
			std::cout << "0x";
			jmid::print_hexascii(&(tc.data[i]),&(tc.data[i])+1,std::back_inserter(temp_s));
			std::cout << temp_s;
			if (i!=(tc.data.size()-1)) {
				std::cout << ",";
			}
		}
		std::cout << "},";
		std::cout << "0x";
		std::string temp_s;
		jmid::print_hexascii(&(tc.midisb_prev_event),&(tc.midisb_prev_event)+1,
			std::back_inserter(temp_s));
		temp_s += ",0x";
		jmid::print_hexascii(&(tc.applic_midi_status),&(tc.applic_midi_status)+1,
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
		return jmid::read_vlq(fld.begin(),fld.end()).val;

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
		jmid::write_vlq(curr.dt_value,std::back_inserter(curr.data));

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
		jmid::write_vlq(curr.payload_length,std::back_inserter(curr.data));
		
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

		curr.data_size = curr.data.size()-jmid::vlq_field_size(curr.dt_value);

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
			jmid::print_hexascii(&(tc.data[i]),&(tc.data[i])+1,std::back_inserter(temp_s));
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
		jmid::print_hexascii(&(tc.type_byte),&(tc.type_byte)+1,std::back_inserter(temp_s));
		std::cout << temp_s;
		std::cout << "u,";
		std::cout << std::to_string(tc.payload_length) << ",";
		std::cout << std::to_string(tc.data_size) << "},";
		std::cout << std::endl;
	}
	std::cout << std::endl;
}


}  // namespace rand
}  // namespace jmid




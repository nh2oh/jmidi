#pragma once
#include <vector>
#include <cstdint>
#include <array>

//
// Q&D functions and raw data for making randomized tests of the midi lib.
//
// Absolutely zero effort has been made to make these routines effecient.
//
namespace testdata {
struct vlq_max64_interpreted {
	uint64_t val {0};
	int8_t N {0};
	bool is_valid {false};
};
template<typename InIt>
vlq_max64_interpreted read_vlq_max64(InIt it) {
	vlq_max64_interpreted result {};
	result.val = 0;
	while (true) {
		result.val += (*it & 0x7Fu);
		++(result.N);
		if (!(*it & 0x80u) || result.N==8) { // the high-bit is not set
			break;
		} else {
			result.val <<= 7;  // result.val << 7;
			++it;
		}
	}
	result.is_valid = !(*it & 0x80u);
	return result;
};

struct vlq_testcase_t {
	uint64_t input_value {0};  // [0,uint_max]
	uint32_t ans_value {0};  // [0,0x0FFFFFFFu]
	int8_t input_field_size {0};
	int8_t norm_field_size {0};
	std::array<unsigned char,8> input_encoded {};
	std::array<unsigned char,8> normalized_encoded {};
};

vlq_testcase_t make_vlq_testcase();


struct dt_fields_t {
	std::vector<unsigned char> data {};
	uint32_t value {0};
	uint8_t field_size {0};
};

struct meta_events_t {
	std::vector<unsigned char> data {};
	uint32_t data_length {};  // == data.size() == 2 + size-of-vl-length-field + value-of-vl-length-field
	uint32_t payload_size {};  // value of the "length" field
};
// arg 1: force type-byte (>= 128 => random)
// arg 2: force payload length; -1=>random
meta_events_t random_meta_event(uint8_t=128u,int=-1);

struct sysex_events_t {
	std::vector<unsigned char> data {};
	uint32_t data_length {};  // == data.size() == 1 + size-of-vl-length-field + value-of-vl-length-field
	uint32_t payload_size {};  // value of the "length" field
};

struct midi_events_t {
	std::vector<unsigned char> data {};
	unsigned char midisb_prev_event {};  // "midi status byte previous event"
	unsigned char applic_midi_status {};  // "applicable midi status byte"
	
	// Expectation based on value of applic_midi_status
	uint8_t n_data_bytes {};  
	
	// data_length == n_data_bytes + 1 if there is an event-local midi 
	// status byte;  == n_data_bytes + if in running status (no event-local
	// midi status byte).  
	uint32_t data_length {};
};

struct midi_tests_t {
	std::vector<unsigned char> data {};  // data.size()==dt_field_size+data_length
	unsigned char midisb_prev_event {};  // "midi status byte previous event"
	unsigned char applic_midi_status {};  // "applicable midi status byte"
	bool in_running_status {};
	uint8_t n_data_bytes {};  // Based on value of applic_midi_status
	uint32_t data_length {};  // n_data_bytes (+ 1 if not in running status)
	uint32_t dt_value {};
	uint8_t dt_field_size {};
};

std::vector<midi_tests_t> make_random_midi_tests();
void print_midi_test_cases();

uint8_t random_midi_status_byte(int=0);
uint8_t random_midi_data_byte(bool=false);  // true => (result&0b01111000u)==true
std::array<unsigned char,4> random_dt_field(int=0);


struct meta_test_t {
	std::vector<unsigned char> data {};  // data.size()==dt_field_size+data_length
	uint32_t dt_value {};
	uint8_t type_byte {};
	uint32_t payload_length {};
	uint32_t data_size {};  // 2 + length-vl-field.N + length-vl-field.val
};
// this function hardcodes a database of known meta msg type-bytes and 
// payload-length pairs
std::vector<meta_test_t> make_random_meta_tests(int);
void print_meta_tests(const std::vector<meta_test_t>&);




};  // namespace testdata




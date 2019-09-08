#pragma once
#include <vector>
#include <cstdint>
#include <array>
#include <limits>

//
// Q&D functions and raw data for making randomized tests of the midi lib.
//
// Absolutely zero effort has been made to make these routines effecient.
//
namespace jmid {
namespace rand {
struct vlq_max64_interpreted {
	std::uint64_t val {0};
	std::int8_t N {0};
	bool is_valid {false};
};
template<typename InIt>
vlq_max64_interpreted read_vlq_max64(InIt it) {
	vlq_max64_interpreted result {};
	result.val = 0;
	while (true) {
		result.val += (*it & 0x7Fu);
		++(result.N);
		if (!(*it & 0x80u) || result.N==8) { 
			break;
		} else {
			result.val <<= 7;
			++it;
		}
	}
	result.is_valid = !(*it & 0x80u);
	return result;
};
template<typename T, typename OIt>
OIt write_vlq_max64(T val, OIt it) {
	uint64_t uval;
	if (val < 0) {
		uval = 0;
	} else if (val > (std::numeric_limits<std::uint64_t>::max()>>8)) {
		uval = (std::numeric_limits<std::uint64_t>::max()>>8);
	} else {
		uval = static_cast<uint64_t>(val);
	}

	auto s = 7*7u;
	uint64_t mask = (0b01111111u<<(7u*7u));
	while (((uval&mask)==0u) && (mask>0u)) {
		mask>>=7u;
		s-=7u;
	}
	while (mask>0b01111111u) {
		*it++ = 0x80u|((uval&mask)>>(s));
		mask>>=7u;
		s-=7u;
	}
	*it++ = uval&mask;

	return it;
};
struct vlq_testcase {
	std::array<unsigned char,8> field {0,0,0,0,0,0,0,0};
	uint64_t encoded_value {0};  // [0,0x0FFFFFFFu]
	int8_t field_size {0};
};
vlq_testcase make_random_vlq();


struct meta_event {
	std::vector<unsigned char> data;
	std::uint8_t type_byte;
	std::int32_t length;  // value of the "length" field
	std::uint32_t expect_size;  // == 2 + size-of-vlq-length-field + length
};
// arg 1: type-byte (< 0 => random, otherwise static_cast<>()'d to uint8_t)
// arg 2: force payload length; -1=>random
meta_event random_meta_event(int, int=-1);

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

int make_example_midifile_081019();


}  // namespace rand
}  // namespace jmid



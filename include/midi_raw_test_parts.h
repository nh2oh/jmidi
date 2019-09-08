#pragma once
#include <vector>
#include <cstdint>
#include <array>
#include <limits>
#include <random>

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

	unsigned int s = 49u;
	uint64_t mask = (std::uint64_t(0b01111111u)<<(49));
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
	std::array<unsigned char,8> canonical_field {0,0,0,0,0,0,0,0};
	uint64_t encoded_value;
	int8_t field_size;
	int8_t canonical_field_size;
	bool is_canonical;
		// redundant: is_canonical = (field_size==canonical_field_size)
};
struct make_random_vlq_opts {
	int a {5};  // non-canonical
	int b {2};  // early break
};
vlq_testcase make_random_vlq(std::mt19937&,make_random_vlq_opts={});


struct random_meta_event {
	std::uint64_t dt_val;
	std::uint64_t length_val;
	std::int8_t dt_field_sz;
	std::int8_t length_field_sz;
	unsigned char type_byte;
	bool dt_field_is_canonical;
	bool length_field_is_canonical;
	bool is_valid;
};
random_meta_event make_random_meta_valid(std::mt19937&, 
							std::vector<unsigned char>&, int);
// _Possibly_ invalid due to dt, length, or type byte.  The value of the
// length field always == the actual payload length.  Caller can truncate
// or supplement the tail of the output vector if desired.  
random_meta_event make_random_meta(std::mt19937&, 
							std::vector<unsigned char>&, int);


struct random_ch_event {
	std::uint64_t dt_val;
	std::int8_t dt_field_sz;
	unsigned char s;
	unsigned char p1;
	unsigned char p2;
	bool dt_field_is_canonical;
	bool is_valid;
};
random_ch_event make_random_ch_valid(std::mt19937&, 
							std::vector<unsigned char>&);
// _Possibly_ invalid due to dt, length, type byte, or data bytes.  
random_ch_event make_random_ch(std::mt19937&, 
							std::vector<unsigned char>&);


void make_random_sequence(std::mt19937&, std::vector<unsigned char>&);




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


}  // namespace rand
}  // namespace jmid



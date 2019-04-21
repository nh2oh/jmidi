#pragma once
#include <vector>
#include <cstdint>

//
// Q&D functions and raw data for making randomized tests of the midi lib.
//
// Absolutely zero effort has been made to make these routines effecient.
//
namespace testdata {
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
	uint8_t n_data_bytes {};  // Based on value of applic_midi_status
	uint32_t data_length {};  // n_data_bytes (+ 1 if not in running status)
	uint32_t dt_value {};
	uint8_t dt_field_size {};
};

std::vector<midi_tests_t> make_random_midi_tests();
std::vector<midi_tests_t> make_random_midi_tests2();
void print_midi_test_cases();

uint8_t random_midi_status_byte(int=0);
uint8_t random_midi_data_byte(bool=false);  // true => (result&0b01111000u)==true

};  // namespace testdata




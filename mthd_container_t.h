#pragma once
#include "midi_raw.h"
#include <cstdint>
#include <string>


//
// Why not a generic midi_chunk_container_t<T> template?  Because MThd and MTrk containers
// are radically different and it really does not make sense to write generic functions to 
// operate on either type.  
//
// int16_t mthd_container_t::format():  Returns 0, 1, 2
//  -> 0 => File contains a single multi-channel track
//  -> 1 => File contains one or more simultaneous tracks
//  -> 3 =>  File contains one or more sequentially independent single-track patterns.  
// int16_t mthd_container_t::ntrks() is always == 1 for a fmt-type 0 file.  
//
//
class mthd_container_t {
public:
	mthd_container_t(const validate_mthd_chunk_result_t&);

	// NB:  Second arg is the _exact_ size, not a max size
	mthd_container_t(const unsigned char *p, int32_t sz) 
		: p_(p),size_(sz) {};

	int16_t format() const;
	int16_t ntrks() const;
	uint16_t division() const;

	// Does not include the 4 byte "MThd" and 4 byte data-length fields
	int32_t data_size() const;
	// Includes the "MThd" and data-length fields
	int32_t size() const;	
private:
	const unsigned char *p_ {};  // points at the 'M' of "MThd..."
	int32_t size_ {0};
};

enum class midi_time_division_field_type_t {
	ticks_per_quarter,
	SMPTE
};
midi_time_division_field_type_t detect_midi_time_division_type(uint16_t);
uint16_t interpret_tpq_field(uint16_t);  // assumes midi_time_division_field_type_t::ticks_per_quarter
struct midi_smpte_field {
	int8_t time_code_fmt {0};
	uint8_t units_per_frame {0};
};
midi_smpte_field interpret_smpte_field(uint16_t);  // assumes midi_time_division_field_type_t::SMPTE
std::string print(const mthd_container_t&);



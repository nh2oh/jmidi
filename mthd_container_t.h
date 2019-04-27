#pragma once
#include "midi_raw.h"
#include <cstdint>
#include <string>


//
// Why not a generic midi_chunk_container_t<T> template?  Because MThd and MTrk containers
// are radically different and it really does not make sense to write generic functions to 
// operate on either type.  
//


class smf_t;

//
// mthd_view_t
//
// An mthd_view_t is a non-owning view into a sequence of bytes representing
// a valid MThd chunk.  There is no "invalid" state; posession of an 
// mthd_view_t is a guarantee that the underlying byte sequence is a valid
// MThd chunk.  
//
// Unlike mtrk_view_t, mthd_view_t is not very useful; it is hard to imagine
// a situation where users working with an smf_t would want to pass around
// lightweight views of the MThd rather than just copies of the underlying
// data fields.  Consider that sizeof(mthd_view_t)==16, but most (all?) 
// MThd chunks are only 14 bytes long.  It is included for completness, and
// because it is used internally by the smf_t member functions .format() 
// and .division(), which alias the corresponding mthd_view_t member funtions
// (see the documentation for smf_t for discussion).  
//
class mthd_view_t {
public:
	mthd_view_t(const validate_mthd_chunk_result_t&);

	uint16_t format() const;
	uint16_t ntrks() const;
	uint16_t division() const;

	// Does not include the 4 byte "MThd" and 4 byte data-length fields
	uint32_t data_length() const;
	// Includes the "MThd" and 4-byte data-length fields
	uint32_t size() const;	
private:
	// Second arg is the _exact_ size, not a max size
	mthd_view_t(const unsigned char *, uint32_t);

	const unsigned char *p_ {};  // points at the 'M' of "MThd..."
	uint32_t size_ {0};

	friend class smf_t;
};
std::string print(const mthd_view_t&);


//
// SMPTE => Society of Motion Picture and Television Engineers 
// 16-bit:  [[1] frames-per-second] [resolution-within-frame]
//
enum class midi_time_division_field_type_t {
	ticks_per_quarter,
	SMPTE
};
midi_time_division_field_type_t detect_midi_time_division_type(uint16_t);
uint16_t interpret_tpq_field(uint16_t);  // assumes midi_time_division_field_type_t::ticks_per_quarter
struct midi_smpte_field {
	int8_t time_code_fmt {0};  // ~frames-per-second
	uint8_t units_per_frame {0};  // ~ticks-per-frame
};
midi_smpte_field interpret_smpte_field(uint16_t);  // assumes midi_time_division_field_type_t::SMPTE


double ticks_per_second();
double seconds_per_tick();

//
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
	mthd_container_t(const unsigned char *p, uint32_t sz) 
		: p_(p),size_(sz) {};

	uint16_t format() const;
	uint16_t ntrks() const;
	uint16_t division() const;

	// Does not include the 4 byte "MThd" and 4 byte data-length fields
	int32_t data_size() const;
	// Includes the "MThd" and 4-byte data-length fields
	uint32_t size() const;	
private:
	const unsigned char *p_ {};  // points at the 'M' of "MThd..."
	uint32_t size_ {0};
};
std::string print(const mthd_container_t&);

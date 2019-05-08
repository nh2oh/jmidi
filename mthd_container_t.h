#pragma once
#include "midi_raw.h"
#include <cstdint>
#include <string>


//
// Why not a generic midi_chunk_container_t<T> template?  
// Because MThd and MTrk chunks are radically different and it does not make
// sense to write generic containers to store either type; their 
// functionality is completely orthogonal.  
//


class smf_t;
class mthd_t;

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
	friend class mthd_t;
};
std::string print(const mthd_view_t&);


//
// Owns the underlying data
// Methods alias to those in mthd_view_t & simply use the embedded
// view object v_ to call the corresponding view functions.  
//
class mthd_t {
public:
	mthd_t();
	uint16_t format() const;
	uint16_t ntrks() const;
	uint16_t division() const;

	// Does not include the 4 byte "MThd" and 4 byte data-length fields
	uint32_t data_length() const;
	// Includes the "MThd" and 4-byte data-length fields; not d_.size(),
	// since d_ could easily be over-allocated
	uint32_t size() const;	
private:
	mthd_view_t v_;
	std::vector<unsigned char> d_;
};





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
	// The SMPTE time code is the negative of the number of 
	// frames-per-second.  Ex, time_code_fmt==-30 => 30 frames-per-second
	// One exception:  -29 => 29.97 frames-per-second
	// The std values are:  -24, -25, -29 (=>29.97), -30
	int8_t time_code_fmt {0};  

	// Number of subframes per frame (=> ticks per frame)
	// Common values (std p.133): "4 (MIDI time code resolution),
	// 8, 10, 80 (bit resolution), or 100"
	// 
	uint8_t units_per_frame {0};  // ~ticks-per-frame
};
midi_smpte_field interpret_smpte_field(uint16_t);  // assumes midi_time_division_field_type_t::SMPTE


double ticks_per_second();
double seconds_per_tick();









class time_div_t {
public:
	enum class type {
		ticks_per_quarter,
		SMPTE
	};
	time_div_t::type type() const;

	// Default arg gives the # of us per quarter-note (the payload of a 
	// set-tempo meta msg: FF 51 03 tttttt).  If type()==SMPTE, this value
	// is used to convert ticks/sec (the interpretation of the SMPTE 
	// payload) to ticks/quarter.
	uint16_t ticks_per_quarter(uint16_t=120) const;
	
	// If SMPTE, returns the product of ticks_per_frame() and 
	// frames_per_sec().  
	// If type()==ticks_per_quarter, the default-arg (which represents the
	// payload of a set-tempo meta msg => usec/quarter nt) is used to convert.  
	uint16_t ticks_per_sec(uint16_t=120) const;

	// If type()==SMPTE, these return the values of the first and second bytes,
	// respectively.  If type()==ticks_per_quarter, i could convert given
	// a tempo (us/quarter) and a ticks/frame (==24, 30, ...), but for now
	// I am just going to return 0.  
	uint8_t ticks_per_frame() const;
	uint8_t frames_per_sec() const;
private:
	uint16_t d_;
};


class SMPTE_time_div_t {
public:
	SMPTE_time_div_t()=default;
	SMPTE_time_div_t(uint8_t,uint8_t);

	// Default arg gives the # of us per quarter-note (the payload of a 
	// set-tempo meta msg: FF 51 03 tttttt).  This value is used to convert 
	// ticks/sec (the interpretation of the SMPTE payload) to ticks/quarter
	// note.
	uint16_t ticks_per_quarter(uint16_t=120) const;
	// Returns the product of ticks_per_frame() and frames_per_sec().  
	uint16_t ticks_per_sec() const;
	// Return the values of the first and second bytes, respectively.  
	uint8_t ticks_per_frame() const;
	uint8_t frames_per_sec() const;
private:
	uint16_t d_ {0x8000u};
};


class tpq_time_div_t {
public:
	tpq_time_div_t()=default;
	explicit tpq_time_div_t(uint8_t);

	uint16_t ticks_per_quarter() const;
	// The default-arg (which represents the payload of a set-tempo meta msg
	// => usec/quarter nt) is used to convert from ticks-per-quarter to 
	// ticks-per-sec.  
	uint16_t ticks_per_sec(uint16_t=120) const;
private:
	uint16_t d_ {120};
};





//
//
// int16_t mthd_container_t::format():  Returns 0, 1, 2
//  -> 0 => File contains a single multi-channel track
//  -> 1 => File contains one or more simultaneous tracks
//  -> 3 =>  File contains one or more sequentially independent single-track patterns.  
// int16_t mthd_container_t::ntrks() is always == 1 for a fmt-type 0 file.  
//
//


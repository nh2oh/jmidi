#pragma once
#include "midi_raw.h"
#include "..\..\generic_iterator.h"
#include <cstdint>
#include <string>
#include <vector>


//
// Why not a generic midi_chunk_container_t<T> template?  
// Because MThd and MTrk chunks are radically different and it does not make
// sense to write generic containers to store either type.  
//

//
// Class mthd_t
//
// Container adapter around std::vector<unsigned char>
//
struct mthd_container_types_t {
	using value_type = unsigned char;
	using size_type = int64_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
};
class mthd_t {
public:
	using value_type = mthd_container_types_t::value_type;
	using size_type = mthd_container_types_t::size_type;
	using difference_type = mthd_container_types_t::difference_type;
	using reference = mthd_container_types_t::reference;
	using const_reference = mthd_container_types_t::const_reference;
	using pointer = mthd_container_types_t::pointer;
	using const_pointer = mthd_container_types_t::const_pointer;
	using iterator = generic_ra_iterator<mthd_container_types_t>;
	using const_iterator = generic_ra_const_iterator<mthd_container_types_t>;

	size_type size() const;

	pointer data();
	const_pointer data() const;

	int32_t format() const;
	int32_t ntrks() const;  // TODO:  Is this really, nchks (not _trks_)?
	int32_t division() const;
	
	bool set(const std::vector<unsigned char>&);
	bool set(const validate_mthd_chunk_result_t&);
private:
	std::vector<unsigned char> d_ {};
};
std::string print(const mthd_t&);







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


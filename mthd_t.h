#pragma once
#include "midi_raw.h"
#include "..\..\generic_iterator.h"
#include <cstdint>
#include <string>
#include <vector>

//
// Class mthd_t
//
// Container adapter around std::vector<unsigned char> representing an 
// MThd chunk.
//
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

	// Returns 0, 1, 2
	// -> 0 => One track (always => ntrks() == 1)
	// -> 1 => One or more simultaneous tracks
	// -> 3 => One or more sequential tracks
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
	uint8_t units_per_frame {0};  // ~ticks-per-frame
};
// assumes midi_time_division_field_type_t::SMPTE
midi_smpte_field interpret_smpte_field(uint16_t);  
double ticks_per_second();
double seconds_per_tick();



//
//

//
//


#pragma once
#include "midi_raw.h"
#include "..\..\generic_iterator.h"
#include <cstdint>
#include <string>
#include <vector>


struct time_division_t {
	enum type : uint8_t {
		ticks_per_quarter,
		smpte
	};
	uint16_t val_;
};

//
// Class mthd_t
//
// Container adapter around std::vector<unsigned char> representing an 
// MThd chunk.  Although every MThd chunk in existence is 14 bytes,
// the MIDI std stipulates that in the future it may be enlarged to 
// contain addnl fields.  
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

	// Synonyms
	size_type size() const;
	size_type nbytes() const;

	pointer data();
	const_pointer data() const;

	reference operator[](size_type);
	const_reference operator[](size_type) const;

	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;
	const_iterator cbegin() const;
	const_iterator cend() const;

	size_type length() const;
	// format();  Returns 0, 1, 2
	// -> 0 => One track (always => ntrks() == 1)
	// -> 1 => One or more simultaneous tracks
	// -> 3 => One or more sequential tracks
	int32_t format() const;
	// Note:  Number of MTrks, not the number of "chunks"
	int32_t ntrks() const;
	time_division_t division() const;
	int32_t set_format(int32_t);
	// Note:  Number of MTrks, not the number of "chunks"
	int32_t set_ntrks(int32_t);
	time_division_t set_division(time_division_t);
	size_type set_length(size_type);
	
	bool verify() const;

	// TODO:  Gross
	bool set(const std::vector<unsigned char>&);
	bool set(const validate_mthd_chunk_result_t&);
private:
	std::vector<unsigned char> d_ {};
};
std::string print(const mthd_t&);


bool verify(time_division_t);
time_division_t get(time_division_t, time_division_t=time_division_t{0xE250u});
// The make_() functions will never return an invalid field.  Invalid
// inputs are silently replaced with valid values.  
// The largest value allowed is 0x7FFFu == 32767
time_division_t make_time_division_tpq(uint16_t);
// int8_t SMPTE time-code-format, uint8_t units-per-frame
// Allowed values for arg 1 are -24, -25, -29, -30.  If an invalid value
// is passed in, -24 is substituted.  
time_division_t make_time_division_smpte(int8_t,uint8_t);
time_division_t::type type(time_division_t);
bool is_smpte(time_division_t);
bool is_tpq(time_division_t);
// The "default" values of 0 specified in the get_() functions are invalid
// for the respective quantity.  If the time_division_t passed in is not
// the correct format for the given get_() function, the caller can detect 
// this by checking for 0.  
// 
// int8_t get_time_code_fmt(time_division_t tdf, int8_t tcf=0);
// The SMPTE "time code format" (tcf) is the negative of the number of 
// frames-per-second.  Ex, time_code_fmt==-30 => 30 frames-per-second
// One exception:  -29 => 29.97 frames-per-second
// The legal values are:  -24, -25, -29 (=>29.97), -30
int8_t get_time_code_fmt(time_division_t, int8_t=0);
uint8_t get_units_per_frame(time_division_t, uint8_t=0);
// uint16_t get_tpq(time_division_t tdf, uint16_t tpq=0);
// The largest value allowed is 0x7FFFu == 32767
uint16_t get_tpq(time_division_t, uint16_t=0);




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



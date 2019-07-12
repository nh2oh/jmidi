#pragma once
#include "midi_vlq.h"  // validate_mthd_result_t etc for mthd ctors
#include "midi_raw.h"  // validate_mthd_result_t etc for mthd ctors
#include "..\..\generic_iterator.h"
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <limits> 

//
// class time_division_t
// Represents a 2-byte time-division field in an MThd chunk.  It is
// impossible for a time_division_t to hold a value that is invalid as a
// time division field.  Ctors silently convert invalid inputs into valid
// values.  
//
// "Support" classes tpq_t, time_code_t, subframes_t
// Represent values of a decoded time division field.  They do not enforce
// any invariants (they _can_ hold invalid values), and are not used by 
// mthd_t and time_division_t member getters or the free-function "getters"
// for these classes.  The only purpose they serve is to allow lib users to
// use the "user"-defined literal operators when passing what would 
// otherwise be ambiguous integer-typed arguments to time_division_t ctors.  
// 
// While they could be used more extensively, type-conversions are a pain
// and I do not want to force users deal w/ the associated problems.  I am 
// putting the fundamental-built-in-type <-> library-defined-type annoyance
// boundry at the time_division_t ctor and nowhere else.  
//
struct tpq_t {  // ticks-per-quarter-note
	uint16_t value;
};
tpq_t operator ""_tpq(unsigned long long);
struct time_code_t {  // SMPTE time-code: -24, -25, -30, etc
	int8_t value;
	time_code_t& operator-();  // Does nothing
};
time_code_t operator ""_time_code(unsigned long long);
struct subframes_t {  // subframes-per-frame / units-per-frame
	uint8_t value;
};
subframes_t operator ""_subframes(unsigned long long);
struct smpte_t {
	int8_t time_code;
	uint8_t subframes;
};
// Ctors can not create invalid values; getters can not return invalid 
// values.  
class time_division_t {
public:
	// SMPTE => Society of Motion Picture and Television Engineers 
	// 16-bit:  [[1] frames-per-second] [resolution-within-frame]
	// Number of subframes per frame ("units per frame," "ticks per frame")
	// Legal values (std p.133): "4 (MIDI time code resolution),
	// 8, 10, 80 (bit resolution), or 100"
	enum type : uint8_t {
		ticks_per_quarter,
		smpte
	};

	// 120 w/ type == ticks_per_quarter
	time_division_t()=default;
	// Argument is the value of the d_ array deserialized as a 
	// BE-encoded quantity.  Same as returned by .raw_value().  
	// TODO:  Consider removing?  Confusing and error-prone.  Users 
	// might expect it to create an obj of whatever (implicit) type 
	// their uint16_t was.  I also will get implicit conversions from
	// narrower int types.  
	explicit time_division_t(uint16_t);  // raw value
	// The high-bit is cleared with whatever consequences for the value.  
	// Since the high-bit is clear, the largest value allowed is
	// 0x7FFFu == 32767.  
	explicit time_division_t(tpq_t);  // ticks-per-quarter
	// Allowed values for arg 1 are -24, -25, -29, -30.  If an invalid 
	// value is passed in, -24 is substituted.  
	explicit time_division_t(time_code_t, subframes_t);
	
	// Getters
	// If called on an object of the wrong type, the value returned 
	// will nonetheless be valid as the requested type of quantity, but 
	// no other guarantees about this value are made.  A time_division_t 
	// /never/ holds or returns any invalid data.  
	// The get_*(time_code_t, T default) free functions allow users to
	// get better-defined behavior when requesting an incorrect data 
	// type.  
	uint16_t raw_value() const;
	smpte_t get_smpte() const;
	uint16_t get_tpq() const;
private:
	std::array<unsigned char,2> d_ {0x00u,0x78u};  // 120 ticks-per-quarter
};

// Factory functions
// These call the time_division_t ctors, which silently "fix" invalid
// values.  
time_division_t make_time_division_tpq(uint16_t);
// int8_t SMPTE time-code-format, uint8_t units-per-frame
time_division_t make_time_division_smpte(int8_t,uint8_t);
time_division_t::type type(time_division_t);
bool is_smpte(time_division_t);
bool is_tpq(time_division_t);
bool is_valid_time_division_raw_value(uint16_t);
// The "default" values of 0 specified in the get_() functions are invalid
// for the respective quantity.  If the time_division_t passed in is not
// the correct format for the given get_() function, the caller can detect 
// this by checking for 0.  
int8_t get_time_code_fmt(time_division_t, int8_t=0);
uint8_t get_units_per_frame(time_division_t, uint8_t=0);
uint16_t get_tpq(time_division_t, uint16_t=0);

//
// Class mthd_t
//
// Represents an MThd chunk, which is an array of bytes with the following
// format:
//    <chunk id> <length>  <format>  <ntrks>   <division> 
//    MThd       uint32_t  uint16_t  uint16_t  uint16_t
//
// The minimum size of an MThd is 14 bytes (with length field == 6), 
// and the smallest value allowed in the lengh field is therefore 6.  
//
// Invariants:
// It is impossible for this container to represent an MThd data that 
// would be invalid according to the MIDI std; hence, all accessors that
// return reference types are const qualified.  
//
// -> bytes 0-3 == {'M','T','h','d'}
// -> 6 >= length() <= std::numeric_limits<int32_t>::max()
//    length() == (size() - 8)
//    size() >= 14
// -> is_valid_time_division_raw_value(division())
//    Meaning, for SMPTE-format <division>'s, time-code == one of:
//    -24,-25,-29,-30
// -> 0 >= format() <= std::numeric_limits<uint16_t>::max()
// -> 0 >= ntrks() <= std::numeric_limits<uint16_t>::max()
//
// For a default-constructed mthd_t: format==1, ntrks==0, division==120 tpq.  
//
// mthd_t is a container adapter around std::vector<unsigned char>.  
// Although every MThd chunk in existence is 14 bytes, the MIDI std 
// stipulates that in the future it may be enlarged to contain addnl 
// fields, hence i can not simply use a std::array<,14>.  
//
struct mthd_container_types_t {
	using value_type = unsigned char;
	using size_type = int32_t;
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
	
	static constexpr size_type length_min = 6;
	static constexpr size_type length_max = std::numeric_limits<size_type>::max()-8;
	static constexpr size_type size_min = 14;
	static constexpr size_type size_max = std::numeric_limits<size_type>::max();

	// Format 1, 0 tracks, 120 tpq
	mthd_t()=default;
	// mthd_t(int32_t fmt, int32_t ntrks, time_division_t tdf);
	explicit mthd_t(int32_t, int32_t, time_division_t);

	// size() and nbytes() are synonyms
	size_type size() const;
	size_type nbytes() const;
	const_pointer data() const;
	const_reference operator[](size_type) const;
	const_iterator begin() const;
	const_iterator end() const;
	const_iterator cbegin() const;
	const_iterator cend() const;

	//
	// Getters
	//
	// Reads the value of the length field
	int32_t length() const;
	// format();  Returns 0, 1, 2
	// -> 0 => One track (always => ntrks() == 1)
	// -> 1 => One or more simultaneous tracks
	// -> 3 => One or more sequential tracks
	int32_t format() const;
	// Note:  Number of MTrks, not the number of "chunks"
	int32_t ntrks() const;
	time_division_t division() const;

	// Setters
	// If illegal values are passed in, for example, a value for set_ntrks()
	// > std::numeric_limits<uint16_t>::max(), or < 0, the value passed in
	// is _silently_ clamped to [0, std::numeric_limits<uint16_t>::max()].  
	int32_t set_format(int32_t);
	// Note:  Number of MTrks, not the number of "chunks"
	int32_t set_ntrks(int32_t);
	time_division_t set_division(time_division_t);
	// The input length is first clamped to [6, <int32_t>::max()].
	// If the new length is < the present length, the array will be 
	// truncated and data may be lost.  
	int32_t set_length(int32_t);
	
	bool verify() const;
private:
	iterator begin();
	iterator end();
	reference operator[](size_type);
	pointer data();

	std::vector<unsigned char> d_ {
		// MThd                   chunk length == 6
		0x4Du,0x54u,0x68u,0x64u,  0x00u,0x00u,0x00u,0x06u,
		// Fmt 1      ntrks         time-div == 120 tpq
		0x00u,0x01u,  0x00u,0x00u,  0x00u,0x78u
	};

	friend void set_from_bytes_unsafe(const unsigned char*,
										const unsigned char*, mthd_t*);
};
void set_from_bytes_unsafe(const unsigned char*, const unsigned char*, mthd_t*);

std::string print(const mthd_t&);
std::string& print(const mthd_t&, std::string&);

// maybe_mthd_t make_mthd(It beg, It end, lib_err_t err=lib_err_t())
// Make an mthd_t from an array of bytes.  
// This template simply wraps make_mthd_impl.  It only exists as a template
// to allow users to use pointers or STL iterators as input.  
struct maybe_mthd_t {
	mthd_t mthd;
	lib_err_t error;
	bool is_valid {false};
	operator bool() const;
};
template<typename It>
maybe_mthd_t make_mthd(It beg, It end, lib_err_t err=lib_err_t()) {
	static_assert(std::is_same<
		std::remove_cv<decltype(*beg)>::type,unsigned char&>::value,
		"It::operator*() must return an unsigned char&");
	static_assert(std::is_same<
			std::iterator_traits<It>::iterator_category,
			std::random_access_iterator_tag
		>::value,
		"It must be a random access iterator.");

	const unsigned char *pbeg = &(*beg);
	const unsigned char *pend = pbeg + (end-beg);
	return make_mthd_impl(pbeg,pend,err);
};

maybe_mthd_t make_mthd_impl(const unsigned char*, const unsigned char*,
							lib_err_t err=lib_err_t());

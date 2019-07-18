#pragma once
#include "generic_chunk_low_level.h"
#include "..\..\generic_iterator.h"
#include "midi_raw.h"  // time_division_t
#include <cstdint>
#include <string>
#include <vector>
#include <limits> 

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
// It is impossible for this container to represent MThd data that 
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
//    If ntrks > 1, format > 0
// -> 0 >= ntrks() <= std::numeric_limits<uint16_t>::max()
//    If format == 0, ntrks <= 1
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
	using difference_type = int32_t;
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

	// size()==14, length()==4; Format 1, 0 tracks, 120 tpq
	mthd_t()=default;
	// mthd_t(int32_t fmt, int32_t ntrks, time_division_t tdf);
	explicit mthd_t(int32_t, int32_t, time_division_t);
	// mthd_t(int32_t fmt, int32_t ntrks, int32_t division (tpq));
	// the value for division is silently clamped to [1,32767].  
	explicit mthd_t(int32_t, int32_t, int32_t);
	// mthd_t(int32_t fmt, int32_t ntrks, int32_t SMPTE-tcf, SMPTE-subdivs);
	explicit mthd_t(int32_t, int32_t, int32_t, int32_t);

	// size() and nbytes() are synonyms
	size_type size() const;
	size_type nbytes() const;
	const_pointer data() const;
	const_reference operator[](size_type) const;
	const_iterator begin() const;
	const_iterator end() const;
	const_iterator cbegin() const;
	const_iterator cend() const;

	// Getters
	int32_t length() const;
	int32_t format() const;
	// Note:  Number of MTrks, not the number of "chunks"
	int32_t ntrks() const;
	time_division_t division() const;

	// Setters
	// Illegal values are _silently_ set to legal values.  
	// If ntrks > 1, the allowed range is [1,0xFFFF]
	// If ntrks <= 1, the allowed range is [0,0xFFFF]
	int32_t set_format(int32_t);
	// Note:  Number of MTrks, not the number of "chunks"
	// If format() == 0, the allowed range is [0,1].  
	// If format() > 0, the allowed range is [0,0xFFFF].  
	int32_t set_ntrks(int32_t);
	time_division_t set_division(time_division_t);
	// Silently clamps the input to [1,0x7FFF] (==32767)
	int32_t set_division_tpq(int32_t);
	// smpte_t set_division_smpte(int32_t tcf, int32_t subdivs);
	smpte_t set_division_smpte(int32_t,int32_t);
	smpte_t set_division_smpte(smpte_t);
	// Sets the length field and resizes the array if necessary
	// The input length is first clamped to 
	// [mthd::length_min,mthd_t::length_max].  If the new length is < the 
	// present length, the array will be truncated and data may be lost.  
	int32_t set_length(int32_t);
private:
	using vec_szt = std::vector<unsigned char>::size_type;
	//static_assert(std::numeric_limits<vec_szt>::min()<=std::numeric_limits<size_type>::min());
	//static_assert(std::numeric_limits<vec_szt>::max()>=std::numeric_limits<size_type>::max());
	static constexpr int32_t format_min = 0;
	static constexpr int32_t format_max = 0xFFFF;
	static_assert(format_min >= std::numeric_limits<uint16_t>::min());
	static_assert(format_min <= std::numeric_limits<uint16_t>::max());
	static constexpr int32_t ntrks_min = 0;
	static constexpr int32_t ntrks_max_fmt_0 = 1;
	static constexpr int32_t ntrks_max_fmt_gt0 = 0xFFFF;
	static_assert(ntrks_min >= std::numeric_limits<uint16_t>::min());
	static_assert(ntrks_max_fmt_gt0 <= std::numeric_limits<uint16_t>::max());

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


//
// Low-level validation & processing of MThd chunks
// <Header Chunk> = <chunk type> <length> <format> <ntrks> <division>  
// - If format == 0, ntrks must be <= 1
// - If division specifies an SMPTE value, the only allowed values of
//   time code are -24, -25, -29, or -30.  
// - The smallest value allowed for the 'length' field is 6.  
// Larger values for 'length' (up to mthd_t::length_max) are allowed.  
// From the MIDI std p.134:
//  "Also, more parameters may be added to the MThd chunk in the 
//  future: it is important to read and honor the length, even if it is 
//  longer than 6."
// Values of format other than 0, 1, or 2 are also allowed.  From the 
// MIDI std p.143:  
//  "We may decide to define other format IDs to support other 
//  structures. A program encountering an unknown format ID may 
//  still read other MTrk chunks it finds from the file, as format
//  1 or 2, if its user can make sense..."
//
//
// maybe_mthd_t make_mthd(It beg, It end, lib_err_t err=lib_err_t())
// And friends...
//
struct mthd_error_t {
	enum class errc : uint8_t {
		header_error,
		overflow,  // in the data section; (end-beg)<14 or (end-beg)<(4+length)
		invalid_id,  // Not == 'MThd'
		length_lt_min,  // length < 6
		length_gt_mthd_max,  // length > ...limits<int32_t>::max()
		invalid_time_division,
		inconsistent_format_ntrks,  // Format == 0 but ntrks > 1
		no_error,
		other
	};
	chunk_header_error_t hdr_error;
	uint32_t length {0u};
	uint16_t format {0u};
	uint16_t ntrks {0u};
	uint16_t division {0u};
	mthd_error_t::errc code {mthd_error_t::errc::no_error};
};
struct maybe_mthd_t {
	mthd_t mthd;
	bool is_valid {false};
	operator bool() const;
};
maybe_mthd_t make_mthd_impl(const unsigned char*, const unsigned char*,
							mthd_error_t*);
maybe_mthd_t make_mthd(const unsigned char*, const unsigned char*,
							mthd_error_t*);
maybe_mthd_t make_mthd(const unsigned char*, const unsigned char*);
std::string explain(const mthd_error_t&);
template<typename It>
maybe_mthd_t make_mthd(It beg, It end, mthd_error_t *err) {
	static_assert(std::is_same<
		std::remove_cv<decltype(*beg)>::type,unsigned char&>::value,
		"It::operator*() must return an unsigned char&");
	static_assert(std::is_same<
			std::iterator_traits<It>::iterator_category,
			std::random_access_iterator_tag
		>::value,
		"It must be a random access iterator.");

	const unsigned char *pbeg = nullptr;
	const unsigned char *pend = nullptr;
	if (beg != end) {
		// Note that if beg==end i can not do *beg
		pbeg = &(*beg);
		pend = pbeg + (end-beg);
	}
	return make_mthd_impl(pbeg,pend,err);
};



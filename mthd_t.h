#pragma once
#include "generic_chunk_low_level.h"
#include "..\..\generic_iterator.h"
#include "midi_raw.h"  // time_division_t
#include "midi_vlq.h"
#include <cstdint>
#include <string>
#include <vector>
#include <limits> 
#include <array>

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

struct maybe_mthd_t;
struct mthd_error_t;

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

	template<typename InIt>
	friend maybe_mthd_t make_mthd(InIt, InIt, mthd_error_t *);
};
std::string print(const mthd_t&);
std::string& print(const mthd_t&, std::string&);

//
// template <typename InIt>
// maybe_mthd_t make_mthd(InIt it, InIt end, mthd_error_t *err)
//
// Low-level validation & processing of MThd chunks
// <Header Chunk> = <chunk type> <length> <format> <ntrks> <division>  
// - If format == 0, ntrks must be <= 1
// - If division specifies an SMPTE value, the only allowed values of
//   time code are -24, -25, -29, or -30.  
// - The smallest value allowed for the 'length' field is 6.  
// Larger values for 'length' (up to mthd_t::length_max) are allowed, but 
//   the specified number of bytes must actually be extractable from the
//   iterators.  
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
struct mthd_error_t {
	enum class errc : uint8_t {
		header_error,
		overflow,  // in the data section; (end-beg)<14 or (end-beg)<(4+length)
		invalid_id,  // Not == 'MThd'
		invalid_length,
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
std::string explain(const mthd_error_t&);
struct maybe_mthd_t {
	mthd_t mthd;
	mthd_error_t::errc code {mthd_error_t::errc::other};
	operator bool() const;
};
template <typename InIt>
maybe_mthd_t make_mthd(InIt it, InIt end, mthd_error_t *err) {
	// <Header Chunk> = <chunk type> <length> <format> <ntrks> <division> 
	//                   MThd uint32_t uint16_t uint16_t uint16_t

	maybe_mthd_t result;
	auto set_error = [&err,&result](mthd_error_t::errc ec, 
						uint32_t length, uint16_t fmt, uint16_t ntrks,
						uint16_t div) -> void {
		result.code = ec;
		if (err) {
			err->length = length;
			err->format = fmt;
			err->ntrks = ntrks;
			err->division = div;
			err->code = ec;
		}
	};
	
	result.mthd.d_.resize(14);
	auto dest = result.mthd.d_.begin();
	int i=0;
	
	//
	// {'M','T','r','k'}
	//
	i = 0;
	while ((it!=end)&&(i<8)) {
		++i;
		*dest++ = *it++;
	}
	if (i!=8) {
		set_error(mthd_error_t::errc::header_error,0u,0,0,0);
		return result;
	}
	if (!is_mthd_header_id(result.mthd.d_.data(),result.mthd.d_.data()+4)) {
		set_error(mthd_error_t::errc::invalid_id,0u,0,0,0);
		return result;
	}
	auto length = read_be<uint32_t>(result.mthd.d_.data()+4,result.mthd.d_.data()+8);
	if ((length < 6) || (length > mthd_t::length_max)) {
		set_error(mthd_error_t::errc::invalid_length,length,0,0,0);
		return result;
	}
	auto slength = static_cast<int32_t>(length);
	
	//
	// Format, ntrks, division
	//
	i = 0;
	while ((it!=end)&&(i<6)) {
		++i;
		*dest++ = *it++;
	}
	if (i!=6) {
		set_error(mthd_error_t::errc::overflow,length,0,0,0);
		return result;
	}
	auto format = read_be<uint16_t>(result.mthd.d_.data()+8,result.mthd.d_.data()+10);
	auto ntrks = read_be<uint16_t>(result.mthd.d_.data()+10,result.mthd.d_.data()+12);
	auto division = read_be<uint16_t>(result.mthd.d_.data()+12,result.mthd.d_.data()+14);
	if ((format==0) && (ntrks >1)) {
		set_error(mthd_error_t::errc::inconsistent_format_ntrks,
			length,format,ntrks,division);
		return result;
	}
	if (!is_valid_time_division_raw_value(division)) {
		set_error(mthd_error_t::errc::invalid_time_division,
			length,format,ntrks,division);
		return result;
	}

	//
	// Data beyond the std-specified format-ntrks-division fields
	//
	if (slength > 6) {
		result.mthd.d_.resize(8+slength);  // NB:  invalidates dest
		dest = result.mthd.d_.begin()+14;
		auto len = slength - 6;
		i=0;
		while ((it!=end) && (i<len)) {
			*dest++ = *it++;
			++i;
		}
		if (i!=len) {
			set_error(mthd_error_t::errc::overflow,
				length,format,ntrks,division);
			return result;
		}
	}
	
	result.code = mthd_error_t::errc::no_error;
	return result;
};



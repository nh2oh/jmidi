#pragma once
#include "generic_chunk_low_level.h"
#include "generic_iterator.h"
#include "small_bytevec_t.h"
#include "midi_time.h"  // jmid::time_division_t
#include "midi_vlq.h"
#include <cstdint>
#include <cstddef>  // std::ptrdiff_t
#include <string>
#include <limits> 
#include <array>
#include <algorithm>

namespace jmid {

//
// Class mthd_t
//
// Represents an MThd chunk, which is an array of bytes with the following
// format:
//    <chunk id> <length>  <format>  <ntrks>   <division> 
//    MThd       uint32_t  uint16_t  uint16_t  uint16_t
//
// Default-ctor:  format==1, ntrks==0, division==120 tpq.  
//
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
// The format <-> ntrks invariant is maintained under calls to set_format() 
// and set_ntrks() by altering the value of format.  An attempt to call 
// set_format(0) on an object with ntrks > 1 results in no change to the 
// value for format.  If the value for ntrks is set to something > 1 on
// an object with format==0, the value of format is set to 1.  That is, 
// calls to set_ntrks() always "succeed" unconditionally, but format() may
// change out from under you.  
//
// mthd_t is a container adapter around small_bytevec_t.  
// Although every MThd chunk in existence is 14 bytes, the MIDI std 
// stipulates that in the future it may be enlarged to contain addnl 
// fields, hence i can not simply use a std::array<,14>.  
//
struct maybe_mthd_t;
struct mthd_error_t;
class mthd_t;

struct mthd_container_types_t {
	using value_type = unsigned char;
	using size_type = std::int32_t;
	using difference_type = std::int32_t;
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
	using iterator = internal::generic_ra_iterator<mthd_container_types_t>;
	using const_iterator = internal::generic_ra_const_iterator<mthd_container_types_t>;
	
	static constexpr size_type length_min = 6;
	static constexpr size_type length_max = std::numeric_limits<size_type>::max()-8;
	static constexpr size_type size_min = 14;
	static constexpr size_type size_max = std::numeric_limits<size_type>::max();

	// size()==14, length()==4; Format 1, 0 tracks, 120 tpq
	mthd_t() noexcept;
	// mthd_t(int32_t fmt, int32_t ntrks, jmid::time_division_t tdf);
	explicit mthd_t(std::int32_t, std::int32_t, jmid::time_division_t) noexcept;
	// mthd_t(int32_t fmt, int32_t ntrks, int32_t division (tpq));
	// the value for division is silently clamped to [1,32767].  
	explicit mthd_t(std::int32_t, std::int32_t, std::int32_t) noexcept;
	// mthd_t(int32_t fmt, int32_t ntrks, int32_t SMPTE-tcf, SMPTE-subdivs);
	explicit mthd_t(std::int32_t, std::int32_t, std::int32_t, std::int32_t) noexcept;

	mthd_t(const mthd_t&);
	mthd_t(mthd_t&&) noexcept;

	mthd_t& operator=(const mthd_t&);
	mthd_t& operator=(mthd_t&&) noexcept;
	~mthd_t() noexcept;

	// size() and nbytes() are synonyms
	size_type size() const noexcept;
	size_type nbytes() const noexcept;
	const_pointer data() const noexcept;
	const_pointer data() noexcept;
	value_type operator[](size_type) const noexcept;
	value_type operator[](size_type) noexcept;
	const_iterator begin() const noexcept;
	const_iterator end() const noexcept;
	const_iterator begin() noexcept;
	const_iterator end() noexcept;
	const_iterator cbegin() const noexcept;
	const_iterator cend() const noexcept;

	// Getters
	std::int32_t length() const noexcept;
	std::int32_t format() const noexcept;
	// Note:  Number of MTrks, not the number of "chunks"
	std::int32_t ntrks() const noexcept;
	jmid::time_division_t division() const noexcept;

	// Setters
	// Illegal values are _silently_ set to legal values.  
	// If ntrks > 1, the allowed range is [1,0xFFFF]
	// If ntrks <= 1, the allowed range is [0,0xFFFF]
	std::int32_t set_format(std::int32_t) noexcept;
	// Number of MTrks, not the number of "chunks"
	// If format() == 0, the allowed range is [0,1].  
	// If format() > 0, the allowed range is [0,0xFFFF].  
	std::int32_t set_ntrks(std::int32_t) noexcept;
	jmid::time_division_t set_division(jmid::time_division_t) noexcept;
	// set_division_tpq/smpte() pass the input to the jmid::time_division_t ctors,
	// which silently correct invalid values.  
	std::int32_t set_division_tpq(std::int32_t) noexcept;
	// smpte_t set_division_smpte(int32_t tcf, int32_t subdivs);
	jmid::smpte_t set_division_smpte(std::int32_t,std::int32_t) noexcept;
	jmid::smpte_t set_division_smpte(jmid::smpte_t) noexcept;
	// Sets the length field and resizes the array if necessary
	// The input length is first clamped to 
	// [mthd::length_min,mthd_t::length_max].  If the new length is < the 
	// present length, the array will be truncated and data may be lost.  
	std::int32_t set_length(std::int32_t);
private:
	static constexpr std::int32_t format_min = 0;
	static constexpr std::int32_t format_max = 0xFFFF;
	static_assert(format_min >= std::numeric_limits<std::uint16_t>::min());
	static_assert(format_min <= std::numeric_limits<std::uint16_t>::max());
	static constexpr std::int32_t ntrks_min = 0;
	static constexpr std::int32_t ntrks_max_fmt_0 = 1;
	static constexpr std::int32_t ntrks_max_fmt_gt0 = 0xFFFF;
	static_assert(ntrks_min >= std::numeric_limits<std::uint16_t>::min());
	static_assert(ntrks_max_fmt_gt0 <= std::numeric_limits<std::uint16_t>::max());

	static const std::array<unsigned char,14> def_;

	struct init_small_w_size_0_t {};
	mthd_t(init_small_w_size_0_t) noexcept;
	void default_init() noexcept;

	jmid::internal::small_bytevec_t d_;

	template <typename InIt>
	friend InIt make_mthd(InIt, InIt, maybe_mthd_t*, mthd_error_t*, std::int32_t);

	template <typename InIt>
	friend InIt make_mthd2(InIt, InIt, mthd_t*, mthd_error_t*);
};
std::string print(const mthd_t&);
std::string& print(const mthd_t&, std::string&);

//
// template <typename InIt>
// InIt make_mthd(InIt it, InIt end, maybe_mthd_t *result,
//					mthd_error_t *err)
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
	enum class errc : std::uint8_t {
		header_overflow,
		non_mthd_id,
		invalid_length,
		length_lt_mthd_min,  // 6
		length_gt_mthd_max,  // length > ...limits<int32_t>::max()-8
		overflow_in_data_section,
		invalid_time_division,
		inconsistent_format_ntrks,  // Format == 0 but ntrks > 1
		no_error,
		other
	};
	// TODO:  This array is dumb
	//std::array<unsigned char, 14> header;
	mthd_error_t::errc code {mthd_error_t::errc::no_error};
};
std::string explain(const mthd_error_t&);
struct maybe_mthd_t {
	mthd_t mthd;
	std::ptrdiff_t nbytes_read;
	mthd_error_t::errc error;
	operator bool() const;
};


//
// Overwrites the mthd_t at result w/ the mthd read in from [it,end).  
//
template <typename InIt>
InIt make_mthd2(InIt it, InIt end, mthd_t *result, mthd_error_t *err) {
	// <Header Chunk> = <chunk type> <length> <format> <ntrks> <division> 
	//                   MThd uint32_t uint16_t uint16_t uint16_t
	auto set_error = [&err](mthd_error_t::errc ec)->void {
		if (err) {
			err->code = ec;
		}
	};
	
	result->d_.resize_nocopy(14);
	auto dest = result->d_.begin();  // TODO:  Returned by resize_nocopy();
	auto dest_beg = dest;
	set_error(mthd_error_t::errc::no_error);

	// {'M','T','h','d', n,n,n,n}
	int i = 0;
	while ((it!=end) && (i<8)) {
		++i;
		*dest++ = static_cast<unsigned char>(*it++);
	}
	if (i!=8) {
		result->d_.resize(dest-result->d_.begin());
		set_error(mthd_error_t::errc::header_overflow);
		return it;
	}
	if (!jmid::is_mthd_header_id(dest_beg,dest)) {
		result->d_.resize(dest-result->d_.begin());
		set_error(mthd_error_t::errc::non_mthd_id);
		return it;
	}
	auto ulen = jmid::read_be<std::uint32_t>(dest_beg+4,dest);
	if ((ulen < 6) || (ulen > mthd_t::length_max)) {
		result->d_.resize(dest-result->d_.begin());
		set_error(mthd_error_t::errc::invalid_length);
		return it;
	}
	auto len = static_cast<std::int32_t>(ulen);
	
	// Format, ntrks, division
	// TODO:  I am reading these fields directly into the mthd_t d_ array, 
	// thereby breaking the invariants of the class.  Wat.  
	// TODO:  ... and in consequence i am repeating the invariant checks
	// below:  if ((format==0) && (ntrks >1)) ...
	i = 0;  // The number of bytes read from the data section (== i-8).  
	while ((it!=end) && (i<6)) {
		++i;
		*dest++ = static_cast<unsigned char>(*it++);
	}
	if (i!=6) {
		result->d_.resize(dest-result->d_.begin());
		set_error(mthd_error_t::errc::overflow_in_data_section);
		return it;
	}
	auto format = jmid::read_be<std::uint16_t>(dest_beg+8,dest_beg+10);
	auto ntrks = jmid::read_be<std::uint16_t>(dest_beg+10,dest_beg+12);
	auto division = jmid::read_be<std::uint16_t>(dest_beg+12,dest_beg+14);
	if ((format==0) && (ntrks >1)) {
		result->d_.resize(dest-result->d_.begin());
		set_error(mthd_error_t::errc::inconsistent_format_ntrks);
		return it;
	}
	if (!jmid::is_valid_time_division_raw_value(division)) {
		result->d_.resize(dest-result->d_.begin());
		set_error(mthd_error_t::errc::invalid_time_division);
		return it;
	}

	// Data beyond the standard 6 bytes
	if (len > 6) {
		auto dest_end = result->d_.begin() + result->d_.size();
		i=0;
		while ((i<len) && (it!=end)) {
			if (dest == dest_end) {
				auto old_sz = result->d_.size();
				result->d_.resize(old_sz+256);  // TODO:  Magic number 256
				dest_beg = result->d_.begin();
				dest = dest_beg + old_sz;
				dest_end = dest_beg + result->d_.size();
			}
			++i;
			*dest++ = static_cast<unsigned char>(*it++);
		}
		if (i!=len) {
			result->d_.resize(dest-result->d_.begin());
			set_error(mthd_error_t::errc::overflow_in_data_section);
			return it;
		}
		// Above, the loop that writes data beyond the standard 6 bytes may 
		// oversize the container, since it calls resize() 256 bytes at a 
		// time.  
		result->d_.resize(dest - result->d_.begin());
	}

	return it;
};

template <typename InIt>
mthd_t make_mthd2(InIt it, InIt end, mthd_error_t *err) {
	// <Header Chunk> = <chunk type> <length> <format> <ntrks> <division> 
	//                   MThd uint32_t uint16_t uint16_t uint16_t
	mthd_t result;
	it = make_mthd2(it,end,&result,err);
	return result;
};




/*
template <typename InIt>
InIt make_mthd(InIt it, InIt end, maybe_mthd_t *result, mthd_error_t *err,
				const std::int32_t max_stream_bytes) {
	// <Header Chunk> = <chunk type> <length> <format> <ntrks> <division> 
	//                   MThd uint32_t uint16_t uint16_t uint16_t

	//result->mthd = mthd_t(mthd_t::init_small_w_size_0_t {}); 
	//result->mthd.d_.resize_small2small_nocopy(14);
	result->mthd.d_.resize_preserve_cap(14);
	auto dest = result->mthd.d_.begin();  // TODO:  Returned by resize_preserve_cap();
	auto dest_beg = dest;
	std::ptrdiff_t i=0;  // The number of bytes read from the input stream

	auto set_error = [&err,&result,&i](mthd_error_t::errc ec)->void {
		result->error = ec;
		result->nbytes_read = i;
		if (err) {
			auto n = std::min(err->header.end()-err->header.begin(),
				result->mthd.d_.end()-result->mthd.d_.begin());
			err->header.fill(0x00u);
			std::copy(result->mthd.d_.begin(),result->mthd.d_.begin()+n,
				err->header.data());
			err->code = ec;
		}
	};
	
	// {'M','T','h','d', n,n,n,n}
	while ((it!=end) && (i<8) && (i<max_stream_bytes)) {
		++i;
		*dest++ = static_cast<unsigned char>(*it++);
	}
	if ((i!=8) || (i>=max_stream_bytes)) {
		set_error(mthd_error_t::errc::header_overflow);
		return it;
	}
	if (!jmid::is_mthd_header_id(dest_beg,dest)) {
		set_error(mthd_error_t::errc::non_mthd_id);
		return it;
	}
	auto ulen = jmid::read_be<std::uint32_t>(dest_beg+4,dest);
	if ((ulen < 6) || (ulen > mthd_t::length_max)) {
		set_error(mthd_error_t::errc::invalid_length);
		return it;
	}
	auto len = static_cast<std::int32_t>(ulen);
	
	// Format, ntrks, division
	// TODO:  I am reading these fields directly into the mthd_t d_ array, 
	// thereby breaking the invariants of the class.  Wat.  
	// TODO:  ... and in consequence i am repeating the invariant checks
	// below:  if ((format==0) && (ntrks >1)) ...
	int j = 0;  // The number of bytes read from the data section (== i-8).  
	while ((it!=end) && (j<6) && (i<max_stream_bytes)) {
		++j;
		++i;
		*dest++ = static_cast<unsigned char>(*it++);
	}
	if ((j!=6)) {  // || (i>=max_stream_bytes)) {
		set_error(mthd_error_t::errc::overflow_in_data_section);
		return it;
	}
	auto format = jmid::read_be<std::uint16_t>(dest_beg+8,dest_beg+10);
	auto ntrks = jmid::read_be<std::uint16_t>(dest_beg+10,dest_beg+12);
	auto division = jmid::read_be<std::uint16_t>(dest_beg+12,dest_beg+14);
	if ((format==0) && (ntrks >1)) {
		set_error(mthd_error_t::errc::inconsistent_format_ntrks);
		return it;
	}
	if (!jmid::is_valid_time_division_raw_value(division)) {
		set_error(mthd_error_t::errc::invalid_time_division);
		return it;
	}

	// Data beyond the standard 6 bytes
	auto dest_end = result->mthd.d_.end();
	while ((j<len) && (it!=end) && (i<max_stream_bytes)) {
		if (dest == dest_end) {
			auto sz = result->mthd.d_.size();
			result->mthd.d_.resize(sz+256);  // Secret hardcoded resize policy
			dest_beg = result->mthd.d_.begin();
			dest = dest_beg + sz;
			dest_end = result->mthd.d_.end();
		}
		++j;
		++i;
		*dest++ = static_cast<unsigned char>(*it++);
	}
	if ((j!=len)) { // || (i>=max_stream_bytes)) {
		set_error(mthd_error_t::errc::overflow_in_data_section);
		return it;
	}*/
	/*
	// Data beyond the std-specified format-ntrks-division fields.  
	// The present size of mthd.d_ is 14
	if (len > j) {  //if (len > 6) {
		result->mthd.d_.resize(8+len);  // NB:  invalidates dest, dest_beg
		// dest = result->mthd.d_.begin()+14; // Always true initially
		//dest_beg = dest;
		auto len = len - 6;
		j=0;
		while ((it!=end) && (j<len)) {
			++j;
			*dest++ = static_cast<unsigned char>(*it++);  ++i;
		}
		if (j!=len) {
			set_error(mthd_error_t::errc::overflow_in_data_section);
			return it;
		}
	}*/
	/*
	result->error = mthd_error_t::errc::no_error;
	result->nbytes_read = i;
	return it;
};

template <typename InIt>
maybe_mthd_t make_mthd(InIt it, InIt end, mthd_error_t *err, 
						std::int32_t max_stream_bytes) {
	// <Header Chunk> = <chunk type> <length> <format> <ntrks> <division> 
	//                   MThd uint32_t uint16_t uint16_t uint16_t
	maybe_mthd_t result;
	it = make_mthd(it,end,&result,err,max_stream_bytes);
	return result;
};
*/


}  // namespace jmid


#pragma once
#include "midi_vlq.h"
#include <cstdint>
#include <type_traits>

namespace jmid {

// True if the input value falls within [0,0x0FFFFFFF] == [0,268,435,455]
bool is_valid_delta_time(std::int32_t);

// Clamps the input value to [0,0x0FFFFFFF] == [0,268,435,455]
std::int32_t to_nearest_valid_delta_time(std::int32_t);
std::int32_t delta_time_field_size(std::int32_t);
std::int32_t delta_time_field_size_unsafe(std::int32_t);

struct dt_field_interpreted {
	std::int32_t val {0};
	std::int8_t N {0};
	bool is_valid {false};
};
// TODO:  Add (result.N > 0) check:  See overload 2
template<typename InIt>
dt_field_interpreted read_delta_time(InIt beg, InIt end) {
	//static_assert(std::is_same<std::remove_reference<decltype(*InIt)>::type,
	//	const unsigned char>::value);

	dt_field_interpreted result {0,0,false};
	std::uint32_t uval = 0;
	unsigned char uc = 0;
	while (beg!=end) {
		uc = *beg++;
		uval += uc&0x7Fu;
		++(result.N);
		if ((uc&0x80u) && (result.N<4)) {
			uval <<= 7;  // Note:  Not shifting on the final iteration
		} else {  // High bit not set => this is the final byte
			break;
		}
	}
	result.val = static_cast<std::int32_t>(uval);
	result.is_valid = !(uc & 0x80u);
	return result;
};
template<typename InIt>
InIt read_delta_time(InIt beg, InIt end, dt_field_interpreted& result) {
	result.N = 0;
	std::uint32_t uval = 0;
	unsigned char uc = 0;
	while (beg!=end) {
		uc = *beg++;
		uval += uc&0x7Fu;
		++(result.N);
		if ((uc&0x80u) && (result.N<4)) {
			uval <<= 7;  // Note:  Not shifting on the final iteration
		} else {  // High bit not set => this is the final byte
			break;
		}
	}
	result.val = static_cast<std::int32_t>(uval);
	result.is_valid = (!(uc & 0x80u)) && (result.N > 0);
	// The (result.N > 0) check will cause result.is_valid==false in the
	// case that the caller passed in iterators where beg==end.  
	return beg;
};
// Advance the iterator to the end of the delta-time vlq; a maximum of 4 
// bytes, or to the end of the range, whichever comes first.  
template<typename InIt>
InIt advance_to_dt_end(InIt beg, InIt end) {
	//static_assert(std::is_same<std::remove_reference<decltype(*beg)>::type,
	//	const unsigned char>::value);
	int n=0;
	while ((beg!=end) && (n<4) && ((*beg++)&0x80u)) {
		++n;
	}
	return beg;
};

template<typename OIt>
OIt write_delta_time(std::int32_t val, OIt it) {
	val = jmid::to_nearest_valid_delta_time(val);
	std::uint32_t uval = static_cast<std::uint32_t>(val);
	return jmid::write_vlq(uval,it);
};
template<typename OIt>
OIt write_delta_time_unsafe(std::int32_t val, OIt it) {
	std::uint32_t uval = static_cast<std::uint32_t>(val);
	return jmid::write_vlq_unsafe(uval,it);
};


}  // namespace jmid


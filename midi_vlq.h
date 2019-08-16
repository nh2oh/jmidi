#pragma once
#include <limits>  // CHAR_BIT
#include <type_traits>  // std::enable_if<>, is_integral<>, is_unsigned<>
#include <cstdint>
#include <cstring>


// TODO:  Need to clarify behavior for iterators w/ value-type char
// vs unsigned char.  

// 
// The max size of a vlq field is 4 bytes, and the largest value it may 
// encode is 0x0FFFFFFFu (BE-encoded as: FF FF FF 7F) => 268,435,455, 
// which fits safely in an int32_t:  
// std::numeric_limits<int32_t>::max() == 2,147,483,647;
//
struct vlq_field_interpreted {
	int32_t val {0};
	int8_t N {0};
	bool is_valid {false};
};
template<typename InIt>
vlq_field_interpreted read_vlq(InIt beg, InIt end) {
	//static_assert(std::is_same<std::remove_reference<decltype(*InIt)>::type,
	//	const unsigned char>::value);

	vlq_field_interpreted result {0,0,false};
	uint32_t uval = 0;
	unsigned char uc = 0;
	while (beg!=end) {
		uc = *beg++;
		uval += uc&0x7Fu;
		++(result.N);
		if ((uc&0x80u) && (result.N<4)) {
			uval <<= 7;  // Note:  Not shifting on the final iteration
		} else {
			break;
		}
	}
	result.val = static_cast<int32_t>(uval);
	result.is_valid = !(uc & 0x80u);
	return result;
};
//
// Advance the iterator to the end of the vlq; do not parse the field
//
template<typename InIt>
InIt advance_to_vlq_end(InIt beg, InIt end) {
	int n=0;
	while((beg!=end) && (n<4) && (*beg++)&0x80u) {
		++n;
	}
	return beg;
};

//
// Computes the size (in bytes) of the field required to encode a given 
// number as a MIDI vlq.  
//
template<typename T>
constexpr int vlq_field_size(T val) {
	uint32_t uval;
	if (val < 0) {
		uval = 0;
	} else if (val > 0x0FFFFFFF) {
		uval = 0x0FFFFFFF;
	} else {
		uval = static_cast<uint32_t>(val);
	}
	int n = 0;
	do {
		uval >>= 7;
		++n;
	} while (uval!=0);
	return n;
};



template<typename T, typename OIt>
OIt write_vlq(T val, OIt it) {
	if (val < 0) {
		uint32_t uval = 0;
		*it++ = static_cast<unsigned char>(0x7Fu&uval);
	} else if (val <= 0x7Fu) {
		uint32_t uval = static_cast<uint32_t>(val);
		*it++ = static_cast<unsigned char>(0x7Fu&uval);
	} else if (val <= (0x3F'FFu)) {
		uint32_t uval = static_cast<uint32_t>(val);
		*it++ = static_cast<unsigned char>((0x80u)|(0xFFu&(uval>>7)));
		*it++ = static_cast<unsigned char>((0x7Fu)&uval);
	} else if (val <= (0x1F'FF'FF)) {
		uint32_t uval = static_cast<uint32_t>(val);
		*it++ = static_cast<unsigned char>((0x80u)|(0xFFu&(uval>>14)));
		*it++ = static_cast<unsigned char>((0x80u)|(0xFFu&(uval>>7)));
		*it++ = static_cast<unsigned char>((0x7Fu)&uval);
	} else if (val <= (0x0F'FF'FF'FFu)) {
		uint32_t uval = static_cast<uint32_t>(val);
		*it++ = static_cast<unsigned char>((0x80u)|(0xFFu&(uval>>21)));
		*it++ = static_cast<unsigned char>((0x80u)|(0xFFu&(uval>>14)));
		*it++ = static_cast<unsigned char>((0x80u)|(0xFFu&(uval>>7)));
		*it++ = static_cast<unsigned char>((0x7Fu)&uval);
	} else {
		uint32_t uval = 0x0F'FF'FF'FF;
		*it++ = static_cast<unsigned char>((0x80u)|(0xFFu&(uval>>21)));
		*it++ = static_cast<unsigned char>((0x80u)|(0xFFu&(uval>>14)));
		*it++ = static_cast<unsigned char>((0x80u)|(0xFFu&(uval>>7)));
		*it++ = static_cast<unsigned char>((0x7Fu)&uval);
	}
	return it;
};

//
// template<typename T> 
// T to_be_byte_order(T val)
//
// Reverses the byte-order in val such that the byte in least-significant 
// position is in most significant position, and the byte in 
// most-significant position is in least significant position, etc.  The
// result can be memcpy()'d into a buffer of unsigned char to serializes the
// input quantity in big-endian format.  
//
// Ex, the value 1 has the BE-representation 0x00'00'00'01 and the 
// LE-representation 0x01'00'00'00.  On an LE arch, passing in 1 for val
// will return 16777216, which is the value with LE-representation
// 0x00'00'00'01.  
//
template<typename T>
T to_be_byte_order(T val) {
	static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value);
	T be_val {0};
	for (int i=0; i<sizeof(T); ++i) {
		be_val <<= CHAR_BIT;
		be_val += (val&0xFFu);
		val >>= CHAR_BIT;
	}
	return be_val;
};


//
// template<typename T, typename InIt>
// read_be<T>(InIt beg, InIt end) 
//
// Where InIt is an input-iterator into an array of unsigned char and T 
// is an unsigned integer type, reads a BE-encoded value on 
// [beg, beg+min(sizeof(T),(end-beg))) into a zero-initialized T
// (T result {0}).  
//
// Note that the first byte in the sequence (*beg) is only interpreted as the
// most-significant byte in T if (end-beg) >= sizeof(T).  For a range
// [beg,end) < sizeof(T), the first byte is sizeof(T)-(end-beg) bytes 'below'
// the high byte of the T.  In all cases, the last byte read in corresponds to
// the least-significant byte in the result.  
// 
// For example, to read a 24-bit BE-encoded uint into a 32-bit uint given a
// data array 'data' encoding the 24-bit value:
// std::array<unsigned char,3> data {0x01u, 0x02u, 0x03u};
// Note that:
// data.end()-data.begin() == 3 
//                         < sizeof(uint32_t) == 4
// auto result = read_be<uint32_t>(data.begin(),data.end());
// yields a uint32_t result == 0x00'01'02'03u
// Note that because (end-beg) < sizeof(uint32_t), the first byte in the 
// array is not the most significant of the result.  
//
template<typename T, typename InIt>
T read_be(InIt beg, InIt end) {
	static_assert(std::is_same<
		std::decay<decltype(*beg)>::type,unsigned char>::value);
	static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value);

	T result {0};
	for (int i=0; (i<sizeof(T) && (beg!=end)); ++i) {
		result <<= CHAR_BIT;
		result += *beg++;
	}

	return result;
};



//
// OIt write_24bit_be(uint32_t val, OIt dest) {
//
// Where OIt is an iterator to an array of unsigned char and T is an 
// unsigned integer type, writes the 3 bytes of val _following_ the most
// significant into dest (in be format).  Returns an iterator pointing 
// to into the destination range one past the last element copied.  
//
template<typename OIt>
OIt write_24bit_be(uint32_t val, OIt dest) {
	static_assert(std::is_same<
		std::decay<decltype(*dest)>::type,unsigned char>::value);
	
	auto be_val = to_be_byte_order(val);
	unsigned char *p = static_cast<unsigned char*>(static_cast<void*>(&be_val));
	++p;  // Truncate the most-significant byte
	for (int i=0; i<3; ++i) {
		*dest++ = *p++;
	}
	return dest;
};
template<typename OIt>
OIt write_16bit_be(uint16_t val, OIt dest) {
	static_assert(std::is_same<
		std::decay<decltype(*dest)>::type,unsigned char>::value);
	
	auto be_val = to_be_byte_order(val);
	unsigned char *p = static_cast<unsigned char*>(static_cast<void*>(&be_val));
	for (int i=0; i<2; ++i) {
		*dest++ = *p++;
	}
	return dest;
};
template<typename OIt>
OIt write_32bit_be(uint32_t val, OIt dest) {
	//static_assert(std::is_same<
	//	std::decay<decltype(*dest)>::type,unsigned char>::value);
	
	auto be_val = to_be_byte_order(val);
	unsigned char *p = static_cast<unsigned char*>(static_cast<void*>(&be_val));
	for (int i=0; i<4; ++i) {
		*dest++ = *p++;
	}
	return dest;
};



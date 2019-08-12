#pragma once
#include <limits>  // CHAR_BIT
#include <type_traits>  // std::enable_if<>, is_integral<>, is_unsigned<>
#include <cstdint>
#include <cstring>


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
	while((beg!=end) && (*beg++)&0x80u) {
		true;
	}
	return beg;
};

//
// Returns an integer with the bit representation of the input encoded 
// as a midi vlq.  Ex for:
// input 0b1111'1111 == 0xFF == 255 
// => 0b1000'0001''0111'1111 == 0x817F == 33151
// The largest value a vlq can accommodate is 0x0FFFFFFF == 268,435,455.  
// Note that the MSByte of the max value is 0, not 7, corresponding to the 4 
// leading 0's in the MSBit of each byte below the most-significant in the 
// field.  
//
// The vlq representation of a given int is not necessarily unique; above,
// 0xFF == 255 => 0x81'7F == 33,151
//             => 0x80'81'7F == 8,421,759
//             => 0x80'80'81'7F == 2,155,905,407
//
template<typename T>
constexpr uint32_t vlq_field_literal_value(T val) {
	//static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value,
	//	"MIDI VL fields only encode unsigned integral values");
	
	uint32_t uval;
	if (val < 0) {
		uval = 0;
	} else if (val > 0x0FFFFFFF) {
		return uval = 0x0FFFFFFF;
	} else {
		uval = static_cast<uint32_t>(val);
	}
	
	uint32_t res {0};
	for (int i=0; uval>0; ++i) {
		if (i > 0) {
			res |= (0x80u<<(i*CHAR_BIT));
		}
		res += (uval & 0x7Fu)<<(i*CHAR_BIT);
		uval>>=7;
	}
	return res;
};
//
// Computes the size (in bytes) of the field required to encode a given 
// number as a MIDI vlq.  
//
template<typename T>
constexpr int vlq_field_size(T val) {
	//static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value,
	//	"MIDI VL fields only encode unsigned integral values");
	
	uint32_t uval;
	if (val < 0) {
		uval = 0;
	} else if (val > 0x0FFFFFFF) {
		return uval = 0x0FFFFFFF;
	} else {
		uval = static_cast<uint32_t>(val);
	}
	int n = 0;
	do {
		val >>= 7;
		++n;
	} while ((val!=0) && (n<4));
	return n;
};

//
// TODO:  This should clamp the input between 0x0FFFFFFF and 0.  
//
template<typename T, typename OIt>
OIt write_vlq(T val, OIt it) {
	static_assert(CHAR_BIT == 8);
	//static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value);
	auto vlval = vlq_field_literal_value(val);  // requires integral, unsigned
	
	uint8_t bytepos = 3;
	uint32_t mask = 0xFF000000;
	while (((vlval&mask)==0) && (mask>0)) {
		mask>>=8;  --bytepos;
	}

	if (mask==0) {  // Means vlval==0; still necessary to write an 0x00u
		*it=0x00u;
		++it;
	} else {
		while (mask>0) {
			*it = (vlval&mask)>>(8*bytepos);
			mask>>=8;  --bytepos;
			++it;
		}
	}
	return it;
};


//
// template<typename T> 
// T to_be_byte_order(T val)
//
// Reorders the bytes in val such that the byte in least-significant position 
// is in most significant position, and the byte in most-significant position
// is in least significant position.  memcpy()ing the result serializes the 
// input in big-endian format.  
//
// Ex:  
// 0x00'00'00'01u (1) => 0x01'00'00'00 (16777216)
// 0x01'00'00'00 (16777216) => 0x00'00'00'01u (1)
// 0x12'34'56'78u (305419896) => 0x78'56'34'12u (2018915346)
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
// most-significant byte in a T if (end-beg) >= sizeof(T).  For a range
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
	
	auto niter = sizeof(T)<=(end-beg) ? sizeof(T) : (end-beg);
	T result {0};
	for (int i=0; i<niter; ++i) {
		result <<= CHAR_BIT;
		result += *beg++;
	}
	return result;
};


//
// OIt write_bytes(T val, OIt dest)
//
// Writes sizeof(T) bytes into the output iterator dest.  Returns an 
// iterator pointing to into the destination range one past the last 
// element copied.  
//
template<typename T, typename OIt>
OIt write_bytes(T val, OIt dest) {
	static_assert(std::is_same<
		std::decay<decltype(*dest)>::type,unsigned char>::value);

	unsigned char *p = static_cast<unsigned char*>(static_cast<void*>(&val));
	for (int i=0; i<sizeof(T); ++i) {
		*dest++ = *p++;
	}
	return dest;
	// Alternatively:
	//auto src = static_cast<unsigned char*>(static_cast<void*>(&val));
	//std::memcpy(dest,src,sizeof(T));
	// However, memcpy() returns a void* (?)
};

// 
// OIt write_bytes(T val, std::size_t n, OIt dest)
//
// Writes min(sizeof(T), n) bytes into the output iterator dest.  Returns 
// an iterator pointing to into the destination range one past the last 
// element copied.  
//
template<typename T, typename OIt>
OIt write_bytes(T val, std::size_t n, OIt dest) {
	static_assert(std::is_same<
		std::decay<decltype(*dest)>::type,unsigned char>::value);

	auto niter = sizeof(T)<=n ? sizeof(T) : n;
	unsigned char *p = static_cast<unsigned char*>(static_cast<void*>(&val));
	for (int i=0; i<niter; ++i) {
		*dest++ = *p++;
	}
	return dest;
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
// TODO:  Could use write_bytes() here
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
// TODO:  Could use write_bytes() here
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



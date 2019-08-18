#pragma once
#include <limits>  // CHAR_BIT
#include <type_traits>  // std::enable_if<>, is_integral<>, is_unsigned<>
#include <cstdint>
#include <cstring>




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
template<typename T, typename OIt>
OIt write_vlq_old1(T val, OIt it) {
	static_assert(CHAR_BIT == 8);

	uint32_t uval;
	if (val < 0) {
		uval = 0;
	} else if (val > 0x0FFFFFFF) {
		uval = 0x0FFFFFFF;
	} else {
		uval = static_cast<uint32_t>(val);
	}

	auto s = 21u;
	uint32_t mask = (0b01111111u<<21);
	while (((uval&mask)==0u) && (mask>0u)) {
		mask>>=7u;
		s-=7u;
	}
	while (mask>0b01111111u) {
		*it++ = 0x80u|((uval&mask)>>(s));
		mask>>=7u;
		s-=7u;
	}
	*it++ = uval&mask;

	return it;
};



//
// TODO:  This should clamp the input between 0x0FFFFFFF and 0.  
//
template<typename T, typename OIt>
OIt write_vlq_old(T val, OIt it) {
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






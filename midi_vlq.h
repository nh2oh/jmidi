#pragma once
#include <limits>  // CHAR_BIT
#include <type_traits>  // std::enable_if<>, is_integral<>, is_unsigned<>
#include <cstdint>
#include <cstring>



//
// be_2_native<T>(InIt beg, InIt end) 
//
// Where InIt is an iterator to an array of unsigned char and T is an 
// unsigned integer type, reads a BE-encoded value on 
// [beg, beg+min(sizeof(T),(end-beg))) into a zero-initialized T:
// T result {0}.  
//
template<typename T, typename InIt>
T be_2_native(InIt beg, InIt end) {
	static_assert(std::is_same<
		std::remove_cvref<decltype(*beg)>::type,unsigned char>::value);
	static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value);
	T result {0};
	auto niter = sizeof(T)<=(end-beg) ? sizeof(T) : (end-beg);
	auto it = beg;
	for (int i=0; i<niter; ++i) {
		result <<= CHAR_BIT;
		result += *it;
		++it;
	}
	return result;
};


//
// template<typename T> native2be(T val)
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
T native2be(T val) {
	static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value);
	T be_val {0};
	T mask {0xFFu};
	for (int i=0; i<sizeof(T); ++i) {
		be_val <<= CHAR_BIT;
		auto temp = ((mask&val) >> (i*CHAR_BIT));
		be_val += temp;
		mask <<= CHAR_BIT;
	}
	return be_val;
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
		std::remove_cvref<decltype(*dest)>::type,unsigned char>::value);

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
		std::remove_cvref<decltype(*dest)>::type,unsigned char>::value);

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
// unsigned integer type, writes the 3 most significant bytes of val
// in be format into dest.  Returns an iterator pointing to into the 
// destination range one past the last element copied.  
//
template<typename OIt>
OIt write_24bit_be(uint32_t val, OIt dest) {
	static_assert(std::is_same<
		std::remove_cvref<decltype(*dest)>::type,unsigned char>::value);
	
	auto be_val = native2be(val);
	return write_bytes(be_val,3,dest);
};


// 
// The max size of a vl field is 4 bytes, and the largest value it may encode is
// 0x0FFFFFFF (BE-encoded as: FF FF FF 7F) => 268,435,455, which fits safely in
// an int32_t:  std::numeric_limits<int32_t>::max() == 2,147,483,647;
//
//
struct midi_vl_field_interpreted {
	int32_t val {0};
	int8_t N {0};
	bool is_valid {false};
};
midi_vl_field_interpreted midi_interpret_vl_field(const unsigned char*);
// Specify the max number of bytes allowed to be read.  If the terminating
// byte is not encountered after reading max_size bytes, returns w/ 
// !is_valid
midi_vl_field_interpreted midi_interpret_vl_field(const unsigned char*, uint32_t);
// Overload for iterators
template<typename InIt>
midi_vl_field_interpreted midi_interpret_vl_field(InIt it) {
	//static_assert(std::is_same<std::remove_reference<decltype(*it)>::type,
	//	const unsigned char>::value);
	midi_vl_field_interpreted result {};
	result.val = 0;
	while (true) {
		result.val += (*it & 0x7F);
		++(result.N);
		if (!(*it & 0x80) || result.N==4) { // the high-bit is not set
			break;
		} else {
			result.val <<= 7;  // result.val << 7;
			++it;
		}
	}
	result.is_valid = !(*it & 0x80);
	return result;
};




//
// Returns an integer with the bit representation of the input encoded as a midi
// vl quantity.  Ex for:
// input 0b1111'1111 == 0xFF == 255 => 0b1000'0001''0111'1111 == 0x817F == 33151
// The largest value a midi vl field can accommodate is 0x0FFFFFFF == 268,435,455
// (note that the MSB of the max value is 0, not 7, corresponding to 4 leading 
// 0's in the MSB of the binary representation).  
//
// The midi-vl representation of a given int is not necessarily unique; above,
// 0xFF == 255 => 0x81'7F == 33,151
//             => 0x80'81'7F == 8,421,759
//             => 0x80'80'81'7F == 2,155,905,407
//
template<typename T>
constexpr uint32_t midi_vl_field_equiv_value(T val) {
	static_assert(CHAR_BIT == 8);
	static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value);
	if (val > 0x0FFFFFFF) {
		return 0;
	}
	uint32_t res {0};

	for (int i=0; val>0; ++i) {
		if (i > 0) {
			res |= (0x80<<(i*8));
		}
		res += (val & 0x7Fu)<<(i*8);
		// The u is needed on the 0x7F; otherwise (val & 0x7F) may be cast to a signed
		// type (ex, if T == uint16_t), for which left-shift is implementation-defined.  
		val>>=7;
	}

	return res;
}
//
// Computes the size (in bytes) of the field required to encode a given 
// number as a vl-quantity.  
// TODO:  Shifting a signed value is UB/ID
//
template<typename T>
constexpr int midi_vl_field_size(T val) {
	static_assert(std::is_integral<T>::value,"MIDI VL fields only encode integral values");
	int n {0};
	do {
		val >>= 7;
		++n;
	} while (val != 0);

	return n;
}
// 
// Encode a value in vl form and write it out into the range [beg,end).  
//
// If end-beg is not large enough to accomodate the number of bytes required
// to encode the value, only end-beg bytes will be written, and the field 
// will be invalid, since the last byte written will not have its MSB==0.  
//
// TODO:  This is overly complex.  I can just make an unsigned char * to vlval
// and write it out...
//
template<typename It, typename T>
It midi_write_vl_field(It beg, It end, T val) {
	static_assert(CHAR_BIT == 8);
	static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value);
	auto vlval = midi_vl_field_equiv_value(val);
	static_assert(std::is_same<decltype(vlval),uint32_t>::value);

	uint8_t bytepos = 3;
	uint32_t mask=0xFF000000;
	while (((vlval&mask)==0) && (mask>0)) {
		mask>>=8;
		--bytepos;
	}

	if (mask==0 && beg!=end) {
		// This means that vlval==0; it is still necessary to write out a
		// 0x00 and increment beg.  
		*beg = 0x00u;
		++beg;
	} else {
		while (mask>0 && beg!=end) {
			*beg = (vlval&mask)>>(8*bytepos);
			mask>>=8;  --bytepos;
			++beg;
		}
	}

	return beg;
}
//
// Overload that can take a std::back_inseter(some_container)
// TODO:  This is overly complex.  I can just make an unsigned char * to vlval
// and write it out...
//
template<typename OutIt, typename T>
OutIt midi_write_vl_field(OutIt it, T val) {
	static_assert(CHAR_BIT == 8);
	static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value);
	auto vlval = midi_vl_field_equiv_value(val);  // requires integral, unsigned
	
	uint8_t bytepos = 3;
	uint32_t mask = 0xFF000000;
	while (((vlval&mask)==0) && (mask>0)) {
		mask>>=8;  --bytepos;
	}

	if (mask==0) {
		// This means that vlval==0; it is still necessary to write out an
		// 0x00u
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
}



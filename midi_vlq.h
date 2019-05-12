#pragma once
#include <array>
#include <limits> // CHAR_BIT
#include <type_traits> // std::enable_if<>, is_integral<>, is_unsigned<>
#include <cstdint>


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



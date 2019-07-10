#pragma once
#include "..\\..\\clamped_value.h"
#include <limits>  // CHAR_BIT
#include <type_traits>  // std::enable_if<>, is_integral<>, is_unsigned<>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <array>
#include <string>

using delta_time_t = clamped_value<uint32_t,0,0x0FFFFFFFu>;


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
		std::remove_cvref<decltype(*beg)>::type,unsigned char>::value);
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
// unsigned integer type, writes the 3 bytes of val _following_ the most
// significant into dest (in be format).  Returns an iterator pointing 
// to into the destination range one past the last element copied.  
//
template<typename OIt>
OIt write_24bit_be(uint32_t val, OIt dest) {
	static_assert(std::is_same<
		std::remove_cvref<decltype(*dest)>::type,unsigned char>::value);
	
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
		std::remove_cvref<decltype(*dest)>::type,unsigned char>::value);
	
	auto be_val = to_be_byte_order(val);
	unsigned char *p = static_cast<unsigned char*>(static_cast<void*>(&be_val));
	for (int i=0; i<3; ++i) {
		*dest++ = *p++;
	}
	return dest;
};
// TODO:  Could use write_bytes() here
template<typename OIt>
OIt write_32bit_be(uint32_t val, OIt dest) {
	static_assert(std::is_same<
		std::remove_cvref<decltype(*dest)>::type,unsigned char>::value);
	
	auto be_val = to_be_byte_order(val);
	unsigned char *p = static_cast<unsigned char*>(static_cast<void*>(&be_val));
	for (int i=0; i<4; ++i) {
		*dest++ = *p++;
	}
	return dest;
};
// 
// The max size of a vl field is 4 bytes, and the largest value it may encode is
// 0x0FFFFFFFu (BE-encoded as: FF FF FF 7F) => 268,435,455, which fits safely in
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
// Advance the iterator to the end of the vlq; do not parse the field
template<typename InIt>
InIt advance_to_vlq_end(InIt it) {
	while((*it++)&0x80u) {
		true;
	}
	return it;
};
// Advance the iterator to the end of the delta-time vlq or by 4 bytes,
// whichever comes first; do not parse the field.  A delta-time vlq may
// occupy a maximum of 4 bytes.  
template<typename InIt>
InIt advance_to_dt_end(InIt it) {
	int n=0;
	while ((n++<4) && ((*it++)&0x80u)) {
		true;
	}
	return it;
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
	static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value,
		"MIDI VL fields only encode unsigned integral values");
	if (val > 0x0FFFFFFF) {
		return 0;
	}
	uint32_t res {0};
	for (int i=0; val>0; ++i) {
		if (i > 0) {
			res |= (0x80u<<(i*CHAR_BIT));
		}
		res += (val & 0x7Fu)<<(i*CHAR_BIT);
		val>>=7;
	}

	return res;
}
//
// Computes the size (in bytes) of the field required to encode a given 
// number as a MIDI vlq-quantity.  
//
template<typename T>
constexpr int midi_vl_field_size(T val) {
	static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value,
		"MIDI VL fields only encode unsigned integral values");
	int n {0};
	do {
		val >>= 7;
		++n;
	} while (val != 0);

	return n;
}
//
// Computes the size (in bytes) of the field required to encode a given 
// value as a delta time.  Functions that write delta time vlq fields 
// clamp the max allowable value to 0x0FFFFFFFu => 4 bytes, hence this never
// returns > 4.  
//
template<typename T>
constexpr int delta_time_field_size(T val) {
	static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value,
		"MIDI VL fields only encode unsigned integral values");
	int n {0};
	do {
		val >>= 7;
		++n;
	} while ((val!=0) && (n<4));

	return n;
}


// 
// Encode a value in vl form and write it out into the range [beg,end).  
//
// If end-beg is not large enough to accomodate the number of bytes required
// to encode the value, only end-beg bytes will be written, and the field 
// will be invalid, since the last byte written will not have its MSB==0.  
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
};

// Clamps the input value to [0,0x0FFFFFFF] == [0,268,435,455]
template<typename T, typename OIt>
OIt write_delta_time(T val, OIt it) {
	static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value,
		"MIDI delta-time fields only encode unsigned integral values");

	val &= 0x0FFFFFFFu;
	return midi_write_vl_field(it,val);
};


//
// For some array (of any type) on [beg,end), writes the underlying byte
// representation as hex ASCII into OIt out.  For each element, the ASCII
// representation of each byte except the last is folllowed by a byte_sep 
// char.  The final byte of each element, except the last element in the 
// array is followed by an elem_sep char.  A seperator w/ the value '\0' 
// is not printed.  
//
// std:: string s {};
// std::array<int16_t,3> arry {0,256,128};
// print_hexascii(arry.begin(),arry.end(),std::back_inserter(s),'\0','|');
// => s == "0000|0010|8000  (note:  on an LE system).  
// print_hexascii(arry.begin(),arry.end(),std::back_inserter(s),'\0',' ');
// => s == "0000 0010 8000  (note:  on an LE system).  
// print_hexascii(arry.begin(),arry.end(),std::back_inserter(s),'\'','|');
// => s == "00'00|00'10|80'00  (note:  on an LE system).  
//
//
template<typename InIt, typename OIt>
OIt print_hexascii(InIt beg, InIt end, OIt out,
			const unsigned char byte_sep = ' ', const unsigned char t_sep = '\0') {
	std::array<char,16> nybble2ascii {'0','1','2','3','4','5',
		'6','7','8','9','A','B','C','D','E','F'};
	
	while (beg!=end) {
		const auto tval = *beg;
		const unsigned char *p = &tval;
		for (int i=0; i<sizeof(tval); ++i) {
			*out++ = nybble2ascii[((*p) & 0xF0u)>>4];
			*out++ = nybble2ascii[((*p) & 0x0Fu)];
			if ((i+1)!=sizeof(tval) && byte_sep!='\0') {
				*out++ = byte_sep;
			}
			++p;
		}
		if (((end-beg)>1) && (t_sep!='\0')) {
			*out++ = t_sep;
		}
		++beg;
	}
	return out;
};


//
// Same deal but w/ more spohisticated prefix, suffix, & seperator options.
// All bytes in the array are prefixed by sep.byte_pfx and suffexed by 
// sep.byte_suffix.  All bytes _except_ the final byte of an element have
// sep.byte_sep added following the sep.byte_suffix.  
// The first byte of each element is prefixed by sep.elem_pfx and suffixed
// by sep.elem_sfx.  The final byte of each element in the array except for
// that of the last element has sep.elem_sep added following the 
// sep.elem_sfx.  
struct sep_t {
	std::string byte_pfx {""};
	std::string byte_sfx {""};
	std::string byte_sep {""};  // not appended to the very last byte
	std::string elem_pfx {""};
	std::string elem_sfx {""};
	std::string elem_sep {""};  // not appended to the very last element
};
template<typename InIt, typename OIt>
OIt print_hexascii(InIt beg, InIt end, OIt out, const sep_t& sep) {
	std::array<char,16> nybble2ascii {'0','1','2','3','4','5',
		'6','7','8','9','A','B','C','D','E','F'};
	
	auto printstdstr = [&out](const std::string& s)->void {
		for (const auto& c : s) {
			*out++ = c;
		}
	};

	while (beg!=end) {
		const auto tval = *beg;
		const unsigned char *p = &tval;

		printstdstr(sep.elem_pfx);
		for (int i=0; i<sizeof(tval); ++i) {
			printstdstr(sep.byte_pfx);
			*out++ = nybble2ascii[((*p) & 0xF0u)>>4];
			*out++ = nybble2ascii[((*p) & 0x0Fu)];
			printstdstr(sep.byte_sfx);
			if ((i+1)!=sizeof(tval)) {  // Not the last byte
				printstdstr(sep.byte_sep);
			}
			++p;
		}
		printstdstr(sep.elem_sfx);
		if ((end-beg)>1) {
			printstdstr(sep.elem_sep);
		}
		++beg;
	}
	return out;
};


/*
//
// A slightly less STL-ish overload...
//
std::string print_hexascii(const unsigned char*, int, const char = ' ');
*/

#pragma once
#include <string>
#include <array>

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

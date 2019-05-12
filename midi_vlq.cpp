#include "midi_vlq.h"
#include <cstdint>


midi_vl_field_interpreted midi_interpret_vl_field(const unsigned char* p) {
	midi_vl_field_interpreted result {};
	result.val = 0;

	while (true) {
		result.val += (*p & 0x7F);
		++(result.N);
		if (!(*p & 0x80) || result.N==4) { // the high-bit is not set
			break;
		} else {
			result.val <<= 7;  // result.val << 7;
			++p;
		}
	}

	result.is_valid = !(*p & 0x80);

	return result;
}


midi_vl_field_interpreted midi_interpret_vl_field(const unsigned char* p, uint32_t max_size) {
	midi_vl_field_interpreted result {};
	result.val = 0;  result.N = 0;

	bool found_field_end = false;
	while (max_size>0 || result.N<4) {
		result.val += (*p & 0x7Fu);
		++(result.N);
		--max_size;
		if (!(*p&0x80u)) {
			found_field_end = true;
			break;
		} else {
			result.val <<= 7;
			++p;
		}
	}
	result.is_valid = found_field_end;
	return result;
}


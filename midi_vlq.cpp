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
};




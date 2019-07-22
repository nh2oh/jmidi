#include "midi_vlq.h"
#include <cstdint>
#include <algorithm>

/*
constexpr int delta_time_field_size(int32_t val) {
	val = std::clamp(val,0,0x0FFFFFFF);
	return delta_time_field_size(static_cast<uint32_t>(val));
}
constexpr int delta_time_field_size(uint32_t val) {
	val = std::clamp(val,0u,0x0FFFFFFFu);
	int n {0};
	do {
		val >>= 7;
		++n;
	} while ((val!=0) && (n<4));

	return n;
}
constexpr int yaaaaaay(uint32_t x) {
	return delta_time_field_size(x);
}

*/


/*
midi_vl_field_interpreted midi_interpret_vl_field(const unsigned char* p) {
	midi_vl_field_interpreted result {};
	result.val = 0;

	while (true) {
		result.val += (*p & 0x7Fu);
		++(result.N);
		if (!(*p & 0x80u) || result.N==4) {  // the high-bit is not set
			break;
		} else {
			result.val <<= 7;  // result.val << 7;
			++p;
		}
	}

	result.is_valid = !(*p & 0x80u);

	return result;
}

midi_vl_field_interpreted midi_interpret_vl_field(const unsigned char* p, int32_t max_size) {
	auto usz = static_cast<uint32_t>(std::clamp(max_size,0,0x0FFFFFFF));
	return midi_interpret_vl_field(p,usz);
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
*/




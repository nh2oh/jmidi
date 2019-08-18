#include "midi_delta_time.h"
#include <cstdint>
#include <algorithm>


bool is_valid_delta_time(int32_t dt) {
	return !((dt>0x0FFFFFFF) || (dt<0));
}
int32_t to_nearest_valid_delta_time(int32_t val) {
	return std::clamp(val,0,0x0FFFFFFF);
}
int32_t delta_time_field_size(int32_t val) {
	val = to_nearest_valid_delta_time(val);
	uint32_t uval = static_cast<uint32_t>(val);
	int32_t n = 0;
	do {
		uval >>= 7;
		++n;
	} while ((uval!=0) && (n<4));
	return n;
}


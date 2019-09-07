#include "midi_delta_time.h"
#include <cstdint>
#include <algorithm>


bool jmid::is_valid_delta_time(std::int32_t dt) {
	return !((dt>0x0FFFFFFF) || (dt<0));
}
std::int32_t jmid::to_nearest_valid_delta_time(std::int32_t val) {
	return std::clamp(val,0,0x0FFFFFFF);
}
std::int32_t jmid::delta_time_field_size(std::int32_t val) {
	val = jmid::to_nearest_valid_delta_time(val);
	std::uint32_t uval = static_cast<std::uint32_t>(val);
	std::int32_t n = 0;
	do {
		uval >>= 7;
		++n;
	} while ((uval!=0) && (n<4));
	return n;
}
std::int32_t jmid::delta_time_field_size_unsafe(std::int32_t val) {
	std::uint32_t uval = static_cast<std::uint32_t>(val);
	if (uval <= 0x7Fu) {
		return 1;
	} else if (uval <= (0x3F'FFu)) {
		return 2;
	} else if (uval <= (0x1F'FF'FFu)) {
		return 3;
	} else {
		return 4;
	}
}




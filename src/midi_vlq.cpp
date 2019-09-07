#include "midi_vlq.h"

bool jmid::is_valid_vlq(std::int32_t val) {
	return ((val >=0) && (val <= 0x0FFFFFFF));
}


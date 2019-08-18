#include "midi_status_byte.h"
#include <string>
#include <cstdint>


std::string print(const status_byte_type& et) {
	if (et == status_byte_type::meta) {
		return "meta";
	} else if (et == status_byte_type::channel) {
		return "channel";
	} else if (et == status_byte_type::sysex_f0) {
		return "sysex_f0";
	} else if (et == status_byte_type::sysex_f7) {
		return "sysex_f7";
	} else if (et == status_byte_type::invalid) {
		return "invalid";
	} else if (et == status_byte_type::unrecognized) {
		return "unrecognized";
	} else {
		return "? status_byte_type";
	}
}
status_byte_type classify_status_byte(unsigned char s) {
	if (is_channel_status_byte(s)) {
		return status_byte_type::channel;
	} else if (is_meta_status_byte(s)) {
		return status_byte_type::meta;
	} else if (is_sysex_status_byte(s)) {
		if (s==0xF0u) {
			return status_byte_type::sysex_f0;
		} else if (s==0xF7u) {
			return status_byte_type::sysex_f7;
		}
	} else if (is_unrecognized_status_byte(s)) {
		return status_byte_type::unrecognized;
	}
	return status_byte_type::invalid;
}
status_byte_type classify_status_byte(unsigned char s, unsigned char rs) {
	s = get_status_byte(s,rs);
	return classify_status_byte(s);
}
bool is_status_byte(const unsigned char s) {
	return (s&0x80u) == 0x80u;
}
bool is_unrecognized_status_byte(const unsigned char s) {
	return (is_status_byte(s) 
		&& !is_channel_status_byte(s)
		&& !is_sysex_or_meta_status_byte(s));
}
bool is_channel_status_byte(const unsigned char s) {
	unsigned char sm = s&0xF0u;
	return ((sm>=0x80u) && (sm!=0xF0u));
}
bool is_sysex_status_byte(const unsigned char s) {
	return ((s==0xF0u) || (s==0xF7u));
}
bool is_meta_status_byte(const unsigned char s) {
	return (s==0xFFu);
}
bool is_sysex_or_meta_status_byte(const unsigned char s) {
	return (is_sysex_status_byte(s) || is_meta_status_byte(s));
}
bool is_data_byte(const unsigned char s) {
	return (s&0x80u)==0x00u;
}
unsigned char get_status_byte(unsigned char s, unsigned char rs) {
	if (is_status_byte(s)) {
		// s could be a valid, but "unrecognized" status byte, ex, 0xF1u.
		// In such a case, the event-local byte wins over the rs,
		// get_running_status_byte(s,rs) will return 0x00u as the rs.  
		return s;
	} else {
		if (is_channel_status_byte(rs)) {
			// Overall, this is equivalent to testing:
			// if (is_channel_status_byte(rs) && is_data_byte(s)); the outter
			// !is_status_byte(s) => is_data_byte(s).  
			return rs;
		}
	}
	return 0x00u;  // An invalid status byte
}
unsigned char get_running_status_byte(unsigned char s, unsigned char rs) {
	if (is_channel_status_byte(s)) {
		return s;  // channel event w/ event-local status byte
	}
	if (is_data_byte(s) && is_channel_status_byte(rs)) {
		return rs;  // channel event in running-status
	}
	return 0x00u;  // An invalid status byte
}
int32_t channel_status_byte_n_data_bytes(unsigned char s) {
	if (is_channel_status_byte(s)) {
		if ((s&0xF0u)==0xC0u || (s&0xF0u)==0xD0u) {
			return 1;
		} else {
			return 2;
		}
	} else {
		return 0;
	}
}



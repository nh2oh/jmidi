#include "midi_status_byte.h"
#include <string>
#include <cstdint>


std::string jmid::print(const jmid::status_byte_type& et) {
	if (et == jmid::status_byte_type::meta) {
		return "meta";
	} else if (et == jmid::status_byte_type::channel) {
		return "channel";
	} else if (et == jmid::status_byte_type::sysex_f0) {
		return "sysex_f0";
	} else if (et == jmid::status_byte_type::sysex_f7) {
		return "sysex_f7";
	} else if (et == jmid::status_byte_type::invalid) {
		return "invalid";
	} else if (et == jmid::status_byte_type::unrecognized) {
		return "unrecognized";
	} else {
		return "? status_byte_type";
	}
}
jmid::status_byte_type jmid::classify_status_byte(unsigned char s) {
	if (is_channel_status_byte(s)) {
		return jmid::status_byte_type::channel;
	} else if (is_meta_status_byte(s)) {
		return jmid::status_byte_type::meta;
	} else if (is_sysex_status_byte(s)) {
		if (s==0xF0u) {
			return jmid::status_byte_type::sysex_f0;
		} else if (s==0xF7u) {
			return jmid::status_byte_type::sysex_f7;
		}
	} else if (is_unrecognized_status_byte(s)) {
		return status_byte_type::unrecognized;
	}
	return jmid::status_byte_type::invalid;
}
jmid::status_byte_type jmid::classify_status_byte(unsigned char s, 
												unsigned char rs) {
	s = jmid::get_status_byte(s,rs);
	return jmid::classify_status_byte(s);
}
bool jmid::is_status_byte(const unsigned char s) {
	return (s&0x80u) == 0x80u;
}
bool jmid::is_unrecognized_status_byte(const unsigned char s) {
	return (jmid::is_status_byte(s) 
		&& !jmid::is_channel_status_byte(s)
		&& !jmid::is_sysex_or_meta_status_byte(s));
}
bool jmid::is_channel_status_byte(const unsigned char s) {
	unsigned char sm = s&0xF0u;
	return ((sm>=0x80u) && (sm!=0xF0u));
}
bool jmid::is_sysex_status_byte(const unsigned char s) {
	return ((s==0xF0u) || (s==0xF7u));
}
bool jmid::is_meta_status_byte(const unsigned char s) {
	return (s==0xFFu);
}
bool jmid::is_sysex_or_meta_status_byte(const unsigned char s) {
	return (jmid::is_sysex_status_byte(s) || jmid::is_meta_status_byte(s));
}
bool jmid::is_data_byte(const unsigned char s) {
	return (s&0x80u)==0x00u;
}
unsigned char jmid::get_status_byte(unsigned char s, unsigned char rs) {
	if (jmid::is_status_byte(s)) {
		// s could be a valid, but "unrecognized" status byte, ex, 0xF1u.
		// In such a case, the event-local byte wins over the rs,
		// get_running_status_byte(s,rs) will return 0x00u as the rs.  
		return s;
	} else {
		if (jmid::is_channel_status_byte(rs)) {
			// Overall, this is equivalent to testing:
			// if (is_channel_status_byte(rs) && is_data_byte(s)); the outter
			// !is_status_byte(s) => is_data_byte(s).  
			return rs;
		}
	}
	return 0x00u;  // An invalid status byte
}
unsigned char jmid::get_running_status_byte(unsigned char s, 
											unsigned char rs) {
	if (jmid::is_channel_status_byte(s)) {
		return s;  // channel event w/ event-local status byte
	}
	if (jmid::is_data_byte(s) && jmid::is_channel_status_byte(rs)) {
		return rs;  // channel event in running-status
	}
	return 0x00u;  // An invalid status byte
}
std::int32_t jmid::channel_status_byte_n_data_bytes(unsigned char s) {
	if (jmid::is_channel_status_byte(s)) {
		if ((s&0xF0u)==0xC0u || (s&0xF0u)==0xD0u) {
			return 1;
		} else {
			return 2;
		}
	} else {
		return 0;
	}
}



#include "generic_chunk_low_level.h"
#include "midi_vlq.h"
#include <string>
#include <cstdint>



bool jmid::is_mthd_header_id(const unsigned char *beg, const unsigned char *end) {
	return jmid::read_be<std::uint32_t>(beg,end) == 0x4D546864u;
}
bool jmid::is_mtrk_header_id(const unsigned char *beg, const unsigned char *end) {
	return jmid::read_be<std::uint32_t>(beg,end) == 0x4D54726Bu;
}
bool jmid::is_valid_header_id(const unsigned char *beg, const unsigned char *end) {
	if ((end-beg)<4) {
		return true;
	}
	for (auto it=beg; it<(beg+4); ++it) {  // Verify the first 4 bytes are valid ASCII
		if (*it>=127 || *it<32) {
			return false;
		}
	}
	return true;
}
bool jmid::is_valid_chunk_length(std::uint32_t len) {
	return len<=jmid::chunk_view_t::length_max;
}
jmid::maybe_header_t::operator bool() const {
	return this->is_valid;
}
jmid::maybe_header_t jmid::read_chunk_header(const unsigned char *beg, 
					const unsigned char *end) {
	return jmid::read_chunk_header(beg,end,nullptr);
}
jmid::maybe_header_t jmid::read_chunk_header(const unsigned char *beg, 
					const unsigned char *end, jmid::chunk_header_error_t *err) {
	jmid::maybe_header_t result;
	if ((end-beg)<8) {
		if (err) {
			err->code = jmid::chunk_header_error_t::errc::overflow;
		}
		return result;
	}
	
	auto id = jmid::read_be<std::uint32_t>(beg,end);
	if (id == 0x4D546864u) {  // Mthd
		result.id = jmid::chunk_id::mthd;
	} else if (id == 0x4D54726Bu) {  // MTrk
		result.id = jmid::chunk_id::mtrk;
	} else {
		result.id = jmid::chunk_id::unknown;
		if (!is_valid_header_id(beg,end)) {
			if (err) {
				err->code = jmid::chunk_header_error_t::errc::invalid_id;
				err->id = id;
			}
			return result;
		}
	}

	auto len = jmid::read_be<std::uint32_t>(beg+4,end);
	if (!jmid::is_valid_chunk_length(len)) {
		if (err) {
			err->code = jmid::chunk_header_error_t::errc::length_exceeds_max;
			err->id = id;
			err->length = len;
		}
		return result;
	}
	result.length = static_cast<int32_t>(len);

	result.is_valid = true;
	return result;
}

std::string jmid::explain(const jmid::chunk_header_error_t& err) {
	std::string s {};
	if (err.code==jmid::chunk_header_error_t::errc::no_error) {
		return s;
	}

	s += "Invalid chunk header:  ";
	if (err.code==jmid::chunk_header_error_t::errc::overflow) {
		s += "The input range is smaller than 8 bytes.  ";
	} else if (err.code==jmid::chunk_header_error_t::errc::invalid_id) {
		s += "ID field contains non-ASCII characters.  ";
	} else if (err.code==jmid::chunk_header_error_t::errc::length_exceeds_max) {
		s += "The length field encodes a value that is too large.  "
			"This library enforces a maximum chunk length of ";
		s += std::to_string(jmid::chunk_view_t::length_max);
		s += " bytes, the largest value representable in a signed 32-bit "
			"int.  length == ";
		s += std::to_string(err.length);
		s += ".  ";
	} else if (err.code==jmid::chunk_header_error_t::errc::other) {
		s += "Error code chunk_header_error_t::errc::other.  ";
	} else {
		s += "Unknown error.  ";
	}
	return s;
}

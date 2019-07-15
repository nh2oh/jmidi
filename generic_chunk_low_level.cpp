#include "generic_chunk_low_level.h"
#include "midi_vlq.h"
#include <string>
#include <cstdint>



bool is_mthd_header_id(const unsigned char *beg, const unsigned char *end) {
	return read_be<uint32_t>(beg,end) == 0x4D546864u;
}
bool is_mtrk_header_id(const unsigned char *beg, const unsigned char *end) {
	return read_be<uint32_t>(beg,end) == 0x4D54726Bu;
}
bool is_valid_header_id(const unsigned char *beg, const unsigned char *end) {
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
bool is_valid_header_length(uint32_t len) {
	return len<=chunk_view_t::length_max;
}
maybe_header_t::operator bool() const {
	return this->is_valid;
}
maybe_header_t read_chunk_header(const unsigned char *beg, 
					const unsigned char *end) {
	return read_chunk_header(beg,end,nullptr);
}
maybe_header_t read_chunk_header(const unsigned char *beg, 
					const unsigned char *end, chunk_header_error_t *err) {
	maybe_header_t result;
	if ((end-beg)<8) {
		if (err) {
			err->code = chunk_header_error_t::errc::overflow;
		}
		return result;
	}
	
	auto id = read_be<uint32_t>(beg,end);
	if (id == 0x4D546864u) {  // Mthd
		result.id = chunk_id::mthd;
	} else if (id == 0x4D54726Bu) {  // MTrk
		result.id = chunk_id::mtrk;
	} else {
		result.id = chunk_id::unknown;
		if (!is_valid_header_id(beg,end)) {
			if (err) {
				err->code = chunk_header_error_t::errc::invalid_id;
				err->id = id;
			}
			return result;
		}
	}

	auto len = read_be<uint32_t>(beg+4,end);
	if (!is_valid_header_length(len)) {
		if (err) {
			err->code = chunk_header_error_t::errc::length_exceeds_max;
			err->id = id;
			err->length = len;
		}
		return result;
	}
	result.length = static_cast<int32_t>(len);

	result.is_valid = true;
	return result;
}

std::string explain(const chunk_header_error_t& err) {
	std::string s {};
	if (err.code==chunk_header_error_t::errc::no_error) {
		return s;
	}

	s += "Invalid chunk header:  ";
	if (err.code==chunk_header_error_t::errc::overflow) {
		s += "The input range is smaller than 8 bytes.  ";
	} else if (err.code==chunk_header_error_t::errc::invalid_id) {
		s += "Invalid chunk ID.  ";
	} else if (err.code==chunk_header_error_t::errc::length_exceeds_max) {
		s += "The length field encodes a value that is too large.  "
			"This library enforces a maximum chunk length of ";
		s += std::to_string(chunk_view_t::length_max);
			" bytes, the largest value representable in a signed 32-bit "
			"int.  length == ";
		s += std::to_string(err.length);
		s += ".  ";
	} else if (err.code==chunk_header_error_t::errc::other) {
		s += "Error code chunk_header_error_t::errc::other.  ";
	} else {
		s += "Unknown error.  ";
	}
	return s;
}

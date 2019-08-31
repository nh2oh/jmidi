#pragma once
#include "midi_vlq.h"
#include <string>
#include <cstdint>
#include <limits>
#include <array>


namespace jmid {

//
// Low-level validation & processing of chunk headers
//
// These functions parse, classify, and validate chunk _headers_
//
// maybe_header_t read_chunk_header(const unsigned char *beg,
//									const unsigned char *end);
//

// TODO:  This is unused except for the static member
class chunk_view_t {
public:
	static constexpr std::int32_t length_max = std::numeric_limits<std::int32_t>::max()-8;
private:
	//const unsigned char *p_;
};

enum class chunk_id : std::uint8_t {
	mthd,  // MThd
	mtrk,  // MTrk
	unknown  // The std requires that unrecognized chunk types be permitted
};
bool is_mthd_header_id(const unsigned char*, const unsigned char*);
bool is_mtrk_header_id(const unsigned char*, const unsigned char*);
bool is_valid_header_id(const unsigned char*, const unsigned char*);
bool is_valid_chunk_length(std::uint32_t);
struct chunk_header_error_t {
	enum errc : std::uint8_t {
		overflow,
		invalid_id,
		length_exceeds_max,
		no_error,
		other
	};
	std::uint32_t length {0u};
	std::uint32_t id {0u};
	errc code {no_error};
};
struct maybe_header_t {
	std::int32_t length {0};
	chunk_id id {chunk_id::unknown};
	bool is_valid {false};
	operator bool() const;
};
maybe_header_t read_chunk_header(const unsigned char*, const unsigned char*);
maybe_header_t read_chunk_header(const unsigned char*, const unsigned char*,
							chunk_header_error_t*);
std::string explain(const chunk_header_error_t&);


struct chunk_header_t {
	std::uint32_t length {0};
	std::array<unsigned char,4> id {0x00u,0x00u,0x00u,0x00u};
};
bool has_mthd_id(const chunk_header_t&);
bool has_mtrk_id(const chunk_header_t&);
bool has_uchk_id(const chunk_header_t&);
bool has_valid_length(const chunk_header_t&);
bool is_valid(const chunk_header_t&);
std::int32_t to_nearest_valid_chunk_length(std::uint32_t);

template <typename InIt>
InIt read_chunk_header(InIt it, InIt end, chunk_header_t *result, 
						chunk_header_error_t *err) {
	auto dest = result->id.data();
	for (int i=0; i<4; ++i) {
		if (it == end) {
			if (err != nullptr) {
				err->code = chunk_header_error_t::errc::overflow;
			}
			return it;
		}
		*dest++ = *it++;
	}

	for (int i=0; i<4; ++i) {
		if (it == end) {
			if (err != nullptr) {
				err->code = chunk_header_error_t::errc::overflow;
			}
			return it;
		}
		result->length = jmid::read_be<std::uint32_t>(it,end,&(result->length));
	}

	return it;
};

}  // namespace jmid

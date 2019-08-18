#pragma once
#include <string>
#include <cstdint>
#include <limits>


//
// Low-level validation & processing of chunk headers
//
// These functions parse, classify, and validate chunk _headers_
//
// maybe_header_t read_chunk_header(const unsigned char *beg,
//									const unsigned char *end);
//
class chunk_view_t {
public:
	static constexpr int32_t length_max = std::numeric_limits<int32_t>::max()-8;
private:
	const unsigned char *p_;
};

enum class chunk_id : uint8_t {
	mthd,  // MThd
	mtrk,  // MTrk
	unknown  // The std requires that unrecognized chunk types be permitted
};
bool is_mthd_header_id(const unsigned char*, const unsigned char*);
bool is_mtrk_header_id(const unsigned char*, const unsigned char*);
bool is_valid_header_id(const unsigned char*, const unsigned char*);
bool is_valid_chunk_length(uint32_t);
struct chunk_header_error_t {
	enum errc : uint8_t {
		overflow,
		invalid_id,
		length_exceeds_max,
		no_error,
		other
	};
	uint32_t length {0u};
	uint32_t id {0u};
	errc code {no_error};
};
struct maybe_header_t {
	int32_t length {0};
	chunk_id id {chunk_id::unknown};
	bool is_valid {false};
	operator bool() const;
};
maybe_header_t read_chunk_header(const unsigned char*, const unsigned char*);
maybe_header_t read_chunk_header(const unsigned char*, const unsigned char*,
							chunk_header_error_t*);
std::string explain(const chunk_header_error_t&);



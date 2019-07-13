#include "midi_raw.h"
#include "midi_vlq.h"
#include "mthd_t.h"  // is_valid_time_division_raw_value()
#include <string>
#include <cstdint>
#include <cmath>  // std::round()
#include <algorithm>  // std::max()


double ticks2sec(const uint32_t& tks, const midi_time_t& t) {
	return tks*(1.0/t.tpq)*(t.uspq)*(1.0/1000000.0);
}
uint32_t sec2ticks(const double& sec, const midi_time_t& t) {
	// s = tks*(1.0/t.tpq)*(t.uspq)*(1.0/1000000.0);
	return static_cast<uint32_t>(std::round(sec*(t.tpq)*(1.0/t.uspq)*1000000.0));
}

bool operator==(const midi_timesig_t& rhs, const midi_timesig_t& lhs) {
	return((rhs.clckspclk == lhs.clckspclk)
		&& (rhs.log2denom == lhs.log2denom)
		&& (rhs.ntd32pq == lhs.ntd32pq)
		&& (rhs.num == lhs.num));
}

uint8_t nsharps(const midi_keysig_t& ks) {
	return (ks.sf >= 0) ? ks.sf : 0;
}
uint8_t nflats(const midi_keysig_t& ks) {
	return (ks.sf <= 0) ? (-1)*ks.sf : 0;
}
bool is_major(const midi_keysig_t& ks) {
	return ks.mi==0;
}
bool is_minor(const midi_keysig_t& ks) {
	return ks.mi==1;
}
bool verify(const midi_ch_event_t& md) {
	auto s = ((md.status_nybble & 0xF0u) + (md.ch & 0x0Fu));
	if (!is_channel_status_byte(s)) {
		return false;
	}
	if (md.ch > 0x0Fu) {
		return false;
	}
	if (!is_data_byte(md.p1)) {
		return false;
	}
	if ((channel_status_byte_n_data_bytes(s)==2) && !is_data_byte(md.p2)) {
		return false;
	}
	return true;
}

midi_ch_event_t normalize(midi_ch_event_t md) {
	md.status_nybble = 0xF0u&(0x80u|(md.status_nybble));
	md.ch &= 0x0Fu; // std::max(md.ch,15);
	md.p1 &= 0x7Fu; 
	md.p2 &= 0x7Fu; 
	return md;
}
bool is_note_on(const midi_ch_event_t& md) {
	return (verify(md)
		&& md.status_nybble==0x90u
		&& (md.p2 > 0));
}
bool is_note_off(const midi_ch_event_t& md) {
	return (verify(md)
		&& ((md.status_nybble==0x80u)
			|| (md.status_nybble==0x90u && (md.p2==0))));
}

bool is_key_pressure(const midi_ch_event_t& md) {  // 0xAnu
	return (verify(md)
		&& (md.status_nybble==0xA0u));
}
bool is_control_change(const midi_ch_event_t& md) {
	return (verify(md)
		&& (md.status_nybble==0xB0u)
		&& (md.p1 < 120));
	// The final condition verifies md is a channel_voice (not channel_mode)
	// event.  120  == 0b01111000u
}
bool is_program_change(const midi_ch_event_t& md) {
	return (verify(md)
		&& (md.status_nybble==0xC0u));
}
bool is_channel_pressure(const midi_ch_event_t& md) {  // 0xDnu
	return (verify(md)
		&& (md.status_nybble==0xD0u));
}
bool is_pitch_bend(const midi_ch_event_t& md) {
	return (verify(md)
		&& (md.status_nybble==0xE0u));
}
bool is_channel_mode(const midi_ch_event_t& md) {
	return (verify(md)
		&& (md.status_nybble==0xB0u)
		&& (md.p1 >= 120));
	// The final condition verifies md is not a control_change event.  
	// 120  == 0b01111000u
}





chunk_type chunk_type_from_id(const unsigned char *beg, const unsigned char *end) {
	if ((end-beg)<4) {
		return chunk_type::invalid;
	}
	uint32_t id = read_be<uint32_t>(beg,end);
	if (id == 0x4D546864u) {  // Mthd
		return chunk_type::header;
	} else if (id == 0x4D54726Bu) {  // MTrk
		return chunk_type::track;
	} else {  // Verify all 4 bytes are valid ASCII
		for (int n=0; n<4; ++n) {
			if (*beg>=127 || *beg<32) {
				return chunk_type::invalid;
			}
			++beg;
		}
	}
	return chunk_type::unknown;
}
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
	for (int n=0; n<4; ++n) {  // Verify the first 4 bytes are valid ASCII
		if (*beg>=127 || *beg<32) {
			return false;
		}
		++beg;
	}
	return true;
}
int32_t get_chunk_length(const unsigned char *beg, const unsigned char *end,
						int32_t def) {
	if ((end-beg)<8) {
		return def;
	}
	uint32_t len = read_be<uint32_t>(beg,end);
	uint32_t max = static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
	if (len>max) {
		return def;
	}
	return len;
}
maybe_header_t read_chunk_header(const unsigned char *beg, const unsigned char *end,
					chunk_header_error_t *err) {
	maybe_header_t result;
	if ((end-beg)<8) {
		if (err) {
			err->code = chunk_header_error_t::errc::overflow;
			err->offset = 0;
		}
		return result;
	}
	
	result.id = chunk_type_from_id(beg,end);
	if ((result.id==chunk_id::unknown) && !is_valid_header_id(beg,end)) {
		if (err) {
			err->code = chunk_header_error_t::errc::invalid_id;
			std::copy(beg,beg+4,err->id[0]);
			err->offset = 0;
		}
		return result;
	}

	result.length = get_chunk_length(beg,end,-1);
	if (result.length == -1) {
		if (err) {
			err->code = chunk_header_error_t::errc::length_exceeds_max;
			std::copy(beg,beg+4,err->id[0]);
			err->offset = 4;
			err->len = read_be<uint32_t>(beg+4,end);
		}
		return result;
	}

	result.is_valid = true;
	return result;
}

validate_chunk_header_result_t validate_chunk_header(const unsigned char *p, uint32_t max_size) {
	validate_chunk_header_result_t result {};
	result.p = p;
	if (max_size < 8) {
		result.type = chunk_type::invalid;
		result.error = chunk_validation_error::chunk_header_size_exceeds_underlying;
		return result;
	}

	result.type = chunk_type_from_id(p,p+max_size);
	if (result.type == chunk_type::invalid) {
		result.error = chunk_validation_error::invalid_type_field;
		return result;
	}

	p += 4;  // Skip past 'MThd'
	result.data_size = read_be<uint32_t>(p,p+max_size);
	result.size = 8 + result.data_size;

	if (result.size>max_size) {
		result.type = chunk_type::invalid;
		result.error = chunk_validation_error::chunk_data_size_exceeds_underlying;
		return result;
	}

	result.error = chunk_validation_error::no_error;
	return result;
}
std::string print_error(const validate_chunk_header_result_t& chunk) {
	std::string result ="Invalid chunk header:  \n";
	if (chunk.error != chunk_validation_error::chunk_header_size_exceeds_underlying) {
		print_hexascii(chunk.p, chunk.p+8, std::back_inserter(result), '\0',' ');
	}
	result += "\n";
	switch (chunk.error) {
		case chunk_validation_error::chunk_header_size_exceeds_underlying:
			result += "The underlying array is not large enough to accommodate "
				"an 8-byte chunk header.";
			break;
		case chunk_validation_error::chunk_data_size_exceeds_underlying:
			result += "The length field in the chunk header implies that the " 
				"data section would exceed the underlying array.";
			break;
		case chunk_validation_error::invalid_type_field:
			result += "The first 4 bytes of an SMF chunk must be valid ASCII "
				"(>=32 && <= 127).  ";
			break;
		case chunk_validation_error::unknown_error:
			result += "Unknown error.";
			break;
	}
	return result;
}
int32_t mthd_get_ntrks(const unsigned char *p, uint32_t max_size, int32_t def) {
	if ((max_size < 12) || (chunk_type_from_id(p,p+max_size) != chunk_type::header)) {
		return def;
	}
	return read_be<uint16_t>(p+10,p+12);
}

validate_mthd_chunk_result_t validate_mthd_chunk(const unsigned char *p, uint32_t max_size) {
	validate_mthd_chunk_result_t result {};

	auto chunk_detect = validate_chunk_header(p,max_size);
	if (chunk_detect.type != chunk_type::header) {
		if (chunk_detect.type==chunk_type::invalid) {
			result.error = mthd_validation_error::invalid_chunk;
		} else {
			result.error = mthd_validation_error::non_header_chunk;
		}
		return result;
	}
	if (chunk_detect.data_size < 6) {
		result.error = mthd_validation_error::data_length_invalid;
		return result;
	}
	// Ensuring chunk_detect.data_size >= 6 means overall size is @ least
	// 14 bytes (and that max_size >= 14)

	// <Header Chunk> = <chunk type> <length> <format> <ntrks> <division> 
	//                   MThd uint32_t uint16_t uint16_t uint16_t
	uint16_t format = read_be<uint16_t>(p+8,p+max_size);
	uint16_t ntrks = read_be<uint16_t>(p+10,p+max_size);
	uint16_t division = read_be<uint16_t>(p+12,p+max_size);
	if (ntrks==0) {
		result.error = mthd_validation_error::zero_tracks;
		return result;
	}
	if (format==0 && ntrks!=1) {
		result.error = mthd_validation_error::inconsistent_ntrks_format_zero;
		return result;
	}
	if (!is_valid_time_division_raw_value(division)) {
		result.error = mthd_validation_error::invalid_time_division_field;
		return result;
	}
	result.error = mthd_validation_error::no_error;
	result.size = chunk_detect.size;
	result.p = p;  // pointer to the 'M' of "MThd"..., ie the _start_ of the mthd chunk
	return result;
}

std::string print_error(const validate_mthd_chunk_result_t& mthd) {
	std::string result ="Invalid MThd chunk:  ";

	switch (mthd.error) {
		case mthd_validation_error::invalid_chunk:
			result += "Not a valid SMF chunk.  The 4-char ASCII 'type' field "
				"may be missing, or the calculated chunk size may exceed the "
				" size of the underlying array.";
			break;
		case mthd_validation_error::non_header_chunk:
			result += "chunk_detect.type != chunk_type::header.";
			break;
		case mthd_validation_error::data_length_invalid:
			result += "MThd chunks must have a data length >= 6.";
			break;
		case mthd_validation_error::zero_tracks:
			result += "MThd chunks must report at least 1 track.";
			break;
		case mthd_validation_error::inconsistent_ntrks_format_zero:
			result += "The value of field 'format' == 0, but ntrks > 1; "
				"format 0 SMF's must have exactly 1 track.";
			break;
		case mthd_validation_error::invalid_time_division_field:
			result += "The value of field 'division' is invalid.  It is probably "
				"a SMPTE-type field trying to encode a time-code of something other "
				"than -24, -25, -29, or -30.  ";
			break;
		case mthd_validation_error::unknown_error:
			result += "Unknown error.";
			break;
	}
	return result;
}

validate_mtrk_event_result_t validate_mtrk_event_dtstart(
			const unsigned char *p, unsigned char rs, uint32_t max_size) {
	validate_mtrk_event_result_t result {};
	// All mtrk events begin with a delta-time occupying a maximum of 4 bytes
	// and a minimum of 1 byte.  Note that even a delta-time of 0 occupies 1
	// byte.  
	auto dt = midi_interpret_vl_field(p,max_size);
	if (!dt.is_valid) {
		result.type = smf_event_type::invalid;
		result.error = mtrk_event_validation_error::invalid_dt_field;
		return result;
	}
	if (dt.N >= max_size) {
		result.type = smf_event_type::invalid;
		result.error = mtrk_event_validation_error::event_size_exceeds_max;
		return result;
	}
	p+=dt.N;  max_size-=dt.N;
	result.delta_t = dt.val;
	result.size += dt.N;

	auto ev_type = classify_status_byte(*p,rs);
	if (ev_type == smf_event_type::unrecognized
			|| ev_type == smf_event_type::invalid) {
		result.type = smf_event_type::invalid;
		result.error = mtrk_event_validation_error::invalid_or_unrecognized_status_byte;
		return result;
	}

	// TODO:  mtrk_event_get_data_size() classifies the status byte, which
	// i have already done.  Should switch on ev_type and call the appropriate
	// size calc func.  
	auto sz = mtrk_event_get_data_size(p,rs,max_size);
	if ((sz==0) || (sz>max_size)) {  // Error condition
		// There is no such thing as a 0-sized event.  
		// mtrk_event_get_data_size() returns 0 in the event of an error 
		// condition, ex, overflow of max_size.  
		result.type = smf_event_type::invalid;
		result.error = mtrk_event_validation_error::unknown_error;
		
		// Attempt to troubleshoot the error and set result.error to 
		// something informative.  
		if (ev_type == smf_event_type::channel) {
			result.error = mtrk_event_validation_error::event_size_exceeds_max;
		} else if (ev_type==smf_event_type::meta) {
			if (max_size<2) {  // overflow in header
				result.error = mtrk_event_validation_error::sysex_or_meta_overflow_in_header;
			} else {  // no overflow in header
				p+=2;  max_size-=2;  // 0xFF,type
				auto len = midi_interpret_vl_field(p,max_size);
				if (!len.is_valid) {
					result.error = mtrk_event_validation_error::sysex_or_meta_invalid_length_field;
				} else {
					if ((len.N+len.val)>max_size) {
						result.error = mtrk_event_validation_error::sysex_or_meta_length_implies_overflow;
					}
				}
			}
		} else if (ev_type==smf_event_type::sysex_f0 
						|| ev_type==smf_event_type::sysex_f7) {
			if (max_size < 1) {  // overflow in header
				result.error = mtrk_event_validation_error::sysex_or_meta_overflow_in_header;
			} else {  // no overflow in header
				++p;  max_size-=1;  // 0xF0 || 0xF7
				auto len = midi_interpret_vl_field(p,max_size);
				if (!len.is_valid) {
					result.error = mtrk_event_validation_error::sysex_or_meta_invalid_length_field;
				} else {  // len field is valid
					if ((len.N+len.val)>max_size) {
						result.error = mtrk_event_validation_error::sysex_or_meta_length_implies_overflow;
					}
				}
			}  // if (max_size < 1) 
		}  // ev_type == sysex of some sort
		return result;
	}  // if ((sz==0) || (sz>max_size)) {  // Error condition

	// No errors
	result.type = ev_type;
	result.running_status = get_running_status_byte(*p,rs);
	result.size += sz;
	result.error = mtrk_event_validation_error::no_error;
	return result;
}

std::string print(const mtrk_event_validation_error& err) {
	std::string result = "No error";
	switch (err) {
		case mtrk_event_validation_error::invalid_dt_field:
			result = "Invalid delta-time field.  ";
			break;
		case mtrk_event_validation_error::invalid_or_unrecognized_status_byte:
			result = "Invalid or unrecognized status_byte.  ";
			break;
		case mtrk_event_validation_error::unable_to_determine_size:
			result = "Unable to determine event size.  ";
			break;
		case mtrk_event_validation_error::channel_event_missing_data_byte:
			result = "Channel event has too few data bytes.  ";
			break;
		case mtrk_event_validation_error::sysex_or_meta_overflow_in_header:
			result = "Overflow.  ";
			break;
		case mtrk_event_validation_error::sysex_or_meta_invalid_length_field:
			result = "Invalid length field for a sysex or meta event.  ";
			break;
		case mtrk_event_validation_error::sysex_or_meta_length_implies_overflow:
			result = "The length field for the sysex or meta event "
				"imples overflow.  ";
			break;
		case mtrk_event_validation_error::event_size_exceeds_max:
			result = "Overflow.  ";
			break;
		case mtrk_event_validation_error::unknown_error:
			result = "Unknown_error :(";
			break;
		case mtrk_event_validation_error::no_error:
			result = "No error";
			break;
	}
	return result;
}


uint8_t channel_status_byte_n_data_bytes(unsigned char s) {
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

std::string print(const smf_event_type& et) {
	if (et == smf_event_type::meta) {
		return "meta";
	} else if (et == smf_event_type::channel) {
		return "channel";
	} else if (et == smf_event_type::sysex_f0) {
		return "sysex_f0";
	} else if (et == smf_event_type::sysex_f7) {
		return "sysex_f7";
	} else if (et == smf_event_type::invalid) {
		return "invalid";
	} else if (et == smf_event_type::unrecognized) {
		return "unrecognized";
	} else {
		return "? smf_event_type";
	}
}


smf_event_type classify_status_byte(unsigned char s) {
	if (is_channel_status_byte(s)) {
		return smf_event_type::channel;
	} else if (is_meta_status_byte(s)) {
		return smf_event_type::meta;
	} else if (is_sysex_status_byte(s)) {
		if (s==0xF0u) {
			return smf_event_type::sysex_f0;
		} else if (s==0xF7u) {
			return smf_event_type::sysex_f7;
		}
	} else if (is_unrecognized_status_byte(s)) {
		return smf_event_type::unrecognized;
	} else {
		return smf_event_type::invalid;
	}
}
smf_event_type classify_status_byte(unsigned char s, unsigned char rs) {
	s = get_status_byte(s,rs);
	return classify_status_byte(s);
}

bool is_status_byte(const unsigned char s) {
	return (s&0x80u) == 0x80u;
}
bool is_unrecognized_status_byte(const unsigned char s) {
	// Returns true for things like 0xF1u that are "status bytes" but
	// that are invalid in an smf.  
	return is_status_byte(s) 
		&& !is_channel_status_byte(s)
		&& !is_sysex_or_meta_status_byte(s);
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
	return ((s&0x80u)!=0x80u);
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
smf_event_type classify_mtrk_event_dtstart(const unsigned char *p,
									unsigned char rs, uint32_t max_sz) {
	auto dt = midi_interpret_vl_field(p,max_sz);
	if (dt.N >= max_sz || !dt.is_valid) {
		// dt.N==max_sz => 0 bytes following the delta-time field, but the 
		// smallest running-status channel_voice events have at least 1 data 
		// byte.  The smallest meta, sysex events are 3 and 2 bytes 
		// respectively.  
		return smf_event_type::invalid;
	}
	p += dt.N;
	max_sz -= dt.N;
	return classify_status_byte(*p,rs);
}
smf_event_type classify_mtrk_event_dtstart_unsafe(const unsigned char *p,
									unsigned char rs) {
	p = advance_to_vlq_end(p);
	return classify_status_byte(*p,rs);
}

uint32_t channel_event_get_data_size(const unsigned char *p, unsigned char rs) {
	auto s = get_status_byte(*p,rs);
	auto n = channel_status_byte_n_data_bytes(s);
	if (*p==s) {
		++n;  // Event-local status byte (not in running-status)
	}
	return n;
}
uint32_t meta_event_get_data_size(const unsigned char *p, uint32_t max_size) {
	uint32_t n = 0;
	if (max_size <= 2) {
		return 0;
	}
	p += 2;  n += 2;  max_size -= 2;
	auto len = midi_interpret_vl_field(p,max_size);
	n += (len.N + len.val);
	return n;
}
uint32_t sysex_event_get_data_size(const unsigned char *p, uint32_t max_size) {
	uint32_t n = 0;
	if (max_size <= 1) {
		return 0;
	}
	p += 1;  n += 1;  max_size -= 1;
	auto len = midi_interpret_vl_field(p,max_size);
	n += (len.N + len.val);
	return n;
}

uint32_t mtrk_event_get_data_size(const unsigned char *p, unsigned char rs,
								uint32_t max_size) {
	// TODO:  Need to check max_size > 0
	uint32_t result = 0;
	auto ev_type = classify_status_byte(*p,rs);  
	if (ev_type==smf_event_type::channel) {
		// TODO:  Ned to check max_size > 2 (?)
		result += channel_event_get_data_size(p,rs);
	} else if (ev_type==smf_event_type::meta) {
		result += meta_event_get_data_size(p,max_size);
	} else if (ev_type==smf_event_type::sysex_f0
						|| ev_type==smf_event_type::sysex_f7) {
		result += sysex_event_get_data_size(p,max_size);
	} else {  // smf_event_type::invalid || ::unrecognized
		return 0;
	}
	return result;
}
uint32_t mtrk_event_get_size_dtstart(const unsigned char *p, unsigned char rs,
								uint32_t max_size) {
	auto dt = midi_interpret_vl_field(p,max_size);
	if ((!dt.is_valid) || (dt.N >= max_size)) {
		return 0;
	}
	max_size-=dt.N;  p+=dt.N;
	return (dt.N + mtrk_event_get_data_size(p,rs,max_size));
}
uint32_t mtrk_event_get_data_size_unsafe(const unsigned char *p,
										unsigned char rs) {
	// TODO:  This duplicates functionality in the 
	// {meta,channel,...}_event_get_data_size(p,max_size) family of funcs, 
	// because this family has no _unsafe_ variant.  
	uint32_t result = 0;
	auto ev_type = classify_status_byte(*p,rs);  
	if (ev_type==smf_event_type::channel) {
		auto s = get_status_byte(*p,rs);
		result += channel_status_byte_n_data_bytes(s);
		if (*p==s) {  // event-local status byte (not in running-status)
			result += 1;
		}
	} else if (ev_type==smf_event_type::meta) {
		p+=2;  result+=2;  // 0xFFu,type-byte
		auto len = midi_interpret_vl_field(p);
		result += (len.N + len.val);
	} else if (ev_type==smf_event_type::sysex_f0
						|| ev_type==smf_event_type::sysex_f7) {
		p+=1;  result+=1;  // 0xF0u||0xF7u
		auto len = midi_interpret_vl_field(p);
		result += (len.N + len.val);
	} else {
		// smf_event_type::invalid || ::unrecognized
		return 0;
	}
	return result;
}
uint32_t mtrk_event_get_size_dtstart_unsafe(const unsigned char *p,
											unsigned char rs) {
	auto dt = midi_interpret_vl_field(p);
	p+=dt.N;
	return (dt.N + mtrk_event_get_data_size_unsafe(p,rs));
}
uint32_t mtrk_event_get_data_size_dtstart_unsafe(const unsigned char *p,
												unsigned char rs) {
	auto dt = midi_interpret_vl_field(p);
	p+=dt.N;
	return mtrk_event_get_data_size_unsafe(p,rs);
}




unsigned char mtrk_event_get_midi_p1_dtstart_unsafe(const unsigned char *p,
													unsigned char s) {
	p += midi_interpret_vl_field(p).N;
	s = get_status_byte(*p,s);
	if (!is_channel_status_byte(s)) { // p does not indicate a midi event
		return 0x80u;  // Invalid data byte
	}
	
	// p _does_ indicate a valid midi event; running-status may (=>*p is not
	// a status byte) or may not (=> *p is a status byte) be in effect
	if (is_channel_status_byte(*p)) {
		return *++p;  // Not in running status
	} else {
		return *p;  // In running status; p is the first data byte
	}
}
unsigned char mtrk_event_get_midi_p2_dtstart_unsafe(const unsigned char *p, unsigned char s) {
	p += midi_interpret_vl_field(p).N;
	s = get_status_byte(*p,s);
	if (!is_channel_status_byte(s)) { // p does not indicate a midi event
		return 0x80u;  // Invalid data byte
	}
	
	// p _does_ indicate a valid midi event; running-status may (=>*p is not
	// a status byte) or may not (=> *p is a status byte) be in effect.  
	if (((s&0xF0u)==0xC0u) || ((s&0xF0u)==0xD0u)) { 
		// This type of midi event has no p2
		return 0x80u;  // Invalid data byte
	}
	if (is_channel_status_byte(*p)) {
		return *(p+=2);  // Not in running status
	} else {
		return *++p;  // In running status; p is the first data byte
	}
}






unsigned char mtrk_event_get_meta_type_byte_dtstart_unsafe(const unsigned char *p) {
	p += midi_interpret_vl_field(p).N;
	p += 1;  // Skip the 0xFF;
	return *p;
}
midi_vl_field_interpreted mtrk_event_get_meta_length_field_dtstart_unsafe(const unsigned char *p) {
	p += midi_interpret_vl_field(p).N;
	p += 2;  // Skip the 0xFF and type byte;
	return  midi_interpret_vl_field(p);
}
uint32_t mtrk_event_get_meta_payload_offset_dtstart_undafe(const unsigned char *p) {
	uint32_t o = midi_interpret_vl_field(p).N;
	p += o;  // inc past the delta-time
	o += 2;  p += o;  // inc past the 0xFF and type byte;
	o += midi_interpret_vl_field(p).N;  // inc past the length field
	return o;
}
parse_meta_event_unsafe_result_t mtrk_event_parse_meta_dtstart_unsafe(const unsigned char *p) {
	parse_meta_event_unsafe_result_t result {0x00u,0x00u,0x00u,0x00u};
	auto dt = midi_interpret_vl_field(p);
	result.dt = dt.val;
	p += dt.N;
	result.payload_offset += dt.N;

	p += 1;  // Skip the 0xFF
	result.payload_offset += 1;
	result.type = *p;
	p += 1;
	result.payload_offset += 1;

	auto len = midi_interpret_vl_field(p);
	result.length = len.val;
	result.payload_offset += len.N;

	return result;
}





parse_meta_event_result_t parse_meta_event(const unsigned char *p, int32_t max_size) {
	parse_meta_event_result_t result {};
	result.delta_t = midi_interpret_vl_field(p);
	if (result.delta_t.N > 4) {
		result.is_valid = false;
		return result;
	}
	p += result.delta_t.N;
	max_size -= result.delta_t.N;
	if (max_size <= 2) {
		result.is_valid = false;
		return result;
	}

	if (*p != 0xFF) {
		result.is_valid = false;
		return result;
	}
	++p;  --max_size;

	result.type = *p;
	++p;  --max_size;

	result.length = midi_interpret_vl_field(p);
	result.data_length = 2 + result.length.N + result.length.val;
	result.size = result.data_length + result.delta_t.N;
	max_size -= (result.length.N + result.length.val);
	if (max_size < 0) {
		result.is_valid = false;
		return result;
	}

	result.is_valid = true;
	return result;
}

parse_sysex_event_result_t parse_sysex_event(const unsigned char *p, int32_t max_size) {
	parse_sysex_event_result_t result {};
	result.delta_t = midi_interpret_vl_field(p);
	if (result.delta_t.N > 4) {
		result.is_valid = false;
		return result;
	}
	p += result.delta_t.N;
	max_size -= result.delta_t.N;
	if (max_size <= 0) {
		result.is_valid = false;
		return result;
	}

	if (*p != 0xF0 && *p != 0xFF) {
		result.is_valid = false;
		return result;
	}
	result.type = *p;
	++p;  --max_size;
	if (max_size <= 0) {
		result.is_valid = false;
		return result;
	}

	result.length = midi_interpret_vl_field(p);
	result.data_length = 1 + result.length.N + result.length.val;
	result.size = result.data_length + result.delta_t.N;
	max_size -= (result.length.N + result.length.val);
	if (max_size < 0) {
		result.is_valid = false;
		return result;
	}

	result.has_terminating_f7 = (*(--p) == 0xF7);
	result.is_valid = true;
	return result;
}













unsigned char *midi_rewrite_dt_field_unsafe(uint32_t dt, unsigned char *p, unsigned char s) {
	auto old_dt = midi_interpret_vl_field(p);
	auto new_dt_size = midi_vl_field_size(dt);
	auto old_size = mtrk_event_get_size_dtstart_unsafe(p,s);
	auto old_data_size = old_size-old_dt.N;

	if (old_dt.N==new_dt_size) {
		//...
	} else if (old_dt.N>new_dt_size) {
		auto p_new_dataend = std::copy(p+old_dt.N,p+old_size,p+new_dt_size);
		std::fill(p_new_dataend,p+old_size,0x00u);
	} else if (old_dt.N<new_dt_size) {
		auto new_size = new_dt_size+old_data_size;
		std::copy_backward(p+old_dt.N,p+old_size,p+new_size);
	}
	return write_delta_time(dt,p);
}



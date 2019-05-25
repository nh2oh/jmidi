#include "midi_raw.h"
#include "midi_vlq.h"
#include "dbklib\byte_manipulation.h"
#include <string>
#include <exception>
#include <cstdint>


validate_chunk_header_result_t validate_chunk_header(const unsigned char *p, uint32_t max_size) {
	validate_chunk_header_result_t result {};
	if (max_size < 8) {
		result.type = chunk_type::invalid;
		result.error = chunk_validation_error::size_exceeds_underlying;
		return result;
	}

	result.type = chunk_type_from_id(p);
	if (result.type == chunk_type::invalid) {
		result.error = chunk_validation_error::invalid_type_field;
		return result;
	}

	p += 4;  // Skip past 'MThd'
	result.data_size = dbk::be_2_native<uint32_t>(p);
	result.size = 8 + result.data_size;

	if (result.size>max_size) {
		result.type = chunk_type::invalid;
		result.error = chunk_validation_error::size_exceeds_underlying;
		return result;
	}

	result.error = chunk_validation_error::no_error;
	return result;
}
chunk_type chunk_type_from_id(const unsigned char *p) {
	uint32_t id = dbk::be_2_native<uint32_t>(p);
	if (id == 0x4D546864u) {  // Mthd
		return chunk_type::header;
	} else if (id == 0x4D54726Bu) {  // MTrk
		return chunk_type::track;
	} else {  // Verify all 4 bytes are valid ASCII
		for (int n=0; n<4; ++n) {
			if (*p>=127 || *p<32) {
				return chunk_type::invalid;
			}
		}
		return chunk_type::unknown;
	}
}

std::string print_error(const validate_chunk_header_result_t& chunk) {
	std::string result ="Invalid chunk header:  ";
	switch (chunk.error) {
		case chunk_validation_error::size_exceeds_underlying:
			result += "The calculated chunk size exceeds "
				"the size of the underlying array.";
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
	
	// <Header Chunk> = <chunk type> <length> <format> <ntrks> <division> 
	//                   MThd uint32_t uint16_t uint16_t uint16_t
	uint16_t format = dbk::be_2_native<uint16_t>(p+8);
	uint16_t ntrks = dbk::be_2_native<uint16_t>(p+10);

	if (ntrks==0) {
		result.error = mthd_validation_error::zero_tracks;
		return result;
	}
	if (format==0 && ntrks!=1) {
		result.error = mthd_validation_error::inconsistent_ntrks_format_zero;
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
		case mthd_validation_error::unknown_error:
			result += "Unknown error.";
			break;
	}

	return result;
}

// TODO:  Update to be consistent w/ make_mtrk()
validate_mtrk_chunk_result_t validate_mtrk_chunk(const unsigned char *p, uint32_t max_size) {
	validate_mtrk_chunk_result_t result {};
	auto chunk_detect = validate_chunk_header(p,max_size);
	if (chunk_detect.type != chunk_type::track) {
		if (chunk_detect.type==chunk_type::invalid) {
			result.error = mtrk_validation_error::invalid_chunk;
		} else {
			result.error = mtrk_validation_error::non_track_chunk;
		}
		return result;
	}
	
	// Process the data section of the mtrk chunk until the number of bytes
	// processed == chunk_detect.data_size, or an end-of-track meta event is
	// encountered.  Note that an end-of-track could be hit before processing
	// chunk_detect.data_size bytes if the chunk is 0-padded past the end of 
	// the end-of-track msg.  
	bool found_eot = false;
	uint32_t mtrk_data_size = chunk_detect.data_size;
	uint32_t i = 0;  // offset into the chunk data section
	p += 8;  // skip "MTrk" & the 4-byte length
	unsigned char rs {0};  // value of the running-status
	validate_mtrk_event_result_t curr_event;
	while ((i<mtrk_data_size) && !found_eot) {
		curr_event = validate_mtrk_event_dtstart(p,rs,mtrk_data_size-i);
		if (curr_event.error!=mtrk_event_validation_error::no_error) {
			result.error = mtrk_validation_error::event_error;
			return result;
		}

		rs = curr_event.running_status;

		if (curr_event.type==smf_event_type::meta) {
			// Test for end-of-track msg
			if (curr_event.size>=4) {
				uint32_t last_four = dbk::be_2_native<uint32_t>(p+curr_event.size-4);
				if ((last_four&0x00FFFFFFu)==0x00FF2F00u) {
					found_eot = true;
				}
			}
		}
		i += curr_event.size;
		p += curr_event.size;
	}

	// The track data must not overrun the reported size in the MTrk header,
	// and the final event must be a meta-end-of-track msg.  
	// Note that it is possible that found_eot==true even though 
	// i<mtrk_data_size.  I am allowing for the possibility that 
	// i<mtrk_data_size (which could perhaps => that the track is 0-padded 
	// after the end-of-track msg), but forbidding i>mtrk_data_size.  In any 
	// case, the 'length' field in the MTrk header section must be >= than
	// the actual amount of track data preceeding the EOT message.  
	if (i>mtrk_data_size) {
		result.error = mtrk_validation_error::data_length_not_match;
		return result;
	}
	if (!found_eot) {
		result.error = mtrk_validation_error::no_end_of_track;
		return result;
	}

	result.size = chunk_detect.size;
	result.data_size = chunk_detect.data_size;
	result.p = p;  // pointer to the 'M' of "MTrk"...
	result.error = mtrk_validation_error::no_error;
	return result;
}

std::string print_error(const validate_mtrk_chunk_result_t& mtrk) {
	std::string result = "Invalid MTrk chunk:  ";
	switch (mtrk.error) {
		case mtrk_validation_error::invalid_chunk:
			result += "detect_chunk_type(p,max_size)==chunk_type::invalid; "
				"call detect_chunk_type() and examine the 'error' field of "
				"the result.  ";
			break;
		case mtrk_validation_error::non_track_chunk:
			result += "The first 4 bytes at p are not equal to the ASCII values "
				"for 'MTrk'";
			break;
		case mtrk_validation_error::data_length_not_match:
			result += "Data-length mismatch:  ???";
			break;
		case mtrk_validation_error::event_error:
			result += "Some kind of event-error";
			break;
		case mtrk_validation_error::unknown_error:
			result += "Unknown_error :(";
			break;
		case mtrk_validation_error::no_end_of_track:
			result += "Track does not terminate in an end-of-track meta-event.";
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
	uint32_t result = 0;
	auto ev_type = classify_status_byte(*p,rs);  
	if (ev_type==smf_event_type::channel) {
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



channel_msg_type mtrk_event_get_ch_msg_type_dtstart_unsafe(const unsigned char *p, unsigned char s) {
	auto dt = midi_interpret_vl_field(p);
	s = get_status_byte(*(p+dt.N),s);

	channel_msg_type result = channel_msg_type::invalid;
	if ((s&0xF0u) != 0xB0u) {
		switch (s&0xF0u) {
			case 0x80u:  result = channel_msg_type::note_off; break;
			case 0x90u: result = channel_msg_type::note_on; break;
			case 0xA0u:  result = channel_msg_type::key_pressure; break;
			//case 0xB0:  ....
			case 0xC0u:  result = channel_msg_type::program_change; break;
			case 0xD0u:  result = channel_msg_type::channel_pressure; break;
			case 0xE0u:  result = channel_msg_type::pitch_bend; break;
			default: result = channel_msg_type::invalid; break;
		}
	} else {  // (s&0xF0u) == 0xB0u
		// To distinguish a channel_mode from a control_change msg, have to look at the
		// first data byte.  
		unsigned char p1 = mtrk_event_get_midi_p1_dtstart_unsafe(p,s);
		if (p1 >= 121 && p1 <= 127) {
			result = channel_msg_type::channel_mode;
		} else {
			result = channel_msg_type::control_change;
		}
	}

	return result;
}

//
// TODO:  There are some other conditions to verify:  I think only certain types of 
// channel_msg_type are allowed to be in running_status mode.  
//
parse_channel_event_result_t parse_channel_event(const unsigned char *p,
				unsigned char prev_status_byte,	int32_t max_size) {
	parse_channel_event_result_t result {};
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

	result.data_length = 0;
	if ((*p & 0x80) == 0x80) {  // Present message has a status byte
		result.has_status_byte = true;
		result.status_byte = *p;
		result.data_length += 1;
		++p; --max_size;
		if (max_size <= 0) {
			result.is_valid = false;
			return result;
		}
	} else {
		result.has_status_byte = false;
		if ((prev_status_byte & 0x80) != 0x80) {
			// prev_status_byte is invalid
			result.is_valid = false;
			return result;
		}
		result.status_byte = prev_status_byte;
		// Not incrementing p; *p is the first data byte in running-status mode
	}
	// At this point, p points at the first data byte

	// To distinguish a channel_mode from a control_change msg, have to look at the
	// first data byte.  
	// TODO:  Should probably check max_size before doing this ???
	// TODO:  parse_mtrk_event_type has already made this classification as 
	// smf_event_type::channel_mode || smf_event_type::channel_voice
	result.type = channel_msg_type_from_status_byte(result.status_byte, *p);

	if (result.type == channel_msg_type::invalid) {
		result.is_valid = false;
		return result;
	}

	if ((result.status_byte & 0xF0) == 0xC0 || (result.status_byte & 0xF0) == 0xD0) {
		result.n_data_bytes = 1;
	} else {
		result.n_data_bytes = 2;
	}
	result.data_length += result.n_data_bytes;
	max_size -= result.n_data_bytes;
	if (max_size < 0) {
		result.is_valid = false;
		return result;
	}

	// Check that the first and second (if appropriate) data bytes do not have the high bit set, 
	// a state only valid for status bytes.  
	if ((*p & 0x80) != 0) {
		result.is_valid = false;
		return result;
	}
	if (result.n_data_bytes == 2) {
		++p;
		if ((*p & 0x80) != 0) {
			result.is_valid = false;
			return result;
		}
	}
	result.size = result.data_length + result.delta_t.N;
	result.is_valid = true;
	return result;
}


bool midi_event_has_status_byte(const unsigned char *p) {
	auto delta_t_vl = midi_interpret_vl_field(p);
	if (delta_t_vl.N > 4) {
		std::abort();
	}
	p += delta_t_vl.N;
	return (*p) & 0x80;
}
unsigned char midi_event_get_status_byte(const unsigned char* p) {
	auto delta_t_vl = midi_interpret_vl_field(p);
	if (delta_t_vl.N > 4) {
		std::abort();
	}
	p += delta_t_vl.N;
	return *p;
}
// TODO:  This is wrong.  It treats 0xF0u,0xF7u,0xFFu as valid status bytes
int midi_channel_event_n_bytes(unsigned char p, unsigned char s) {
	int N = 0;
	if ((p & 0x80)==0x80) {
		++N;
		s=p;
	} else if ((s & 0x80) != 0x80) {  // Both p,s are non-status bytes
		return 0;
	}

	// At this point, s is the status byte of interest.  If p was a valid status byte,
	// the value of p has replaced the value passed in as s and N == 1; if p was not
	// a valid status byte but s was, N == 0.  If neither p, s were valid status bytes
	// this point will not be reached.  
	if ((s & 0xF0) == 0xC0 || (s & 0xF0) == 0xD0) {
		N += 1;
	} else {
		N += 2;
	}

	return N;
}
int8_t channel_number_from_status_byte_unsafe(unsigned char s) {
	return static_cast<uint8_t>(s&0x0Fu + 1);
}
channel_msg_type channel_msg_type_from_status_byte(unsigned char s, unsigned char p1) {
	channel_msg_type result = channel_msg_type::invalid;
	if ((s & 0xF0) != 0xB0) {
		switch (s & 0xF0) {
			case 0x80:  result = channel_msg_type::note_off; break;
			case 0x90: result = channel_msg_type::note_on; break;
			case 0xA0:  result = channel_msg_type::key_pressure; break;
			//case 0xB0:  ....
			case 0xC0:  result = channel_msg_type::program_change; break;
			case 0xD0:  result = channel_msg_type::channel_pressure; break;
			case 0xE0:  result = channel_msg_type::pitch_bend; break;
			default: result = channel_msg_type::invalid; break;
		}
	} else if ((s & 0xF0) == 0xB0) {
		// To distinguish a channel_mode from a control_change msg, have to look at the
		// first data byte.  
		if (p1 >= 121 && p1 <= 127) {
			result = channel_msg_type::channel_mode;
		} else {
			result = channel_msg_type::control_change;
		}
	}

	return result;
}




unsigned char *midi_rewrite_dt_field_unsafe(uint32_t dt, unsigned char *p, unsigned char s) {
	auto old_dt = midi_interpret_vl_field(p);
	auto new_dt_size = midi_vl_field_size(dt);
	auto old_size = mtrk_event_get_size_dtstart_unsafe(p,s);
	auto old_data_size = old_size-old_dt.N;

	//unsigned char *p_new_datastart;
	if (old_dt.N==new_dt_size) {
		//p_new_datastart = midi_write_vl_field(p,dt);
	} else if (old_dt.N>new_dt_size) {
		//p_new_datastart = midi_write_vl_field(p,dt);
		//auto p_new_dataend = std::copy(p+old_dt.N,p+old_size,p_new_datastart);
		auto p_new_dataend = std::copy(p+old_dt.N,p+old_size,p+new_dt_size);
		std::fill(p_new_dataend,p+old_size,0x00u);
	} else if (old_dt.N<new_dt_size) {
		auto new_size = new_dt_size+old_data_size;
		std::copy_backward(p+old_dt.N,p+old_size,p+new_size);
		//p_new_datastart = midi_write_vl_field(p,dt);
	}

	return midi_write_vl_field(p,dt);
}



//
//TODO:  The offset checks are repetitive...
//
validate_smf_result_t validate_smf(const unsigned char *p, int32_t offset_end, 
									const std::string& fname) {
	validate_smf_result_t result {};
	int n_tracks {0};
	int n_unknown {0};
	std::vector<chunk_idx_t> chunk_idxs {};

	int32_t offset {0};
	while (offset<offset_end) {  // For each chunk...
		auto curr_chunk = validate_chunk_header(p+offset,offset_end-offset);
		if (curr_chunk.type == chunk_type::invalid) {
			result.is_valid = false;
			result.msg += "curr_chunk.type == chunk_type::invalid\ncurr_chunk_type.msg== ";
			result.msg += print_error(curr_chunk);
			return result;
		} else if (curr_chunk.type == chunk_type::header) {
			if (offset > 0) {
				result.is_valid = false;
				result.msg += "curr_chunk.type == chunk_type::header but offset > 0.  ";
				result.msg += "A valid midi file must contain only one MThd @ the very start.  \n";
				return result;
			}
			validate_mthd_chunk_result_t mthd = validate_mthd_chunk(p+offset, offset_end-offset);
			if (mthd.error != mthd_validation_error::no_error) {
				result.is_valid = false;
				result.msg += "mthd.error != mthd_validation_error::no_error";
				return result;
			}
		} else if (curr_chunk.type == chunk_type::track) {
			if (offset == 0) {
				result.is_valid = false;
				result.msg += "curr_chunk.type == chunk_type::track but offset == 0.  ";
				result.msg += "A valid midi file must begin w/an MThd chunk.  \n";
				return result;
			}
			validate_mtrk_chunk_result_t curr_track = validate_mtrk_chunk(p+offset,offset_end-offset);
			if (curr_track.error != mtrk_validation_error::no_error) {
				result.msg += "!curr_track.is_valid\ncurr_track.msg==";
				result.msg += print_error(curr_track);
				//result.msg += curr_track.msg;
				return result;
			}
			++n_tracks;
		} else if (curr_chunk.type == chunk_type::unknown) {
			++n_unknown;
			if (offset == 0) {
				result.is_valid = false;
				result.msg += "curr_chunk.type == chunk_type::unknown but offset == 0.  ";
				result.msg += "A valid midi file must begin w/an MThd chunk.  \n";
				return result;
			}
		}

		chunk_idxs.push_back({curr_chunk.type,offset,curr_chunk.size});
		offset += curr_chunk.size;
	}

	if (offset != offset_end) {
		result.is_valid = false;
		result.msg = "offset != offset_end.";
		return result;
	}

	result.is_valid = true;
	result.fname = fname;
	result.chunk_idxs = chunk_idxs;
	result.n_mtrk = n_tracks;
	result.n_unknown = n_unknown;
	result.size = offset;
	result.p = p;

	return result;
}

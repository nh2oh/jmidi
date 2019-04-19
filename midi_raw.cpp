#include "midi_raw.h"
#include "dbklib\byte_manipulation.h"
#include <string>
#include <array>
#include <vector>
#include <exception>
#include <cstdint>
#include <iostream>


midi_vl_field_interpreted midi_interpret_vl_field(const unsigned char* p) {
	midi_vl_field_interpreted result {};
	result.val = 0;

	while (true) {
		result.val += (*p & 0x7F);
		++(result.N);
		if (!(*p & 0x80) || result.N==4) { // the high-bit is not set
			break;
		} else {
			result.val <<= 7;  // result.val << 7;
			++p;
		}
	}

	result.is_valid = !(*p & 0x80);

	return result;
};



// Default value of sep == ' '; see header
std::string print_hexascii(const unsigned char *p, int n, const char sep) {
	std::string s {};  s.reserve(3*n);

	std::array<unsigned char,16> nybble2ascii {'0','1','2','3','4','5',
		'6','7','8','9','A','B','C','D','E','F'};

	for (int i=0; i<n; ++i) {
		s += nybble2ascii[((*p) & 0xF0)>>4];
		s += nybble2ascii[((*p) & 0x0F)];
		s += sep;
		++p;
	}
	if (n>0) { s.pop_back(); }

	return s;
}


detect_chunk_type_result_t detect_chunk_type(const unsigned char *p, uint32_t max_size) {
	detect_chunk_type_result_t result {};
	if (max_size < 8) {
		result.type = chunk_type::invalid;
		result.error = chunk_validation_error::size_exceeds_underlying;
		return result;
	}

	// All chunks begin w/ A 4-char ASCII identifier
	uint32_t id = dbk::be_2_native<uint32_t>(p);
	if (id == 0x4D546864) {  // Mthd
		result.type = chunk_type::header;
		p+=4;
	} else if (id == 0x4D54726B) {  // MTrk
		result.type = chunk_type::track;
		p+=4;
	} else {
		result.type = chunk_type::unknown;
		int n = 0;
		while (n<4) {
			if (*p>=127 || *p<32) {
				result.type = chunk_type::invalid;
				result.error = chunk_validation_error::invalid_type_field;
				return result;
			}
			++p; ++n;
		}
	}

	result.data_length = dbk::be_2_native<uint32_t>(p);
	result.size = 8 + result.data_length;

	if (result.size>max_size) {
		result.type = chunk_type::invalid;
		result.error = chunk_validation_error::size_exceeds_underlying;
		return result;
	}

	result.error = chunk_validation_error::no_error;
	return result;
}


std::string print_error(const detect_chunk_type_result_t& chunk) {
	std::string result {};

	switch (chunk.error) {
		case chunk_validation_error::size_exceeds_underlying:
			result += "Invalid chunk header:  the calculated chunk size exceeds "
				"the size of the underlying array.";
			break;
		case chunk_validation_error::invalid_type_field:
			result += "Invalid chunk header:  The 4-char ASCII 'type' field is missing.";
			break;
		case chunk_validation_error::unknown_error:
			result += "Invalid chunk header:  Unknown error.";
			break;
	}

	return result;
}


validate_mthd_chunk_result_t validate_mthd_chunk(const unsigned char *p, uint32_t max_size) {
	validate_mthd_chunk_result_t result {};

	detect_chunk_type_result_t chunk_detect = detect_chunk_type(p,max_size);
	if (chunk_detect.type != chunk_type::header) {
		result.is_valid = false;
		if (chunk_detect.type==chunk_type::invalid) {
			result.error = mthd_validation_error::invalid_chunk;
		} else {
			result.error = mthd_validation_error::non_header_chunk;
		}
		return result;
	}

	if (chunk_detect.data_length < 6) {
		result.is_valid = false;
		result.error = mthd_validation_error::data_length_invalid;
		return result;
	}
	
	// <Header Chunk> = <chunk type> <length> <format> <ntrks> <division> 
	//                   MThd uint32_t uint16_t uint16_t uint16_t
	uint16_t format = dbk::be_2_native<uint16_t>(p+8);
	uint16_t ntrks = dbk::be_2_native<uint16_t>(p+10);

	// Note that i am making it legal to have 0 tracks, for example, to represent 
	// an smf under construction.
	if (format==0 && ntrks>1) {
		result.error = mthd_validation_error::inconsistent_ntrks_format_zero;
		result.is_valid = false;
		return result;
	}

	result.is_valid = true;
	result.error = mthd_validation_error::no_error;
	result.size = chunk_detect.size;
	result.p = p;  // pointer to the 'M' of "MThd"..., ie the _start_ of the mthd chunk
	return result;
}


std::string print_error(const validate_mthd_chunk_result_t& mthd) {
	std::string result {};

	switch (mthd.error) {
		case mthd_validation_error::invalid_chunk:
			result += "Not a valid SMF chunk.  The 4-char ASCII 'type' field may be "
				"missing, or the calculated chunk size may exceed the size of the "
				"underlying array.";
			break;
		case mthd_validation_error::non_header_chunk:
			result += "chunk_detect.type != chunk_type::header.";
			break;
		case mthd_validation_error::data_length_invalid:
			result += "MThd chunks must have a data length >= 6";
			break;
		case mthd_validation_error::inconsistent_ntrks_format_zero:
			result += "format==0 but ntrks>1; format 0 smf's have exactly 1 track.";
			break;
		case mthd_validation_error::unknown_error:
			result += "Invalid MThd chunk:  Unknown error.";
			break;
	}

	return result;
}



//
// 
//
validate_mtrk_chunk_result_t validate_mtrk_chunk(const unsigned char *p, int32_t max_size) {
	validate_mtrk_chunk_result_t result {};

	detect_chunk_type_result_t chunk_detect = detect_chunk_type(p,max_size);
	if (chunk_detect.type != chunk_type::track) {
		if (chunk_detect.type==chunk_type::invalid) {
			result.error = mtrk_validation_error::invalid_chunk;
		} else {
			result.error = mtrk_validation_error::non_track_chunk;
		}
		return result;
	}


	// Validate each mtrk event in the chunk
	auto mtrk_reported_size = chunk_detect.size;  // be_2_native<uint32_t>(p+4)+8;
	if (chunk_detect.size != (dbk::be_2_native<uint32_t>(p+4)+8)) {
		std::cout << "debug assertion fail: chunk_detect.size != "
			"(be_2_native<uint32_t>(p+4)+8)" << std::endl;
	}
	int32_t i {8};  // skips "MTrk" & the 4-byte length
	unsigned char most_recent_midi_status_byte {0};
	while (i<mtrk_reported_size) {
		// Classify the present event as an smf_event_type (=> midi_channel, 
		// sysex_f0/f7, meta) and compute its length.  
		auto curr_event = parse_mtrk_event_type(p+i,most_recent_midi_status_byte,mtrk_reported_size-i);
		if (curr_event.type == smf_event_type::invalid) {
			result.error = mtrk_validation_error::event_error;
			return result;
		}

		if (curr_event.type == smf_event_type::sysex_f0 
					|| curr_event.type == smf_event_type::sysex_f7) {
			// From the std (p.136):
			// Sysex events and meta-events cancel any running status which was in effect.  
			// Running status does not apply to and may not be used for these messages.
			//
			most_recent_midi_status_byte = unsigned char {0};
			auto sx = parse_sysex_event(p+i,mtrk_reported_size-i);
			if (!(sx.is_valid)) {
				result.error = mtrk_validation_error::event_error;
				return result;
			}
		} else if (curr_event.type == smf_event_type::meta) {
			// From the std (p.136):
			// Sysex events and meta-events cancel any running status which was in effect.  
			// Running status does not apply to and may not be used for these messages.
			//
			most_recent_midi_status_byte = unsigned char {0};
			auto mt = parse_meta_event(p+i,mtrk_reported_size-i);
			if (!(mt.is_valid)) {
				result.error = mtrk_validation_error::event_error;
				return result;
			}
		} else if (curr_event.type == smf_event_type::channel_voice 
					|| curr_event.type == smf_event_type::channel_mode) {
			auto md = parse_channel_event(p+i,most_recent_midi_status_byte,mtrk_reported_size-i);
			if (!(md.is_valid)) {
				result.error = mtrk_validation_error::event_error;
				return result;
			}
			most_recent_midi_status_byte = md.status_byte;
		} else {
			result.is_valid = false;
			result.error = mtrk_validation_error::unknown_error;
			return result;
		}

		i += curr_event.size;
	}

	if (i != mtrk_reported_size) {
		result.is_valid = false;
		result.error = mtrk_validation_error::data_length_not_match;
		return result;
	}

	result.is_valid = true;
	result.size = i;
	result.data_length = i-8;  // -4 for "MTrk", -4 for the length field
	result.p = p;  // pointer to the 'M' of "MTrk"..., ie the _start_ of the mtrk chunk
	return result;
}


std::string print_error(const validate_mtrk_chunk_result_t& mtrk) {
	std::string result {};

	switch (mtrk.error) {
	case mtrk_validation_error::invalid_chunk:
		result += "Invalid chunk:  detect_chunk_type(p,max_size)==chunk_type::invalid; "
			"call detect_chunk_type() and examine the 'error' field of the result.  ";
		break;
	case mtrk_validation_error::non_track_chunk:
		result += "Non-track chunk:  Does p point at the 'M' of the 4-ASCII char "
			"sequence \"MTrk\"?";
		break;
	case mtrk_validation_error::data_length_not_match:
		result += "Data-length mismatch:  The track length reported in the MTrk"
			" chunk header is either too long or too short to accomodate the "
			"event list.  ";
		break;
	case mtrk_validation_error::event_error:
		result += "Event_error:  Could be a size issue, midi-status byte issue, "
			" sysx_f0/f7 or meta -length-field issue, ...";
		break;
	case mtrk_validation_error::unknown_error:
		result += "Unknown_error:  :(";
		break;
	}

	return result;
}


/*
std::string print(const event_type_deprecated& et) {
	if (et == event_type_deprecated::meta) {
		return "meta";
	} else if (et == event_type_deprecated::midi) {
		return "midi";
	} else if (et == event_type_deprecated::sysex) {
		return "sysex";
	}
}*/
std::string print(const smf_event_type& et) {
	if (et == smf_event_type::meta) {
		return "meta";
	} else if (et == smf_event_type::channel_voice) {
		return "channel_voice";
	} else if (et == smf_event_type::channel_mode) {
		return "channel_mode";
	} else if (et == smf_event_type::sysex_f0) {
		return "sysex_f0";
	} else if (et == smf_event_type::sysex_f7) {
		return "sysex_f7";
	} else if (et == smf_event_type::invalid) {
		return "invalid";
	} else {
		return "std::string print(const smf_event_type&) can't deal with this type of smf_event.";
	}
}





smf_event_type detect_mtrk_event_type_dtstart_unsafe(const unsigned char *p, unsigned char s) {
	auto dt = midi_interpret_vl_field(p);
	p += dt.N;
	return detect_mtrk_event_type_unsafe(p,s);
}

//
// Pointer to the first data byte following the delta-time, _not_ to the start of
// the delta-time.  This function may have to increment the pointer by 1 byte
smf_event_type detect_mtrk_event_type_unsafe(const unsigned char *p, unsigned char s) {
	// The tests below are made on s, not p.  If p is a valid status byte (high bit is set),
	// the value of s is overwritten with *p.  If p is _not_ a valid status byte, ::invalid is 
	// returned if s == FF || F0 || F7; although these are valid status bytes, they are not
	// valid _running_-status bytes.  
	//
	// If p _is_ a valid status byte (high bit set), overwrite s with *p, then attempt to 
	// classify s as channel_mode or channel_voice
	if ((*p & 0x80) != 0x80) {  // *p is _not_ a valid status byte
		if (s == 0xFF || s == 0xF0 || s == 0xF7) {
			return smf_event_type::invalid;
		}
	} else {  // *p _is_ a valid status byte
		s = *p;
	}

	if ((s & 0xF0) >= 0x80 && (s & 0xF0) <= 0xE0) {  // channel msg
		if ((s & 0xF0) == 0xB0) {
			// The first data byte should not have its high bit set.  
			// I need the first data byte.  If we're using s, *p is the first data byte; if
			// we're using *p, *++p is the first data byte.  
			if ((*p & 0x80) == 0x80) {  // We're using *p
				++p;
			}
			if ((*p & 0x80) == 0x80) {  // First data byte has its high bit set
				return smf_event_type::invalid;
			}
			if (*p >= 121 && *p <= 127) {
				return smf_event_type::channel_mode;
			} else {
				return smf_event_type::channel_voice;
			}
		}
		return smf_event_type::channel_voice;
	} else if (s == 0xFF) { 
		// meta event in an smf; system_realtime in a stream.  All system_realtime messages
		// are single byte events (status byte only); in a meta-event, the status byte is followed
		// by an "event type" byte then a vl-field giving the number of subsequent data bytes.
		return smf_event_type::meta;
	} else if (s == 0xF7) {
		return smf_event_type::sysex_f7;
	} else if (s == 0xF0) {
		return smf_event_type::sysex_f0;
	}
	return smf_event_type::invalid;
}

//
// Takes a pointer to the first byte of the vl delta-time field.
//
// TODO:  Should return an error if the reported length exceeds the max_inc
parse_mtrk_event_result_t parse_mtrk_event_type(const unsigned char *p, unsigned char s, int32_t max_inc) {
	parse_mtrk_event_result_t result {};
	if (max_inc == 0) {
		result.type = smf_event_type::invalid;
		return result;
	}
	// All mtrk events begin with a delta-time occupying a maximum of 4 bytes
	result.delta_t = midi_interpret_vl_field(p);
	if (result.delta_t.N > 4) {
		result.type = smf_event_type::invalid;
		return result;
	}
	p += result.delta_t.N;

	max_inc -= result.delta_t.N;
	if (max_inc <= 0) {
		result.type = smf_event_type::invalid;
		return result;
	}

	if (((*p & 0xF0)==0xB0 && max_inc<2) || ((*p & 0x80)!=0x80 && (s & 0xF0)==0xB0 && max_inc < 1)) {
		// indicates a channel_mode or channel_voice message, both of which have 2 data bytes
		// following the status byte *p.  detect_mtrk_event_type_unsafe(p) will increment p to
		// the first data byte to classify this type of event, which is obv a problem if the
		// array is to short.  
		result.type = smf_event_type::invalid;
	}
	result.type = detect_mtrk_event_type_unsafe(p,s);

	//
	// TODO:  Compute size, data_length ...
	//
	if (result.type == smf_event_type::meta) {
		// *p == 0xFF || 0xF0 || 0xF7; running-status is not allowed for these values
		++p;  // 0xFF
		++p;  // type-byte
		auto length_field = midi_interpret_vl_field(p);
		result.size = result.delta_t.N + 2 + length_field.N + length_field.val;
		result.data_length = result.size - result.delta_t.N;
	} else if (result.type == smf_event_type::sysex_f0 
		|| result.type == smf_event_type::sysex_f7) {
		++p;  // 0xF0 || 0xF7
		auto length_field = midi_interpret_vl_field(p);
		result.size = result.delta_t.N + 1+ length_field.N + length_field.val;
		result.data_length = result.size - result.delta_t.N;
	} else if (result.type == smf_event_type::channel_mode 
		|| result.type == smf_event_type::channel_voice) {
		//...
		result.data_length = midi_channel_event_n_bytes(*p,s);
		result.size = result.data_length + result.delta_t.N;
	}

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
	return int8_t {(s & 0x0F) + 1};
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
		detect_chunk_type_result_t curr_chunk = detect_chunk_type(p+offset,offset_end-offset);
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
			if (!curr_track.is_valid) {
				result.msg += "!curr_track.is_valid\ncurr_track.msg==";
				result.msg += curr_track.msg;
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

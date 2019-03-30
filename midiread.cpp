#include "midiread.h"
/*
#include "dbklib\binfile.h"
#include <filesystem>
#include <algorithm>
#include <iterator>  // std::back_inserter()
#include <iostream>
#include <exception>
#include <string>

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

	return s;
}

detect_chunk_result detect_chunk_type(const unsigned char* p) {
	detect_chunk_result result {midi_chunk_t::unknown,0,false};
	
	// All chunks begin w/ A 4-char identifier
	std::string idstr {};
	std::copy(p,p+4,std::back_inserter(idstr));
	if (idstr == "MThd") {
		result.type = midi_chunk_t::header;
	} else if (idstr == "MTrk") {
		result.type = midi_chunk_t::track;
	} else {
		result.is_valid = false;
		return result;
	}
	p+=4;

	result.data_length = midi_raw_interpret<int32_t>(p);
	p+=4;
	if (result.data_length < 0) {
		result.is_valid = false;
		return result;
	}
	if (result.type == midi_chunk_t::header && result.data_length != 6) {
		result.is_valid = false;
		return result;
	}

	result.is_valid = true;
	return result;
}


midi_chunk read_midi_chunk(const dbk::binfile& bf, size_t offset) {
	midi_chunk result {};

	std::copy(bf.d.begin()+offset,bf.d.begin()+offset+4,result.id.begin());
	offset += 4;

	result.length = midi_raw_interpret<int>(&(bf.d[offset]));
	offset += 4;

	result.data.reserve(result.length);
	std::copy(bf.d.begin()+offset,bf.d.begin()+offset+result.length,std::back_inserter(result.data));

	return result;
}

midi_file_header_data read_midi_fheaderchunk(const midi_chunk& chunk) {
	if (chunk.data.size() != 6) {
		std::cout << "The \"global\" header chunk of a midi file must have a 6-byte data section.";
		std::abort();
	}
	midi_file_header_data result {};
	size_t offset {0};

	result.fmt_type = midi_raw_interpret<int16_t>(&(chunk.data[offset]));
	offset += 2;

	result.num_trks = midi_raw_interpret<int16_t>(&(chunk.data[offset]));
	offset += 2;

	result.time_div = midi_raw_interpret<int16_t>(&(chunk.data[offset]));
	offset += 2;

	return result;
}


midi_file_header_data read_midi_fheaderchunk(const unsigned char *p) {
	detect_chunk_result chunk_type = detect_chunk_type(p);
	if (chunk_type.type != midi_chunk_t::header) {
		std::cout << "p is not pointing at the start of a midi header.";
		std::abort();
	}
	p += 4;  // MThd
	p += 4;  // 4-byte field giving the length of the data segment (chunk_type.data_length)

	if (chunk_type.data_length != 6) {
		std::cout << "The \"global\" header chunk of a midi file must have a 6-byte data section.";
		std::abort();
	}
	midi_file_header_data header_data {};
	header_data.fmt_type = midi_raw_interpret<int16_t>(p);
	p += 2;

	header_data.num_trks = midi_raw_interpret<int16_t>(p);
	p += 2;

	header_data.time_div = midi_raw_interpret<int16_t>(p);
	p += 2;

	return header_data;
}


midi_time_division_field_type_t detect_midi_time_division_type(const unsigned char *p) {
	if ((*p)>>7 == 1) {
		return midi_time_division_field_type_t::SMPTE;
	} else {
		return midi_time_division_field_type_t::ticks_per_quarter;
	}
}
// assumes midi_time_division_field_type_t::ticks_per_quarter
uint16_t interpret_tpq_field(const unsigned char *p) {
	std::array<unsigned char,2> field {(*p)&0x7F, *(++p)};
	return midi_raw_interpret<uint16_t>(&(field[0]));
}
// assumes midi_time_division_field_type_t::SMPTE
midi_smpte_field interpret_smpte_field(const unsigned char *p) {
	midi_smpte_field result {};
	result.time_code_fmt = static_cast<int8_t>(*p);
	result.units_per_frame = static_cast<uint8_t>(*(++p));
	return result;
}

detect_mtrk_event_result detect_mtrk_event_type(const unsigned char* p) {
	detect_mtrk_event_result result {mtrk_event_t::unknown,midi_vl_field_interpreted {},false};
	
	// All mtrk events begin with a delta-time occupying a maximum of 4 bytes
	result.delta_t = midi_interpret_vl_field(p);
	if (result.delta_t.N > 4) {
		result.is_valid = false;
		return result;
	}
	p += result.delta_t.N;

	if (*p == static_cast<unsigned char>(0xF0) || *p == static_cast<unsigned char>(0xF7)) {
		// sysex event; 0xF0==240, 0xF7==247
		result.type = mtrk_event_t::sysex;
		result.is_valid = true;
	} else if (*p == static_cast<unsigned char>(0xFF)) {  
		// meta event; 0xFF == 255
		result.type = mtrk_event_t::meta;
		result.is_valid = true;
	} else {
		// midi event
		result.type = mtrk_event_t::midi;
		result.is_valid = true;
	}

	return result;
}


sysex_event parse_sysex_event(const unsigned char* p) {
	sysex_event result {};

	result.delta_t = midi_interpret_vl_field(p);
	p += result.delta_t.N;

	result.id = *p;  // uchar -> uint8_t
	++p;

	result.length = midi_interpret_vl_field(p);
	p += result.length.N;

	result.data.reserve(result.length.val);
	std::copy(p,p+result.length.val,std::back_inserter(result.data));

	return result;
}

meta_event parse_meta_event(const unsigned char* p) {
	meta_event result {};

	result.delta_t = midi_interpret_vl_field(p);
	p += result.delta_t.N;

	result.id = *p;  // char -> uint8_t;  must == 0xFF
	if (result.id != 0xFF) {
		std::abort();
	}
	++p;

	result.type = *p;  // char -> uint8_t;  ex: 58 => ts
	if (result.type > 128) {
		std::abort();
	}
	++p;

	result.length = midi_interpret_vl_field(p);
	p += result.length.N;

	result.data.reserve(result.length.val);
	std::copy(p,p+result.length.val,std::back_inserter(result.data));

	return result;
}

//
// MIDI messages consist of either:
// 1)  A sequence of data bytes, or
// 2)  A single status byte followed by a sequence of data bytes.  
// Messges not containing a status byte have an implied status set by the most recent status
// byte encountered in the stream.  Status bytes have the MSBit set
//
midi_event parse_midi_event(const unsigned char *p, const unsigned char prev_stat) {
	midi_event result {};

	result.delta_t = midi_interpret_vl_field(p);
	p += result.delta_t.N;

	unsigned char high_bit = 0x80;
	unsigned char high_4bits = 0xF0;

	if ((*p & high_bit) == 0x80) {
		result.running_status = false;
		result.status = *p;
		++p;
	} else {
		result.running_status = true;
		result.status = prev_stat;
		// Not incrementing p; *p is the first data byte in running-status mode
	}
	// result.status is always the valid status byte applicable to the present message, even if
	// the present message is part of a "running status" stream and does not itself contain the
	// status byte.  
	if ((result.status & high_bit) != 0x80) { std::abort(); }

	//
	// Logic is duplicated in:
	// uint8_t midi_event_num_data_bytes(const midi_event& event_in);
	uint8_t n_data_bytes {0};
	if ((result.status & high_4bits) == 0xC0 || (result.status & 0xF0) == 0xD0) {  // MSBit == 1 => byte is a status byte
		n_data_bytes = 1;
	} else {
		n_data_bytes = 2;
	}

	// At this point, p points at the first data byte of the message
	if ((*p & high_bit) == 0x80) {
		std::abort();  // The MSB of all data bytes is 0
	}
	result.p1 = *p;
	
	if (n_data_bytes == 2) {
		++p; 
		if ((*p & high_bit) == 0x80) {
			std::abort();  // The MSB of all data bytes is 0
		}
		result.p2 = *p;
	}

	return result;
}



status_byte_type classify_status_byte(const unsigned char *p) {
	unsigned char high = 0xF0;

	status_byte_type result {status_byte_type::unknown};
	switch (*p & high) {
		case 0x80:  result = status_byte_type::channel; break;
		case 0x90:  result = status_byte_type::channel; break;
		case 0xA0:  result = status_byte_type::channel; break;
		case 0xB0:  result = status_byte_type::channel; break;
		case 0xC0:  result = status_byte_type::channel; break;
		case 0xD0:  result = status_byte_type::channel; break;
		case 0xE0:  result = status_byte_type::channel; break;
		case 0xF0:  result = status_byte_type::system; break;
		default:    result = status_byte_type::unknown; break;
	}

	return result;
}

midi_byte classify_byte(const unsigned char *p) {
	if ((*p & 0x80) == 0x80) {
		return midi_byte::status;
	}
	return midi_byte::data;
}

ch_msg_type classify_channel_status_byte(const unsigned char* p) {
	if ((*p & 0xF0) == 0xB0) {
		if ((*(p+1) & 0xF8) == 0x78) {  // 0b01111000 == 120
			return ch_msg_type::mode;
		}
	}
	return ch_msg_type::voice;
}

int channel_number(const unsigned char* p) {
	return (*p & 0x0F) + 1;  // + 1 b/c there is no channel 0
}


sys_msg_type classify_system_status_byte(const unsigned char* p) {
	sys_msg_type result {sys_msg_type::unknown};
	if (*p == 0xF0) {
		result = sys_msg_type::exclusive;
	} else if ((*p & 0xF8) == 0xF0) {
		// 0b11110sss where sss : [1,7]
		if ((*p & 0x07) == 0) {
			result = sys_msg_type::unknown;
		} else {
			result = sys_msg_type::common;
		}
	} else if ((*p & 0xF8) == 0xF8) {
		// 0b11110ttt where ttt : [0,7]
		result = sys_msg_type::realtime;
	}

	return result;
}



uint8_t midi_event_num_data_bytes(const midi_event& event_in) {
	if ((event_in.status & 0xF0) == 0xC0 || (event_in.status & 0xF0) == 0xD0) {
		return 1;
	} else {
		return 2;
	}
}
	
midi_file::midi_file(const std::filesystem::path& fp) {
	this->fdata_ = dbk::readfile(fp).d;
	
	uint64_t i {0};
	detect_chunk_result curr_chunk_type = detect_chunk_type(&(this->fdata_[i]));
	if (!curr_chunk_type.is_valid || curr_chunk_type.type != midi_chunk_t::header) {
		std::cout << "!curr_chunk_type.is_valid || curr_chunk_type.type != midi_chunk_t::header\n"
			<< "for this->fdata_[i] with i == " << i << ".  A valid midi file must begin with a "
			<< "midi header chunk.  \nAborting...\n";
		std::abort();
	}
	this->chunk_idx_.push_back({i,static_cast<uint64_t>(8+curr_chunk_type.data_length),midi_chunk_t::header});
	i += this->chunk_idx_.back().length;

	while (i<this->fdata_.size()) {  // For each Track trunk...
		curr_chunk_type = detect_chunk_type(&(this->fdata_[i]));
		if (!curr_chunk_type.is_valid || curr_chunk_type.type != midi_chunk_t::track) {
			std::cout << "!curr_chunk_type.is_valid || curr_chunk_type.type != midi_chunk_t::track\n"
			<< "for this->fdata_[i] with i == " << i << ".  A valid midi file must contain a "
			<< "sequence of track chunks following the header chunk.  \nAborting...\n";
			std::abort();
		}
		this->chunk_idx_.push_back({i,static_cast<uint64_t>(8+curr_chunk_type.data_length),midi_chunk_t::track});
		
		
		// Now that we know that the present chunk (location this->fdata_[i]) is a Track chunk 
		// ("MTrk"), move to the data section (begins 8 bytes following the "MTrk") and iterate 
		// over the MTrk events contained within.   
		std::vector<midi_file::mtrk_event_idx> curr_mtrk_event_idx {};
		uint64_t j = i+8;
		unsigned char prev_midi_status_byte {0};  // TODO: Rename running_midi_status_byte ?
		while (j < (this->chunk_idx_.back().offset+this->chunk_idx_.back().length)) {
			// Classify the present event as midi, sysex, or meta
			detect_mtrk_event_result curr_event = detect_mtrk_event_type(&(this->fdata_[j]));
			if (!curr_event.is_valid) {
				std::abort();
			}

			uint64_t curr_event_length {0};
			if (curr_event.type == mtrk_event_t::sysex) {
				// From the std (p.136):
				// Sysex events and meta-events cancel any running status which was in effect.  
				// Running status does not apply to and may not be used for these messages.
				//
				prev_midi_status_byte = unsigned char {0};
				sysex_event sx = parse_sysex_event(&(this->fdata_[j]));
				curr_event_length = curr_event.delta_t.N + 1 + sx.length.N + sx.length.val;
					// length:  +1 for the F0 or F7
			} else if (curr_event.type == mtrk_event_t::meta) {
				// From the std (p.136):
				// Sysex events and meta-events cancel any running status which was in effect.  
				// Running status does not apply to and may not be used for these messages.
				//
				prev_midi_status_byte = unsigned char {0};
				meta_event mt = parse_meta_event(&(this->fdata_[j]));
				curr_event_length = curr_event.delta_t.N + 2 + mt.length.N + mt.length.val;
					// length: +2 => +1 for the leading FF, +1 for the type-byte
			} else {  // midi event
				midi_event md = parse_midi_event(&(this->fdata_[j]),prev_midi_status_byte);
				prev_midi_status_byte = md.status;
				if (!(md.running_status)) {
					curr_event_length = curr_event.delta_t.N + 1 + midi_event_num_data_bytes(md);
					// +1 for the status byte
				} else {
					curr_event_length = curr_event.delta_t.N + midi_event_num_data_bytes(md);
					// no status byte in running-status
				}
			}

			curr_mtrk_event_idx.push_back({j,curr_event_length,curr_event.type,prev_midi_status_byte});
			j += curr_event_length;
		}
		this->mtrk_event_idx_.push_back(curr_mtrk_event_idx);
		i+=this->chunk_idx_.back().length;
	}  // to next midi_chunk (Track chunk)

}


std::string midi_file::print() const {
	std::string s {};




	return s;
}


std::string midi_file::print_mthd() const {
	std::string s {};

	midi_file_header_data header_data 
		= read_midi_fheaderchunk(&(this->fdata_[chunk_idx_[0].offset]));

	s += ("Format type = " + std::to_string(header_data.fmt_type) + "    \n");
	s += ("Num Tracks = " + std::to_string(header_data.num_trks) + "    \n");
	s += ("Time Division = " + std::to_string(header_data.time_div) + "    \n");

	return s;
}

std::string midi_file::print_mtrk_seq() const {
	std::string s {};

	for (int trkn=0; trkn<mtrk_event_idx_.size(); ++trkn) {
		s += ("Track " + std::to_string(trkn) + "\n");
		for (int evntn=0; evntn<mtrk_event_idx_[trkn].size(); ++evntn) {
			auto curr_offset = mtrk_event_idx_[trkn][evntn].offset;
			auto curr_dptr = &(this->fdata_[curr_offset]);
			
			s += ("EvntNum = " + std::to_string(evntn) + "; ");
			s += ("File Offset = " + std::to_string(curr_offset) + "; ");
			s += ("Length = " + std::to_string(mtrk_event_idx_[trkn][evntn].length) + '\n');
			s += ('\t' + print_hexascii(curr_dptr,mtrk_event_idx_[trkn][evntn].length) + '\n');

			if (mtrk_event_idx_[trkn][evntn].type == mtrk_event_t::midi) {
				midi_event md = parse_midi_event(curr_dptr, mtrk_event_idx_[trkn][evntn].midi_status);
				s += ('\t' + print_midi_event(md));
			} else if (mtrk_event_idx_[trkn][evntn].type == mtrk_event_t::meta) {
				meta_event mt = parse_meta_event(curr_dptr);
				s += ('\t' + print_meta_event(mt));
			} else if (mtrk_event_idx_[trkn][evntn].type == mtrk_event_t::sysex) {
				sysex_event sx = parse_sysex_event(curr_dptr);
				s += ('\t' + print_sysex_event(sx));
			} else if (mtrk_event_idx_[trkn][evntn].type == mtrk_event_t::unknown) {
				s += ('\t' + "mtrk_event_t::unknown");
			}
			s += "\n";
		}
		s += ("End of track " + std::to_string(trkn) + "--------------------------\n");
	}

	return s;
}


std::string print_midi_event(const midi_event& md) {
	std::string s {};
	std::string sep {"  "};
	s += "MIDI:  ";
	s += ("delta-t = " + std::to_string(md.delta_t.val) + sep);

	std::string status_str {};
	status_byte_type sb = classify_status_byte(&(md.status));
	if (sb == status_byte_type::channel) {
		status_str += ("ch:" + std::to_string((md.status & 0x0F)+1) + " ");
		ch_msg_type chsb = classify_channel_status_byte(&(md.status));
		if (chsb == ch_msg_type::voice) {
			status_str += "voice:";
			if ((md.status & 0xF0) == 0x80) {
				status_str += "Note-off";
			} else if ((md.status & 0xF0) == 0x90) {
				status_str += "Note-on";
			} else if ((md.status & 0xF0) == 0xA0) {
				status_str += "Aftertouch/Key-pressure";
			} else if ((md.status & 0xF0) == 0xB0) {
				status_str += "Control-change";
			} else if ((md.status & 0xF0) == 0xC0) {
				status_str += "Program-change";
			} else if ((md.status & 0xF0) == 0xD0) {
				status_str += "Aftertouch/Channel-pressure";
			} else if ((md.status & 0xF0) == 0xE0) {
				status_str += "Pitch-bend-change";
			} else {
				status_str += "Unknown-channel-voice-status-byte";
			}
		} else if (chsb == ch_msg_type::mode) {
			status_str += "mode:";
			if ((md.status & 0xF0) == 0xB0) {
				status_str += "Select-channel-mode";
			} else {
				status_str += "Unknown-channel-mode-status-byte";
			}
		}
	} else if (sb == status_byte_type::system) {
		status_str += "sy:";
		sys_msg_type sysb = classify_system_status_byte(&(md.status));
		if (sysb == sys_msg_type::exclusive) {
			status_str += "ex";
		} else if (sysb == sys_msg_type::common) {
			status_str += "cm";
		} else if (sysb == sys_msg_type::realtime) {
			status_str += "rt";
		} else if (sysb == sys_msg_type::unknown) {
			status_str += "uk";
		}
	} else if (sb == status_byte_type::unknown) {
		status_str += "uk:";
		status_str += "_____";
	}
	s += "status-byte = ";
	if (md.running_status) {
		s += ("(Running)  [" + status_str + ']');
	} else {
		s += ("(Explicit)  [" + status_str + ']');
	}
	s += sep;

	s += ("p1 = " + std::to_string(md.p1));
	if (midi_event_num_data_bytes(md) == 2) {
		s += (sep + "p2 = " + std::to_string(md.p2));
	}

	return s;
}

std::string print_meta_event(const meta_event& mt) {
	std::string s {};
	std::string sep {"    "};
	s += "META:  ";
	s += ("delta-t=" + std::to_string(mt.delta_t.val) + sep);

	auto print_data = [](const std::vector<unsigned char>& data) -> std::string {
		std::string s {};
		for (const auto& e : data) {
			s.push_back(static_cast<char>(e));
		}
		return s;
	};

	std::string event_type {"Type = "};
	if (mt.type == 0x00) {
		event_type += "Sequence-Number";
	} else if (mt.type == 0x01) {
		event_type += "Text-Event";
	} else if (mt.type == 0x02) {
		event_type += "Copyright Notice";
	} else if (mt.type == 0x03) {
		event_type += "Sequence/Track Name";
	} else if (mt.type == 0x04) {
		event_type += "Instrument Name";
	} else if (mt.type == 0x05) {
		event_type += "Lyric";
	} else if (mt.type == 0x06) {
		event_type += "Marker";
	} else if (mt.type == 0x07) {
		event_type += "Cue Point";
	} else if (mt.type == 0x20) {
		event_type += "MIDI Channel Prefix";
	} else if (mt.type == 0x2F) {
		event_type += "End-of-Track";
	} else if (mt.type == 0x51) {
		event_type += "Set-Tempo";
	} else if (mt.type == 0x54) {
		event_type += "SMPTE Offset";
	} else if (mt.type == 0x58) {
		event_type += "Time-Signature";
	} else if (mt.type == 0x59) {
		event_type += "Key-Signature";
	} else if (mt.type == 0x7F) {
		event_type += "Sequencer-Specific Meta-Event";
	} else {
		event_type += "Not-Recognized";
	}
	s += (event_type + sep);

	s += ("Length = " + std::to_string(mt.length.val) + sep);
	s += ("Data = " + print_data(mt.data));
	
	return s;
}

std::string print_sysex_event(const sysex_event& sx) {
	std::string s {"SYSEX:  "};

	return s;
}
*/



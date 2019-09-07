#include "aux_types.h"
#include "midi_status_byte.h"
#include "midi_delta_time.h"
#include "midi_vlq.h"
#include <string>
#include <cstdint>
#include <algorithm>



jmid::delta_time::delta_time() {
	this->d_ = 0;
}
jmid::delta_time::delta_time(std::int32_t dt) {
	this->d_ = jmid::to_nearest_valid_delta_time(dt);
}
const std::int32_t& jmid::delta_time::get() const {
	return this->d_;
}




jmid::meta_header_data::operator bool() const {
	return (jmid::is_meta_status_byte(this->type) 
		&& jmid::is_valid_vlq(this->length));
}
bool jmid::operator==(const jmid::meta_header_data& lhs,
						const jmid::meta_header_data& rhs) {
	return ((lhs.length == rhs.length) && (lhs.type == rhs.type));
}
bool jmid::operator!=(const jmid::meta_header_data& lhs,
						const jmid::meta_header_data& rhs) {
	return !(lhs == rhs);
}

jmid::meta_header::meta_header() {
	this->d_.type = 0x01u;
	this->d_.length = 0;
}
jmid::meta_header::meta_header(std::uint8_t type, std::int32_t len) {
	this->set_type(type);
	this->d_.length = jmid::to_nearest_valid_vlq(len);
}
jmid::meta_header::meta_header(std::uint8_t type, std::uint64_t len) {
	this->set_type(type);
	this->d_.length = jmid::to_nearest_valid_vlq(len);
}
jmid::meta_header::meta_header(std::uint8_t type, std::int64_t len) {
	this->set_type(type);
	this->d_.length = jmid::to_nearest_valid_vlq(len);
}
jmid::meta_header::meta_header(meta_header_data mt) {
	this->set_type(mt.type);
	this->d_.length = jmid::to_nearest_valid_vlq(mt.length);
}
jmid::meta_header_data jmid::meta_header::get() const {
	return this->d_;
}
std::int32_t jmid::meta_header::length() const {
	return this->d_.length;
}
std::uint8_t jmid::meta_header::type_byte() const {
	return this->d_.type;
}
std::int32_t jmid::meta_header::set_length(std::int32_t len) {
	this->d_.length = jmid::to_nearest_valid_vlq(len);
	return this->d_.length;
}
std::uint8_t jmid::meta_header::set_type(std::uint8_t type) {
	// From the MIDI Std. p. 136:
	// All meta-events begin with FF, then have an event type byte (which 
	// is always less than 128), and then...
	if (!jmid::is_meta_type_byte(type)) {
		type = 127;
	}
	this->d_.type = type;
	return this->d_.type;
}
bool jmid::operator==(const jmid::meta_header& lhs,
						const jmid::meta_header& rhs) {
	return (lhs.d_ == rhs.d_);
}
bool jmid::operator!=(const jmid::meta_header& lhs,
						const jmid::meta_header& rhs) {
	return !(lhs == rhs);
}



jmid::sysex_header_data::operator bool() const {
	return (jmid::is_sysex_status_byte(this->type) 
		&& jmid::is_valid_vlq(this->length));
}
bool jmid::operator==(const jmid::sysex_header_data& lhs,
						const jmid::sysex_header_data& rhs) {
	return ((lhs.length == rhs.length) && (lhs.type == rhs.type));
}
bool jmid::operator!=(const jmid::sysex_header_data& lhs,
						const jmid::sysex_header_data& rhs) {
	return !(lhs == rhs);
}
jmid::sysex_header::sysex_header() {
	this->d_.type=0xF7u;
	this->d_.length = 0;
}
jmid::sysex_header::sysex_header(std::uint8_t type, std::int32_t len) {
	this->set_type(type);
	this->d_.length = jmid::to_nearest_valid_vlq(len);
}
jmid::sysex_header::sysex_header(std::uint8_t type, std::uint64_t len) {
	this->set_type(type);
	this->d_.length = jmid::to_nearest_valid_vlq(len);
}
jmid::sysex_header::sysex_header(std::uint8_t type, std::int64_t len) {
	this->set_type(type);
	this->d_.length = jmid::to_nearest_valid_vlq(len);
}
jmid::sysex_header::sysex_header(sysex_header_data sxd) {
	this->set_type(sxd.type);
	this->d_.length = jmid::to_nearest_valid_vlq(sxd.length);
}
jmid::sysex_header_data jmid::sysex_header::get() const {
	return this->d_;
}
std::int32_t jmid::sysex_header::length() const {
	return this->d_.length;
}
std::uint8_t jmid::sysex_header::type_byte() const {
	return this->d_.type;
}
std::uint8_t jmid::sysex_header::set_type(std::uint8_t type) {
	if (jmid::is_sysex_status_byte(type)) {
		this->d_.type = type;
	} else {
		this->d_.type = 0xF7u;
	}
	return this->d_.type;
}
std::int32_t jmid::sysex_header::set_length(std::int32_t len) {
	this->d_.length = jmid::to_nearest_valid_vlq(len);
	return this->d_.length;
}
bool jmid::operator==(const jmid::sysex_header& lhs,
						const jmid::sysex_header& rhs) {
	return (lhs.d_ == rhs.d_);
}
bool jmid::operator!=(const jmid::sysex_header& lhs,
						const jmid::sysex_header& rhs) {
	return !(lhs == rhs);
}

bool jmid::operator==(const jmid::midi_timesig_t& rhs, 
						const jmid::midi_timesig_t& lhs) {
	return ((rhs.clckspclk == lhs.clckspclk)
		&& (rhs.log2denom == lhs.log2denom)
		&& (rhs.ntd32pq == lhs.ntd32pq)
		&& (rhs.num == lhs.num));
}
bool jmid::operator!=(const jmid::midi_timesig_t& rhs, 
						const jmid::midi_timesig_t& lhs) {
	return !(rhs==lhs);
}

std::int8_t jmid::nsharps(const jmid::midi_keysig_t& ks) {
	return (ks.sf >= 0) ? ks.sf : 0;
}
std::int8_t jmid::nflats(const jmid::midi_keysig_t& ks) {
	return (ks.sf <= 0) ? (-1)*ks.sf : 0;
}
bool jmid::is_major(const jmid::midi_keysig_t& ks) {
	return ks.mi==0;
}
bool jmid::is_minor(const jmid::midi_keysig_t& ks) {
	return ks.mi==1;
}
jmid::ch_event::ch_event(const jmid::ch_event_data_t& md) {
	this->d_ = jmid::normalize(md);
}
jmid::ch_event::ch_event(int sn, int ch,
												int p1, int p2) {
	this->d_ = jmid::make_midi_ch_event_data(sn,ch,p1,p2);
}
jmid::ch_event_data_t jmid::ch_event::get() const {
	return this->d_;
}
std::uint8_t jmid::ch_event::status_nybble() const {
	return this->d_.status_nybble;
}
std::uint8_t jmid::ch_event::ch() const {
	return this->d_.ch;
}
std::uint8_t jmid::ch_event::p1() const {
	return this->d_.p1;
}
std::uint8_t jmid::ch_event::p2() const {
	return this->d_.p2;
}
jmid::ch_event_data_t jmid::make_midi_ch_event_data(int sn, int ch,
													int p1, int p2) {
	jmid::ch_event_data_t md;
	md.status_nybble = 0xF0u & static_cast<std::uint8_t>(std::clamp(sn,0x80,0xF0));
	md.ch = std::clamp(ch,0,15);
	md.p1 = std::clamp(p1,0,0x7F);
	md.p2 = std::clamp(p2,0,0x7F);
	return md;
}
std::string jmid::print(const jmid::ch_event_data_t& md) {
	std::string s;
	if (!md) {
		s += "Invalid";
		return s;
	}

	switch ((md.status_nybble)&0xF0u) {
	case 0x80u:
		s += "Note-off";
		break;
	case 0x90u:  // TODO:  Handle note-off w/ v==0
		s += "Note-on";
		break;
	case 0xA0u:
		s += "Key pressure";
		break;
	case 0xB0u:
		if (jmid::p1_implies_channel_mode_msg(md.p1)) {
			s += "Channel mode";
		} else {
			s += "Control change";
		}
		break;
	case 0xC0u:
		s += "Program change";
		break;
	case 0xD0u:
		s += "Channel pressure";
		break;
	case 0xE0u:
		s += "Pitch bend";
		break;
	default:
		s += "Invalid";
		return s;
	}

	s += ":  ch=" + std::to_string(md.p1-1) + ", p1=" + std::to_string((md.p1));
	if (channel_status_byte_n_data_bytes((md.status_nybble)|(md.ch)) == 2) {
		s += ", p2=" + std::to_string(md.p2);
	}

	return s;
}

bool jmid::verify(const jmid::ch_event_data_t& md) {
	auto s = ((md.status_nybble & 0xF0u) + (md.ch & 0x0Fu));
	if (!jmid::is_channel_status_byte(s)) {
		return false;
	}
	if (md.ch > 0x0Fu) {
		return false;
	}
	if (!jmid::is_data_byte(md.p1)) {
		return false;
	}
	if ((jmid::channel_status_byte_n_data_bytes(s)==2) 
			&& !jmid::is_data_byte(md.p2)) {
		return false;
	}
	return true;
}
jmid::ch_event_data_t::operator bool() const {
	return jmid::verify(*this);
}

jmid::ch_event_data_t jmid::normalize(jmid::ch_event_data_t md) {
	md.status_nybble &= 0xF0u;
	md.status_nybble = std::clamp(md.status_nybble,
		std::uint8_t{0x80},std::uint8_t{0xE0u});
	// I use std::clamp() instead of masking or std::max() b/c it works w/
	// signed types and I may want to make these data members signed in the
	// future.  
	md.ch = std::clamp(md.ch,std::uint8_t{0},std::uint8_t{15});
	md.p1 = std::clamp(md.p1,std::uint8_t{0},std::uint8_t{0x7F});
	md.p2 = std::clamp(md.p2,std::uint8_t{0},std::uint8_t{0x7F});
	return md;
}
bool jmid::is_note_on(const jmid::ch_event_data_t& md) {
	return (jmid::verify(md)
		&& md.status_nybble==0x90u
		&& (md.p2 > 0));
}
bool jmid::is_note_off(const jmid::ch_event_data_t& md) {
	return (jmid::verify(md)
		&& ((md.status_nybble==0x80u)
			|| (md.status_nybble==0x90u && (md.p2==0))));
}
bool jmid::is_key_pressure(const jmid::ch_event_data_t& md) {  // 0xAnu
	return (jmid::verify(md)
		&& (md.status_nybble==0xA0u));
}
bool jmid::is_control_change(const jmid::ch_event_data_t& md) {
	return (jmid::verify(md)
		&& (md.status_nybble==0xB0u)
		&& (md.p1 < 120));
	// The final condition verifies md is a channel_voice (not channel_mode)
	// event.  120  == 0b01111000u
}
bool jmid::is_program_change(const jmid::ch_event_data_t& md) {
	return (jmid::verify(md)
		&& (md.status_nybble==0xC0u));
}
bool jmid::is_channel_pressure(const jmid::ch_event_data_t& md) {  // 0xDnu
	return (jmid::verify(md)
		&& (md.status_nybble==0xD0u));
}
bool jmid::is_pitch_bend(const jmid::ch_event_data_t& md) {
	return (jmid::verify(md)
		&& (md.status_nybble==0xE0u));
}
bool jmid::is_channel_mode(const jmid::ch_event_data_t& md) {
	return (verify(md)
		&& (md.status_nybble==0xB0u)
		&& (md.p1 >= 120));
	// The final condition verifies md is not a control_change event.  
	// 120  == 0b01111000u
}



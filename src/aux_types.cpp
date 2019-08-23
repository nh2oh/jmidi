#include "aux_types.h"
#include "midi_status_byte.h"
#include <string>
#include <cstdint>
#include <algorithm>




bool jmid::operator==(const jmid::midi_timesig_t& rhs, const jmid::midi_timesig_t& lhs) {
	return ((rhs.clckspclk == lhs.clckspclk)
		&& (rhs.log2denom == lhs.log2denom)
		&& (rhs.ntd32pq == lhs.ntd32pq)
		&& (rhs.num == lhs.num));
}
bool jmid::operator!=(const jmid::midi_timesig_t& rhs, const jmid::midi_timesig_t& lhs) {
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
jmid::ch_event_data_t jmid::make_midi_ch_event_data(int sn, int ch,
												int p1, int p2) {
	jmid::ch_event_data_t md;
	md.status_nybble = 0xF0u & static_cast<std::uint8_t>(std::clamp(sn,0x80,0xF0));
	md.ch = std::clamp(ch,0,15);
	md.p1 = std::clamp(p1,0,0x7F);
	md.p2 = std::clamp(p2,0,0x7F);
	return md;
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



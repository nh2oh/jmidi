#include "aux_types.h"
#include "midi_status_byte.h"
#include <string>
#include <cstdint>
#include <algorithm>




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
// TODO:  Dumb, gross, etc
ch_event_data_t make_midi_ch_event_data(int sn, int ch, int p1, int p2) {
	ch_event_data_t md;
	md.status_nybble = 0xF0u & static_cast<uint8_t>(std::clamp(sn,0x80,0xF0));
	md.ch = std::clamp(ch,0,15);
	md.p1 = std::clamp(p1,0,0x7F);
	md.p2 = std::clamp(p2,0,0x7F);
	return md;
}
bool verify(const ch_event_data_t& md) {
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
ch_event_data_t::operator bool() const {
	return verify(*this);
}

ch_event_data_t normalize(ch_event_data_t md) {
	md.status_nybble &= 0xF0u;
	md.status_nybble = std::clamp(md.status_nybble,
		uint8_t{0x80},uint8_t{0xE0u});
	md.status_nybble = 0xF0u&(0x80u|(md.status_nybble));  // TODO:  No longer does anything?...
	// std::clamp() over std::max() b/c i may want to make these data 
	// members signed at some point.  
	md.ch = std::clamp(md.ch,uint8_t{0},uint8_t{15});
	md.p1 = std::clamp(md.p1,uint8_t{0},uint8_t{0x7F});
	md.p2 = std::clamp(md.p2,uint8_t{0},uint8_t{0x7F});
	//md.ch &= 0x0Fu; // std::max(md.ch,15);
	//md.p1 &= 0x7Fu; 
	//md.p2 &= 0x7Fu; 
	return md;
}
bool is_note_on(const ch_event_data_t& md) {
	return (verify(md)
		&& md.status_nybble==0x90u
		&& (md.p2 > 0));
}
bool is_note_off(const ch_event_data_t& md) {
	return (verify(md)
		&& ((md.status_nybble==0x80u)
			|| (md.status_nybble==0x90u && (md.p2==0))));
}
bool is_key_pressure(const ch_event_data_t& md) {  // 0xAnu
	return (verify(md)
		&& (md.status_nybble==0xA0u));
}
bool is_control_change(const ch_event_data_t& md) {
	return (verify(md)
		&& (md.status_nybble==0xB0u)
		&& (md.p1 < 120));
	// The final condition verifies md is a channel_voice (not channel_mode)
	// event.  120  == 0b01111000u
}
bool is_program_change(const ch_event_data_t& md) {
	return (verify(md)
		&& (md.status_nybble==0xC0u));
}
bool is_channel_pressure(const ch_event_data_t& md) {  // 0xDnu
	return (verify(md)
		&& (md.status_nybble==0xD0u));
}
bool is_pitch_bend(const ch_event_data_t& md) {
	return (verify(md)
		&& (md.status_nybble==0xE0u));
}
bool is_channel_mode(const ch_event_data_t& md) {
	return (verify(md)
		&& (md.status_nybble==0xB0u)
		&& (md.p1 >= 120));
	// The final condition verifies md is not a control_change event.  
	// 120  == 0b01111000u
}



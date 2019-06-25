#include "mtrk_event_methods.h"
#include "mtrk_event_t2.h"
#include "mtrk_event_iterator_t2.h"
#include "midi_raw.h"
#include "midi_vlq.h"
#include "dbklib\byte_manipulation.h"
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>



meta_event_t classify_meta_event_impl(const uint16_t& d16) {
	if (d16==static_cast<uint16_t>(meta_event_t::seqn)) {
		return meta_event_t::seqn;
	} else if (d16==static_cast<uint16_t>(meta_event_t::text)) {
		return meta_event_t::text;
	} else if (d16==static_cast<uint16_t>(meta_event_t::copyright)) {
		return meta_event_t::copyright;
	} else if (d16==static_cast<uint16_t>(meta_event_t::trackname)) {
		return meta_event_t::trackname;
	} else if (d16==static_cast<uint16_t>(meta_event_t::instname)) {
		return meta_event_t::instname;
	} else if (d16==static_cast<uint16_t>(meta_event_t::lyric)) {
		return meta_event_t::lyric;
	} else if (d16==static_cast<uint16_t>(meta_event_t::marker)) {
		return meta_event_t::marker;
	} else if (d16==static_cast<uint16_t>(meta_event_t::cuepoint)) {
		return meta_event_t::cuepoint;
	} else if (d16==static_cast<uint16_t>(meta_event_t::chprefix)) {
		return meta_event_t::chprefix;
	} else if (d16==static_cast<uint16_t>(meta_event_t::eot)) {
		return meta_event_t::eot;
	} else if (d16==static_cast<uint16_t>(meta_event_t::tempo)) {
		return meta_event_t::tempo;
	} else if (d16==static_cast<uint16_t>(meta_event_t::smpteoffset)) {
		return meta_event_t::smpteoffset;
	} else if (d16==static_cast<uint16_t>(meta_event_t::timesig)) {
		return meta_event_t::timesig;
	} else if (d16==static_cast<uint16_t>(meta_event_t::keysig)) {
		return meta_event_t::keysig;
	} else if (d16==static_cast<uint16_t>(meta_event_t::seqspecific)) {
		return meta_event_t::seqspecific;
	} else {
		if ((d16&0xFF00u)==0xFF00u) {
			return meta_event_t::unknown;
		} else {
			return meta_event_t::invalid;
		}
	}
}
bool meta_hastext_impl(const uint16_t& t) {
	auto mttype = classify_meta_event_impl(t);
	return (mttype==meta_event_t::text 
			|| mttype==meta_event_t::copyright
			|| mttype==meta_event_t::instname
			|| mttype==meta_event_t::trackname
			|| mttype==meta_event_t::lyric
			|| mttype==meta_event_t::marker
			|| mttype==meta_event_t::cuepoint);
}

std::string print(const meta_event_t& mt) {
	if (mt==meta_event_t::seqn) {
		return "seqn";
	} else if (mt==meta_event_t::text) {
		return "text";
	} else if (mt==meta_event_t::copyright) {
		return "copyright";
	} else if (mt==meta_event_t::trackname) {
		return "trackname";
	} else if (mt==meta_event_t::instname) {
		return "instname";
	} else if (mt==meta_event_t::lyric) {
		return "lyric";
	} else if (mt==meta_event_t::marker) {
		return "marker";
	} else if (mt==meta_event_t::cuepoint) {
		return "cuepoint";
	} else if (mt==meta_event_t::chprefix) {
		return "chprefix";
	} else if (mt==meta_event_t::eot) {
		return "eot";
	} else if (mt==meta_event_t::tempo) {
		return "tempo";
	} else if (mt==meta_event_t::smpteoffset) {
		return "smpteoffset";
	} else if (mt==meta_event_t::timesig) {
		return "timesig";
	} else if (mt==meta_event_t::keysig) {
		return "keysig";
	} else if (mt==meta_event_t::seqspecific) {
		return "seqspecific";
	} else if (mt==meta_event_t::invalid) {
		return "invalid";
	} else if (mt==meta_event_t::unknown) {
		return "unknown";
	} else {
		return "?";
	}
}

meta_event_t classify_meta_event(const mtrk_event_t2& ev) {
	if (ev.type()!=smf_event_type::meta) {
		return meta_event_t::invalid;
	}
	auto it = ev.event_begin();
	//auto p = ev.data_skipdt();
	uint16_t d16 = dbk::be_2_native<uint16_t>(&*it); // (p);
	return classify_meta_event_impl(d16);
}
bool is_meta(const mtrk_event_t2& ev) {
	return ev.type()==smf_event_type::meta;
}
bool is_meta(const mtrk_event_t2& ev, const meta_event_t& mtype) {
	if (ev.type()==smf_event_type::meta) {
		auto it = ev.event_begin();
		return classify_meta_event_impl(dbk::be_2_native<uint16_t>(&*it))==mtype;
	}
	return false;
}
bool is_seqn(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::seqn);
}
bool is_text(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::text);
}
bool is_copyright(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::copyright);
}
bool is_trackname(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::trackname);
}
bool is_instname(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::instname);
}
bool is_lyric(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::lyric);
}
bool is_marker(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::marker);
}
bool is_cuepoint(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::cuepoint);
}
bool is_chprefix(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::chprefix);
}
bool is_eot(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::eot);
}
bool is_tempo(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::tempo);
}
bool is_smpteoffset(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::smpteoffset);
}
bool is_timesig(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::timesig);
}
bool is_keysig(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::keysig);
}
bool is_seqspecific(const mtrk_event_t2& ev) {
	return is_meta(ev, meta_event_t::seqspecific);
}

bool meta_has_text(const mtrk_event_t2& ev) {
	auto mttype = classify_meta_event(ev);
	return meta_hastext_impl(static_cast<uint16_t>(mttype));
}

std::string meta_generic_gettext(const mtrk_event_t2& ev) {
	std::string s {};
	if (!meta_has_text(ev)) {
		return s;
	}

	s.reserve()
	s = ev.text_payload();
	return s;
}

uint32_t get_tempo(const mtrk_event_t2& ev, uint32_t def) {
	if (!is_tempo(ev)) {
		return def;
	}
	return ev.uint32_payload();
}

midi_timesig_t get_timesig(const mtrk_event_t2& ev, midi_timesig_t def) {
	if (!is_timesig(ev)) {
		return def;
	}
	auto it = ev.payload_begin();
	if ((ev.end()-it) != 4) {
		return def;
	}
	midi_timesig_t result {};
	result.num = *it++;
	result.log2denom = *it++;
	result.clckspclk = *it++;
	result.ntd32pq = *it++;
	return result;
}
midi_keysig_t get_keysig(const mtrk_event_t2& ev, midi_keysig_t def) {
	if (!is_keysig(ev)) {
		return def;
	}
	auto it = ev.payload_begin();
	if ((ev.end()-it) != 2) {
		return def;
	}
	midi_keysig_t result {};
	result.sf = *it++;
	result.mi = *it++;
	return result;
}
std::vector<unsigned char> get_seqspecific(const mtrk_event_t2& ev) {
	auto v = std::vector<unsigned char>();
	return get_seqspecific(ev,v);
}
std::vector<unsigned char> get_seqspecific(const mtrk_event_t2& ev, 
								std::vector<unsigned char>& data) {
	if (!is_seqspecific(ev)) {
		return data;
	}
	data.clear();
	for (auto it=ev.payload_begin(); it!=ev.end(); ++it) {
		data.push_back(*it);
	}
	return data;
}
mtrk_event_t2 make_seqn(const uint32_t& dt, const uint16_t& seqn) {
	std::array<unsigned char,5> evdata {0xFFu,0x00u,0x02u,0x00u,0x00u};
	write_16bit_be(seqn, evdata.begin()+3);
	auto result = mtrk_event_t2(dt,evdata.data(),evdata.size(),0x00u);
	return result;
}
mtrk_event_t2 make_chprefix(const uint32_t& dt, const uint8_t& ch) {
	std::array<unsigned char,4> evdata {0xFFu,0x20u,0x01u,0x00u};
	write_bytes(ch, evdata.begin()+3);
	auto result = mtrk_event_t2(dt,evdata.data(),evdata.size(),0x00u);
	return result;
}
mtrk_event_t2 make_tempo(const uint32_t& dt, const uint32_t& uspqn) {
	std::array<unsigned char,6> evdata {0xFFu,0x51u,0x03u,0x00u,0x00u,0x00u};
	write_24bit_be((uspqn>0xFFFFFFu ? 0xFFFFFFu : uspqn), evdata.begin()+3);
	auto result = mtrk_event_t2(dt,evdata.data(),evdata.size(),0x00u);
	return result;
}
mtrk_event_t2 make_eot(const uint32_t& dt) {
	std::array<unsigned char,3> evdata {0xFFu,0x2Fu,0x00u};
	auto result = mtrk_event_t2(dt,evdata.data(),evdata.size(),0x00u);
	return result;
}
mtrk_event_t2 make_timesig(const uint32_t& dt, const midi_timesig_t& ts) {
	std::array<unsigned char,7> evdata {0xFFu,0x58u,0x04u,
		ts.num,ts.log2denom,ts.clckspclk,ts.ntd32pq};
	auto result = mtrk_event_t2(dt,evdata.data(),evdata.size(),0x00u);
	return result;
}
mtrk_event_t2 make_instname(const uint32_t& dt, const std::string& s) {
	return make_meta_generic_text(dt,meta_event_t::instname,s);
}
mtrk_event_t2 make_trackname(const uint32_t& dt, const std::string& s) {
	return make_meta_generic_text(dt,meta_event_t::trackname,s);
}
mtrk_event_t2 make_lyric(const uint32_t& dt, const std::string& s) {
	return make_meta_generic_text(dt,meta_event_t::lyric,s);
}
mtrk_event_t2 make_marker(const uint32_t& dt, const std::string& s) {
	return make_meta_generic_text(dt,meta_event_t::marker,s);
}
mtrk_event_t2 make_cuepoint(const uint32_t& dt, const std::string& s) {
	return make_meta_generic_text(dt,meta_event_t::cuepoint,s);
}
mtrk_event_t2 make_text(const uint32_t& dt, const std::string& s) {
	return make_meta_generic_text(dt,meta_event_t::text,s);
}
mtrk_event_t2 make_copyright(const uint32_t& dt, const std::string& s) {
	return make_meta_generic_text(dt,meta_event_t::copyright,s);
}
mtrk_event_t2 make_meta_generic_text(const uint32_t& dt, const meta_event_t& type, 
									const std::string& s) {
	auto type_int16 = static_cast<uint16_t>(type);  // TODO:  Gross
	if (!meta_hastext_impl(type_int16)) {
		// TODO:  This probably breaks nrvo?
		return mtrk_event_t2();
	}
	std::vector<unsigned char> evdata;
	evdata.reserve(sizeof(mtrk_event_t2));
	auto it = midi_write_vl_field(std::back_inserter(evdata),dt);
	evdata.push_back(0xFFu);
	evdata.push_back(static_cast<uint8_t>(type_int16&0x00FFu));
	it = midi_write_vl_field(std::back_inserter(evdata),s.size());
	std::copy(s.begin(),s.end(),std::back_inserter(evdata));
	auto result = mtrk_event_t2(evdata.data(),evdata.size(),0x00u);
	return result;
}







bool is_channel(const mtrk_event_t2& ev) {  // voice or mode
	auto s = ev.status_byte();
	return ((s&0x80u)==0x80u && (s&0xF0u)!=0xF0u);
}
bool is_channel_voice(const mtrk_event_t2& ev) {
	auto md = ev.midi_data();
	if (!md.is_valid || (md.status_nybble == 0xB0u)) {
		return false;
	}
	return (((md.p1>>3)&0x0Fu)!=0x0Fu);
}
bool is_channel_mode(const mtrk_event_t2& ev) {
	auto md = ev.midi_data();
	if (!md.is_valid || (md.status_nybble != 0xB0u)) {
		return false;
	}
	return (((md.p1>>3)&0x0Fu)==0x0Fu);
}
bool is_note_on(const mtrk_event_t2& ev) {
	auto md = ev.midi_data();
	return (md.is_valid 
		&& md.status_nybble==0x90u && (md.p2 > 0));
}
bool is_note_off(const mtrk_event_t2& ev) {
	auto md = ev.midi_data();
	return (md.is_valid && 
		((md.status_nybble==0x80u)
			|| (md.status_nybble==0x90u && (md.p2==0))));
}
bool is_key_aftertouch(const mtrk_event_t2& ev) {
	return ((ev.status_byte() & 0xF0u) == 0xA0u);
}
bool is_control_change(const mtrk_event_t2& ev) {
	return (((ev.status_byte()&0xF0u)==0xB0u) 
		&& is_channel_voice(ev));
}
bool is_program_change(const mtrk_event_t2& ev) {
	return ((ev.status_byte() & 0xF0u) == 0xC0u);
}
bool is_channel_aftertouch(const mtrk_event_t2& ev) {
	return ((ev.status_byte() & 0xF0u) == 0xD0u);
}
bool is_pitch_bend(const mtrk_event_t2& ev) {
	return ((ev.status_byte() & 0xF0u) == 0xE0u);
}
bool is_onoff_pair(const mtrk_event_t2& on, const mtrk_event_t2& off) {
	if (!is_note_on(on) || !is_note_off(off)) {
		return false;
	}
	auto on_md = on.midi_data();
	auto off_md = off.midi_data();
	return is_onoff_pair(on_md.ch,on_md.p1,off_md.ch,off_md.p1);
}
bool is_onoff_pair(int on_ch, int on_note, const mtrk_event_t2& off) {
	if (!is_note_off(off)) {
		return false;
	}
	auto off_md = off.midi_data();
	return is_onoff_pair(on_ch,on_note,off_md.ch,off_md.p1);
}
bool is_onoff_pair(int on_ch, int on_note, int off_ch, int off_note) {
	return (on_ch==off_ch && on_note==off_note);
}

midi_ch_event_t get_channel_event(const mtrk_event_t2& ev, midi_ch_event_t def) {
	if (!is_channel(ev)) {
		return def;
	}
	auto md = ev.midi_data();
	midi_ch_event_t result {};
	result.status_nybble = md.status_nybble;
	result.ch = md.ch;
	result.p1 = md.p1;
	result.p2 = md.p2;
	return result;
}

mtrk_event_t2 make_ch_event_generic_unsafe(const uint32_t& dt, const midi_ch_event_t& md) {
	std::array<unsigned char,3> evdata {(md.status_nybble|md.ch),md.p1,md.p2};
	auto result = mtrk_event_t2(dt,evdata.data(),evdata.size(),0x00u);
	return result;
}
mtrk_event_t2 make_note_on(const uint32_t& dt, midi_ch_event_t md) {
	md = normalize(md);
	md.status_nybble = 0x90u;
	md.p2 = md.p2 > 0 ? md.p2 : 1;
	return make_ch_event_generic_unsafe(dt,md);
}
mtrk_event_t2 make_note_off(const uint32_t& dt, midi_ch_event_t md) {
	md = normalize(md);
	md.status_nybble = 0x80u;
	return make_ch_event_generic_unsafe(dt,md);
}
mtrk_event_t2 make_note_off90(const uint32_t& dt, midi_ch_event_t md) {
	md = normalize(md);
	md.status_nybble = 0x90u;
	md.p2 = 0;
	return make_ch_event_generic_unsafe(dt,md);
}


#include "mtrk_event_methods.h"
#include "mtrk_event_t.h"
#include "midi_raw.h"
#include "midi_vlq.h"
#include "dbklib\byte_manipulation.h"
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>




std::string print(const mtrk_event_t& evnt, mtrk_sbo_print_opts opts) {
	std::string s {};
	s += ("delta_time = " + std::to_string(evnt.delta_time()) + ", ");
	s += ("type = " + print(evnt.type()) + ", ");
	s += ("size = " + std::to_string(evnt.size()) + ", ");
	s += ("data_size = " + std::to_string(evnt.data_size()) + "\n");
	
	s += "\t[";
	dbk::print_hexascii(evnt.dt_begin(),evnt.dt_end(),std::back_inserter(s),'\0',' ');
	s += "] ";
	dbk::print_hexascii(evnt.event_begin(),evnt.end(),std::back_inserter(s),'\0',' ');
	
	if (opts == mtrk_sbo_print_opts::detail || opts == mtrk_sbo_print_opts::debug) {
		if (evnt.type()==smf_event_type::meta) {
			s += "\n";
			s += ("\tmeta type: " + print(classify_meta_event(evnt)) + "; ");
			if (meta_has_text(evnt)) {
				s += ("text payload: \"" + meta_generic_gettext(evnt) + "\"; ");
			} else if (is_tempo(evnt)) {
				s += ("value = " + std::to_string(get_tempo(evnt)) + " us/q; ");
			} else if (is_timesig(evnt)) {
				auto data = get_timesig(evnt);
				s += ("value = " + std::to_string(data.num) + "/" 
					+ std::to_string(static_cast<int>(std::exp2(data.log2denom)))
					+ ", " + std::to_string(data.clckspclk) + " MIDI clocks/click, "
					+ std::to_string(data.ntd32pq) + " notated 32'nd nts / MIDI q nt; ");
			}
		}
	} 
	if (opts == mtrk_sbo_print_opts::debug) {
		s += "\n\t";
		if (evnt.is_small()) {
			s += "sbo=>small = ";
		} else {
			s += "sbo=>big   = ";
		}
		s += "{";
		dbk::print_hexascii(evnt.raw_begin(),evnt.raw_end(),
			std::back_inserter(s),'\0',' ');
		s += "}; \n";
		s += "\tbigsmall_flag = ";
		auto f = evnt.flags();
		s += dbk::print_hexascii(&f, 1, ' ');
	}
	return s;
}



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

meta_event_t classify_meta_event(const mtrk_event_t& ev) {
	if (ev.type()!=smf_event_type::meta) {
		return meta_event_t::invalid;
	}
	auto it = ev.event_begin();
	uint16_t d16 = dbk::be_2_native<uint16_t>(&*it); // (p);
	return classify_meta_event_impl(d16);
}
bool is_meta(const mtrk_event_t& ev) {
	return ev.type()==smf_event_type::meta;
}
bool is_meta(const mtrk_event_t& ev, const meta_event_t& mtype) {
	if (ev.type()==smf_event_type::meta) {
		auto it = ev.event_begin();
		return classify_meta_event_impl(dbk::be_2_native<uint16_t>(&*it))==mtype;
	}
	return false;
}
bool is_seqn(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::seqn);
}
bool is_text(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::text);
}
bool is_copyright(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::copyright);
}
bool is_trackname(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::trackname);
}
bool is_instname(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::instname);
}
bool is_lyric(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::lyric);
}
bool is_marker(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::marker);
}
bool is_cuepoint(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::cuepoint);
}
bool is_chprefix(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::chprefix);
}
bool is_eot(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::eot);
}
bool is_tempo(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::tempo);
}
bool is_smpteoffset(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::smpteoffset);
}
bool is_timesig(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::timesig);
}
bool is_keysig(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::keysig);
}
bool is_seqspecific(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::seqspecific);
}

bool meta_has_text(const mtrk_event_t& ev) {
	auto mttype = classify_meta_event(ev);
	return meta_hastext_impl(static_cast<uint16_t>(mttype));
}

std::string meta_generic_gettext(const mtrk_event_t& ev) {
	std::string s {};
	if (!meta_has_text(ev)) {
		return s;
	}
	auto its = ev.payload_range();
	s.reserve(its.end-its.begin);
	std::copy(its.begin,its.end,std::back_inserter(s));
	return s;
}

uint32_t get_tempo(const mtrk_event_t& ev, uint32_t def) {
	if (!is_tempo(ev)) {
		return def;
	}
	auto its = ev.payload_range();
	return be_2_native<uint32_t>(its.begin,its.end);
}

midi_timesig_t get_timesig(const mtrk_event_t& ev, midi_timesig_t def) {
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
midi_keysig_t get_keysig(const mtrk_event_t& ev, midi_keysig_t def) {
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
std::vector<unsigned char> get_seqspecific(const mtrk_event_t& ev) {
	auto v = std::vector<unsigned char>();
	return get_seqspecific(ev,v);
}
std::vector<unsigned char> get_seqspecific(const mtrk_event_t& ev, 
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
mtrk_event_t make_seqn(const uint32_t& dt, const uint16_t& seqn) {
	std::array<unsigned char,5> evdata {0xFFu,0x00u,0x02u,0x00u,0x00u};
	write_16bit_be(seqn, evdata.begin()+3);
	auto result = mtrk_event_t(dt,evdata.data(),evdata.size(),0x00u);
	return result;
}
mtrk_event_t make_chprefix(const uint32_t& dt, const uint8_t& ch) {
	std::array<unsigned char,4> evdata {0xFFu,0x20u,0x01u,0x00u};
	write_bytes(ch, evdata.begin()+3);
	auto result = mtrk_event_t(dt,evdata.data(),evdata.size(),0x00u);
	return result;
}
mtrk_event_t make_tempo(const uint32_t& dt, const uint32_t& uspqn) {
	std::array<unsigned char,6> evdata {0xFFu,0x51u,0x03u,0x00u,0x00u,0x00u};
	write_24bit_be((uspqn>0xFFFFFFu ? 0xFFFFFFu : uspqn), evdata.begin()+3);
	auto result = mtrk_event_t(dt,evdata.data(),evdata.size(),0x00u);
	return result;
}
mtrk_event_t make_eot(const uint32_t& dt) {
	std::array<unsigned char,3> evdata {0xFFu,0x2Fu,0x00u};
	auto result = mtrk_event_t(dt,evdata.data(),evdata.size(),0x00u);
	return result;
}
mtrk_event_t make_timesig(const uint32_t& dt, const midi_timesig_t& ts) {
	std::array<unsigned char,7> evdata {0xFFu,0x58u,0x04u,
		ts.num,ts.log2denom,ts.clckspclk,ts.ntd32pq};
	auto result = mtrk_event_t(dt,evdata.data(),evdata.size(),0x00u);
	return result;
}
mtrk_event_t make_instname(const uint32_t& dt, const std::string& s) {
	return make_meta_generic_text(dt,meta_event_t::instname,s);
}
mtrk_event_t make_trackname(const uint32_t& dt, const std::string& s) {
	return make_meta_generic_text(dt,meta_event_t::trackname,s);
}
mtrk_event_t make_lyric(const uint32_t& dt, const std::string& s) {
	return make_meta_generic_text(dt,meta_event_t::lyric,s);
}
mtrk_event_t make_marker(const uint32_t& dt, const std::string& s) {
	return make_meta_generic_text(dt,meta_event_t::marker,s);
}
mtrk_event_t make_cuepoint(const uint32_t& dt, const std::string& s) {
	return make_meta_generic_text(dt,meta_event_t::cuepoint,s);
}
mtrk_event_t make_text(const uint32_t& dt, const std::string& s) {
	return make_meta_generic_text(dt,meta_event_t::text,s);
}
mtrk_event_t make_copyright(const uint32_t& dt, const std::string& s) {
	return make_meta_generic_text(dt,meta_event_t::copyright,s);
}
mtrk_event_t make_meta_generic_text(const uint32_t& dt, const meta_event_t& type, 
									const std::string& s) {
	auto type_int16 = static_cast<uint16_t>(type);  // TODO:  Gross
	if (!meta_hastext_impl(type_int16)) {
		// TODO:  This probably breaks nrvo
		return mtrk_event_t();
	}
	std::vector<unsigned char> evdata;
	evdata.reserve(sizeof(mtrk_event_t));
	auto it = write_delta_time(dt,std::back_inserter(evdata));
	evdata.push_back(0xFFu);
	evdata.push_back(static_cast<uint8_t>(type_int16&0x00FFu));
	// TODO:  midi_write_vl_field does not enforce a max size for the length
	// field of 4 bytes.  
	it = midi_write_vl_field(std::back_inserter(evdata),s.size());
	std::copy(s.begin(),s.end(),std::back_inserter(evdata));
	auto result = mtrk_event_t(evdata.data(),evdata.size(),0x00u);
	return result;
}






midi_ch_event_t get_channel_event(const mtrk_event_t& ev, midi_ch_event_t def) {
	auto result = get_channel_event_impl(ev);
	if (!verify(result)) {
		return def;
	}
	return result;
}
midi_ch_event_t get_channel_event_impl(const mtrk_event_t& ev) {
	auto its = ev.payload_range();
	midi_ch_event_t result {};
	if (its.begin == its.end) {
		return result;
	}
	result.status_nybble = ((*its.begin) & 0xF0u);
	result.ch = ((*its.begin) & 0x0Fu);
	++its.begin;
	if (its.begin == its.end) {
		return result;
	}
	result.p1 = *its.begin;
	++its.begin;
	if (its.begin == its.end) {
		return result;
	}
	result.p2 = *its.begin;
	return result;
}
bool is_channel(const mtrk_event_t& ev) {  // voice or mode
	return verify(get_channel_event_impl(ev));
}
bool is_channel_voice(const mtrk_event_t& ev) {
	auto md = get_channel_event_impl(ev);
	if (!verify(md)) {
		return false;
	}
	if ((md.status_nybble==0xB0u) && ((md.p1&0x78u)==0x78u)) {
		// => ev is a channel_mode, not channel_voice event
		// 0x78u  == 0b01111000u; see Table I of the MIDI std
		return false;
	}
	return true;
}
bool is_channel_mode(const mtrk_event_t& ev) {
	auto md = get_channel_event_impl(ev);
	if (!verify(md)) {
		return false;
	}
	if ((md.status_nybble!=0xB0u) || ((md.p1&0x78u)!=0x78u)) {
		// 0x78u  == 0b01111000u; see Table I of the MIDI std
		return false;
	}
	return true;
}
bool is_note_on(const mtrk_event_t& ev) {
	return is_note_on(get_channel_event_impl(ev));
}
bool is_note_off(const mtrk_event_t& ev) {
	return is_note_off(get_channel_event_impl(ev));
}
bool is_key_pressure(const mtrk_event_t& ev) {
	return is_key_pressure(get_channel_event_impl(ev));
}
bool is_control_change(const mtrk_event_t& ev) {
	return is_control_change(get_channel_event_impl(ev));
}
bool is_program_change(const mtrk_event_t& ev) {
	return is_program_change(get_channel_event_impl(ev));
}
bool is_channel_pressure(const mtrk_event_t& ev) {
	return is_channel_pressure(get_channel_event_impl(ev));
}
bool is_pitch_bend(const mtrk_event_t& ev) {
	return ((ev.status_byte() & 0xF0u) == 0xE0u);
}
bool is_onoff_pair(const mtrk_event_t& on, const mtrk_event_t& off) {
	auto on_md = get_channel_event_impl(on);
	auto off_md = get_channel_event_impl(off);
	if (!is_note_on(on_md) || !is_note_off(off_md)) {
		return false;
	}
	return is_onoff_pair(on_md.ch,on_md.p1,off_md.ch,off_md.p1);
}
bool is_onoff_pair(int on_ch, int on_note, const mtrk_event_t& off) {
	auto off_md = get_channel_event_impl(off);
	if (!is_note_off(off_md)) {
		return false;
	}
	return is_onoff_pair(on_ch,on_note,off_md.ch,off_md.p1);
}
bool is_onoff_pair(int on_ch, int on_note, int off_ch, int off_note) {
	return (on_ch==off_ch && on_note==off_note);
}



mtrk_event_t make_ch_event_generic_unsafe(const uint32_t& dt, const midi_ch_event_t& md) {
	//std::array<unsigned char,3> evdata {(md.status_nybble|md.ch),md.p1,md.p2};
	//auto result = mtrk_event_t(dt,evdata.data(),evdata.size(),0x00u);
	//return result;
	return mtrk_event_t(dt,md);
}
mtrk_event_t make_note_on(const uint32_t& dt, midi_ch_event_t md) {
	md = normalize(md);
	md.status_nybble = 0x90u;
	md.p2 = md.p2 > 0 ? md.p2 : 1;  // A note-on event must have a velocity > 0
	return mtrk_event_t(dt,md);
}
mtrk_event_t make_note_off(const uint32_t& dt, midi_ch_event_t md) {
	md = normalize(md);
	md.status_nybble = 0x80u;
	return mtrk_event_t(dt,md);
}
mtrk_event_t make_note_off90(const uint32_t& dt, midi_ch_event_t md) {
	md = normalize(md);
	md.status_nybble = 0x90u;
	md.p2 = 0;
	return mtrk_event_t(dt,md);
}
mtrk_event_t make_key_pressure(const uint32_t& dt, midi_ch_event_t md) {
	md = normalize(md);
	md.status_nybble = 0xA0u;
	return mtrk_event_t(dt,md);
}
mtrk_event_t make_control_change(const uint32_t& dt, midi_ch_event_t md) {
	md = normalize(md);
	md.status_nybble = 0xB0u;
	md.p1 >= 120 ? 119 : md.p1;  // p1 >=120 (==0b01111000) => select_ch_mode
	return mtrk_event_t(dt,md);
}
mtrk_event_t make_program_change(const uint32_t& dt, midi_ch_event_t md) {
	md = normalize(md);
	md.status_nybble = 0xC0u;
	md.p2 = 0x00u;
	return mtrk_event_t(dt,md);
}
mtrk_event_t make_channel_pressure(const uint32_t& dt, midi_ch_event_t md) {
	md = normalize(md);
	md.status_nybble = 0xD0u;
	md.p2 = 0x00u;
	return mtrk_event_t(dt,md);
}
mtrk_event_t make_pitch_bend(const uint32_t& dt, midi_ch_event_t md) {
	md = normalize(md);
	md.status_nybble = 0xE0u;
	return mtrk_event_t(dt,md);
}
mtrk_event_t make_channel_mode(const uint32_t& dt, midi_ch_event_t md) {
	md = normalize(md);
	md.status_nybble = 0xB0u;
	md.p1 < 120 ? 120 : md.p1;  // p1 <120 (==0b01111000) => control_change
	return mtrk_event_t(dt,md);
}


bool is_sysex(const mtrk_event_t& ev) {
	return (is_sysex_f0(ev) || is_sysex_f7(ev));
}
bool is_sysex_f0(const mtrk_event_t& ev) {
	return ev.type()==smf_event_type::sysex_f0;
}
bool is_sysex_f7(const mtrk_event_t& ev) {
	return ev.type()==smf_event_type::sysex_f7;
}
mtrk_event_t make_sysex_generic_impl(const uint32_t& dt, unsigned char type, 
					bool f7_terminate, const std::vector<unsigned char>& pyld) {
	auto payload_eff_size = pyld.size();
	if ((pyld.size()>0) && (pyld.back()!=0xF7u) && f7_terminate) {
		++payload_eff_size;
	}
	auto sz_reserve = midi_vl_field_size(dt) + 1 + midi_vl_field_size(payload_eff_size)
		+ payload_eff_size;
	
	auto result = mtrk_event_t();
	result.reserve(sz_reserve);
	auto it = result.begin();
	it = midi_write_vl_field(it,dt);
	*it++ = type;
	it = midi_write_vl_field(it,payload_eff_size);
	it = std::copy(pyld.begin(),pyld.end(),it);
	if (pyld.size()!=payload_eff_size) {  // pyld is not F7-capped && f7_terminate
		*it++ = 0xF7u;
	}
	return result;
}
mtrk_event_t make_sysex_f0(const uint32_t& dt, const std::vector<unsigned char>& pyld) {
	return make_sysex_generic_impl(dt,0xF0u,true,pyld);
}
mtrk_event_t make_sysex_f7(const uint32_t& dt, const std::vector<unsigned char>& pyld) {
	return make_sysex_generic_impl(dt,0xF7u,true,pyld);
}

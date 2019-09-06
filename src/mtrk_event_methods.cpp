#include "mtrk_event_methods.h"
#include "mtrk_event_t.h"
#include "aux_types.h"
#include "midi_time.h"
#include "midi_status_byte.h"
#include "midi_vlq.h"
#include "midi_delta_time.h"
#include "print_hexascii.h"
#include "make_mtrk_event.h"
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>
#include <cmath>



std::string jmid::print_type(const jmid::mtrk_event_t& evnt) {
	std::string s;
	if (jmid::is_channel(evnt)) {
		s += "channel::";
		auto ch = evnt.get_channel_event_data();
		s += print(ch);
	} else if (is_meta(evnt)) {
		s += "meta::";
		s += jmid::print(jmid::classify_meta_event(evnt));
	} else if (is_sysex_f0(evnt)) {
		s += "sysex_F0";
		//auto sx = evnt.get_sysex();
	} else if (is_sysex_f7(evnt)) {
		s += "sysex_F7";
		//auto sx = evnt.get_sysex();
	}
	return s;
}
std::string jmid::print(const jmid::mtrk_event_t& evnt, jmid::mtrk_sbo_print_opts opts) {
	std::string s {};
	s += ("delta-time = " + std::to_string(evnt.delta_time()) + ", ");
	s += "type = " + print(jmid::classify_status_byte(evnt.status_byte())) + ", ";
	s += ("size = " + std::to_string(evnt.size()) + ", ");
	s += ("data_size = " + std::to_string(evnt.data_size()) + "\n");
	
	s += "\t[";
	jmid::print_hexascii(evnt.dt_begin(),evnt.dt_end(),std::back_inserter(s),'\0',' ');
	s += "] ";
	jmid::print_hexascii(evnt.event_begin(),evnt.end(),std::back_inserter(s),'\0',' ');
	
	if (opts == mtrk_sbo_print_opts::detail || opts == mtrk_sbo_print_opts::debug) {
		if (jmid::is_meta(evnt)) {
			s += "\n";
			s += ("\tmeta type: " + print(jmid::classify_meta_event(evnt)) + "; ");
			if (jmid::meta_has_text(evnt)) {
				s += ("text payload: \"" + jmid::meta_generic_gettext(evnt) + "\"; ");
			} else if (jmid::is_tempo(evnt)) {
				s += ("value = " + std::to_string(get_tempo(evnt)) + " us/q; ");
			} else if (jmid::is_timesig(evnt)) {
				auto data = get_timesig(evnt);
				s += ("value = " + std::to_string(data.num) + "/" 
					+ std::to_string(static_cast<int>(std::exp2(data.log2denom)))
					+ ", " + std::to_string(data.clckspclk) + " MIDI clocks/click, "
					+ std::to_string(data.ntd32pq) + " notated 32'nd nts / MIDI q nt; ");
			}
		}
	} 
	if (opts == jmid::mtrk_sbo_print_opts::debug) {
		auto dbg = debug_info(evnt);
		s += "\n\t";
		if (!dbg.is_big) {
			s += "sbo=>small = ";
		} else {
			s += "sbo=>big   = ";
		}
		s += "{";
		jmid::print_hexascii(dbg.raw_beg,dbg.raw_end,
			std::back_inserter(s),'\0',' ');
		s += "}; \n";
		s += "\tbigsmall_flag = ";
		auto f = dbg.flags;
		jmid::print_hexascii(&f, &f+1, std::back_inserter(s), ' ');
	}
	return s;
}

bool jmid::is_eq_ignore_dt(const mtrk_event_t& lhs, const mtrk_event_t& rhs) {
	auto lhs_beg = lhs.event_begin();  auto lhs_end = lhs.end();
	auto rhs_beg = rhs.event_begin();  auto rhs_end = rhs.end();
	while (lhs_beg!=lhs_end && rhs_beg!=rhs_end) {
		if (*lhs_beg++ != *rhs_beg++) {
			return false;
		}
	}
	return true;
}

jmid::meta_event_t jmid::classify_meta_event_impl(const std::uint16_t& d16) {
	if (d16==static_cast<std::uint16_t>(meta_event_t::seqn)) {
		return jmid::meta_event_t::seqn;
	} else if (d16==static_cast<std::uint16_t>(meta_event_t::text)) {
		return jmid::meta_event_t::text;
	} else if (d16==static_cast<std::uint16_t>(meta_event_t::copyright)) {
		return jmid::meta_event_t::copyright;
	} else if (d16==static_cast<std::uint16_t>(meta_event_t::trackname)) {
		return jmid::meta_event_t::trackname;
	} else if (d16==static_cast<std::uint16_t>(meta_event_t::instname)) {
		return jmid::meta_event_t::instname;
	} else if (d16==static_cast<std::uint16_t>(meta_event_t::lyric)) {
		return jmid::meta_event_t::lyric;
	} else if (d16==static_cast<std::uint16_t>(meta_event_t::marker)) {
		return jmid::meta_event_t::marker;
	} else if (d16==static_cast<std::uint16_t>(meta_event_t::cuepoint)) {
		return jmid::meta_event_t::cuepoint;
	} else if (d16==static_cast<std::uint16_t>(meta_event_t::chprefix)) {
		return jmid::meta_event_t::chprefix;
	} else if (d16==static_cast<std::uint16_t>(meta_event_t::eot)) {
		return jmid::meta_event_t::eot;
	} else if (d16==static_cast<std::uint16_t>(meta_event_t::tempo)) {
		return jmid::meta_event_t::tempo;
	} else if (d16==static_cast<std::uint16_t>(meta_event_t::smpteoffset)) {
		return jmid::meta_event_t::smpteoffset;
	} else if (d16==static_cast<std::uint16_t>(meta_event_t::timesig)) {
		return jmid::meta_event_t::timesig;
	} else if (d16==static_cast<std::uint16_t>(meta_event_t::keysig)) {
		return jmid::meta_event_t::keysig;
	} else if (d16==static_cast<std::uint16_t>(meta_event_t::seqspecific)) {
		return jmid::meta_event_t::seqspecific;
	} else {
		if ((d16&0xFF00u)==0xFF00u) {
			return jmid::meta_event_t::unknown;
		} else {
			return jmid::meta_event_t::invalid;
		}
	}
}
bool jmid::meta_hastext_impl(const std::uint16_t& t) {
	auto mttype = jmid::classify_meta_event_impl(t);
	return (mttype==jmid::meta_event_t::text 
			|| mttype==jmid::meta_event_t::copyright
			|| mttype==jmid::meta_event_t::instname
			|| mttype==jmid::meta_event_t::trackname
			|| mttype==jmid::meta_event_t::lyric
			|| mttype==jmid::meta_event_t::marker
			|| mttype==jmid::meta_event_t::cuepoint);
}

std::string jmid::print(const meta_event_t& mt) {
	if (mt==jmid::meta_event_t::seqn) {
		return "seqn";
	} else if (mt==jmid::meta_event_t::text) {
		return "text";
	} else if (mt==jmid::meta_event_t::copyright) {
		return "copyright";
	} else if (mt==jmid::meta_event_t::trackname) {
		return "trackname";
	} else if (mt==jmid::meta_event_t::instname) {
		return "instname";
	} else if (mt==jmid::meta_event_t::lyric) {
		return "lyric";
	} else if (mt==jmid::meta_event_t::marker) {
		return "marker";
	} else if (mt==jmid::meta_event_t::cuepoint) {
		return "cuepoint";
	} else if (mt==jmid::meta_event_t::chprefix) {
		return "chprefix";
	} else if (mt==jmid::meta_event_t::eot) {
		return "eot";
	} else if (mt==jmid::meta_event_t::tempo) {
		return "tempo";
	} else if (mt==jmid::meta_event_t::smpteoffset) {
		return "smpteoffset";
	} else if (mt==jmid::meta_event_t::timesig) {
		return "timesig";
	} else if (mt==jmid::meta_event_t::keysig) {
		return "keysig";
	} else if (mt==jmid::meta_event_t::seqspecific) {
		return "seqspecific";
	} else if (mt==jmid::meta_event_t::invalid) {
		return "invalid";
	} else if (mt==jmid::meta_event_t::unknown) {
		return "unknown";
	} else {
		return "?";
	}
}

jmid::meta_event_t jmid::classify_meta_event(const mtrk_event_t& ev) {
	if (!jmid::is_meta(ev)) {
		return jmid::meta_event_t::invalid;
	}
	std::uint16_t d16 = jmid::read_be<uint16_t>(ev.event_begin(),ev.end());
	return jmid::classify_meta_event_impl(d16);
}
bool jmid::is_meta(const mtrk_event_t& ev) {
	auto s = ev.status_byte();
	return jmid::is_meta_status_byte(s);
}
bool jmid::is_meta(const mtrk_event_t& ev, const meta_event_t& mtype) {
	if (!jmid::is_meta(ev)) {
		return false;
	}
	return jmid::classify_meta_event_impl(jmid::read_be<std::uint16_t>(ev.event_begin(),ev.end()))==mtype;
}
bool jmid::is_seqn(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::seqn);
}
bool jmid::is_text(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::text);
}
bool jmid::is_copyright(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::copyright);
}
bool jmid::is_trackname(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::trackname);
}
bool jmid::is_instname(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::instname);
}
bool jmid::is_lyric(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::lyric);
}
bool jmid::is_marker(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::marker);
}
bool jmid::is_cuepoint(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::cuepoint);
}
bool jmid::is_chprefix(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::chprefix);
}
bool jmid::is_eot(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::eot);
}
bool jmid::is_tempo(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::tempo);
}
bool jmid::is_smpteoffset(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::smpteoffset);
}
bool jmid::is_timesig(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::timesig);
}
bool jmid::is_keysig(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::keysig);
}
bool jmid::is_seqspecific(const mtrk_event_t& ev) {
	return jmid::is_meta(ev, jmid::meta_event_t::seqspecific);
}

bool jmid::meta_has_text(const mtrk_event_t& ev) {
	auto mttype = jmid::classify_meta_event(ev);
	return jmid::meta_hastext_impl(static_cast<std::uint16_t>(mttype));
}

std::string jmid::meta_generic_gettext(const mtrk_event_t& ev) {
	std::string s {};
	if (!jmid::meta_has_text(ev)) {
		return s;
	}
	auto its = ev.payload_range();
	s.reserve(its.end-its.begin);
	std::copy(its.begin,its.end,std::back_inserter(s));
	return s;
}

std::int32_t jmid::get_tempo(const mtrk_event_t& ev, std::int32_t def) {
	if (!jmid::is_tempo(ev)) {
		return def;
	}
	auto its = ev.payload_range();
	// Note that a tempo is a 24-bit value (its.end-its.begin == 3), so the
	// value can not possibly overflow an int32_t
	return static_cast<std::int32_t>(jmid::read_be<std::uint32_t>(its.begin,its.end));
}

jmid::midi_timesig_t jmid::get_timesig(const mtrk_event_t& ev, jmid::midi_timesig_t def) {
	if (!jmid::is_timesig(ev)) {
		return def;
	}
	auto it = ev.payload_begin();
	if ((ev.end()-it) != 4) {
		return def;
	}
	jmid::midi_timesig_t result {};
	result.num = *it++;
	result.log2denom = *it++;
	result.clckspclk = *it++;
	result.ntd32pq = *it++;
	return result;
}
jmid::midi_keysig_t jmid::get_keysig(const mtrk_event_t& ev, jmid::midi_keysig_t def) {
	if (!jmid::is_keysig(ev)) {
		return def;
	}
	auto it = ev.payload_begin();
	if ((ev.end()-it) != 2) {
		return def;
	}
	jmid::midi_keysig_t result {};
	result.sf = *it++;
	result.mi = *it++;
	return result;
}
std::vector<unsigned char> jmid::get_seqspecific(const mtrk_event_t& ev) {
	auto v = std::vector<unsigned char>();
	return jmid::get_seqspecific(ev,v);
}
std::vector<unsigned char> jmid::get_seqspecific(const mtrk_event_t& ev, 
								std::vector<unsigned char>& data) {
	if (!jmid::is_seqspecific(ev)) {
		return data;
	}
	data.clear();
	for (auto it=ev.payload_begin(); it!=ev.end(); ++it) {
		data.push_back(*it);
	}
	return data;
}
jmid::mtrk_event_t jmid::make_seqn(const std::int32_t& dt, 
									const std::uint16_t& seqn) {
	std::uint16_t pyld = jmid::to_be_byte_order(seqn);
	auto p = static_cast<unsigned char*>(static_cast<void*>(&pyld));
	return jmid::mtrk_event_t(jmid::delta_time(dt),
		jmid::meta_header(0x00u,2),p,p+2);
}
jmid::mtrk_event_t jmid::make_chprefix(const std::int32_t& dt,
										const uint8_t& ch) {
	const unsigned char pyld = ch;
	return jmid::mtrk_event_t(jmid::delta_time(dt),
		jmid::meta_header(0x20u,1),&pyld,&pyld+1);
}
jmid::mtrk_event_t jmid::make_tempo(const std::int32_t& dt, 
									const std::uint32_t& uspqn) {
	std::array<unsigned char,3> pyld;
	jmid::write_24bit_be((uspqn>0xFFFFFFu ? 0xFFFFFFu : uspqn), pyld.data());
	return jmid::mtrk_event_t(jmid::delta_time(dt),
				jmid::meta_header(0x51u,pyld.size()),pyld.data(),
				pyld.data()+pyld.size());
}
jmid::mtrk_event_t jmid::make_eot(const std::int32_t& dt) {
	unsigned char pyld_dummy = 0;
	return jmid::mtrk_event_t(jmid::delta_time(dt),
			jmid::meta_header(0x2Fu,0),&pyld_dummy,&pyld_dummy);
}
jmid::mtrk_event_t jmid::make_timesig(const std::int32_t& dt, 
									const jmid::midi_timesig_t& ts) {
	std::array<unsigned char,4> pyld {
		static_cast<unsigned char>(ts.num),
		static_cast<unsigned char>(ts.log2denom),
		static_cast<unsigned char>(ts.clckspclk),
		static_cast<unsigned char>(ts.ntd32pq)};
	return jmid::mtrk_event_t(jmid::delta_time(dt),
		jmid::meta_header(0x58u,pyld.size()),pyld.data(),
		pyld.data()+pyld.size());
}
jmid::mtrk_event_t jmid::make_instname(const std::int32_t& dt, 
										const std::string& s) {
	auto p = reinterpret_cast<const unsigned char*>(s.data());
	return jmid::mtrk_event_t(jmid::delta_time(dt),
				jmid::meta_header(0x04u,s.size()),p,
				p+s.size());
}
jmid::mtrk_event_t jmid::make_trackname(const std::int32_t& dt,
										const std::string& s) {
	auto p = reinterpret_cast<const unsigned char*>(s.data());
	return jmid::mtrk_event_t(jmid::delta_time(dt),
				jmid::meta_header(0x03u,s.size()),p,
				p+s.size());
}
jmid::mtrk_event_t jmid::make_lyric(const std::int32_t& dt,
									const std::string& s) {
	auto p = reinterpret_cast<const unsigned char*>(s.data());
	return jmid::mtrk_event_t(jmid::delta_time(dt),
				jmid::meta_header(0x05u,s.size()),p,
				p+s.size());
}
jmid::mtrk_event_t jmid::make_marker(const std::int32_t& dt, 
										const std::string& s) {
	auto p = reinterpret_cast<const unsigned char*>(s.data());
	return jmid::mtrk_event_t(jmid::delta_time(dt),
				jmid::meta_header(0x06u,s.size()),p,
				p+s.size());
}
jmid::mtrk_event_t jmid::make_cuepoint(const std::int32_t& dt, 
										const std::string& s) {
	auto p = reinterpret_cast<const unsigned char*>(s.data());
	return jmid::mtrk_event_t(jmid::delta_time(dt),
				jmid::meta_header(0x07u,s.size()),p,
				p+s.size());
}
jmid::mtrk_event_t jmid::make_text(const std::int32_t& dt, 
									const std::string& s) {
	auto p = reinterpret_cast<const unsigned char*>(s.data());
	return jmid::mtrk_event_t(jmid::delta_time(dt),
				jmid::meta_header(0x01u,s.size()),p,
				p+s.size());
}
jmid::mtrk_event_t jmid::make_copyright(const std::int32_t& dt, 
										const std::string& s) {
	auto p = reinterpret_cast<const unsigned char*>(s.data());
	return jmid::mtrk_event_t(jmid::delta_time(dt),
				jmid::meta_header(0x02u,s.size()),p,
				p+s.size());
}


jmid::ch_event_data_t jmid::get_channel_event(const mtrk_event_t& ev, jmid::ch_event_data_t def) {
	auto result = jmid::get_channel_event_impl(ev);
	if (!result) {
		return def;
	}
	return result;
}
jmid::ch_event_data_t jmid::get_channel_event_impl(const mtrk_event_t& ev) {
	// Note that get_channel_event(), which calls get_channel_event_impl(),
	// relies on the ability to detect invalid values of a ch_event_data_t
	// in deciding whether to return the result of get_channel_event_impl()
	// or the default ch_event_data_t.  Hence it is important that non- 
	// channel events fill result w/ invalid data.  
	jmid::ch_event_data_t result;  
	result.status_nybble=0x00u;  // an invalid status nybble
	auto its = ev.payload_range();
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
bool jmid::is_channel(const jmid::mtrk_event_t& ev) {  // voice or mode
	return jmid::is_channel_status_byte(ev.status_byte());
}
bool jmid::is_channel_voice(const jmid::mtrk_event_t& ev) {
	auto s = ev.status_byte();
	if (!jmid::is_channel_status_byte(s)) {
		return false;
	}
	if ((s&0xF0u)!=0xB0u) {
		return true;
	}
	auto it = ev.event_begin();
	auto p1 = *(++it);
	return (p1>>3)!=0b00001111u;
}
bool jmid::is_channel_mode(const jmid::mtrk_event_t& ev) {
	auto it = ev.event_begin();
	auto s = *it++;
	auto p1 = *it++;
	return (((s&0xF0u)==0xB0u) && ((p1>>3)==0b00001111u));
}
bool jmid::is_note_on(const jmid::mtrk_event_t& ev) {
	auto md = jmid::get_channel_event_impl(ev);
	return (md.status_nybble==0x90u && (md.p2 > 0));
}
bool jmid::is_note_off(const jmid::mtrk_event_t& ev) {
	auto md = jmid::get_channel_event_impl(ev);
	return ((md.status_nybble==0x80u)
			|| (md.status_nybble==0x90u && (md.p2==0)));
}
bool jmid::is_key_pressure(const jmid::mtrk_event_t& ev) {
	return ((ev.status_byte()&0xF0u)==0xA0u);
}
bool jmid::is_control_change(const jmid::mtrk_event_t& ev) {
	auto md = jmid::get_channel_event_impl(ev);
	return ((md.status_nybble==0xB0u) && (md.p1 < 120));
}
bool jmid::is_program_change(const jmid::mtrk_event_t& ev) {
	return ((ev.status_byte()&0xF0u)==0xC0u);
}
bool jmid::is_channel_pressure(const jmid::mtrk_event_t& ev) {
	return ((ev.status_byte()&0xF0u)==0xD0u);
}
bool jmid::is_pitch_bend(const jmid::mtrk_event_t& ev) {
	return ((ev.status_byte()&0xF0u)==0xE0u);
}
bool jmid::is_onoff_pair(const jmid::mtrk_event_t& on, const jmid::mtrk_event_t& off) {
	auto on_md = jmid::get_channel_event_impl(on);
	auto off_md = jmid::get_channel_event_impl(off);
	if (!jmid::is_note_on(on_md) || !jmid::is_note_off(off_md)) {
		return false;
	}
	return jmid::is_onoff_pair(on_md.ch,on_md.p1,off_md.ch,off_md.p1);
}
bool jmid::is_onoff_pair(int on_ch, int on_note, const jmid::mtrk_event_t& off) {
	auto off_md = jmid::get_channel_event_impl(off);
	if (!jmid::is_note_off(off_md)) {
		return false;
	}
	return jmid::is_onoff_pair(on_ch,on_note,off_md.ch,off_md.p1);
}
bool jmid::is_onoff_pair(int on_ch, int on_note, int off_ch, int off_note) {
	return (on_ch==off_ch && on_note==off_note);
}


jmid::mtrk_event_t jmid::make_ch_event(std::int32_t dt, const jmid::ch_event_data_t& md) {
	return jmid::mtrk_event_t(jmid::delta_time(dt),
		jmid::ch_event_data_strong_t(md));
}
jmid::mtrk_event_t jmid::make_ch_event(std::int32_t dt, int sn, int ch, int p1, int p2) {
	return jmid::mtrk_event_t(jmid::delta_time(dt),
		jmid::ch_event_data_strong_t(sn,ch,p1,p2));
}
jmid::mtrk_event_t jmid::make_note_on(std::int32_t dt,
									jmid::ch_event_data_t md) {
	md.status_nybble = 0x90u;
	md.p2 = md.p2 > 0 ? md.p2 : 1;  // A note-on event must have a velocity > 0
	return jmid::mtrk_event_t(jmid::delta_time(dt),
		jmid::ch_event_data_strong_t(md));
}
jmid::mtrk_event_t jmid::make_note_on(std::int32_t dt, int ch, int p1, int p2) {
	return jmid::make_note_on(dt,jmid::make_midi_ch_event_data(0,ch,p1,p2));
}
jmid::mtrk_event_t jmid::make_note_off(std::int32_t dt, jmid::ch_event_data_t md) {
	md.status_nybble = 0x80u;
	return jmid::make_ch_event(dt,md);
}
jmid::mtrk_event_t jmid::make_note_off(std::int32_t dt, int ch, int p1, int p2) {
	return jmid::make_note_off(dt,jmid::make_midi_ch_event_data(0,ch,p1,p2));
}
jmid::mtrk_event_t jmid::make_note_off90(std::int32_t dt, jmid::ch_event_data_t md) {
	md.status_nybble = 0x90u;
	md.p2 = 0;
	return jmid::make_ch_event(dt,md);
}
jmid::mtrk_event_t jmid::make_key_pressure(std::int32_t dt, jmid::ch_event_data_t md) {
	md.status_nybble = 0xA0u;
	return jmid::make_ch_event(dt,md);
}
jmid::mtrk_event_t jmid::make_key_pressure(std::int32_t dt, int ch, int p1, int p2) {
	return jmid::make_key_pressure(dt,jmid::make_midi_ch_event_data(0,ch,p1,p2));
}
jmid::mtrk_event_t jmid::make_control_change(std::int32_t dt, jmid::ch_event_data_t md) {
	md.status_nybble = 0xB0u;
	if (md.p1 >= 120) {  // p1 >=120 (==0b01111000) => select_ch_mode
		md.p1 = 119;
	}
	return jmid::make_ch_event(dt,md);
}
jmid::mtrk_event_t jmid::make_control_change(std::int32_t dt, int ch, int p1, int p2) {
	return jmid::make_control_change(dt,jmid::make_midi_ch_event_data(0,ch,p1,p2));
}
jmid::mtrk_event_t jmid::make_program_change(std::int32_t dt, jmid::ch_event_data_t md) {
	md.status_nybble = 0xC0u;
	md.p2 = 0x00u;
	return jmid::make_ch_event(dt,md);
}
jmid::mtrk_event_t jmid::make_program_change(std::int32_t dt, int ch, int p1) {
	return jmid::make_program_change(dt,jmid::make_midi_ch_event_data(0,ch,p1,0));
}
jmid::mtrk_event_t jmid::make_channel_pressure(std::int32_t dt, jmid::ch_event_data_t md) {
	md.status_nybble = 0xD0u;
	md.p2 = 0x00u;
	return jmid::make_ch_event(dt,md);
}
jmid::mtrk_event_t jmid::make_channel_pressure(std::int32_t dt, int ch, int p1) {
	return jmid::make_channel_pressure(dt,jmid::make_midi_ch_event_data(0,ch,p1,0));
}
jmid::mtrk_event_t jmid::make_pitch_bend(std::int32_t dt, jmid::ch_event_data_t md) {
	md.status_nybble = 0xE0u;
	return jmid::make_ch_event(dt,md);
}
jmid::mtrk_event_t jmid::make_pitch_bend(std::int32_t dt, int ch, int p1, int p2) {
	return jmid::make_pitch_bend(dt,jmid::make_midi_ch_event_data(0,ch,p1,p2));
}
jmid::mtrk_event_t jmid::make_channel_mode(std::int32_t dt, jmid::ch_event_data_t md) {
	md.status_nybble = 0xB0u;
	if (md.p1 < 120) {  // p1 <120 (==0b01111000) => control_change
		md.p1 = 120;
	}
	return jmid::make_ch_event(dt,md);
}
jmid::mtrk_event_t jmid::make_channel_mode(std::int32_t dt, int ch, int p1, int p2) {
	return jmid::make_channel_mode(dt,jmid::make_midi_ch_event_data(0,ch,p1,p2));
}

jmid::onoff_pair_t jmid::make_onoff_pair(std::int32_t duration, int ch, int nt, int vel_on, int vel_off) {
	return {jmid::make_note_on(0,ch,nt,vel_on),jmid::make_note_off(duration,ch,nt,vel_off)};
}
jmid::mtrk_event_t jmid::make_matching_off(std::int32_t dt, const jmid::mtrk_event_t& on_ev) {
	return jmid::make_note_off(dt,jmid::get_channel_event(on_ev));
}
jmid::mtrk_event_t jmid::make_matching_off90(std::int32_t dt, const jmid::mtrk_event_t& on_ev) {
	return jmid::make_note_off90(dt,jmid::get_channel_event(on_ev));
}

bool jmid::is_sysex(const jmid::mtrk_event_t& ev) {
	return jmid::is_sysex_status_byte(ev.status_byte());
}
bool jmid::is_sysex_f0(const jmid::mtrk_event_t& ev) {
	auto s = ev.status_byte();
	return (s==0xF0u);
}
bool jmid::is_sysex_f7(const jmid::mtrk_event_t& ev) {
	auto s = ev.status_byte();
	return (s==0xF7u);
}
jmid::mtrk_event_t jmid::make_sysex_f0(const std::int32_t& dt,
								const std::vector<unsigned char>& pyld) {
	return jmid::mtrk_event_t(jmid::delta_time(dt),
		jmid::sysex_header(0xF0u,pyld.size()),
		pyld.data(),pyld.data()+pyld.size());
}
jmid::mtrk_event_t jmid::make_sysex_f7(const std::int32_t& dt,
								const std::vector<unsigned char>& pyld) {
	return jmid::mtrk_event_t(jmid::delta_time(dt),
		jmid::sysex_header(0xF7u,pyld.size()),
		pyld.data(),pyld.data()+pyld.size());
}



#include "midi_raw.h"
#include "midi_delta_time.h"
#include "mthd_t.h"  // is_valid_time_division_raw_value()
#include <string>
#include <cstdint>
#include <cmath>  // std::round()
#include <algorithm>  // std::clamp() in time_division_t


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



//
// time-division_t class methods
//
time_division_t::time_division_t(int32_t tpq) {
	auto tpq_u16 = static_cast<uint16_t>(std::clamp(tpq,1,0x7FFF));
	this->d_[0] = (tpq_u16>>8);
	this->d_[1] = (tpq_u16&0x00FFu);
}
time_division_t::time_division_t(int32_t tcf, int32_t subdivs) {
	if ((tcf != -24) && (tcf != -25) && (tcf != -29) && (tcf != -30)) {
		tcf = -24;
	}
	auto tcf_i8 = static_cast<int8_t>(tcf);
	auto subdivs_ui8 = static_cast<uint8_t>(std::clamp(subdivs,1,0xFF));
	// The high-byte of val has the bit 8 set, and bits [7,0] have the 
	// bit-pattern of tcf.  The low-byte has the bit-pattern of upf.  
	auto psrc = static_cast<unsigned char*>(static_cast<void*>(&tcf));
	this->d_[0] = *psrc;
	psrc = static_cast<unsigned char*>(static_cast<void*>(&subdivs_ui8));
	this->d_[1] = *psrc;
}
time_division_t::time_division_t(smpte_t smpte) {
	*this = time_division_t(smpte.time_code, smpte.subframes);
}
time_division_t::type time_division_t::get_type() const {
	if (((this->d_[0])&0x80u)==0x80u) {
		return time_division_t::type::smpte;
	}
	return time_division_t::type::ticks_per_quarter;
}
smpte_t time_division_t::get_smpte() const {
	smpte_t result;
	int8_t tcf_i8 = 0;
	auto pdest = static_cast<unsigned char*>(static_cast<void*>(&tcf_i8));
	*pdest = this->d_[0];
	if ((tcf_i8 != -24) && (tcf_i8 != -25) && (tcf_i8 != -29) 
			&& (tcf_i8 != -30)) {
		tcf_i8 = -24;
	}
	result.time_code = static_cast<int32_t>(tcf_i8);
	result.subframes = static_cast<int32_t>(this->d_[1]);
	result.subframes = std::clamp(result.subframes,1,0xFF);
	return result;
}
int32_t time_division_t::get_tpq() const {
	auto p = &(this->d_[0]);
	auto tpq_u16 = read_be<uint16_t>(p,p+this->d_.size());
	auto result = static_cast<int32_t>(tpq_u16);
	result = std::clamp(result,1,0x7FFF);
	return result;
}
// Since the time_division_t ctors enforce the always-valid-data invariant,
// this always returns a valid raw value; no validity-checks are needed.  
uint16_t time_division_t::get_raw_value() const {
	uint16_t result = 0;
	result += this->d_[0];
	result <<= 8;
	result += this->d_[1];
	return result;
}
bool time_division_t::operator==(const time_division_t& rhs) const {
	return ((this->d_[0]==rhs.d_[0])&&(this->d_[1]==rhs.d_[1]));
}
bool time_division_t::operator!=(const time_division_t& rhs) const {
	return !(*this==rhs);
}
//
// time_division_t non-member methods
//
maybe_time_division_t make_time_division_tpq(int32_t tpq) {
	maybe_time_division_t result {time_division_t(tpq),false};
	if (tpq == std::clamp(tpq,1,0x7FFF)) {
		result.is_valid = true;
	}
	return result;
}
maybe_time_division_t make_time_division_smpte(smpte_t smpte) {
	return make_time_division_smpte(smpte.time_code,smpte.subframes);
}
maybe_time_division_t make_time_division_smpte(int32_t tcf, int32_t upf) {
	maybe_time_division_t result {time_division_t(tcf,upf),false};
	bool valid_tcf = ((tcf==-24) || (tcf==-25) || (tcf==-29) || (tcf==-30));
	bool valid_upf = ((upf>0)&&(upf<=0xFF));
	result.is_valid = (valid_tcf&&valid_upf);
	return result;
}
maybe_time_division_t make_time_division_from_raw(uint16_t raw) {
	if ((raw>>15)==1) {  // SMPTE
		uint8_t tcf_value = static_cast<uint8_t>(raw>>8);
		int8_t tcf = 0;
		auto pdest = static_cast<unsigned char*>(static_cast<void*>(&tcf));
		auto psrc = static_cast<unsigned char*>(static_cast<void*>(&tcf_value));
		*pdest = *psrc;
		return make_time_division_smpte(tcf,(raw&0x00FFu));
	}
	return make_time_division_tpq(raw&0x7FFFu);
}
bool is_smpte(time_division_t tdiv) {
	return (tdiv.get_type()==time_division_t::type::smpte);
}
bool is_tpq(time_division_t tdiv) {
	return (tdiv.get_type()==time_division_t::type::ticks_per_quarter);
}
bool is_valid_time_division_raw_value(uint16_t val) {
	if ((val>>15) == 1) {  // smpte
		unsigned char high = (val>>8);
		// Interpret the bit-pattern of high as a signed int8
		int8_t tcf = 0;  // "time code format"
		auto p_tcf = static_cast<unsigned char*>(static_cast<void*>(&tcf));
		*p_tcf = high;
		if ((tcf != -24) && (tcf != -25) 
				&& (tcf != -29) && (tcf != -30)) {
			return false;
		}
		if ((val&0x00FFu) == 0) {
			return false;
		}
	} else {  // tpq; high bit of val is clear
		if ((val&0x7FFFu) == 0) {
			return false;
		}
	}
	return true;
}
smpte_t get_smpte(time_division_t tdiv, smpte_t def) {
	smpte_t result;
	if (is_smpte(tdiv)) {
		return tdiv.get_smpte();
	}
	return def;
}
int32_t get_tpq(time_division_t tdiv, int32_t def) {
	int32_t result;
	if (is_tpq(tdiv)) {
		return tdiv.get_tpq();
	}
	return def;
}


double ticks2sec(const uint32_t& tks, const time_division_t& tdiv,
					int32_t tempo) {
	if (is_tpq(tdiv)) {
		auto tpq = static_cast<double>(tdiv.get_tpq());
		auto secpq = static_cast<double>(tempo)*(1.0/1000000.0);
		return static_cast<double>(tks)*(secpq/tpq);
	} else {
		auto smpte = tdiv.get_smpte();
		auto frames_sec = static_cast<double>(-1*smpte.time_code);
		auto tks_frame = static_cast<double>(smpte.subframes);
		return static_cast<double>(tks)/(frames_sec*tks_frame);
	}
}
int32_t sec2ticks(const double& sec, const time_division_t& tdiv,
					int32_t tempo) {
	if (is_tpq(tdiv)) {
		auto tpq = static_cast<double>(tdiv.get_tpq());
		auto secpq = static_cast<double>(tempo)*(1.0/1000000.0);
		return static_cast<int32_t>(sec*(tpq/secpq));
	} else {
		auto smpte = tdiv.get_smpte();
		auto frames_sec = static_cast<double>(-1*smpte.time_code);
		auto tks_frame = static_cast<double>(smpte.subframes);
		return static_cast<int32_t>(sec/(frames_sec*tks_frame));
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
	}
	return smf_event_type::invalid;
}
smf_event_type classify_status_byte(unsigned char s, unsigned char rs) {
	s = get_status_byte(s,rs);
	return classify_status_byte(s);
}
bool is_status_byte(const unsigned char s) {
	return (s&0x80u) == 0x80u;
}
bool is_unrecognized_status_byte(const unsigned char s) {
	return (is_status_byte(s) 
		&& !is_channel_status_byte(s)
		&& !is_sysex_or_meta_status_byte(s));
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
	return (s&0x80u)==0x00u;
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









validate_mtrk_event_result_t validate_mtrk_event_dtstart(
			const unsigned char *p, unsigned char rs, uint32_t max_size) {
	validate_mtrk_event_result_t result {};
	// All mtrk events begin with a delta-time occupying a maximum of 4 bytes
	// and a minimum of 1 byte.  Note that even a delta-time of 0 occupies 1
	// byte.  
	auto dt = read_delta_time(p,p+max_size);
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
				auto len = read_vlq(p,p+max_size);
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
				auto len = read_vlq(p,p+max_size);
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


smf_event_type classify_mtrk_event_dtstart(const unsigned char *p,
									unsigned char rs, uint32_t max_sz) {
	auto dt = read_delta_time(p,p+max_sz);
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
	p = advance_to_dt_end(p,p+4);
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
	auto len = read_vlq(p,p+max_size);
	n += (len.N + len.val);
	return n;
}
uint32_t sysex_event_get_data_size(const unsigned char *p, uint32_t max_size) {
	uint32_t n = 0;
	if (max_size <= 1) {
		return 0;
	}
	p += 1;  n += 1;  max_size -= 1;
	auto len = read_vlq(p,p+max_size);
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
	auto dt = read_delta_time(p,p+max_size);
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
		auto len = read_vlq(p,p+4);
		result += (len.N + len.val);
	} else if (ev_type==smf_event_type::sysex_f0
						|| ev_type==smf_event_type::sysex_f7) {
		p+=1;  result+=1;  // 0xF0u||0xF7u
		auto len = read_vlq(p,p+4);
		result += (len.N + len.val);
	} else {
		// smf_event_type::invalid || ::unrecognized
		return 0;
	}
	return result;
}
uint32_t mtrk_event_get_size_dtstart_unsafe(const unsigned char *p,
											unsigned char rs) {
	auto dt = read_delta_time(p,p+4);
	p+=dt.N;
	return (dt.N + mtrk_event_get_data_size_unsafe(p,rs));
}
uint32_t mtrk_event_get_data_size_dtstart_unsafe(const unsigned char *p,
												unsigned char rs) {
	auto dt = read_delta_time(p,p+4);
	p+=dt.N;
	return mtrk_event_get_data_size_unsafe(p,rs);
}




unsigned char mtrk_event_get_midi_p1_dtstart_unsafe(const unsigned char *p,
													unsigned char s) {
	p = advance_to_dt_end(p,p+4);
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
	p = advance_to_dt_end(p,p+4);
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
	p = advance_to_dt_end(p,p+4);
	p += 1;  // Skip the 0xFF;
	return *p;
}



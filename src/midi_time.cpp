#include "midi_time.h"
#include "midi_vlq.h"
#include <algorithm>  // std::clamp()
#include <cmath>
#include <cstdint>
#include <string>


//
// time-division_t class methods
//
jmid::time_division_t::time_division_t(std::int32_t tpq) {
	auto tpq_u16 = static_cast<std::uint16_t>(std::clamp(tpq,1,0x7FFF));
	this->d_[0] = (tpq_u16>>8);
	this->d_[1] = (tpq_u16&0x00FFu);
}
jmid::time_division_t::time_division_t(std::int32_t tcf, std::int32_t subdivs) {
	if ((tcf != -24) && (tcf != -25) && (tcf != -29) && (tcf != -30)) {
		tcf = -24;
	}
	auto tcf_i8 = static_cast<std::int8_t>(tcf);
	auto subdivs_ui8 = static_cast<std::uint8_t>(std::clamp(subdivs,1,0xFF));
	// The high-byte of val has the bit 8 set, and bits [7,0] have the 
	// bit-pattern of tcf.  The low-byte has the bit-pattern of upf.  
	auto psrc = static_cast<unsigned char*>(static_cast<void*>(&tcf_i8));
	this->d_[0] = *psrc;
	psrc = static_cast<unsigned char*>(static_cast<void*>(&subdivs_ui8));
	this->d_[1] = *psrc;
}
jmid::time_division_t::time_division_t(jmid::smpte_t smpte) {
	*this = jmid::time_division_t(smpte.time_code, smpte.subframes);
}
jmid::time_division_t::type jmid::time_division_t::get_type() const {
	if (((this->d_[0])&0x80u)==0x80u) {
		return jmid::time_division_t::type::smpte;
	}
	return jmid::time_division_t::type::ticks_per_quarter;
}
jmid::smpte_t jmid::time_division_t::get_smpte() const {
	jmid::smpte_t result;
	std::int8_t tcf_i8 = 0;
	auto pdest = static_cast<unsigned char*>(static_cast<void*>(&tcf_i8));
	*pdest = this->d_[0];
	if ((tcf_i8 != -24) && (tcf_i8 != -25) && (tcf_i8 != -29) 
			&& (tcf_i8 != -30)) {
		tcf_i8 = -24;
	}
	result.time_code = static_cast<std::int32_t>(tcf_i8);
	result.subframes = static_cast<std::int32_t>(this->d_[1]);
	result.subframes = std::clamp(result.subframes,1,0xFF);
	return result;
}
std::int32_t jmid::time_division_t::get_tpq() const {
	auto p = &(this->d_[0]);
	auto tpq_u16 = jmid::read_be<std::uint16_t>(p,p+this->d_.size());
	auto result = static_cast<int32_t>(tpq_u16);
	result = std::clamp(result,1,0x7FFF);
	return result;
}
// Since the jmid::xxjmid::time_division_t ctors enforce the always-valid-data invariant,
// this always returns a valid raw value; no validity-checks are needed.  
std::uint16_t jmid::time_division_t::get_raw_value() const {
	std::uint16_t result = 0;
	result += this->d_[0];
	result <<= 8;
	result += this->d_[1];
	return result;
}
bool jmid::time_division_t::operator==(const jmid::time_division_t& rhs) const {
	return ((this->d_[0]==rhs.d_[0])&&(this->d_[1]==rhs.d_[1]));
}
bool jmid::time_division_t::operator!=(const jmid::time_division_t& rhs) const {
	return !(*this==rhs);
}
//
// jmid::xxjmid::time_division_t non-member methods
//
jmid::maybe_time_division_t jmid::make_time_division_tpq(std::int32_t tpq) {
	jmid::maybe_time_division_t result {jmid::time_division_t(tpq),false};
	if (tpq == std::clamp(tpq,1,0x7FFF)) {
		result.is_valid = true;
	}
	return result;
}
jmid::maybe_time_division_t jmid::make_time_division_smpte(jmid::smpte_t smpte) {
	return jmid::make_time_division_smpte(smpte.time_code,smpte.subframes);
}
jmid::maybe_time_division_t jmid::make_time_division_smpte(std::int32_t tcf, std::int32_t upf) {
	jmid::maybe_time_division_t result {jmid::time_division_t(tcf,upf),false};
	bool valid_tcf = ((tcf==-24) || (tcf==-25) || (tcf==-29) || (tcf==-30));
	bool valid_upf = ((upf>0)&&(upf<=0xFF));
	result.is_valid = (valid_tcf&&valid_upf);
	return result;
}
jmid::maybe_time_division_t jmid::make_time_division_from_raw(std::uint16_t raw) {
	if ((raw>>15)==1) {  // SMPTE
		std::uint8_t tcf_value = static_cast<std::uint8_t>(raw>>8);
		std::int8_t tcf = 0;
		auto pdest = static_cast<unsigned char*>(static_cast<void*>(&tcf));
		auto psrc = static_cast<unsigned char*>(static_cast<void*>(&tcf_value));
		*pdest = *psrc;
		return jmid::make_time_division_smpte(tcf,(raw&0x00FFu));
	}
	return jmid::make_time_division_tpq(raw&0x7FFFu);
}
bool jmid::is_smpte(jmid::time_division_t tdiv) {
	return (tdiv.get_type()==jmid::time_division_t::type::smpte);
}
bool jmid::is_tpq(jmid::time_division_t tdiv) {
	return (tdiv.get_type()==jmid::time_division_t::type::ticks_per_quarter);
}
bool jmid::is_valid_time_division_raw_value(std::uint16_t val) {
	if ((val>>15) == 1) {  // smpte
		unsigned char high = (val>>8);
		// Interpret the bit-pattern of high as a signed int8
		std::int8_t tcf = 0;  // "time code format"
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
jmid::smpte_t jmid::get_smpte(jmid::time_division_t tdiv, jmid::smpte_t def) {
	if (jmid::is_smpte(tdiv)) {
		return tdiv.get_smpte();
	}
	return def;
}
std::int32_t jmid::get_tpq(jmid::time_division_t tdiv, std::int32_t def) {
	if (jmid::is_tpq(tdiv)) {
		return tdiv.get_tpq();
	}
	return def;
}


double jmid::ticks2sec(std::int32_t tks, const jmid::time_division_t& tdiv,
					std::int32_t tempo) {
	if (jmid::is_tpq(tdiv)) {
		double n = static_cast<double>(tempo)*static_cast<double>(tks);  // (tk*us)/q
		double d = (tdiv.get_tpq())*1000000.0;  // (tk*us)/(q*s)
		return n/d;
	} else {
		auto smpte = tdiv.get_smpte();
		auto frames_sec = static_cast<double>(-1.0*smpte.time_code);
		auto tks_frame = static_cast<double>(smpte.subframes);
		return static_cast<double>(tks)/(frames_sec*tks_frame);
	}
}
std::int32_t jmid::sec2ticks(const double& sec, const jmid::time_division_t& tdiv,
					std::int32_t tempo) {
	if (jmid::is_tpq(tdiv)) {
		if (tempo==0  || sec<=0.0) {
			return 0;
		}
		double n = sec*1000000.0*static_cast<double>(tdiv.get_tpq());
		return static_cast<int32_t>(n/static_cast<double>(tempo));
	} else {
		auto smpte = tdiv.get_smpte();
		auto frames_sec = static_cast<double>(-1.0*smpte.time_code);
		auto tks_frame = static_cast<double>(smpte.subframes);
		return static_cast<int32_t>(sec/(frames_sec*tks_frame));
	}
}

std::int32_t jmid::note2ticks(std::int32_t pow2, std::int32_t ndots, 
					const jmid::time_division_t& tdiv) {
	if ((!jmid::is_tpq(tdiv)) || (ndots<0)) {
		return 0;
	}

	double dot_mod = 1.0+((std::exp2(ndots)-1.0)/std::exp2(ndots));
	double nw = std::exp2(-pow2)*dot_mod;  // number-of-whole-notes
	double nq = 4*nw;  // number-of-quarter-notes
	return static_cast<std::int32_t>(nq*(tdiv.get_tpq()));
}
jmid::note_value_t jmid::ticks2note(std::int32_t nticks, const jmid::time_division_t& tdiv) {
	if ((!jmid::is_tpq(tdiv)) || (nticks<0)) {
		return {11,0};  // 2^11==2048; a 2048'th note
	}
	// TODO:  Appalling
	// In addition to the obvious, note that the outter loop moves from
	// _large_ towards _small_ note-values, but the inner loop moves from
	// _small_ to _large_ ...
	jmid::note_value_t best {-3,0};
	std::int32_t err_best = 0x0FFFFFFF;
	for (int bexp=-3; bexp<10; ++bexp) {
		for (int ndot=0; ndot<4; ++ndot) {
			jmid::note_value_t curr {bexp,ndot};
			auto calc_ntk = jmid::note2ticks(bexp,ndot,tdiv);
			auto err_curr = std::abs(nticks-calc_ntk);
			if (err_curr < err_best) {
				best = curr;
				err_best = err_curr;
			} else if (err_curr == err_best) {
				if (curr.ndot < best.ndot) {
					best = curr;
					err_best = err_curr;
				}
			}
		}
	}
	return best;
}
std::string jmid::print(const jmid::note_value_t& nv) {
	std::string s;
	switch (nv.exp) {
		case -3:  s = "ow";  // octuple-whole
			break;
		case -2:  s = "qw";
			break;
		case -1:  s = "dw";
			break;
		case 0:  s = "w";
			break;
		case 1:  s = "h";
			break;
		case 2:  s = "q";
			break;
		case 3:  s = "e";
			break;
		case 4:  s = "sx";
			break;
		case 5:  s = "ths";  // 32'nd
			break;
		case 6:  s = "sxf";
			break;
		case 7:  s = "ote";  // 128'th
			break;
		default:  s = "?";
			break;
	};

	for (int i=0; i<nv.ndot; ++i) {
		s += '.';
	}

	return s;
}



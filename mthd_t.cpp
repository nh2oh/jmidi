#include "mthd_t.h"
#include <cstdint>
#include <string>
#include <algorithm>  // std::copy() in mthd_t ctor(s), std::clamp()
#include <limits>  // in format, ntrks, ... setters

//
// time-division field stuff
//
tpq_t operator ""_tpq(unsigned long long val) {
	return tpq_t {static_cast<uint16_t>(val)};
}
time_code_t operator ""_time_code(unsigned long long val) {
	long long sval = static_cast<long long>(val);
	return time_code_t {static_cast<int8_t>(sval)};
}
time_code_t& time_code_t::operator-() {
	return *this;
}
subframes_t operator ""_subframes(unsigned long long val) {
	return subframes_t {static_cast<uint8_t>(val)};
}

time_division_t::time_division_t(uint16_t val) {
	// val is the value of the d_ array deserialized as a BE-encoded quantity
	if ((val>>15) == 1) {  // smpte
		// TODO:  By my reading of the std, it seems to me i should be
		// masking w/ 0x7Fu below...
		unsigned char high = (val>>8);//&0x7Fu;
		// Interpret the bit-pattern of high as a signed int8
		int8_t tcf = 0;
		auto p_tcf = static_cast<unsigned char*>(static_cast<void*>(&tcf));
		*p_tcf = high;
		if ((tcf != -24) && (tcf != -25) 
				&& (tcf != -29) && (tcf != -30)) {
			tcf = -24;
		}
		this->d_[0] = *p_tcf;
		this->d_[0] |= 0x80u;
		this->d_[1] = (val&0x00FFu);
	} else {  // tpq; high bit of val is clear
		this->d_[0] = (val>>8);
		this->d_[1] = (val&0x00FFu);
	}
}
time_division_t::time_division_t(tpq_t tpq) {
	this->d_[0] = ((tpq.value)>>8);
	this->d_[1] = ((tpq.value)&0x00FFu);
}
time_division_t::time_division_t(time_code_t tcf, subframes_t upf) {
	if ((tcf.value != -24) && (tcf.value != -25) 
			&& (tcf.value != -29) && (tcf.value != -30)) {
		tcf.value = -24;
	}
	// The high-byte of val has the bit 8 set, and bits [7,0] have the 
	// bit-pattern of tcf.  The low-byte has the bit-pattern of upf.  
	auto p_tcf = static_cast<unsigned char*>(static_cast<void*>(&(tcf.value)));
	this->d_[0] = *p_tcf;
	this->d_[0] |= 0x80u;
	this->d_[1] = upf.value;
}
// Since the time_division_t ctors enforce the always-valid-data invariant,
// this always returns a valid raw value, and no checks made in the other
// getters are needed here.  
uint16_t time_division_t::raw_value() const {
	uint16_t result = 0;
	result += this->d_[0];
	result <<= 8;
	result += this->d_[1];
	return result;
}
smpte_t time_division_t::get_smpte() const {
	smpte_t result;
	result.time_code = static_cast<int8_t>(this->d_[0]);
	result.subframes = static_cast<int8_t>(this->d_[1]);
	if ((result.time_code != -24) && (result.time_code != -25) 
			&& (result.time_code != -29) && (result.time_code != -30)) {
		result.time_code = -24;
	}
	return result;
}
uint16_t time_division_t::get_tpq() const {
	uint16_t result = ((this->raw_value()) & 0x7FFFu);
	return result;
}
time_division_t make_time_division_tpq(uint16_t tpq) {
	time_division_t result(tpq_t{tpq});
	return result;
}
time_division_t make_time_division_smpte(int8_t tcf, uint8_t upf) {
	// "time-code-format," and "units-per-frame"
	time_division_t result(time_code_t{tcf}, subframes_t{upf});
	return result;
}
time_division_t::type type(time_division_t tdf) {
	if ((tdf.raw_value()>>15) == 1) {
		return time_division_t::type::smpte;
	} else {
		return time_division_t::type::ticks_per_quarter;
	}
}
bool is_smpte(time_division_t tdf) {
	return (tdf.raw_value()>>15==1);
}
bool is_tpq(time_division_t tdf) {
	return (tdf.raw_value()>>15==0);
}
bool is_valid_time_division_raw_value(uint16_t val) {
	if ((val>>15) == 1) {  // smpte
		// TODO:  By my reading of the std, it seems to me i should be
		// masking w/ 0x7Fu below...
		unsigned char high = (val>>8);//&0x7Fu;
		// Interpret the bit-pattern of high as a signed int8
		int8_t tcf = 0;
		auto p_tcf = static_cast<unsigned char*>(static_cast<void*>(&tcf));
		*p_tcf = high;
		if ((tcf != -24) && (tcf != -25) 
				&& (tcf != -29) && (tcf != -30)) {
			return false;
		}
	} else {  // tpq; high bit of val is clear
		//...
	}
	return true;
}
int8_t get_time_code_fmt(time_division_t tdf, int8_t def) {
	if (is_smpte(tdf)) {
		return static_cast<int8_t>(tdf.raw_value()>>8);
	} else {
		return def;
	}
}
uint8_t get_units_per_frame(time_division_t tdf, uint8_t def) {
	if (is_smpte(tdf)) {
		return static_cast<uint8_t>(tdf.raw_value() & 0x00FFu);
	} else {
		return def;
	}
}
uint16_t get_tpq(time_division_t tdf, uint16_t def) {
	if (is_tpq(tdf)) {
		return (tdf.raw_value() & 0x7FFFu);
	} else {
		return def;
	}
}


//
// mthd_t class & related methods
//
mthd_t::mthd_t(int32_t fmt, int32_t ntrks, time_division_t tdf) {
	this->set_format(fmt);
	this->set_ntrks(ntrks);
	this->set_division(tdf);
}
mthd_t::mthd_t(const validate_mthd_chunk_result_t& mthd_v) {
	if (mthd_v.error == mthd_validation_error::no_error) {
		*this = mthd_t(mthd_v.p,mthd_v.size);
	}
}
mthd_t::mthd_t(const unsigned char *p, mthd_t::size_type n) {
	this->d_.clear();
	this->d_.reserve(n);
	std::copy(p,p+n,std::back_inserter(this->d_));
}

mthd_t::size_type mthd_t::size() const {
	return this->d_.size();
}
mthd_t::size_type mthd_t::nbytes() const {
	return this->d_.size();
}
mthd_t::pointer mthd_t::data() {
	return this->d_.data();
}
mthd_t::const_pointer mthd_t::data() const {
	return this->d_.data();
}
mthd_t::reference mthd_t::operator[](mthd_t::size_type idx) {
	return this->d_[idx];
}
mthd_t::const_reference mthd_t::operator[](mthd_t::size_type idx) const {
	return this->d_[idx];
}
mthd_t::iterator mthd_t::begin() {
	return mthd_t::iterator(this->d_.data());
}
mthd_t::iterator mthd_t::end() {
	return mthd_t::iterator(this->d_.data()) + this->d_.size();
}
mthd_t::const_iterator mthd_t::begin() const {
	return mthd_t::const_iterator(this->d_.data());
}
mthd_t::const_iterator mthd_t::end() const {
	return mthd_t::const_iterator(this->d_.data()) + this->d_.size();
}
mthd_t::const_iterator mthd_t::cbegin() const {
	return mthd_t::const_iterator(this->d_.data());
}
mthd_t::const_iterator mthd_t::cend() const {
	return mthd_t::const_iterator(this->d_.data()) + this->d_.size();
}

uint32_t mthd_t::length() const {
	int32_t l = read_be<uint32_t>(this->d_.cbegin()+4,this->d_.cend());
	return l;
}
int32_t mthd_t::format() const {
	int32_t f = read_be<uint16_t>(this->d_.cbegin()+8,this->d_.cend());
	return std::clamp(f,0,2);
}
int32_t mthd_t::ntrks() const {
	return read_be<uint16_t>(this->d_.cbegin()+8+2,this->d_.cend());
}
time_division_t mthd_t::division() const {
	return time_division_t(read_be<uint16_t>(this->d_.cbegin()+12,this->d_.cend()));
}
int32_t mthd_t::set_format(int32_t f) {
	if (this->d_.size() < 14) {
		this->d_.resize(14,0x00u);
	}
	f = std::clamp(f,0,2);
	write_16bit_be(static_cast<uint16_t>(f),this->d_.begin()+8);
	return this->format();
}
int32_t mthd_t::set_ntrks(int32_t ntrks) {
	if (this->d_.size() < 14) {
		this->d_.resize(14,0x00u);
	}
	ntrks = std::clamp(ntrks,0,
		static_cast<int32_t>(std::numeric_limits<uint16_t>::max()));
	write_16bit_be(static_cast<uint16_t>(ntrks),this->d_.begin()+10);
	return this->ntrks();
}
time_division_t mthd_t::set_division(time_division_t tdf) {
	if (this->d_.size() < 14) {
		this->d_.resize(14,0x00u);
	}
	write_16bit_be(tdf.raw_value(),this->d_.begin()+12);
	return this->division();
}
uint32_t mthd_t::set_length(uint32_t len) {
	if (this->d_.size() < 14) {
		this->d_.resize(14,0x00u);
	}
	write_32bit_be(len,this->d_.begin()+4);
	return this->length();
}
bool mthd_t::verify() const {
	auto v = validate_mthd_chunk(this->d_.data(),this->d_.size());
	return (v.error == mthd_validation_error::no_error);
}
void mthd_t::set_to_default_value() {
	auto def = mthd_t();
	this->d_ = def.d_;
}


std::string print(const mthd_t& mthd) {
	std::string s;  s.reserve(200);
	return print(mthd,s);
}
std::string& print(const mthd_t& mthd, std::string& s) {
	s += ("Header (MThd):  size() = " + std::to_string(mthd.size()) + ":\n");
	s += ("\tFormat type = " + std::to_string(mthd.format()) + ", ");
	s += ("Num Tracks = " + std::to_string(mthd.ntrks()) + ", ");
	s += "Time Division = ";
	auto timediv_type = type(mthd.division());
	if (timediv_type == time_division_t::type::smpte) {
		s += "(SMPTE) WTF";
	} else if (timediv_type == time_division_t::type::ticks_per_quarter) {
		s += "(ticks-per-quarter-note) ";
		s += std::to_string(get_tpq(mthd.division()));
	}
	s += "\n\t";
	print_hexascii(mthd.cbegin(), mthd.cend(), std::back_inserter(s), '\0',' ');

	return s;
}




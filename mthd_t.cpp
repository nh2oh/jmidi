#include "mthd_t.h"
#include "dbklib\byte_manipulation.h"
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>  // std::copy() in mthd_t::set(), std::clamp()
#include <cmath>  // std::abs()

mthd_t::mthd_t(int32_t fmt, int32_t ntrks, time_division_t tdf) {
	this->set_format(fmt);
	this->set_ntrks(ntrks);
	this->set_division(tdf);
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
	time_division_t result;
	result.val_ = read_be<uint16_t>(this->d_.cbegin()+8+2+2,this->d_.cend());
	return get(result);
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
	tdf = get(tdf);
	write_16bit_be(tdf.val_,this->d_.begin()+12);
	return this->division();
}
uint32_t mthd_t::set_length(uint32_t len) {
	if (this->d_.size() < 14) {
		this->d_.resize(14,0x00u);
	}
	write_32bit_be(len,this->d_.begin()+4);
	return this->length();
}

void mthd_t::set_to_default_value() {
	auto def = mthd_t();
	this->d_ = def.d_;
}





std::string print(const mthd_t& mthd) {
	std::string s {};

	s += ("Header (MThd);  size() = " + std::to_string(mthd.size()) + ":\n");
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
	s += dbk::print_hexascii(mthd.data(), mthd.size(), ' ');

	return s;
}


//
// time-division field stuff
//
bool verify(time_division_t tdf) {
	if (is_smpte(tdf)) {
		auto tcf = get_time_code_fmt(tdf);
		if ((tcf != -24) && (tcf != -25) 
				&& (tcf != -29) && (tcf != -30)) {
			return false;
		}
	}
	return true;
}
time_division_t get(time_division_t tdf, time_division_t def) {
	if (!verify(tdf)) {
		return def;
	} else {
		return tdf;
	}
}
time_division_t make_time_division_tpq(uint16_t tpq) {
	time_division_t result;
	result.val_ = (tpq & 0x7FFFu);
	return result;
}
time_division_t make_time_division_smpte(int8_t tcf, uint8_t upf) {
	// "time-code-format," and "units-per-frame"
	time_division_t result;
	if ((tcf != -24) && (tcf != -25) 
				&& (tcf != -29) && (tcf != -30)) {
		tcf = -24;
	}
	auto p_src = static_cast<unsigned char*>(static_cast<void*>(&tcf));
	auto p_dest = static_cast<unsigned char*>(static_cast<void*>(&(result.val_)));
	*p_dest++ = *p_src;
	result.val_ |= 0x8000u;
	result.val_ += (0x00FFu & upf);
	return result;
}
time_division_t::type type(time_division_t tdf) {
	if ((tdf.val_>>15) == 1) {
		return time_division_t::type::smpte;
	} else {
		return time_division_t::type::ticks_per_quarter;
	}
}
bool is_smpte(time_division_t tdf) {
	return (tdf.val_>>15==1);
}
bool is_tpq(time_division_t tdf) {
	return (tdf.val_>>15==0);
}
int8_t get_time_code_fmt(time_division_t tdf, int8_t def) {
	if (is_smpte(tdf)) {
		return static_cast<int8_t>(tdf.val_>>8);
	} else {
		return def;
	}
}
uint8_t get_units_per_frame(time_division_t tdf, uint8_t def) {
	if (is_smpte(tdf)) {
		return static_cast<uint8_t>(tdf.val_ & 0x00FFu);
	} else {
		return def;
	}
}
uint16_t get_tpq(time_division_t tdf, uint16_t def) {
	if (is_tpq(tdf)) {
		return (tdf.val_ & 0x7FFFu);
	} else {
		return def;
	}
}



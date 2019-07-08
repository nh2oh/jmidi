#include "mthd_t.h"
#include "dbklib\byte_manipulation.h"
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>  // std::copy() in mthd_t::set(), std::clamp()
#include <cmath>  // std::abs()

mthd_t::size_type mthd_t::size() const {
	return this->d_.size();
}
mthd_t::pointer mthd_t::data() {
	return this->d_.data();
}
mthd_t::const_pointer mthd_t::data() const {
	return this->d_.data();
}
int32_t mthd_t::format() const {
	//auto p = this->data();
	//return read_be<uint16_t>(p+8,p+this->size());
	int32_t f = read_be<uint16_t>(this->d_.cbegin()+8,this->d_.cend());
	return std::clamp(f,0,2);
}
int32_t mthd_t::ntrks() const {
	//auto p = this->data();
	//return read_be<uint16_t>(p+8+2,p+this->size());
	return read_be<uint16_t>(this->d_.cbegin()+8+2,this->d_.cend());
}
time_division_t mthd_t::division() const {
	//auto p = this->data();
	//return read_be<uint16_t>(p+8+2+2,p+this->size());
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
bool mthd_t::set(const std::vector<unsigned char>& d) {
	this->d_ = d;
	return true;
}
bool mthd_t::set(const validate_mthd_chunk_result_t& mthd_val) {
	if (mthd_val.error==mthd_validation_error::no_error) {
		this->d_.clear();
		this->d_.reserve(mthd_val.size);
		std::copy(mthd_val.p,mthd_val.p+mthd_val.size,
			std::back_inserter(this->d_));
		return true;
	}
	return false;
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












midi_time_division_field_type_t detect_midi_time_division_type(uint16_t division_field) {
	auto x = 5;
	if ((division_field>>15) == 1) {
		return midi_time_division_field_type_t::SMPTE;
	} else {
		return midi_time_division_field_type_t::ticks_per_quarter;
	}
}
// assumes midi_time_division_field_type_t::ticks_per_quarter
uint16_t interpret_tpq_field(uint16_t division_field) {
	return division_field&(0x7FFF);
}
// assumes midi_time_division_field_type_t::SMPTE
midi_smpte_field interpret_smpte_field(uint16_t division_field) {
	midi_smpte_field result {};
	result.time_code_fmt = static_cast<int8_t>(division_field>>8);
	// To get the magnitude-value of result.time_code_fmt (ex, if 
	// time_code_fmt==-30, get the number 30), do: 
	// (~(division_field>>8))+1;
	
	// Number of "subframes" per frame; also => midi-ticks per frame
	result.units_per_frame = static_cast<uint8_t>(division_field&0xFF);

	return result;
}




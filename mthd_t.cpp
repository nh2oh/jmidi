#include "mthd_t.h"
#include "dbklib\byte_manipulation.h"
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>



mthd_view_t::mthd_view_t(const validate_mthd_chunk_result_t& mthd) {
	if (mthd.error != mthd_validation_error::no_error) {
		std::abort();
	}
	this->p_=mthd.p;
	this->size_=mthd.size;
}
// Private ctor for smf_t
mthd_view_t::mthd_view_t(const unsigned char *p, uint32_t sz) {
	this->p_=p;
	this->size_=sz;
}
uint16_t mthd_view_t::format() const {
	return dbk::be_2_native<uint16_t>(this->p_+8);
}
uint16_t mthd_view_t::ntrks() const {
	return dbk::be_2_native<uint16_t>(this->p_+8+2);
}
uint16_t mthd_view_t::division() const {
	return dbk::be_2_native<uint16_t>(this->p_+8+2+2);
}
uint32_t mthd_view_t::size() const {
	return this->size_;
}
uint32_t mthd_view_t::data_size() const {
	return this->size_-8;
}
const unsigned char *mthd_view_t::raw_data() const {
	return this->p_;
}

std::string print(const mthd_view_t& mthd) {
	std::string s {};

	s += "Header (MThd)\tdata_size = " 
		+ std::to_string(mthd.data_size())
		+ ", size = " + std::to_string(mthd.size()) + ":\n";

	s += ("Format type = " + std::to_string(mthd.format()) + ", ");
	s += ("Num Tracks = " + std::to_string(mthd.ntrks()) + ", ");
	s += "Time Division = ";
	auto timediv_type = detect_midi_time_division_type(mthd.division());
	if (timediv_type == midi_time_division_field_type_t::SMPTE) {
		s += "(SMPTE) WTF";
	} else if (timediv_type == midi_time_division_field_type_t::ticks_per_quarter) {
		s += "(ticks-per-quarter-note) ";
		s += std::to_string(interpret_tpq_field(mthd.division()));
	}
	s += "\n\t";
	s += dbk::print_hexascii(mthd.raw_data(), mthd.size(), ' ');

	return s;
}



uint16_t mthd_t::format() const {
	auto tv = mthd_view_t(this->d_.data(),this->d_.size());
	return tv.format();
}
uint16_t mthd_t::ntrks() const {
	auto tv = mthd_view_t(this->d_.data(),this->d_.size());
	return tv.ntrks();
}
uint16_t mthd_t::division() const {
	auto tv = mthd_view_t(this->d_.data(),this->d_.size());
	return tv.division();
}
uint32_t mthd_t::data_size() const {
	auto tv = mthd_view_t(this->d_.data(),this->d_.size());
	return tv.data_size();
}
uint32_t mthd_t::size() const {
	return this->d_.size();
}
mthd_view_t mthd_t::get_view() const {
	return mthd_view_t(this->d_.data(),this->d_.size());
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
	return print(mthd.get_view());
}


//
// time-division field stuff
//

midi_time_division_field_type_t detect_midi_time_division_type(uint16_t division_field) {
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




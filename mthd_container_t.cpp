#include "mthd_container_t.h"
#include <cstdint>
#include <string>


mthd_container_t::mthd_container_t(const validate_mthd_chunk_result_t& mthd) {
	if (!mthd.is_valid) {
		std::abort();
	}
	this->p_=mthd.p;
	this->size_=mthd.size;
}
int16_t mthd_container_t::format() const {
	return be_2_native<int16_t>(this->p_+8);
}
int16_t mthd_container_t::ntrks() const {
	return be_2_native<int16_t>(this->p_+8+2);
}
uint16_t mthd_container_t::division() const {
	return be_2_native<uint16_t>(this->p_+8+2+2);
}
uint32_t mthd_container_t::size() const {
	return this->size_;
}
int32_t mthd_container_t::data_size() const {
	return this->size_-8;
}

std::string print(const mthd_container_t& mthd) {
	std::string s {};
	s += ("Data size = " + std::to_string(mthd.data_size()) + "    \n");
	s += ("Format type = " + std::to_string(mthd.format()) + "    \n");
	s += ("Num Tracks = " + std::to_string(mthd.ntrks()) + "    \n");

	s += "Time Division = ";
	auto timediv_type = detect_midi_time_division_type(mthd.division());
	if (timediv_type == midi_time_division_field_type_t::SMPTE) {
		s += "(SMPTE) WTF";
	} else if (timediv_type == midi_time_division_field_type_t::ticks_per_quarter) {
		s += "(ticks-per-quarter-note) ";
		s += std::to_string(interpret_tpq_field(mthd.division()));
	}
	s += "\n";

	return s;
}


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
	result.units_per_frame = static_cast<uint8_t>(division_field&0xFF);
	return result;
}

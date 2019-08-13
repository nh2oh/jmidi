#include "mthd_t.h"
#include "mtrk_event_t_internal.h"
#include "generic_chunk_low_level.h"
#include "midi_raw.h"
#include "midi_vlq.h"
#include "print_hexascii.h"
#include <cstdint>
#include <cstddef>  // std::ptrdiff_t
#include <cstring>  // std::memcpy()
#include <string>
#include <algorithm>  // std::clamp()


const std::array<unsigned char,14> mthd_t::def_ {
	0x4Du,0x54u,0x68u,0x64u, 0x00u,0x00u,0x00u,0x06u,
	0x00u,0x01u, 0x00u,0x00u, 0x00u,0x78u
};

void mthd_t::default_init() noexcept {  // private
	this->d_ = mtrk_event_t_internal::small_bytevec_t();
	this->d_.resize_small2small_nocopy(14);
	std::memcpy(this->d_.begin(),&(mthd_t::def_[0]),mthd_t::def_.size());
}

mthd_t::mthd_t() noexcept {
	this->default_init();
}
mthd_t::mthd_t(mthd_t::init_small_w_size_0_t) noexcept {
	this->d_ = mtrk_event_t_internal::small_bytevec_t();
}
mthd_t::mthd_t(int32_t fmt, int32_t ntrks, time_division_t tdf) noexcept {
	this->default_init();
	this->set_format(fmt);
	this->set_ntrks(ntrks);
	this->set_division(tdf);
}
mthd_t::mthd_t(int32_t fmt, int32_t ntrks, int32_t tpq) noexcept {
	this->default_init();
	this->set_format(fmt);
	this->set_ntrks(ntrks);
	this->set_division(time_division_t(tpq));
}
mthd_t::mthd_t(int32_t fmt, int32_t ntrks, int32_t tcf, int32_t subdivs) noexcept {
	this->default_init();
	this->set_format(fmt);
	this->set_ntrks(ntrks);
	this->set_division(time_division_t(tcf,subdivs));
}
mthd_t::size_type mthd_t::size() const noexcept {
	return this->d_.size();
}
mthd_t::size_type mthd_t::nbytes() const noexcept {
	return this->size();
}
mthd_t::const_pointer mthd_t::data() noexcept {
	return this->d_.begin();
}
mthd_t::const_pointer mthd_t::data() const noexcept {
	return this->d_.begin();
}
mthd_t::value_type mthd_t::operator[](mthd_t::size_type idx) noexcept {
	return *(this->d_.begin()+idx);
}
mthd_t::value_type mthd_t::operator[](mthd_t::size_type idx) const noexcept {
	return *(this->d_.begin()+idx);
}
mthd_t::const_iterator mthd_t::begin() noexcept {
	return mthd_t::const_iterator(this->d_.begin());
}
mthd_t::const_iterator mthd_t::end() noexcept {
	return mthd_t::const_iterator(this->d_.end());
}
mthd_t::const_iterator mthd_t::begin() const noexcept {
	return mthd_t::const_iterator(this->d_.begin());
}
mthd_t::const_iterator mthd_t::end() const noexcept {
	return mthd_t::const_iterator(this->d_.end());
}
mthd_t::const_iterator mthd_t::cbegin() const noexcept {
	return mthd_t::const_iterator(this->d_.begin());
}
mthd_t::const_iterator mthd_t::cend() const noexcept {
	return mthd_t::const_iterator(this->d_.end());
}

int32_t mthd_t::length() const noexcept {
	auto l = read_be<uint32_t>(this->d_.begin()+4,this->d_.end());
	return static_cast<int32_t>(l);
}
int32_t mthd_t::format() const noexcept {
	int32_t f = read_be<uint16_t>(this->d_.begin()+8,this->d_.end());
	return f;
}
int32_t mthd_t::ntrks() const noexcept {
	return read_be<uint16_t>(this->d_.begin()+8+2,this->d_.end());
}
time_division_t mthd_t::division() const noexcept {
	auto raw_val = read_be<uint16_t>(this->d_.begin()+12,this->d_.end());
	return make_time_division_from_raw(raw_val).value;
}
int32_t mthd_t::set_format(int32_t f) noexcept {
	if (this->ntrks() > 1) {
		f = std::clamp(f,1,mthd_t::format_max);
	} else {
		f = std::clamp(f,mthd_t::format_min,mthd_t::format_max);
	}
	write_16bit_be(static_cast<uint16_t>(f),this->d_.begin()+8);
	return f;
}
int32_t mthd_t::set_ntrks(int32_t ntrks) noexcept {
	auto f = this->format();
	if (f==0) {
		ntrks = std::clamp(ntrks,mthd_t::ntrks_min,
			mthd_t::ntrks_max_fmt_0);
	} else {
		ntrks = std::clamp(ntrks,mthd_t::ntrks_min,
			mthd_t::ntrks_max_fmt_gt0);
	}
	write_16bit_be(static_cast<uint16_t>(ntrks),this->d_.begin()+10);
	return ntrks;
}
time_division_t mthd_t::set_division(time_division_t tdf) noexcept {
	write_16bit_be(tdf.get_raw_value(),this->d_.begin()+12);
	return tdf;
}
int32_t mthd_t::set_division_tpq(int32_t tpq) noexcept {
	auto tdiv = time_division_t(tpq);
	write_16bit_be(tdiv.get_raw_value(),this->d_.begin()+12);
	return tdiv.get_tpq();
}
smpte_t mthd_t::set_division_smpte(int32_t tcf, int32_t subdivs) noexcept {
	auto tdiv = time_division_t(tcf,subdivs);
	write_16bit_be(tdiv.get_raw_value(),this->d_.begin()+12);
	return tdiv.get_smpte();
}
smpte_t mthd_t::set_division_smpte(smpte_t smpte) noexcept {
	return this->set_division_smpte(smpte.time_code,smpte.subframes);
}
int32_t mthd_t::set_length(int32_t new_len) {
	new_len = std::clamp(new_len,mthd_t::length_min,mthd_t::length_max);
	int32_t l = this->length();
	if (new_len != l) {
		l = this->d_.resize(new_len+8)-8;
		write_32bit_be(static_cast<uint32_t>(l),this->d_.begin()+4);
	}
	return l;
}

std::string explain(const mthd_error_t& err) {
	std::string s;
	if (err.code==mthd_error_t::errc::no_error) {
		return s;
	}
	s.reserve(250);

	s += "Invalid MThd chunk:  ";
	if (err.code==mthd_error_t::errc::header_overflow) {
		s += "Encountered end-of-input after reading < 8 bytes.  ";
	} else if (err.code==mthd_error_t::errc::non_mthd_id) {
		s += "Invalid ID field; expected the first 4 bytes to be "
			"'MThd' (0x4D,54,68,64).  ";
	} else if (err.code==mthd_error_t::errc::invalid_length) {
		s += "The length field in the chunk header encodes the value ";
		s += std::to_string(read_be<uint32_t>(err.header.data()+4,err.header.data()+8));
		s += ".  This library requires that MThd chunks have length >= 6 && <= "
			"mthd_t::length_max == ";
		s += std::to_string(mthd_t::length_max);
		s += ".  ";
	} else if (err.code==mthd_error_t::errc::overflow_in_data_section) {
		auto p = err.header.data();
		s += "Encountered end-of-input after reading < 'length' bytes; "
			"length == " + std::to_string(read_be<uint32_t>(p+4,p+8))
			+ ".  ";
	} else if (err.code==mthd_error_t::errc::invalid_time_division) {
		int8_t time_code = 0;
		uint16_t division = read_be<uint16_t>(err.header.data()+12,err.header.data()+14);
		uint8_t subframes = (division&0x00FFu);
		uint8_t high = (division>>8);
		auto psrc = static_cast<const unsigned char*>(static_cast<const void*>(&high));
		auto pdest = static_cast<unsigned char*>(static_cast<void*>(&time_code));
		*pdest = *psrc;
		s += "The value of field 'division' is invalid.  It is probably an "
			"SMPTE-type field attempting to specify a time-code of something "
			"other than -24, -25, -29, or -30.  division == ";
		s += std::to_string(division);
		s += " => time-code == ";
		s += std::to_string(time_code);
		s += " => ticks-per-frame == ";
		s += std::to_string(subframes);
		s += ".  ";
	} else if (err.code==mthd_error_t::errc::inconsistent_format_ntrks) {
		uint16_t format = read_be<uint16_t>(err.header.data()+8,err.header.data()+10);
		uint16_t ntrks = read_be<uint16_t>(err.header.data()+10,err.header.data()+12);
		s += "The values encoded by 'format' 'division' are inconsistent.  "
			"In a format==0 SMF, ntrks must be <= 1.  format == ";
		s += std::to_string(format);
		s += ", ntrks == ";
		s += std::to_string(ntrks);
		s += ".  ";
	} else if (err.code==mthd_error_t::errc::other) {
		s += "mthd_error_t::errc::other.  ";
	} else {
		s += "Unknown error.  ";
	}
	return s;
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
	auto timediv_type = mthd.division().get_type();
	if (timediv_type == time_division_t::type::smpte) {
		s += "SMPTE";
	} else if (timediv_type == time_division_t::type::ticks_per_quarter) {
		s += std::to_string(get_tpq(mthd.division()));
		s += " ticks-per-quarter-note ";
	}
	s += "\n\t";
	print_hexascii(mthd.cbegin(), mthd.cend(), std::back_inserter(s), '\0',' ');

	return s;
}


maybe_mthd_t::operator bool() const {
	return this->error==mthd_error_t::errc::no_error;
}


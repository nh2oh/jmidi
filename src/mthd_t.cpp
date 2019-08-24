#include "mthd_t.h"
#include "mtrk_event_t_internal.h"
#include "generic_chunk_low_level.h"
#include "midi_time.h"
#include "midi_vlq.h"
#include "print_hexascii.h"
#include <cstdint>
#include <cstddef>  // std::ptrdiff_t
#include <cstring>  // std::memcpy()
#include <string>
#include <algorithm>  // std::clamp()


const std::array<unsigned char,14> jmid::mthd_t::def_ {
	0x4Du,0x54u,0x68u,0x64u, 0x00u,0x00u,0x00u,0x06u,
	0x00u,0x01u, 0x00u,0x00u, 0x00u,0x78u
};

void jmid::mthd_t::default_init() noexcept {  // private
	this->d_ = mtrk_event_t_internal::small_bytevec_t();
	this->d_.resize_small2small_nocopy(14);
	std::memcpy(this->d_.begin(),&(jmid::mthd_t::def_[0]),jmid::mthd_t::def_.size());
}

jmid::mthd_t::mthd_t() noexcept {
	this->default_init();
}
jmid::mthd_t::mthd_t(jmid::mthd_t::init_small_w_size_0_t) noexcept {
	this->d_ = mtrk_event_t_internal::small_bytevec_t();
}
jmid::mthd_t::mthd_t(std::int32_t fmt, std::int32_t ntrks, jmid::time_division_t tdf) noexcept {
	this->default_init();
	this->set_ntrks(ntrks);
	this->set_format(fmt);
	this->set_division(tdf);
}
jmid::mthd_t::mthd_t(std::int32_t fmt, std::int32_t ntrks, std::int32_t tpq) noexcept {
	this->default_init();
	this->set_ntrks(ntrks);
	this->set_format(fmt);
	this->set_division(jmid::time_division_t(tpq));
}
jmid::mthd_t::mthd_t(std::int32_t fmt, std::int32_t ntrks, std::int32_t tcf, std::int32_t subdivs) noexcept {
	this->default_init();
	this->set_format(fmt);
	this->set_ntrks(ntrks);
	this->set_division(jmid::time_division_t(tcf,subdivs));
}
jmid::mthd_t::mthd_t(const jmid::mthd_t& rhs) {
	this->d_=rhs.d_;
}
jmid::mthd_t& jmid::mthd_t::operator=(const jmid::mthd_t& rhs) {
	this->d_ = rhs.d_;
	return *this;
}
jmid::mthd_t::mthd_t(jmid::mthd_t&& rhs) noexcept {
	this->d_ = std::move(rhs.d_);
	rhs.default_init();
}
jmid::mthd_t& jmid::mthd_t::operator=(jmid::mthd_t&& rhs) noexcept {
	this->d_ = std::move(rhs.d_);
	rhs.default_init();
	return *this;
}
jmid::mthd_t::~mthd_t() noexcept {  // dtor
	//...
}

jmid::mthd_t::size_type jmid::mthd_t::size() const noexcept {
	return this->d_.size();
}
jmid::mthd_t::size_type jmid::mthd_t::nbytes() const noexcept {
	return this->size();
}
jmid::mthd_t::const_pointer jmid::mthd_t::data() noexcept {
	return this->d_.begin();
}
jmid::mthd_t::const_pointer jmid::mthd_t::data() const noexcept {
	return this->d_.begin();
}
jmid::mthd_t::value_type jmid::mthd_t::operator[](jmid::mthd_t::size_type idx) noexcept {
	return *(this->d_.begin()+idx);
}
jmid::mthd_t::value_type jmid::mthd_t::operator[](jmid::mthd_t::size_type idx) const noexcept {
	return *(this->d_.begin()+idx);
}
jmid::mthd_t::const_iterator jmid::mthd_t::begin() noexcept {
	return jmid::mthd_t::const_iterator(this->d_.begin());
}
jmid::mthd_t::const_iterator jmid::mthd_t::end() noexcept {
	return jmid::mthd_t::const_iterator(this->d_.end());
}
jmid::mthd_t::const_iterator jmid::mthd_t::begin() const noexcept {
	return jmid::mthd_t::const_iterator(this->d_.begin());
}
jmid::mthd_t::const_iterator jmid::mthd_t::end() const noexcept {
	return jmid::mthd_t::const_iterator(this->d_.end());
}
jmid::mthd_t::const_iterator jmid::mthd_t::cbegin() const noexcept {
	return jmid::mthd_t::const_iterator(this->d_.begin());
}
jmid::mthd_t::const_iterator jmid::mthd_t::cend() const noexcept {
	return jmid::mthd_t::const_iterator(this->d_.end());
}

std::int32_t jmid::mthd_t::length() const noexcept {
	auto l = read_be<std::uint32_t>(this->d_.begin()+4,this->d_.end());
	return static_cast<std::int32_t>(l);
}
std::int32_t jmid::mthd_t::format() const noexcept {
	std::int32_t f = read_be<std::uint16_t>(this->d_.begin()+8,this->d_.end());
	return f;
}
std::int32_t jmid::mthd_t::ntrks() const noexcept {
	return read_be<std::uint16_t>(this->d_.begin()+8+2,this->d_.end());
}
jmid::time_division_t jmid::mthd_t::division() const noexcept {
	auto raw_val = read_be<std::uint16_t>(this->d_.begin()+12,this->d_.end());
	return jmid::make_time_division_from_raw(raw_val).value;
}
std::int32_t jmid::mthd_t::set_format(std::int32_t f) noexcept {
	f = std::clamp(f,jmid::mthd_t::format_min,jmid::mthd_t::format_max);
	if (f==0) {
		if (this->ntrks() > 1) {
			return this->format();
		}
	}
	write_16bit_be(static_cast<std::uint16_t>(f),this->d_.begin()+8);
	return f;
}
std::int32_t jmid::mthd_t::set_ntrks(std::int32_t ntrks) noexcept {
	ntrks = std::clamp(ntrks,0,jmid::mthd_t::ntrks_max_fmt_gt0);
	write_16bit_be(static_cast<std::uint16_t>(ntrks),this->d_.begin()+10);
	if (ntrks > 1) {
		auto f = this->format();
		if (f==0) {
			this->set_format(1);
		}
	}
	return ntrks;
}
jmid::time_division_t jmid::mthd_t::set_division(jmid::time_division_t tdf) noexcept {
	write_16bit_be(tdf.get_raw_value(),this->d_.begin()+12);
	return tdf;
}
std::int32_t jmid::mthd_t::set_division_tpq(std::int32_t tpq) noexcept {
	auto tdiv = jmid::time_division_t(tpq);
	write_16bit_be(tdiv.get_raw_value(),this->d_.begin()+12);
	return tdiv.get_tpq();
}
jmid::smpte_t jmid::mthd_t::set_division_smpte(std::int32_t tcf, std::int32_t subdivs) noexcept {
	auto tdiv = jmid::time_division_t(tcf,subdivs);
	write_16bit_be(tdiv.get_raw_value(),this->d_.begin()+12);
	return tdiv.get_smpte();
}
jmid::smpte_t jmid::mthd_t::set_division_smpte(jmid::smpte_t smpte) noexcept {
	return this->set_division_smpte(smpte.time_code,smpte.subframes);
}
std::int32_t jmid::mthd_t::set_length(std::int32_t new_len) {
	new_len = std::clamp(new_len,jmid::mthd_t::length_min,jmid::mthd_t::length_max);
	int32_t l = this->length();
	if (new_len != l) {
		l = this->d_.resize(new_len+8)-8;
		write_32bit_be(static_cast<std::uint32_t>(l),this->d_.begin()+4);
	}
	return l;
}

std::string jmid::explain(const jmid::mthd_error_t& err) {
	std::string s;
	if (err.code==jmid::mthd_error_t::errc::no_error) {
		return s;
	}
	s.reserve(250);

	s += "Invalid MThd chunk:  ";
	if (err.code==jmid::mthd_error_t::errc::header_overflow) {
		s += "Encountered end-of-input after reading < 8 bytes.  ";
	} else if (err.code==jmid::mthd_error_t::errc::non_mthd_id) {
		s += "Invalid ID field; expected the first 4 bytes to be "
			"'MThd' (0x4D,54,68,64).  ";
	} else if (err.code==jmid::mthd_error_t::errc::invalid_length) {
		s += "The length field in the chunk header \nencodes the value ";
		s += std::to_string(read_be<std::uint32_t>(err.header.data()+4,err.header.data()+8));
		s += ".  This library requires that MThd chunks have \nlength >= 6 && <= "
			"mthd_t::length_max == ";
		s += std::to_string(jmid::mthd_t::length_max);
		s += ".  ";
	} else if (err.code==jmid::mthd_error_t::errc::overflow_in_data_section) {
		auto p = err.header.data();
		s += "Encountered end-of-input after reading \n< 'length' bytes; "
			"length == " + std::to_string(read_be<std::uint32_t>(p+4,p+8))
			+ ".  ";
	} else if (err.code==jmid::mthd_error_t::errc::invalid_time_division) {
		std::int8_t time_code = 0;
		std::uint16_t division = read_be<uint16_t>(err.header.data()+12,err.header.data()+14);
		std::uint8_t subframes = (division&0x00FFu);
		std::uint8_t high = (division>>8);
		auto psrc = static_cast<const unsigned char*>(static_cast<const void*>(&high));
		auto pdest = static_cast<unsigned char*>(static_cast<void*>(&time_code));
		*pdest = *psrc;
		s += "The value of field 'division' is invalid.  \nIt is probably an "
			"SMPTE-type field attempting to specify a time-code \nof something "
			"other than -24, -25, -29, or -30.  \ndivision == ";
		s += std::to_string(division);
		s += " => time-code == ";
		s += std::to_string(time_code);
		s += " => ticks-per-frame == ";
		s += std::to_string(subframes);
		s += ".  ";
	} else if (err.code==jmid::mthd_error_t::errc::inconsistent_format_ntrks) {
		std::uint16_t format = read_be<std::uint16_t>(err.header.data()+8,err.header.data()+10);
		std::uint16_t ntrks = read_be<std::uint16_t>(err.header.data()+10,err.header.data()+12);
		s += "The values encoded by 'format' 'division' are \ninconsistent.  "
			"In a format==0 SMF, ntrks must be <= 1.  \nformat == ";
		s += std::to_string(format);
		s += ", ntrks == ";
		s += std::to_string(ntrks);
		s += ".  ";
	} else if (err.code==jmid::mthd_error_t::errc::other) {
		s += "mthd_error_t::errc::other.  ";
	} else {
		s += "Unknown error.  ";
	}
	return s;
}
std::string jmid::print(const jmid::mthd_t& mthd) {
	std::string s;  s.reserve(200);
	return jmid::print(mthd,s);
}
std::string& jmid::print(const jmid::mthd_t& mthd, std::string& s) {
	s += ("Header (MThd):  size() = " + std::to_string(mthd.size()) + ":\n");
	s += ("\tFormat type = " + std::to_string(mthd.format()) + ", ");
	s += ("Num Tracks = " + std::to_string(mthd.ntrks()) + ", ");
	s += "Time Division = ";
	auto timediv_type = mthd.division().get_type();
	if (timediv_type == jmid::time_division_t::type::smpte) {
		s += "SMPTE";
	} else if (timediv_type == jmid::time_division_t::type::ticks_per_quarter) {
		s += std::to_string(jmid::get_tpq(mthd.division()));
		s += " ticks-per-quarter-note ";
	}
	s += "\n\t";
	print_hexascii(mthd.cbegin(), mthd.cend(), std::back_inserter(s), '\0',' ');

	return s;
}

jmid::maybe_mthd_t::operator bool() const {
	return this->error==jmid::mthd_error_t::errc::no_error;
}


#include "mtrk_event_t.h"
#include "small_bytevec_t.h"
#include "midi_status_byte.h"
#include "midi_vlq.h"
#include "midi_delta_time.h"
#include "aux_types.h"
#include "print_hexascii.h"
#include <cstdint>
#include <algorithm>
#include <string>
#include <utility>  // std::move()


jmid::mtrk_event_t::mtrk_event_t(jmid::delta_time dt, 
								jmid::ch_event md) noexcept {
	auto s = (md.status_nybble()|md.ch());
	auto dest_beg = this->d_.resize_nocopy(7);
	auto dest = jmid::write_delta_time_unsafe(dt.get(),dest_beg);
	*dest++ = s;
	*dest++ = md.p1();
	if (jmid::channel_status_byte_n_data_bytes(s)==2) {
		*dest++ = md.p2();
	}
	this->d_.resize(dest-dest_beg);
}
jmid::mtrk_event_t::mtrk_event_t(jmid::delta_time dt, 
					jmid::meta_header mt, const unsigned char *beg, 
					const unsigned char *end) {
	if ((end-beg) != mt.length()) {
		this->d_.resize(0);
		return;
	}
	// 4 + 1 + 1 + 4 == 10; dt + 0xFF + mt-type + vlq-len
	auto dest_beg = this->d_.resize_nocopy(10);  // Probably oversized
	auto dest_end = jmid::write_delta_time_unsafe(dt.get(),dest_beg);
	*dest_end++ = 0xFFu;
	*dest_end++ = mt.type_byte();
	dest_end = jmid::write_vlq_unsafe(mt.length(),dest_end);
	dest_end = this->d_.resize((dest_end-dest_beg)+mt.length()) 
		+ (dest_end-dest_beg);
	dest_end = std::copy(beg,end,dest_end);
	// An alternative strategy is to resize to 10+mt.length(), write
	// everything, then resize to dest_end-dest_beg at the end.  This is 
	// suboptimal because the initial oversized resize may cause an 
	// unnecessary allocation.  
}
jmid::mtrk_event_t::mtrk_event_t(jmid::delta_time dt, 
					jmid::sysex_header sx, const unsigned char *beg, 
					const unsigned char *end) {
	if ((end-beg) != sx.length()) {
		this->d_.resize(0);
		return;
	}
	// 4 + 1 + 4 == 9; dt + 0xF0/F7 + vlq-len
	auto dest_beg = this->d_.resize_nocopy(9);  // Probably oversized
	auto dest_end = jmid::write_delta_time_unsafe(dt.get(),dest_beg);
	*dest_end++ = sx.type_byte();
	dest_end = jmid::write_vlq_unsafe(sx.length(),dest_end);
	dest_end = this->d_.resize((dest_end-dest_beg)+sx.length()) 
		+ (dest_end-dest_beg);
	dest_end = std::copy(beg,end,dest_end);
}
jmid::mtrk_event_t::mtrk_event_t(const jmid::mtrk_event_t& rhs) {
	this->d_=rhs.d_;
}
jmid::mtrk_event_t& jmid::mtrk_event_t::operator=(const jmid::mtrk_event_t& rhs) {
	this->d_ = rhs.d_;
	return *this;
}
jmid::mtrk_event_t::mtrk_event_t(jmid::mtrk_event_t&& rhs) noexcept {
	this->d_ = std::move(rhs.d_);
}
jmid::mtrk_event_t& jmid::mtrk_event_t::operator=(jmid::mtrk_event_t&& rhs) noexcept {
	this->d_ = std::move(rhs.d_);
	return *this;
}
jmid::mtrk_event_t::~mtrk_event_t() noexcept {  // dtor
	//...
}
void jmid::mtrk_event_t::clear() noexcept {
	this->d_.resize_nocopy(0);
}
void jmid::mtrk_event_t::replace_unsafe(std::int32_t dt,
										jmid::ch_event_data_t md) {
	auto s = (md.status_nybble|md.ch);
	auto dest_beg = this->d_.resize_nocopy(7);
	auto dest = jmid::write_delta_time_unsafe(dt,dest_beg);
	*dest++ = s;
	*dest++ = md.p1;
	if (jmid::channel_status_byte_n_data_bytes(s)==2) {
		*dest++ = md.p2;
	}
	this->d_.resize(dest-dest_beg);
}

jmid::mtrk_event_t::size_type jmid::mtrk_event_t::size() const noexcept {
	return this->d_.size();
}
constexpr jmid::mtrk_event_t::size_type jmid::mtrk_event_t::max_size() const noexcept {
	return 0x0FFFFFFF;
}
jmid::mtrk_event_t::size_type jmid::mtrk_event_t::capacity() const noexcept {
	return this->d_.capacity();
}
jmid::mtrk_event_t::size_type jmid::mtrk_event_t::reserve(jmid::mtrk_event_t::size_type new_cap) {
	new_cap = std::clamp(new_cap,0,this->max_size());
	return this->d_.reserve(new_cap);
}
bool jmid::mtrk_event_t::is_empty() const {
	return this->d_.size()==0;
}
const unsigned char *jmid::mtrk_event_t::data() noexcept {
	return this->d_.begin();
}
const unsigned char *jmid::mtrk_event_t::data() const noexcept {
	return this->d_.begin();
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::begin() noexcept {
	return jmid::mtrk_event_t::const_iterator(this->d_.begin());
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::begin() const noexcept {
	return jmid::mtrk_event_t::const_iterator(this->d_.begin());
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::cbegin() noexcept {
	return jmid::mtrk_event_t::const_iterator(this->d_.begin());
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::cbegin() const noexcept {
	return jmid::mtrk_event_t::const_iterator(this->d_.begin());
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::end() noexcept {
	return jmid::mtrk_event_t::const_iterator(this->d_.end());
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::end() const noexcept {
	return jmid::mtrk_event_t::const_iterator(this->d_.end());
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::cend() noexcept {
	return jmid::mtrk_event_t::const_iterator(this->d_.end());
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::cend() const noexcept {
	return jmid::mtrk_event_t::const_iterator(this->d_.end());
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::dt_begin() const noexcept {
	return jmid::mtrk_event_t::const_iterator(this->d_.begin());
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::dt_begin() noexcept {
	return jmid::mtrk_event_t::const_iterator(this->d_.begin());
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::dt_end() const noexcept {
	return jmid::advance_to_dt_end(this->d_.begin(),this->d_.end());
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::dt_end() noexcept {
	return jmid::advance_to_dt_end(this->d_.begin(),this->d_.end());
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::event_begin() const noexcept {
	return jmid::advance_to_dt_end(this->d_.begin(),this->d_.end());
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::event_begin() noexcept {
	return jmid::advance_to_dt_end(this->d_.begin(),this->d_.end());
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::payload_begin() const noexcept {
	return this->payload_range_impl().begin;
}
jmid::mtrk_event_t::const_iterator jmid::mtrk_event_t::payload_begin() noexcept {
	return this->payload_range_impl().begin;
}
jmid::mtrk_event_iterator_range_t jmid::mtrk_event_t::payload_range() const noexcept {
	return this->payload_range_impl();
}
jmid::mtrk_event_iterator_range_t jmid::mtrk_event_t::payload_range() noexcept {
	return this->payload_range_impl();
}
unsigned char jmid::mtrk_event_t::operator[](jmid::mtrk_event_t::size_type i) const noexcept {
	return *(this->d_.begin()+i);
};
unsigned char jmid::mtrk_event_t::operator[](jmid::mtrk_event_t::size_type i) noexcept {
	return *(this->d_.begin()+i);
};

jmid::mtrk_event_iterator_range_t jmid::mtrk_event_t::payload_range_impl() const noexcept {
	auto its = this->d_.data_range();
	if (its.begin!=its.end) {
		// If the object is_empty(), the manual increments of its.begin
		// for the meta and sysex cases will result in hard UB.  
		its.begin = jmid::advance_to_dt_end(its.begin,its.end);
		auto s = *(its.begin);
		if (jmid::is_meta_status_byte(s)) {
			its.begin += 2;  // 0xFFu, type-byte
			its.begin = jmid::advance_to_vlq_end(its.begin,its.end);
		} else if (jmid::is_sysex_status_byte(s)) {
			its.begin += 1;  // 0xF0u or 0xF7u
			its.begin = jmid::advance_to_vlq_end(its.begin,its.end);
		}
	}
	return {its.begin,its.end};
}
std::int32_t jmid::mtrk_event_t::delta_time() const noexcept {
	return jmid::read_delta_time(this->d_.begin(),this->d_.end()).val;
}
unsigned char jmid::mtrk_event_t::status_byte() const noexcept {
	auto its = this->d_.data_range();
	auto p = jmid::advance_to_dt_end(its.begin,its.end);
	return *p;
}
unsigned char jmid::mtrk_event_t::running_status() const noexcept {
	auto its = this->d_.data_range();
	auto p = jmid::advance_to_dt_end(its.begin,its.end);
	return jmid::get_running_status_byte(*p,0x00u);
}
jmid::mtrk_event_t::size_type jmid::mtrk_event_t::data_size() const noexcept {  // Not including delta-t
	auto its = this->d_.data_range();
	auto p = jmid::advance_to_dt_end(its.begin,its.end);
	return its.end - p;
}

jmid::ch_event_data_t jmid::mtrk_event_t::get_channel_event_data() const noexcept {
	// TODO:
	// The reason this is most efficiently implemented as a member function
	// is that if big, it can safely read d_.u_.b_.pad_ directly, skipping
	// the its.end-its.begin checks.  The present implementation is no
	// better than the free function impl.  
	jmid::ch_event_data_t result;
	result.status_nybble = 0x00u;  // Causes result to test invalid

	auto its = this->d_.pad_or_data_range();
	its.begin = jmid::advance_to_dt_end(its.begin,its.end);
	if (its.end-its.begin <= 2) {
		return result;
	}
	auto s = *(its.begin);
	if (!jmid::is_channel_status_byte(s)) {
		return result;
	}
	result.status_nybble = s&0xF0u;
	result.ch = s&0x0Fu;
	++(its.begin);
	result.p1 = *(its.begin);
	if (jmid::channel_status_byte_n_data_bytes(s)==2) {
		++(its.begin);
		result.p2 = *(its.begin);
	} else {
		result.p2 = 0x00u;
	}
	
	return result;
}
jmid::meta_header_data jmid::mtrk_event_t::get_meta() const noexcept {
	// TODO:
	// The reason this is most efficiently implemented as a member function
	// is that if big, it can safely read d_.u_.b_.pad_ directly, skipping
	// the its.end-its.begin checks and the pointer-chasing into the 
	// remote buffer.  
	jmid::meta_header_data result;

	auto its = this->d_.pad_or_data_range();
	its.begin = jmid::advance_to_dt_end(its.begin,its.end);
	if (its.end-its.begin < 3) {
		return result;
	}
	auto s = *(its.begin);
	if (!jmid::is_meta_status_byte(s)) {
		return result;
	}
	++(its.begin);
	result.type = *(its.begin);
	++(its.begin);
	result.length = jmid::read_vlq(its.begin,its.end).val;
	return result;
}
jmid::sysex_header_data jmid::mtrk_event_t::get_sysex() const noexcept {
	// TODO:
	// The reason this is most efficiently implemented as a member function
	// is that if big, it can safely read d_.u_.b_.pad_ directly, skipping
	// the its.end-its.begin checks and the pointer-chasing into the 
	// remote buffer.  
	jmid::sysex_header_data result;

	auto its = this->d_.pad_or_data_range();
	its.begin = jmid::advance_to_dt_end(its.begin,its.end);
	if (its.end-its.begin < 2) {
		return result;
	}
	result.type = *(its.begin);
	++(its.begin);
	result.length = jmid::read_vlq(its.begin,its.end).val;
	return result;
}

std::int32_t jmid::mtrk_event_t::set_delta_time(std::int32_t dt) {
	auto new_dt_size = jmid::delta_time_field_size(dt);
	auto beg = this->d_.begin();  auto end = this->d_.end();
	auto curr_dt_size = jmid::advance_to_dt_end(beg,end)-beg;
	if (curr_dt_size == new_dt_size) {
		jmid::write_delta_time(dt,beg);
	} else if (new_dt_size < curr_dt_size) {  // shrink present event
		auto curr_event_beg = beg+curr_dt_size;
		auto it = jmid::write_delta_time(dt,beg);
		it = std::copy(curr_event_beg,end,it);
		this->d_.resize(it-beg);
	} else if (new_dt_size > curr_dt_size) {  // grow present event
		auto old_size = this->d_.size();
		auto new_size = old_size + (new_dt_size-curr_dt_size);
		this->d_.resize(new_size);  // NB:  Invalidates iterators!
		auto new_beg = this->d_.begin();
		auto new_end = this->d_.end();
		std::copy_backward(new_beg,new_beg+old_size,new_end);
		jmid::write_delta_time(dt,this->d_.begin());
	}
	return this->delta_time();
}
std::string jmid::mtrk_event_t::debug_print() const {
	std::string s;
	if (this->d_.capacity() 
		<= jmid::internal::small_bytevec_t::capacity_small) {
		s += "small; {";
	} else {
		s += "big;   {";
	}
	jmid::print_hexascii(this->d_.raw_begin(), this->d_.raw_end(),
		std::back_inserter(s),'\0',' ');
	s += "};";
	return s;
}


bool jmid::operator==(const jmid::mtrk_event_t& lhs, 
						const jmid::mtrk_event_t& rhs) noexcept {
	auto l = lhs.d_.data_range();
	auto r = rhs.d_.data_range();
	if ((l.end-l.begin) != (r.end-r.begin)) {
		return false;
	}
	while ((l.begin!=l.end) && (r.begin!=r.end)) {
		if (*(l.begin)++ != *(r.begin)++) {
			return false;
		}
	}
	return true;
}
bool jmid::operator!=(const jmid::mtrk_event_t& lhs, 
						const jmid::mtrk_event_t& rhs) noexcept {
	return !(lhs==rhs);
}

std::string jmid::print(jmid::mtrk_event_error_t::errc ec) {
	std::string s;
	switch (ec) {
	case jmid::mtrk_event_error_t::errc::invalid_delta_time:
		s = "mtrk_event_error_t::errc::invalid_delta_time";
		break;
	case jmid::mtrk_event_error_t::errc::no_data_following_delta_time:
		s = "mtrk_event_error_t::errc::no_data_following_delta_time";
		break;
	case jmid::mtrk_event_error_t::errc::invalid_status_byte:
		s = "mtrk_event_error_t::errc::invalid_status_byte";
		break;
	case jmid::mtrk_event_error_t::errc::channel_calcd_length_exceeds_input:
		s = "mtrk_event_error_t::errc::channel_calcd_length_exceeds_input";
		break;
	case jmid::mtrk_event_error_t::errc::channel_invalid_data_byte:
		s = "mtrk_event_error_t::errc::channel_invalid_data_byte";
		break;
	case jmid::mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header:
		s = "mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header";
		break;
	case jmid::mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length:
		s = "mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length";
		break;
	case jmid::mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input:
		s = "mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input";
		break;
	case jmid::mtrk_event_error_t::errc::other:
		s = "mtrk_event_error_t::errc::other";
		break;
	default:
		s = "mtrk_event_error_t::errc::?";
		break;
	}
	return s;
}
std::string jmid::explain(const jmid::mtrk_event_error_t& err) {
	std::string s;  
	if (err.code==jmid::mtrk_event_error_t::errc::no_error) {
		return s;
	}
	s = "Invalid MTrk event:  ";

	if (err.code==jmid::mtrk_event_error_t::errc::invalid_delta_time) {
		s += "Invalid delta-time.  ";
	} else if (err.code==jmid::mtrk_event_error_t::errc::no_data_following_delta_time) {
		s += "Encountered end-of-input immediately following the delta-time field.";
	} else if (err.code==jmid::mtrk_event_error_t::errc::invalid_status_byte) {
		s += "Invalid status byte s == " 
			+ std::to_string(err.s) + ", rs == "
			+ std::to_string(err.rs) + ".  ";
	} else if (err.code==jmid::mtrk_event_error_t::errc::channel_calcd_length_exceeds_input) {
		s += "Encountered end-of-input prior to reading the number of expected "
			"data bytes for the present channel event.  ";
	} else if (err.code==jmid::mtrk_event_error_t::errc::channel_invalid_data_byte) {
		s += "Invalid channel event data byte.  ";
	} else if (err.code==jmid::mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header) {
		s += "Encountered end-of-input while processing the header of the present "
			"sysex of meta event.  ";
	} else if (err.code==jmid::mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length) {
		s += "The present sysex or meta event encodes an invalid length.  ";
	} else if (err.code==jmid::mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input) {
		s += "Encountered end-of-input while reading in the payload of the "
			"present sysex or meta event.  ";
	} else if (err.code==jmid::mtrk_event_error_t::errc::other) {
		s += "mtrk_event_error_t::errc::other.  ";
	} else {
		s += "Unknown error.  ";
	}
	return s;
}

jmid::validate_channel_event_result_t::operator bool() const {
	return this->error==jmid::mtrk_event_error_t::errc::no_error;
}
jmid::validate_meta_event_result_t::operator bool() const {
	return this->error==jmid::mtrk_event_error_t::errc::no_error;
}
jmid::validate_sysex_event_result_t::operator bool() const {
	return this->error==jmid::mtrk_event_error_t::errc::no_error;
}

jmid::validate_channel_event_result_t
jmid::validate_channel_event(const unsigned char *beg, const unsigned char *end,
						unsigned char rs) {
	jmid::validate_channel_event_result_t result;
	result.error = jmid::mtrk_event_error_t::errc::other;
	if (!end || !beg || ((end-beg)<1)) {
		result.error = jmid::mtrk_event_error_t::errc::no_data_following_delta_time;
		return result;
	}
	auto p = beg;
	auto s = jmid::get_status_byte(*p,rs);
	if (!jmid::is_channel_status_byte(s)) {
		result.error = jmid::mtrk_event_error_t::errc::invalid_status_byte;
		return result;
	}
	result.data.status_nybble = s&0xF0u;
	result.data.ch = s&0x0Fu;
	int expect_n_data_bytes = jmid::channel_status_byte_n_data_bytes(s);
	if (*p==s) {
		++p;  // The event has a local status-byte
	}
	if ((end-p)<expect_n_data_bytes) {
		result.error = jmid::mtrk_event_error_t::errc::channel_calcd_length_exceeds_input;
		return result;
	}

	result.data.p1 = *p++;
	if (!jmid::is_data_byte(result.data.p1)) {
		result.error = jmid::mtrk_event_error_t::errc::channel_invalid_data_byte;
		return result;
	}
	if (expect_n_data_bytes==2) {
		result.data.p2 = *p++;
		if (!jmid::is_data_byte(result.data.p2)) {
			result.error = jmid::mtrk_event_error_t::errc::channel_invalid_data_byte;
			return result;
		}
	}
	result.size = p-beg;
	result.error=jmid::mtrk_event_error_t::errc::no_error;
	return result;
}

jmid::validate_meta_event_result_t
jmid::validate_meta_event(const unsigned char *beg, const unsigned char *end) {
	jmid::validate_meta_event_result_t result;
	result.error = jmid::mtrk_event_error_t::errc::other;
	if (!end || !beg || ((end-beg)<3)) {
		result.error = jmid::mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header;
		return result;
	}
	if (!jmid::is_meta_status_byte(*beg)) {
		result.error = jmid::mtrk_event_error_t::errc::invalid_status_byte;
		return result;
	}
	auto p = beg+2;
	auto len = jmid::read_vlq(p,end);
	if (!len.is_valid) {
		result.error = jmid::mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length;
		return result;
	}
	if ((end-p)<(len.N+len.val)) {
		result.error = jmid::mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input;
		return result;
	}
	// TODO:  Should return len.val and re-compute its normalized vlq rep
	result.error = jmid::mtrk_event_error_t::errc::no_error;
	result.begin = beg;
	result.end = beg + 2 + len.N + len.val;
	return result;
}


jmid::validate_sysex_event_result_t
jmid::validate_sysex_event(const unsigned char *beg, const unsigned char *end) {
	jmid::validate_sysex_event_result_t result;
	result.error = jmid::mtrk_event_error_t::errc::other;
	if (!end || !beg || ((end-beg)<2)) {
		result.error = jmid::mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header;
		return result;
	}
	if (!jmid::is_sysex_status_byte(*beg)) {
		result.error = jmid::mtrk_event_error_t::errc::invalid_status_byte;
		return result;
	}
	auto p = beg+1;
	auto len = jmid::read_vlq(p,end);
	if (!len.is_valid) {
		result.error = jmid::mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length;
		return result;
	}
	if ((end-p)<(len.N+len.val)) {
		result.error = jmid::mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input;
		return result;
	}
	// TODO:  Should return len.val and re-compute its normalized vlq rep
	result.error = jmid::mtrk_event_error_t::errc::no_error;
	result.begin = beg;
	result.end = beg + 1 + len.N + len.val;
	return result;
}



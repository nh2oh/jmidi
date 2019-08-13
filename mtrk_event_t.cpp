#include "mtrk_event_t.h"
#include "midi_raw.h"
#include "midi_vlq.h"
#include "midi_delta_time.h"
#include <cstdint>
#include <algorithm>
#include <utility>  // std::move()


mtrk_event_t::mtrk_event_t() noexcept {
	this->default_init(0);
}
mtrk_event_t::mtrk_event_t(mtrk_event_t::init_small_w_size_0_t) noexcept {  // private
	this->d_ = mtrk_event_t_internal::small_bytevec_t();
}
mtrk_event_t::mtrk_event_t(int32_t dt) noexcept {
	this->default_init(dt);
}
mtrk_event_t::mtrk_event_t(int32_t dt, ch_event_data_t md) noexcept {
	this->d_ = mtrk_event_t_internal::small_bytevec_t();
	md = normalize(md);
	unsigned char s = (md.status_nybble)|(md.ch);
	auto n = channel_status_byte_n_data_bytes(s);
	// NB:  n==0 if s is invalid, but this is impossible after a call
	// to normalize().  
	this->d_.resize_small2small_nocopy(delta_time_field_size(dt)+1+n);  // +1=>s
	auto dest = write_delta_time(dt,this->d_.begin());
	*dest++ = s;
	*dest++ = md.p1;
	if (n==2) {
		*dest++ = md.p2;
	}
}
mtrk_event_t::mtrk_event_t(const mtrk_event_t& rhs) {
	this->d_=rhs.d_;
}
mtrk_event_t& mtrk_event_t::operator=(const mtrk_event_t& rhs) {
	this->d_ = rhs.d_;
	return *this;
}
mtrk_event_t::mtrk_event_t(mtrk_event_t&& rhs) noexcept {
	this->d_ = std::move(rhs.d_);
	rhs.default_init(0);
}
mtrk_event_t& mtrk_event_t::operator=(mtrk_event_t&& rhs) noexcept {
	this->d_ = std::move(rhs.d_);
	rhs.default_init(0);
	return *this;
}
mtrk_event_t::~mtrk_event_t() noexcept {  // dtor
	//...
}

mtrk_event_t::size_type mtrk_event_t::size() const noexcept {
	return this->d_.size();
}
mtrk_event_t::size_type mtrk_event_t::capacity() const noexcept {
	return this->d_.capacity();
}
mtrk_event_t::size_type mtrk_event_t::reserve(mtrk_event_t::size_type new_cap) {
	new_cap = std::clamp(new_cap,0,mtrk_event_t::size_max);
	return this->d_.reserve(new_cap);
}
const unsigned char *mtrk_event_t::data() noexcept {
	return this->d_.begin();
}
const unsigned char *mtrk_event_t::data() const noexcept {
	return this->d_.begin();
}
mtrk_event_t::const_iterator mtrk_event_t::begin() noexcept {
	return mtrk_event_t::const_iterator(this->d_.begin());
}
mtrk_event_t::const_iterator mtrk_event_t::begin() const noexcept {
	return mtrk_event_t::const_iterator(this->d_.begin());
}
mtrk_event_t::const_iterator mtrk_event_t::cbegin() noexcept {
	return mtrk_event_t::const_iterator(this->d_.begin());
}
mtrk_event_t::const_iterator mtrk_event_t::cbegin() const noexcept {
	return mtrk_event_t::const_iterator(this->d_.begin());
}
mtrk_event_t::const_iterator mtrk_event_t::end() noexcept {
	return mtrk_event_t::const_iterator(this->d_.end());
}
mtrk_event_t::const_iterator mtrk_event_t::end() const noexcept {
	return mtrk_event_t::const_iterator(this->d_.end());
}
mtrk_event_t::const_iterator mtrk_event_t::cend() noexcept {
	return mtrk_event_t::const_iterator(this->d_.end());
}
mtrk_event_t::const_iterator mtrk_event_t::cend() const noexcept {
	return mtrk_event_t::const_iterator(this->d_.end());
}
mtrk_event_t::const_iterator mtrk_event_t::dt_begin() const noexcept {
	return mtrk_event_t::const_iterator(this->d_.begin());
}
mtrk_event_t::const_iterator mtrk_event_t::dt_begin() noexcept {
	return mtrk_event_t::const_iterator(this->d_.begin());
}
mtrk_event_t::const_iterator mtrk_event_t::dt_end() const noexcept {
	return advance_to_dt_end(this->d_.begin(),this->d_.end());
}
mtrk_event_t::const_iterator mtrk_event_t::dt_end() noexcept {
	return advance_to_dt_end(this->d_.begin(),this->d_.end());
}
mtrk_event_t::const_iterator mtrk_event_t::event_begin() const noexcept {
	return advance_to_dt_end(this->d_.begin(),this->d_.end());
}
mtrk_event_t::const_iterator mtrk_event_t::event_begin() noexcept {
	return advance_to_dt_end(this->d_.begin(),this->d_.end());
}
mtrk_event_t::const_iterator mtrk_event_t::payload_begin() const noexcept {
	return this->payload_range_impl().begin;
	return this->payload_range_impl().begin;
}
mtrk_event_t::const_iterator mtrk_event_t::payload_begin() noexcept {
	return this->payload_range_impl().begin;
}
mtrk_event_iterator_range_t mtrk_event_t::payload_range() const noexcept {
	return this->payload_range_impl();
}
mtrk_event_iterator_range_t mtrk_event_t::payload_range() noexcept {
	return this->payload_range_impl();
}
unsigned char mtrk_event_t::operator[](mtrk_event_t::size_type i) const noexcept {
	return *(this->d_.begin()+i);
};
unsigned char mtrk_event_t::operator[](mtrk_event_t::size_type i) noexcept {
	return *(this->d_.begin()+i);
};
unsigned char *mtrk_event_t::push_back(unsigned char c) {  // Private
	return this->d_.push_back(c);
}
mtrk_event_iterator_range_t mtrk_event_t::payload_range_impl() const noexcept {
	auto it_end = this->d_.end();
	auto it = advance_to_dt_end(this->d_.begin(),it_end);
	auto t = classify_status_byte(*it);
	if (t==smf_event_type::meta) {
		it += 2;  // 0xFFu, type-byte
		it = advance_to_vlq_end(it,it_end);
	} else if (t==smf_event_type::sysex_f0
					|| t==smf_event_type::sysex_f7) {
		it += 1;  // 0xF0u or 0xF7u
		it = advance_to_vlq_end(it,it_end);
	} // else { smf_event_type::channel_voice, _mode, unknown, invalid...
	return {it,it_end};
}
smf_event_type mtrk_event_t::type() const noexcept {
	auto p = advance_to_dt_end(this->d_.begin(),this->d_.end());
	return classify_status_byte(*p);
}
int32_t mtrk_event_t::delta_time() const noexcept {
	return read_delta_time(this->d_.begin(),this->d_.end()).val;
}
unsigned char mtrk_event_t::status_byte() const noexcept {
	return *advance_to_dt_end(this->d_.begin(),this->d_.end());
}
unsigned char mtrk_event_t::running_status() const noexcept {
	auto p = advance_to_dt_end(this->d_.begin(),this->d_.end());
	return get_running_status_byte(*p,0x00u);
}
mtrk_event_t::size_type mtrk_event_t::data_size() const noexcept {  // Not including delta-t
	auto end = this->d_.end();
	auto p = advance_to_dt_end(this->d_.begin(),end);
	return end-p;
}

int32_t mtrk_event_t::set_delta_time(int32_t dt) {
	auto new_dt_size = delta_time_field_size(dt);
	auto beg = this->d_.begin();  auto end = this->d_.end();
	auto curr_dt_size = advance_to_dt_end(beg,end)-beg;
	if (curr_dt_size == new_dt_size) {
		write_delta_time(dt,beg);
	} else if (new_dt_size < curr_dt_size) {  // shrink present event
		auto curr_event_beg = beg+curr_dt_size;
		auto it = write_delta_time(dt,beg);
		it = std::copy(curr_event_beg,end,it);
		this->d_.resize(it-beg);
	} else if (new_dt_size > curr_dt_size) {  // grow present event
		auto old_size = this->d_.size();
		auto new_size = old_size + (new_dt_size-curr_dt_size);
		this->d_.resize(new_size);  // NB:  Invalidates iterators!
		auto new_beg = this->d_.begin();
		auto new_end = this->d_.end();
		std::copy_backward(new_beg,new_beg+old_size,new_end);
		write_delta_time(dt,this->d_.begin());
	}
	return this->delta_time();
}

bool mtrk_event_t::operator==(const mtrk_event_t& rhs) const noexcept {
	auto it_lhs = this->d_.begin();  auto lhs_end = this->d_.end();
	auto it_rhs = rhs.d_.begin();  auto rhs_end = rhs.d_.end();
	if ((lhs_end-it_lhs) != (rhs_end-it_rhs)) {
		return false;
	}
	while (it_lhs!=lhs_end) {
		if (*it_lhs++ != *it_rhs++) {
			return false;
		}
	}
	return true;
}
bool mtrk_event_t::operator!=(const mtrk_event_t& rhs) const noexcept {
	return !(*this==rhs);
}

void mtrk_event_t::default_init(int32_t dt) noexcept {
	this->d_ = mtrk_event_t_internal::small_bytevec_t();
	this->d_.resize_small2small_nocopy(delta_time_field_size(dt)+3);
	auto it = write_delta_time(dt,this->d_.begin());
	*it++ = 0x90u;  // Note-on, channel "1"
	*it++ = 0x3Cu;  // 0x3C==60=="Middle C" (C4, 261.63Hz)
	*it++ = 0x3Fu;  // 0x3F==63, ~= 127/2
}

const unsigned char *mtrk_event_t::raw_begin() const {
	return this->d_.raw_begin();
}
const unsigned char *mtrk_event_t::raw_end() const {
	return this->d_.raw_end();
}
unsigned char mtrk_event_t::flags() const {
	return *(this->d_.raw_begin());
}
bool mtrk_event_t::is_big() const {
	return !(this->flags()&0x80u);
}
bool mtrk_event_t::is_small() const {
	return !(this->is_big());
}

// Declared as a friend of mtrk_event_t
mtrk_event_t make_meta_sysex_generic_unsafe(int32_t dt, unsigned char type,
		unsigned char mtype, int32_t len, const unsigned char *beg, 
		const unsigned char *end, bool add_f7_cap) {
	// Assumes dt is valid, type is valid (==0xFF,F7,F0), 
	// len==end-beg+add_f7_cap.  
	// Ex, 
	// if end-beg == 7 && add_f7_cap, len will == 8
	// if end-beg == 31 && !add_f7_cap, len will == 31
	int32_t sz = delta_time_field_size(dt) + 1 + vlq_field_size(len)
		+ len;
	if (type == 0xFFu) {
		sz += 1;  // For meta events, 1 extra byte for the mtype byte
	}

	auto result = mtrk_event_t(mtrk_event_t::init_small_w_size_0_t {});
	result.d_.resize(sz);
	auto it = result.d_.begin();
	it = write_delta_time(dt,it);
	*it++ = type;
	if (type==0xFFu) {
		*it++ = mtype;
	}
	it = write_vlq(len,it);
	it = std::copy(beg,end,it);
	if (add_f7_cap) {
		*it++ = 0xF7u;
	}
	return result;
}

mtrk_event_debug_helper_t debug_info(const mtrk_event_t& ev) {
	mtrk_event_debug_helper_t r;
	r.raw_beg = ev.d_.raw_begin();
	r.raw_end = ev.d_.raw_end();
	r.flags = *(r.raw_beg);
	r.is_big = ev.d_.debug_is_big();
	return r;
}

std::string explain(const mtrk_event_error_t& err) {
	std::string s;  
	if (err.code==mtrk_event_error_t::errc::no_error) {
		return s;
	}
	s = "Invalid MTrk event:  ";

	if (err.code==mtrk_event_error_t::errc::invalid_delta_time) {
		s += "Invalid delta-time.  ";
	} else if (err.code==mtrk_event_error_t::errc::no_data_following_delta_time) {
		s += "Encountered end-of-input immediately following the delta-time field.";
	} else if (err.code==mtrk_event_error_t::errc::invalid_status_byte) {
		s += "Invalid status byte s == " 
			+ std::to_string(err.s) + ", rs == "
			+ std::to_string(err.rs) + ".  ";
	} else if (err.code==mtrk_event_error_t::errc::channel_calcd_length_exceeds_input) {
		s += "Encountered end-of-input prior to reading the number of expected "
			"data bytes for the present channel event.  ";
	} else if (err.code==mtrk_event_error_t::errc::channel_invalid_data_byte) {
		s += "Invalid channel event data byte.  ";
	} else if (err.code==mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header) {
		s += "Encountered end-of-input while processing the header of the present "
			"sysex of meta event.  ";
	} else if (err.code==mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length) {
		s += "The present sysex or meta event encodes an invalid length.  ";
	} else if (err.code==mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input) {
		s += "Encountered end-of-input while reading in the payload of the "
			"present sysex or meta event.  ";
	} else if (err.code==mtrk_event_error_t::errc::other) {
		s += "mtrk_event_error_t::errc::other.  ";
	} else {
		s += "Unknown error.  ";
	}
	return s;
}

maybe_mtrk_event_t::operator bool() const {
	auto tf = (this->error==mtrk_event_error_t::errc::no_error);
	return tf;
}
validate_channel_event_result_t::operator bool() const {
	return this->error==mtrk_event_error_t::errc::no_error;
}
validate_meta_event_result_t::operator bool() const {
	return this->error==mtrk_event_error_t::errc::no_error;
}
validate_sysex_event_result_t::operator bool() const {
	return this->error==mtrk_event_error_t::errc::no_error;
}

validate_channel_event_result_t
validate_channel_event(const unsigned char *beg, const unsigned char *end,
						unsigned char rs) {
	validate_channel_event_result_t result;
	result.error = mtrk_event_error_t::errc::other;
	if (!end || !beg || ((end-beg)<1)) {
		result.error = mtrk_event_error_t::errc::no_data_following_delta_time;
		return result;
	}
	auto p = beg;
	auto s = get_status_byte(*p,rs);
	if (!is_channel_status_byte(s)) {
		result.error = mtrk_event_error_t::errc::invalid_status_byte;
		return result;
	}
	result.data.status_nybble = s&0xF0u;
	result.data.ch = s&0x0Fu;
	int expect_n_data_bytes = channel_status_byte_n_data_bytes(s);
	if (*p==s) {
		++p;  // The event has a local status-byte
	}
	if ((end-p)<expect_n_data_bytes) {
		result.error = mtrk_event_error_t::errc::channel_calcd_length_exceeds_input;
		return result;
	}

	result.data.p1 = *p++;
	if (!is_data_byte(result.data.p1)) {
		result.error = mtrk_event_error_t::errc::channel_invalid_data_byte;
		return result;
	}
	if (expect_n_data_bytes==2) {
		result.data.p2 = *p++;
		if (!is_data_byte(result.data.p2)) {
			result.error = mtrk_event_error_t::errc::channel_invalid_data_byte;
			return result;
		}
	}
	result.size = p-beg;
	result.error=mtrk_event_error_t::errc::no_error;
	return result;
}

validate_meta_event_result_t
validate_meta_event(const unsigned char *beg, const unsigned char *end) {
	validate_meta_event_result_t result;
	result.error = mtrk_event_error_t::errc::other;
	if (!end || !beg || ((end-beg)<3)) {
		result.error = mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header;
		return result;
	}
	if (!is_meta_status_byte(*beg)) {
		result.error = mtrk_event_error_t::errc::invalid_status_byte;
		return result;
	}
	auto p = beg+2;
	auto len = read_vlq(p,end);
	if (!len.is_valid) {
		result.error = mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length;
		return result;
	}
	if ((end-p)<(len.N+len.val)) {
		result.error = mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input;
		return result;
	}
	// TODO:  Should return len.val and re-compute its normalized vlq rep
	result.error = mtrk_event_error_t::errc::no_error;
	result.begin = beg;
	result.end = beg + 2 + len.N + len.val;
	return result;
}


validate_sysex_event_result_t
validate_sysex_event(const unsigned char *beg, const unsigned char *end) {
	validate_sysex_event_result_t result;
	result.error = mtrk_event_error_t::errc::other;
	if (!end || !beg || ((end-beg)<2)) {
		result.error = mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header;
		return result;
	}
	if (!is_sysex_status_byte(*beg)) {
		result.error = mtrk_event_error_t::errc::invalid_status_byte;
		return result;
	}
	auto p = beg+1;
	auto len = read_vlq(p,end);
	if (!len.is_valid) {
		result.error = mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length;
		return result;
	}
	if ((end-p)<(len.N+len.val)) {
		result.error = mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input;
		return result;
	}
	// TODO:  Should return len.val and re-compute its normalized vlq rep
	result.error = mtrk_event_error_t::errc::no_error;
	result.begin = beg;
	result.end = beg + 1 + len.N + len.val;
	return result;
}



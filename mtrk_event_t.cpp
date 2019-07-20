#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include "midi_raw.h"
#include "midi_vlq.h"
#include <string>
#include <cstdint>
#include <algorithm>
#include <limits>  // std::numeric_limits<uint32t>::max()



mtrk_event_t::mtrk_event_t() {  
	this->d_ = mtrk_event_t_internal::sbo_t();
}
mtrk_event_t::mtrk_event_t(uint32_t dt) {  
	this->d_.resize(delta_time_field_size(dt));
	write_delta_time(dt,this->d_.begin());
}
//
// For callers who have pre-computed the exact size of the event and who
// can also supply a midi status byte if applicible, ex, an 
// mtrk_container_iterator_t.  
//
mtrk_event_t::mtrk_event_t(const unsigned char *p, uint32_t sz, unsigned char rs) {
	auto psrc_end = p+sz;
	auto dt = midi_interpret_vl_field(p,sz);
	// TODO:  Over-read if if sz<=dt.N;  Also consider possibility of sz==0
	auto s = get_status_byte(*(p+dt.N),rs);
	bool has_local_status = (s==*(p+dt.N));
	auto cap = sz;
	if (!has_local_status) {
		cap += 1;
	}
	
	this->d_.resize(cap);

	auto dest_end = std::copy(p,p+dt.N,this->d_.begin());
	p+=dt.N;
	
	*dest_end++ = s;
	if (has_local_status) {
		++p;
	}

	dest_end = std::copy(p,psrc_end,dest_end);
	//std::fill(dest_end,this->d_.end(),0x00u);
}
// TODO:  Clamp dt to the max allowable value
mtrk_event_t::mtrk_event_t(const uint32_t& dt, const unsigned char *p, 
										uint32_t sz, unsigned char rs) {
	auto src_end = p+sz;
	
	auto dtN = midi_vl_field_size(dt);
	unsigned char s = 0x00u;
	bool has_local_status = false;
	if (sz>0) {
		s = get_status_byte(*p,rs);
		has_local_status = (sz>0) && (s==*p);
	}
	auto cap = sz+dtN;
	if (!has_local_status) {
		cap += 1;
	}

	this->d_.resize(cap);

	auto dest_end = write_delta_time(dt,this->d_.begin());
	
	*dest_end++ = s;
	if (has_local_status) {
		++p;
	}
	dest_end = std::copy(p,src_end,dest_end);
	//std::fill(dest_end,this->d_.end(),0x00u);
}
mtrk_event_t::mtrk_event_t(uint32_t dt, midi_ch_event_t md) {
	// TODO:  Is this correct?  What about ch event's w/o a p2?
	unsigned char s = (md.status_nybble)|(md.ch);
	auto n = channel_status_byte_n_data_bytes(s);
	// NB:  n==0 if s is invalid (ex, s==0xFFu)
	this->d_.resize(delta_time_field_size(dt)+1+n);  // +1=>s
	auto dest_end = write_delta_time(dt,this->d_.begin());
	*dest_end++ = s;
	if (n > 0) {
		*dest_end++ = (md.p1);
	}
	if (n > 1) {
		*dest_end++ = (md.p2);
	}

	/*this->d_.set_flag_small();
	auto dest_end = write_delta_time(dt,this->d_.begin());
	unsigned char s = (md.status_nybble)|(md.ch);
	*dest_end++ = s;
	*dest_end++ = (md.p1);
	*dest_end++ = (md.p2);
	std::fill(dest_end,this->d_.end(),0x00u);*/
}
mtrk_event_t::mtrk_event_t(const mtrk_event_t& rhs) {  // Copy ctor
	this->d_=rhs.d_;
}
mtrk_event_t& mtrk_event_t::operator=(const mtrk_event_t& rhs) {  // Cpy assn
	this->d_ = rhs.d_;
	return *this;
}
mtrk_event_t::mtrk_event_t(mtrk_event_t&& rhs) noexcept {  // Move ctor
	this->d_ = std::move(rhs.d_);
}
mtrk_event_t& mtrk_event_t::operator=(mtrk_event_t&& rhs) noexcept {  // Mv assn
	this->d_ = std::move(rhs.d_);
	return *this;
}
mtrk_event_t::~mtrk_event_t() {  // dtor
	//...
}

uint64_t mtrk_event_t::size() const {
	auto sz = mtrk_event_get_size_dtstart_unsafe(this->d_.begin(),0x00u);
	auto cap = this->capacity();
	return sz > cap ? cap : sz;
}
uint64_t mtrk_event_t::capacity() const {
	return this->d_.capacity();
}
uint64_t mtrk_event_t::resize(uint64_t new_cap) {
	if (new_cap > std::numeric_limits<uint32_t>::max()) {
		new_cap = std::numeric_limits<uint32_t>::max();
	}
	uint32_t new_cap32 = static_cast<uint32_t>(new_cap);
	return this->d_.resize(new_cap32);
}
uint64_t mtrk_event_t::reserve(uint64_t cap_in) {
	auto new_cap = std::max(cap_in,this->capacity());
	return this->resize(new_cap);
}
unsigned char *mtrk_event_t::data() {
	return this->d_.begin();
}
const unsigned char *mtrk_event_t::data() const {
	return this->d_.begin();
}
mtrk_event_t::iterator mtrk_event_t::begin() {
	return mtrk_event_t::iterator(this->data());
}
mtrk_event_t::const_iterator mtrk_event_t::begin() const {
	return mtrk_event_t::const_iterator(this->data());
}
mtrk_event_t::iterator mtrk_event_t::end() {
	return this->begin()+this->size();
}
mtrk_event_t::const_iterator mtrk_event_t::end() const {
	return this->begin()+this->size();
}
mtrk_event_t::const_iterator mtrk_event_t::dt_begin() const {
	return this->begin();
}
mtrk_event_t::iterator mtrk_event_t::dt_begin() {
	return this->begin();
}
mtrk_event_t::const_iterator mtrk_event_t::dt_end() const {
	return advance_to_dt_end(this->begin());
}
mtrk_event_t::iterator mtrk_event_t::dt_end() {
	return advance_to_dt_end(this->begin());
}
mtrk_event_t::const_iterator mtrk_event_t::event_begin() const {
	return advance_to_dt_end(this->begin());
}
mtrk_event_t::iterator mtrk_event_t::event_begin() {
	return advance_to_dt_end(this->begin());
}
mtrk_event_t::const_iterator mtrk_event_t::payload_begin() const {
	auto it = this->event_begin();
	if (this->type()==smf_event_type::meta) {
		it += 2;  // 0xFFu, type-byte
		it = advance_to_dt_end(it);
	} else if (this->type()==smf_event_type::sysex_f0
					|| this->type()==smf_event_type::sysex_f7) {
		it += 1;  // 0xF0u or 0xF7u
		it = advance_to_dt_end(it);
	} // else { smf_event_type::channel_voice, _mode, unknown, invalid...
	return it;
}
mtrk_event_t::iterator mtrk_event_t::payload_begin() {
	auto it = this->event_begin();
	if (this->type()==smf_event_type::meta) {
		it += 2;  // 0xFFu, type-byte
		it = advance_to_dt_end(it);
	} else if (this->type()==smf_event_type::sysex_f0
					|| this->type()==smf_event_type::sysex_f7) {
		it += 1;  // 0xF0u or 0xF7u
		it = advance_to_dt_end(it);
	} // else { smf_event_type::channel_voice, _mode, unknown, invalid...
	return it;
}
iterator_range_t<mtrk_event_t::const_iterator> mtrk_event_t::payload_range() const {
	auto sz = this->size();
	auto it = this->event_begin();
	if (this->type()==smf_event_type::meta) {
		it += 2;  // 0xFFu, type-byte
		it = advance_to_dt_end(it);
	} else if (this->type()==smf_event_type::sysex_f0
					|| this->type()==smf_event_type::sysex_f7) {
		it += 1;  // 0xF0u or 0xF7u
		it = advance_to_dt_end(it);
	} // else { smf_event_type::channel_voice, _mode, unknown, invalid...
	return {it,this->end()};
}
iterator_range_t<mtrk_event_t::iterator> mtrk_event_t::payload_range() {
	auto sz = this->size();
	auto it = this->event_begin();
	if (this->type()==smf_event_type::meta) {
		it += 2;  // 0xFFu, type-byte
		it = advance_to_dt_end(it);
	} else if (this->type()==smf_event_type::sysex_f0
					|| this->type()==smf_event_type::sysex_f7) {
		it += 1;  // 0xF0u or 0xF7u
		it = advance_to_dt_end(it);
	} // else { smf_event_type::channel_voice, _mode, unknown, invalid...
	return {it,this->end()};
}
const unsigned char& mtrk_event_t::operator[](mtrk_event_t::size_type i) const {
	return *(this->data()+i);
};
unsigned char& mtrk_event_t::operator[](mtrk_event_t::size_type i) {
	return *(this->data()+i);
};


smf_event_type mtrk_event_t::type() const {
	auto p = advance_to_dt_end(this->data());
	return classify_status_byte(*p);
	// Faster than classify_mtrk_event_dtstart_unsafe(s,rs), b/c the latter
	// calls classify_status_byte(get_status_byte(s,rs)); here, i do not 
	// need to worry about the possibility of the rs byte.  
}
uint32_t mtrk_event_t::delta_time() const {
	return midi_interpret_vl_field(this->data()).val;
}
unsigned char mtrk_event_t::status_byte() const {
	auto p = advance_to_dt_end(this->data());
	return *p;
}
unsigned char mtrk_event_t::running_status() const {
	auto p = advance_to_dt_end(this->data());
	return get_running_status_byte(*p,0x00u);
}
uint32_t mtrk_event_t::data_size() const {  // Not indluding delta-t
	return mtrk_event_get_data_size_dtstart_unsafe(this->data(),0x00u);
}
bool mtrk_event_t::verify() const {
	// TODO:  This is a bit ad-hoc; validate_mtrk_event_dtstart() does not 
	// validate the event size, nor does it flag an error for 
	// smf_event_type::invalid.  
	// TODO:  This does not investigate "deeper" errors, ex, a timesig
	// event w/ a too-small length field, or invalid data fields.  
	auto cap = this->capacity();
	auto vfy = validate_mtrk_event_dtstart(this->data(),0x00u,cap);
	return (vfy.error==mtrk_event_validation_error::no_error
		&& vfy.type!=smf_event_type::invalid
		&& vfy.size<=cap);
}
std::string mtrk_event_t::verify_explain() const {
	auto cap = this->capacity();
	auto vfy = validate_mtrk_event_dtstart(this->data(),0x00u,cap);
	return print(vfy.error);
}

uint32_t mtrk_event_t::set_delta_time(uint32_t dt) {
	auto new_dt_size = delta_time_field_size(dt);
	auto beg = this->begin();
	auto curr_dt_size = advance_to_dt_end(beg)-beg;
	if (curr_dt_size == new_dt_size) {
		write_delta_time(dt,beg);
	} else if (curr_dt_size > new_dt_size) {
		midi_rewrite_dt_field_unsafe(dt,this->data(),0x00u);
		//auto new_size = this->size()-(curr_dt_size-new_dt_size);
		//this->d_.resize(new_size);
	} else if (curr_dt_size < new_dt_size) {
		auto new_size = this->size()+(new_dt_size-curr_dt_size);
		this->d_.resize(new_size);
		midi_rewrite_dt_field_unsafe(dt,this->data(),0x00u);
	}
	return this->delta_time();
}

bool mtrk_event_t::operator==(const mtrk_event_t& rhs) const {
	auto it_lhs = this->begin();  auto lhs_end = this->end();
	auto it_rhs = rhs.begin();  auto rhs_end = rhs.end();
	if ((lhs_end-it_lhs) != (rhs_end-it_rhs)) {
		return false;
	}
	while (it_lhs!=lhs_end && it_rhs!=rhs_end) {
		if (*it_lhs != *it_rhs) {
			return false;
		}
		++it_lhs;
		++it_rhs;
	}
	return true;
}
bool mtrk_event_t::operator!=(const mtrk_event_t& rhs) const {
	return !(*this==rhs);
}
/*
void mtrk_event_t::zero_init() {
	this->d_.set_flag_small();
	std::fill(this->d_.begin(),this->d_.end(),0x00u);
}
*/
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

bool mtrk_event_unit_test_helper_t::is_big() {
	return this->p_->is_big();
}
bool mtrk_event_unit_test_helper_t::is_small() {
	return this->p_->is_small();
}
const unsigned char *mtrk_event_unit_test_helper_t::raw_begin() {
	return this->p_->raw_begin();
}
const unsigned char *mtrk_event_unit_test_helper_t::raw_end() {
	return this->p_->raw_begin();
}
unsigned char mtrk_event_unit_test_helper_t::flags() {
	return this->p_->flags();
}


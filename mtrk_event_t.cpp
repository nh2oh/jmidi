#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include "midi_raw.h"
#include "midi_vlq.h"
#include "dbklib\byte_manipulation.h"
#include <string>
#include <cstdint>
#include <algorithm>



// Default ctor creates a meta text-event of length 0 
mtrk_event_t::mtrk_event_t() {  
	default_init();
}
mtrk_event_t::mtrk_event_t(uint32_t dt) {  
	zero_init();
	midi_write_vl_field(this->d_.begin(),dt);
}
//
// For callers who have pre-computed the exact size of the event and who
// can also supply a midi status byte if applicible, ex, an 
// mtrk_container_iterator_t.  
//
mtrk_event_t::mtrk_event_t(const unsigned char *p, uint32_t sz, unsigned char rs) {
	auto p_end = p+sz;
	auto cap = sz;
	auto dt = midi_interpret_vl_field(p,sz);
	auto s = get_status_byte(*(p+dt.N),rs);
	bool has_local_status = (s==*(p+dt.N));
	if (!has_local_status) {
		cap += 1;
	}

	if (cap<=this->d_.small_capacity()) {  // small
		this->d_.set_flag_small();
	} else {  // big
		this->d_.set_flag_big();
		unsigned char* new_p = new unsigned char[cap];  
		this->d_.big_adopt(new_p,sz,cap);
	}
	unsigned char *dest = this->d_.begin();
	auto dest_end = std::copy(p,p+dt.N,dest);
	p+=dt.N;
	
	*dest_end++ = s;
	if (has_local_status) {
		++p;
	}

	dest_end = std::copy(p,p_end,dest_end);
	std::fill(dest_end,this->d_.end(),0x00u);
}
// TODO:  Clamp dt to the max allowable value
mtrk_event_t::mtrk_event_t(const uint32_t& dt, const unsigned char *p, 
										uint32_t sz, unsigned char rs) {
	auto p_end = p+sz;
	auto cap = sz;
	auto dtN = midi_vl_field_size(dt);
	cap += dtN;
	unsigned char s = 0x00u;
	bool has_local_status = false;
	if (sz>0) {
		s = get_status_byte(*p,rs);
		has_local_status = (sz>0) && (s==*p);
	}
	if (!has_local_status) {
		cap += 1;
	}

	if (cap<=this->d_.small_capacity()) {  // small
		this->d_.set_flag_small();
	} else {  // big
		this->d_.set_flag_big();
		unsigned char* new_p = new unsigned char[cap];  
		this->d_.big_adopt(new_p,sz,cap);
	}
	unsigned char *dest = this->d_.begin();
	auto dest_end = midi_write_vl_field(dest,dt);
	
	*dest_end++ = s;
	if (has_local_status) {
		++p;
	}
	dest_end = std::copy(p,p_end,dest_end);
	std::fill(dest_end,this->d_.end(),0x00u);
}
mtrk_event_t::mtrk_event_t(uint32_t dt, midi_ch_event_t md) {
	this->d_.set_flag_small();
	auto dest_end = midi_write_vl_field(this->d_.begin(),dt);
	unsigned char s = (md.status_nybble)|(md.ch);
	*dest_end++ = s;
	*dest_end++ = (md.p1);
	*dest_end++ = (md.p2);
	std::fill(dest_end,this->d_.end(),0x00u);
}
//
// Copy ctor
//
mtrk_event_t::mtrk_event_t(const mtrk_event_t& rhs) {
	if (rhs.size()>this->d_.small_capacity()) {
		this->d_.set_flag_big();
		unsigned char *new_p = new unsigned char[rhs.size()];
		auto data_end = std::copy(rhs.begin(),rhs.end(),new_p);
		std::fill(data_end,new_p+rhs.size(),0x00u);
		this->d_.big_adopt(new_p,rhs.size(),rhs.size());
	} else {  // rhs is small
		this->d_.set_flag_small();
		auto data_end = std::copy(rhs.begin(),rhs.end(),this->begin());
		std::fill(data_end,this->end(),0x00u);
	}
}
//
// Copy assignment; overwrites a pre-existing lhs 'this' w/ rhs
//
mtrk_event_t& mtrk_event_t::operator=(const mtrk_event_t& rhs) {
	auto src_beg = rhs.data();  // Saves abt 6 calls to rhs.data()
	auto src_sz = rhs.size();  // Saves abt 10 calls to rhs.size()
	if (src_sz>this->d_.small_capacity()) {  // rhs requires a 'big' container
		if (this->capacity() < src_sz) {
			// ... and the present container (big or small) has insufficient capacity
			this->d_.free_if_big();
			unsigned char *new_p = new unsigned char[src_sz];
			auto data_end = std::copy(src_beg,src_beg+src_sz,new_p);
			std::fill(data_end,new_p+src_sz,0x00u);
			this->d_.set_flag_big();
			this->d_.big_adopt(new_p,src_sz,src_sz);
		} else {
			// ... and the present container (must be big) has sufficient capacity
			this->d_.set_flag_big();
			auto data_end = std::copy(src_beg,src_beg+src_sz,this->begin());
			std::fill(data_end,this->end(),0x00u);
			this->d_.big_adopt(this->data(),src_sz,this->capacity());
		}
	} else {  // rhs requires a small container
		this->d_.free_if_big();
		this->d_.set_flag_small();
		auto data_end = std::copy(src_beg,src_beg+src_sz,this->begin());
		std::fill(data_end,this->end(),0x00u);
	}
	return *this;
}
//
// Move ctor
//
mtrk_event_t::mtrk_event_t(mtrk_event_t&& rhs) noexcept {
	this->d_ = rhs.d_;
	rhs.default_init();
	// Prevents ~rhs() from freeing its memory.  Sets the 'small' flag and
	// zeros all elements of the data array.  
}
//
// Move assign
//
mtrk_event_t& mtrk_event_t::operator=(mtrk_event_t&& rhs) noexcept {
	this->d_.free_if_big();
	this->d_ = rhs.d_;
	rhs.default_init();
	// Prevents ~rhs() from freeing its memory.  Sets the 'small' flag and
	// zeros all elements of the data array.  
	return *this;
}
//
// dtor
//
mtrk_event_t::~mtrk_event_t() {
	this->d_.free_if_big();
}

uint64_t mtrk_event_t::size() const {
	auto sz = mtrk_event_get_size_dtstart_unsafe(this->d_.begin(),0x00u);
	auto cap = this->d_.capacity();
	return sz > cap ? cap : sz;
}
uint64_t mtrk_event_t::capacity() const {
	return this->d_.capacity();
}

unsigned char *mtrk_event_t::data() {
	return this->d_.begin();
}
const unsigned char *mtrk_event_t::data() const {
	return this->d_.begin();
}
mtrk_event_t::iterator mtrk_event_t::begin() {
	return mtrk_event_t::iterator(this->data());
	//return mtrk_event_iterator_t(*this);
}
mtrk_event_t::const_iterator mtrk_event_t::begin() const {
	return mtrk_event_t::const_iterator(this->data());
	//return mtrk_event_const_iterator_t(*this);
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
	auto new_dt_size = midi_vl_field_size(dt);
	auto curr_dt_size = this->dt_end()-this->begin();
	if (curr_dt_size == new_dt_size) {
		midi_write_vl_field(this->begin(),dt);
	} else if (curr_dt_size > new_dt_size) {
		midi_rewrite_dt_field_unsafe(dt,this->data(),0x00u);
	} else if (curr_dt_size < new_dt_size) {
		// The new dt is bigger than the current dt, and w/ the new
		// dt, the event...
		auto new_size = this->size()+(new_dt_size-curr_dt_size);
		if (this->capacity() >= new_size) {  // ... still fits
			midi_rewrite_dt_field_unsafe(dt,this->data(),0x00u);
		} else {  // ... won't fit
			this->d_.resize(new_size);
			midi_rewrite_dt_field_unsafe(dt,this->data(),0x00u);
		}
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


void mtrk_event_t::default_init() {
	this->d_.set_flag_small();
	std::array<unsigned char,4> evdata = {0x00,0xFF,0x01,0x00};
	auto end = std::copy(evdata.begin(),evdata.end(),this->d_.begin());
	std::fill(end,this->d_.end(),0x00u);
}
void mtrk_event_t::zero_init() {
	this->d_.set_flag_small();
	std::fill(this->d_.begin(),this->d_.end(),0x00u);
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


#include "mtrk_event_t.h"
#include "mtrk_event_iterator_t.h"
#include "midi_raw.h"
#include "midi_vlq.h"
#include "dbklib\byte_manipulation.h"
#include <string>
#include <cstdint>
#include <cstring>  // std::memcpy()
#include <exception>  // std::abort()
#include <algorithm>  
#include <utility>  // std::move()


mtrk_event_t::mtrk_event_t() {
	// A meta text-event of length 0 
	this->d_ = {0x00,0xFF,0x01,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00};
	this->midi_status_ = 0x00;
	this->flags_ = 0x00;
	this->set_flag_small();
}
mtrk_event_t::mtrk_event_t(uint32_t dt, const midi_ch_event_t& md) {
	auto it = midi_write_vl_field(this->d_.begin(),this->d_.begin()+4,dt);
	unsigned char s = 0x80u|(md.status_nybble);
	s += 0x0Fu&(md.ch);
	*it++ = s;
	*it++ = 0x7Fu&(md.p1);
	if (channel_status_byte_n_data_bytes(s)==2) {
		*it++ = 0x7Fu&(md.p2);
	}
	this->midi_status_ = s;
	this->flags_ = 0x00;
	this->set_flag_small();
}
mtrk_event_t::mtrk_event_t(const unsigned char *p, 
					const validate_mtrk_event_result_t& ev) {
	if (ev.size<=static_cast<uint64_t>(offs::max_size_sbo)) {  // small
		this->set_flag_small();

		auto end = std::copy(p,p+ev.size,this->d_.begin());
		while (end!=this->d_.end()) {
			*end++ = 0x00u;
		}
		this->midi_status_ = ev.running_status;
	} else {  // big
		auto new_p = new unsigned char[ev.size];
		std::copy(p,p+ev.size,new_p);
		this->init_big(new_p,ev.size,ev.size,ev.running_status);  // adopts new_p
	}
}
//
// For callers who have pre-computed the exact size of the event and who
// can also supply a midi status byte if applicible, ex, an mtrk_container_iterator_t.  
//
mtrk_event_t::mtrk_event_t(const unsigned char *p, uint32_t sz, unsigned char rs) {
	if (sz<=static_cast<uint64_t>(offs::max_size_sbo)) {  // small
		this->set_flag_small();

		auto end = std::copy(p,p+sz,this->d_.begin());
		while (end!=this->d_.end()) {
			*end++ = 0x00u;
		}
		auto dt = midi_interpret_vl_field(this->d_.data(),sz);
		this->midi_status_ = get_running_status_byte(*(p+dt.N),rs);
	} else {  // big
		auto new_p = new unsigned char[sz];
		std::copy(p,p+sz,new_p);
		this->init_big(new_p,sz,sz,rs);  // adopts new_p
		// TODO:  This is blindly trusting rs?  For the small case i call 
		// get_running_status_byte()...  Bug?
	}
}
// TODO:  Clamp dt to the max allowable value
mtrk_event_t::mtrk_event_t(const uint32_t& dt, const unsigned char *p, 
										uint32_t sz, unsigned char rs) {
	auto dt_end = midi_write_vl_field(this->d_.begin(),dt);
	auto dt_sz = (this->d_.begin()-dt_end);
	if ((dt_sz+sz) > static_cast<uint32_t>(offs::max_size_sbo)) {
		// Does not fit in the sbo:
		auto new_p = new unsigned char[sz+dt_sz];
		auto it = std::copy(this->d_.begin(),dt_end,new_p);
		it = std::copy(p,p+sz,it);
		this->init_big(new_p,sz+dt_sz,sz+dt_sz,rs);  // adopts new_p
		// TODO:  This is blindly trusting rs?  For the small case i call 
		// get_running_status_byte()...  Bug?
	} else {
		// Fits in the sbo:
		this->set_flag_small();
		auto it = std::copy(p,p+sz,dt_end);
		while (it!=this->d_.end()) {
			*it++ = 0x00u;
		}
		this->midi_status_ = get_running_status_byte(*(dt_end),rs);
	}
}
//
// Copy ctor
//
mtrk_event_t::mtrk_event_t(const mtrk_event_t& rhs) {
	if (rhs.is_big()) {
		unsigned char *new_p = new unsigned char[rhs.size()];
		std::copy(rhs.big_ptr(),rhs.big_ptr()+rhs.size(),new_p);
		this->init_big(new_p,rhs.size(),rhs.size(),rhs.midi_status_);
	} else {  // rhs is small
		this->d_ = rhs.d_;
	}

	// init_big() calls set_big_flag(), but in the future, other flags may be
	// defined for this field other than just big/small, thus, overwrite 
	// this->flags_ w/ rhs.flags_
	this->flags_ = rhs.flags_;  
	this->midi_status_ = rhs.midi_status_;  // also set by init_big()
}
//
// Copy assignment; overwrites a pre-existing lhs 'this' w/ rhs
//
mtrk_event_t& mtrk_event_t::operator=(const mtrk_event_t& rhs) {
	if (rhs.is_big()) {
		if (this->is_small() || (this->is_big()&&this->big_cap()<rhs.size())) {
			// this is small or is big but w/ insufficient capacity
			// init_big() w/ new buffer, cap set to rhs.size().  
			unsigned char *new_p = new unsigned char[rhs.big_size()];
			std::copy(rhs.data(),rhs.data()+rhs.size(),new_p);
			if (this->is_big()) {
				delete this->big_ptr();
			}
			this->init_big(new_p,rhs.size(),rhs.size(),rhs.midi_status_);
		} else if (this->is_big() && this->big_cap()>=rhs.size()) {
			// this is big and the present capacity _is_ sufficient.  
			// init_big() w/ present ptr, cap, but rhs size() and midi_status_
			std::fill(this->data(),this->data()+this->big_cap(),0x00u);
			std::copy(rhs.data(),rhs.data()+rhs.size(),this->data());
			this->init_big(this->data(),rhs.size(),this->big_cap(),rhs.midi_status_);
		}
	} else {  // rhs is small
		if (this->is_big()) {
			delete this->big_ptr();
		}
		this->d_ = rhs.d_;
	}

	// init_big() calls set_big_flag(), but in the future, other flags may be
	// defined for this field other than just big/small, thus, overwrite 
	// this->flags_ w/ rhs.flags_
	this->flags_ = rhs.flags_;  
	this->midi_status_ = rhs.midi_status_;  // also set by init_big()

	return *this;
}
//
// Move ctor
//
mtrk_event_t::mtrk_event_t(mtrk_event_t&& rhs) noexcept {
	this->d_ = rhs.d_;
	this->midi_status_ = rhs.midi_status_;
	this->flags_ = rhs.flags_;
	if (rhs.is_big()) {
		rhs.clear_nofree();  // prevents ~rhs() from freeing its memory
		// Note that this state of rhs is invalid as everything is set to
		// 0x00u.  
	}
}
//
// Move assign
//
mtrk_event_t& mtrk_event_t::operator=(mtrk_event_t&& rhs) noexcept {
	if (this->is_big()) {
		delete this->big_ptr();
	}

	this->d_ = rhs.d_;
	this->midi_status_ = rhs.midi_status_;
	this->flags_ = rhs.flags_;

	// Overwrite everything in rhs w/ values for the default-constructed
	// state.  rhs.d_,rhs.midi_status_,rhs.flags_ are simply overwritten;
	// no resources are freed if initially rhs.is_big().  This prevents
	// ~rhs() from freeing its resources (if big).  
	rhs.clear_nofree();

	return *this;
}
mtrk_event_t::~mtrk_event_t() {
	if (this->is_big()) {
		delete this->big_ptr();
	}
}

mtrk_event_const_iterator_t mtrk_event_t::begin() const {
	return mtrk_event_const_iterator_t(*this);
}
mtrk_event_iterator_t mtrk_event_t::begin() {
	return mtrk_event_iterator_t(*this);
}
mtrk_event_const_iterator_t mtrk_event_t::dt_begin() const {
	return this->begin();
}
mtrk_event_iterator_t mtrk_event_t::dt_begin() {
	return this->begin();
}
mtrk_event_const_iterator_t mtrk_event_t::dt_end() const {
	auto len = midi_interpret_vl_field(this->data());
	return this->begin()+len.N;
}
mtrk_event_iterator_t mtrk_event_t::dt_end() {
	auto len = midi_interpret_vl_field(this->data());
	return this->begin()+len.N;
}
mtrk_event_const_iterator_t mtrk_event_t::event_begin() const {
	auto len = midi_interpret_vl_field(this->data());
	return this->begin()+len.N;
}
mtrk_event_iterator_t mtrk_event_t::event_begin() {
	auto len = midi_interpret_vl_field(this->data());
	return this->begin()+len.N;
}
mtrk_event_const_iterator_t mtrk_event_t::payload_begin() const {
	auto it = this->event_begin();
	if (this->type()==smf_event_type::meta) {
		it += 2;  // 0xFFu, type-byte
		auto len = midi_interpret_vl_field(it);
		it += len.N;
	} else if (this->type()==smf_event_type::sysex_f0
					|| this->type()==smf_event_type::sysex_f7) {
		it += 1;  // 0xF0u or 0xF7u
		auto len = midi_interpret_vl_field(it);
		it += len.N;
	} else {  // smf_event_type::channel_voice, _mode, unknown, invalid...
		return it;
	}
	return it;
}
mtrk_event_iterator_t mtrk_event_t::payload_begin() {
	auto it = this->event_begin();
	if (this->type()==smf_event_type::meta) {
		it += 2;  // 0xFFu, type-byte
		auto len = midi_interpret_vl_field(it);
		it += len.N;
	} else if (this->type()==smf_event_type::sysex_f0
					|| this->type()==smf_event_type::sysex_f7) {
		it += 1;  // 0xF0u or 0xF7u
		auto len = midi_interpret_vl_field(it);
		it += len.N;
	} else {  // smf_event_type::channel_voice, _mode, unknown, invalid...
		return it;
	}
	return it;
}
mtrk_event_const_iterator_t mtrk_event_t::end() const {
	return this->begin()+this->size();
}
mtrk_event_iterator_t mtrk_event_t::end() {
	return this->begin()+this->size();
}
std::string mtrk_event_t::text_payload() const {
	std::string s {};
	if (this->type()==smf_event_type::meta 
				|| this->type()==smf_event_type::sysex_f0
				|| this->type()==smf_event_type::sysex_f7) {
		std::copy(this->payload_begin(),this->end(),std::back_inserter(s));
	}
	return s;
}
uint32_t mtrk_event_t::uint32_payload() const {
	return be_2_native<uint32_t>(this->payload_begin(),this->end());
}
mtrk_event_t::channel_event_data_t mtrk_event_t::midi_data() const {
	mtrk_event_t::channel_event_data_t result {0x00u,0x80u,0x80u,0x80u};
	result.is_valid = false;
	// Note that 0x00u is invalid as a status nybble, and 0x80u is invalid
	// as a channel number and value for p1, p2.  
	if (this->type()==smf_event_type::channel) {
		result.is_valid = true;
		
		auto dt = midi_interpret_vl_field(this->data());
		auto p = this->data()+dt.N;
		result.is_running_status = *p!=this->midi_status_;

		result.status_nybble = this->midi_status_&0xF0u;
		result.ch = this->midi_status_&0x0Fu;
		if (!result.is_running_status) {
			++p;
		}
		result.p1 = *p++;
		if (channel_status_byte_n_data_bytes(this->midi_status_)==2) {
			result.p2 = *p;
		}
	}

	return result;
}
const unsigned char& mtrk_event_t::operator[](uint32_t i) const {
	const auto& r = *(this->data()+i);
	return r;
};
unsigned char& mtrk_event_t::operator[](uint32_t i) {
	return *(this->data()+i);
};
const unsigned char *mtrk_event_t::data() const {
	if (this->is_small()) {
		return this->small_ptr();
	} else {
		return this->big_ptr();
	}
}
unsigned char *mtrk_event_t::data() {
	if (this->is_small()) {
		return this->small_ptr();
	} else {
		return this->big_ptr();
	}
}
const unsigned char *mtrk_event_t::raw_data() const {
	return &(this->d_[0]);
}
const unsigned char *mtrk_event_t::raw_flag() const {
	return &(this->flags_);
}
uint32_t mtrk_event_t::delta_time() const {
	if (this->is_small()) {
		return midi_interpret_vl_field(this->data()).val;
	} else {
		return this->big_delta_t();
	}
}
unsigned char mtrk_event_t::status_byte() const {
	// Since at present this->midi_status_ is == 0x00u for meta or sysex
	// events, have to check for its validity.  
	if ((this->midi_status_ & 0x80u) == 0x80u) {
		return this->midi_status_;
	} else {
		return *(this->event_begin());
	}
}
unsigned char mtrk_event_t::running_status() const {
	// Since at present this->midi_status_ is == 0x00u for meta or sysex
	// events, have to check for its validity.  
	return get_running_status_byte(this->midi_status_,*(this->event_begin()));
}
smf_event_type mtrk_event_t::type() const {
	if (is_small()) {
		// TODO:  Arbitary max_size==6
		return classify_mtrk_event_dtstart(this->data(),this->midi_status_,6);
	} else {
		return this->big_smf_event_type();
	}
}
uint32_t mtrk_event_t::data_size() const {  // Not indluding delta-t
	if (this->is_small()) {
		return mtrk_event_get_data_size_dtstart_unsafe(this->small_ptr(),this->midi_status_);
	} else {
		return mtrk_event_get_data_size_dtstart_unsafe(this->big_ptr(),this->midi_status_);
	}
}
uint32_t mtrk_event_t::size() const {  // Includes the contrib from delta-t
	if (this->is_small()) {
		return mtrk_event_get_size_dtstart_unsafe(this->small_ptr(),this->midi_status_);
	} else {
		return this->big_size();
	}
}
uint32_t mtrk_event_t::capacity() const {
	if (this->is_small()) {
		return static_cast<uint32_t>(offs::max_size_sbo);
	} else {
		return this->big_cap();
	}
}
bool mtrk_event_t::set_delta_time(uint32_t dt) {
	uint64_t sbo_sz = static_cast<uint64_t>(offs::max_size_sbo);
	auto new_dt_size = midi_vl_field_size(dt);
	auto new_size = this->data_size() + new_dt_size;
	if ((this->is_small() && new_size<=sbo_sz)
				|| (this->is_big() && new_size<=this->big_cap())) {
		// The new value fits in whatever storage was holding the old
		// value.   
		// TODO:  It's possible that the present value is_big() but
		// w/ the new dt the size() is such that it will fit in the sbo.  
		// TODO:  If shrinking the size, does this leave trash at the end
		// of the event?
		midi_rewrite_dt_field_unsafe(dt,this->data(),this->midi_status_);
	} else if (this->is_small() && new_size>sbo_sz) {
		// Present value fits in the sbo, but the new value does not.  
		this->small2big(new_size);
		midi_rewrite_dt_field_unsafe(dt,this->data(),this->midi_status_);
	} else if (this->is_big() && new_size>this->big_cap()) {
		this->big_resize(new_size);
		midi_rewrite_dt_field_unsafe(dt,this->data(),this->midi_status_);
	}
	return true;
}
bool mtrk_event_t::operator==(const mtrk_event_t& rhs) const {
	if (this->size() != rhs.size()) {
		return false;
	}
	auto it_lhs = this->begin();
	auto it_rhs = rhs.begin();
	while (it_lhs!=this->end() && it_rhs!=rhs.end()) {
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
bool mtrk_event_t::validate() const {
	bool tf = true;

	if (this->is_small()) {
		tf &= !this->is_big();
		tf &= (this->small_ptr()==&(this->d_[0]));
		tf &= (this->small_ptr()==this->data());
		tf &= (this->small_ptr()==this->raw_data());

		auto dt_loc = midi_interpret_vl_field(this->small_ptr());
		tf &= (dt_loc.val==this->delta_time());
		tf &= (dt_loc.N==midi_vl_field_size(this->delta_time()));

		auto size_loc = mtrk_event_get_size_dtstart_unsafe(this->small_ptr(),this->midi_status_);
		tf &= (size_loc==this->size());

		// TODO:  arbitrary max_size==6
		auto type_loc = classify_mtrk_event_dtstart(this->small_ptr(),this->midi_status_,6);
		tf &= (type_loc==this->type());

		// data_size() must be consistent w/ manual examination of the remote array
		auto data_size_loc = mtrk_event_get_data_size_dtstart_unsafe(this->small_ptr(),this->midi_status_);
		tf &= (data_size_loc==this->data_size());

		// Capacity must == the size of the sbo
		tf &= (this->capacity()==static_cast<uint32_t>(offs::max_size_sbo));
	} else {  // big
		tf &= this->is_big();
		tf &= !this->is_small();
		tf &= this->big_ptr()==this->data();
		tf &= this->big_ptr()!=this->raw_data();

		// Compare the local "cached" dt, size, type w/ that obtained from
		// reading the remote array.  
		auto dtval_loc = this->big_delta_t();
		auto dt_remote = midi_interpret_vl_field(this->big_ptr());
		tf &= (dt_remote.val==dtval_loc);
		tf &= (dt_remote.N==midi_vl_field_size(dtval_loc));
		tf &= (dt_remote.val==this->delta_time());

		auto size_loc = this->big_size();
		auto size_remote = mtrk_event_get_size_dtstart_unsafe(this->big_ptr(),this->midi_status_);
		tf &= (size_loc==size_remote);
		tf &= (size_loc==this->size());

		auto type_loc = this->big_smf_event_type();
		// TODO:  Arbitrary max_size==6
		auto type_remote = classify_mtrk_event_dtstart(this->big_ptr(),this->midi_status_,6);
		tf &= (type_loc==type_remote);
		tf &= (this->type()==type_loc);

		// data_size() must be consistent w/ manual examination of the remote array
		auto data_size_remote = mtrk_event_get_data_size_dtstart_unsafe(this->big_ptr(),this->midi_status_);
		tf &= (this->data_size()==data_size_remote);

		// Sanity checks for size, capacity values
		tf &= (this->big_size() >= 0);
		tf &= (this->big_cap() >= this->big_size());
	}

	// Local midi_status_ field:  Tests are independent of is_big()/small()
	// For midi events, this->midi_status_ must be == the status byte 
	// applic to the event
	if (this->type()==smf_event_type::channel) {
		// Present event is a midi event
		auto p = this->data()+midi_vl_field_size(this->delta_time());
		if (is_channel_status_byte(*p)) {  // not running status
			// The event-local status byte must match this->midi_status_
			tf &= (*p==this->midi_status_);
		} else {  // running status
			// TODO:  This is confusing as hell...
			// this->midi_status_ must accurately describe the # of midi
			// data bytes.  
			tf &= (is_channel_status_byte(this->midi_status_));
			//auto nbytesmsg = midi_channel_event_n_bytes(this->midi_status_,0x00u)-1;
			auto nbytesmsg = channel_event_get_data_size(p,this->midi_status_);
			tf &= (is_data_byte(*p));
			if (nbytesmsg==2) {
				tf &= (is_data_byte(*++p));
			}
		}
	}
	// For non-midi-events, this->midi_status must ==0x00u
	if (this->type()==smf_event_type::sysex_f0 
			|| this->type()==smf_event_type::sysex_f7
			|| this->type()==smf_event_type::meta
			|| this->type()==smf_event_type::invalid) {
		// Present event is _not_ a midi event
		tf &= (this->midi_status_==0x00u);
	}

	return tf;
}


bool mtrk_event_t::is_big() const {
	return (this->flags_&0x80u)==0x00u;
}
bool mtrk_event_t::is_small() const {
	return !(this->is_big());
}

void mtrk_event_t::clear_nofree() {
	// A meta text-event of length 0 
	this->d_ = {0x00,0xFF,0x01,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00};
	this->midi_status_ = 0x00;
	this->flags_ = 0x00;
	this->set_flag_small();
}

// p (which already contains the object data) is adpoted by the container.  
// The caller should _not_ delete p, as the contents are not copied out into
// a newly created "owned" buffer.  This function does not rely on any 
// local object data members either directly or indirectly through getters
// (midi_status_, this->delta_time(), etc).  It is perfectly fine for the 
// object to be initially in a completely invalid state.  
// If the object is initially big, init_big() does not free the old buffer,
// it merely overwrites the existing ptr, size, cap with their new values.  
// It is the responsibility of the caller to delete the old big_ptr() before
// calling init_big() w/ the ptr to the new buffer, or memory will leak.  
// ptr, size, capacity, running-status
//
// TODO:  Why not an overload of this for when type, delta-time, rs, etc
// is known by the caller (avoid recomputation...) ?
bool mtrk_event_t::init_big(unsigned char *p, uint32_t sz, uint32_t c, unsigned char rs) {
	this->set_flag_big();

	// Set the 3 big-container parameters to point to the remote data
	this->set_big_ptr(p);
	this->set_big_size(sz);
	this->set_big_cap(c);
	
	// Set the local "cached" values of delta-time and event type from the
	// remote data.  These values are stored in the d_ array, so setters
	// that know the offsets and can do the serialization correctly are used.  
	auto dt = midi_interpret_vl_field(p);
	this->set_big_cached_delta_t(dt.val);
	//TODO:  Arbitrary max_size==6
	this->set_big_cached_smf_event_type(classify_mtrk_event_dtstart(p,rs,6));

	// The two d_-external values to set are midi_status_ and flags
	this->midi_status_ = get_running_status_byte(*(p+dt.N),rs);

	// TODO:  Error checking for dt fields etc; perhaps call clear_nofree()
	// and return false
	return true;
}
bool mtrk_event_t::small2big(uint32_t c) {
	if (!is_small()) {
		std::abort();
	}
	unsigned char *p = new unsigned char[c];
	std::copy(this->d_.begin(),this->d_.end(),p);
	init_big(p,this->size(),c,this->midi_status_);

	return true;
}
// Allocates a new unsigned char[] w/ capacity as supplied, copies the
// present data into the new array, frees the old array, and sets the "big"
// params as necessary.  
bool mtrk_event_t::big_resize(uint32_t c) {
	if (!is_big()) {
		std::abort();
	}

	unsigned char *p = new unsigned char[c];
	std::copy(this->data(),this->data()+this->size(),p);
	delete this->big_ptr();
	init_big(p,this->size(),c,this->midi_status_);

	return true;
}


void mtrk_event_t::set_flag_big() {
	this->flags_ &= 0x7Fu;
}
void mtrk_event_t::set_flag_small() {
	this->flags_ |= 0x80u;
}

unsigned char *mtrk_event_t::big_ptr() const {
	unsigned char *p {nullptr};
	uint64_t o = static_cast<uint64_t>(offs::ptr);
	std::memcpy(&p,&(this->d_[o]),sizeof(p));
	return p;
}
unsigned char *mtrk_event_t::small_ptr() const {
	return const_cast<unsigned char*>(&(this->d_[0]));
}
unsigned char *mtrk_event_t::set_big_ptr(unsigned char *p) {
	if (this->is_small()) {
		std::abort();
	}
	uint64_t o = static_cast<uint64_t>(offs::ptr);
	std::memcpy(&(this->d_[o]),&p,sizeof(p));
	return p;
}
uint32_t mtrk_event_t::big_size() const {
	uint32_t sz {0};
	uint64_t o = static_cast<uint64_t>(offs::size);
	std::memcpy(&sz,&(this->d_[o]),sizeof(uint32_t));
	return sz;
}
uint32_t mtrk_event_t::set_big_size(uint32_t sz) {
	if (this->is_small()) {
		std::abort();
	}
	uint64_t o = static_cast<uint64_t>(offs::size);
	std::memcpy(&(this->d_[o]),&sz,sizeof(uint32_t));
	return sz;
}
uint32_t mtrk_event_t::big_cap() const {
	uint32_t c {0};
	uint64_t o = static_cast<uint64_t>(offs::cap);
	std::memcpy(&c,&(this->d_[o]),sizeof(uint32_t));
	return c;
}
uint32_t mtrk_event_t::set_big_cap(uint32_t c) {
	uint64_t o = static_cast<uint64_t>(offs::cap);
	std::memcpy(&(this->d_[o]),&c,sizeof(uint32_t));
	return c;
}
uint32_t mtrk_event_t::big_delta_t() const {
	uint32_t dt {0};
	uint64_t o = static_cast<uint64_t>(offs::dt);
	std::memcpy(&dt,&(this->d_[o]),sizeof(uint32_t));
	return dt;
}
uint32_t mtrk_event_t::set_big_cached_delta_t(uint32_t dt) {
	// Sets the local "cached" dt value
	uint64_t o = static_cast<uint64_t>(offs::dt);
	std::memcpy(&(this->d_[o]),&dt,sizeof(uint32_t));
	return dt;
}
smf_event_type mtrk_event_t::big_smf_event_type() const {
	smf_event_type t {smf_event_type::invalid};
	uint64_t o = static_cast<uint64_t>(offs::type);
	std::memcpy(&t,&(this->d_[o]),sizeof(smf_event_type));
	return t;
}
smf_event_type mtrk_event_t::set_big_cached_smf_event_type(smf_event_type t) {
	uint64_t o = static_cast<uint64_t>(offs::type);
	std::memcpy(&(this->d_[o]),&t,sizeof(smf_event_type));
	return t;
}


std::string print(const mtrk_event_t& evnt, mtrk_sbo_print_opts opts) {
	std::string s {};
	s += ("delta_time = " + std::to_string(evnt.delta_time()) + ", ");
	s += ("type = " + print(evnt.type()) + ", ");
	s += ("size = " + std::to_string(evnt.size()) + ", ");
	s += ("data_size = " + std::to_string(evnt.data_size()) + "\n");
	
	s += "\t[";
	dbk::print_hexascii(evnt.dt_begin(),evnt.dt_end(),std::back_inserter(s),'\0',' ');
	s += "] ";
	dbk::print_hexascii(evnt.event_begin(),evnt.end(),std::back_inserter(s),'\0',' ');
	
	if (opts == mtrk_sbo_print_opts::detail || opts == mtrk_sbo_print_opts::debug) {
		if (evnt.type()==smf_event_type::meta) {
			s += "\n";
			s += ("\tmeta type: " + print(classify_meta_event(evnt)) + "; ");
			if (meta_has_text(evnt)) {
				s += ("text payload: \"" + meta_generic_gettext(evnt) + "\"; ");
			} else if (is_tempo(evnt)) {
				s += ("value = " + std::to_string(get_tempo(evnt)) + " us/q; ");
			} else if (is_timesig(evnt)) {
				auto data = get_timesig(evnt);
				s += ("value = " + std::to_string(data.num) + "/" 
					+ std::to_string(static_cast<int>(std::exp2(data.log2denom)))
					+ ", " + std::to_string(data.clckspclk) + " MIDI clocks/click, "
					+ std::to_string(data.ntd32pq) + " notated 32'nd nts / MIDI q nt; ");
			}
		}
	} 
	if (opts == mtrk_sbo_print_opts::debug) {
		s += "\n\t";
		if (evnt.is_small()) {
			s += "sbo=>small = ";
		} else {
			s += "sbo=>big   = ";
		}
		s += "{";
		s += dbk::print_hexascii(evnt.raw_data(), sizeof(mtrk_event_t), ' ');
		s += "}; \n";
		s += "\tbigsmall_flag = ";
		s += dbk::print_hexascii(evnt.raw_flag(), 1, ' ');
	}
	return s;
}





meta_event_t classify_meta_event_impl(const uint16_t& d16) {
	if (d16==static_cast<uint16_t>(meta_event_t::seqn)) {
		return meta_event_t::seqn;
	} else if (d16==static_cast<uint16_t>(meta_event_t::text)) {
		return meta_event_t::text;
	} else if (d16==static_cast<uint16_t>(meta_event_t::copyright)) {
		return meta_event_t::copyright;
	} else if (d16==static_cast<uint16_t>(meta_event_t::trackname)) {
		return meta_event_t::trackname;
	} else if (d16==static_cast<uint16_t>(meta_event_t::instname)) {
		return meta_event_t::instname;
	} else if (d16==static_cast<uint16_t>(meta_event_t::lyric)) {
		return meta_event_t::lyric;
	} else if (d16==static_cast<uint16_t>(meta_event_t::marker)) {
		return meta_event_t::marker;
	} else if (d16==static_cast<uint16_t>(meta_event_t::cuepoint)) {
		return meta_event_t::cuepoint;
	} else if (d16==static_cast<uint16_t>(meta_event_t::chprefix)) {
		return meta_event_t::chprefix;
	} else if (d16==static_cast<uint16_t>(meta_event_t::eot)) {
		return meta_event_t::eot;
	} else if (d16==static_cast<uint16_t>(meta_event_t::tempo)) {
		return meta_event_t::tempo;
	} else if (d16==static_cast<uint16_t>(meta_event_t::smpteoffset)) {
		return meta_event_t::smpteoffset;
	} else if (d16==static_cast<uint16_t>(meta_event_t::timesig)) {
		return meta_event_t::timesig;
	} else if (d16==static_cast<uint16_t>(meta_event_t::keysig)) {
		return meta_event_t::keysig;
	} else if (d16==static_cast<uint16_t>(meta_event_t::seqspecific)) {
		return meta_event_t::seqspecific;
	} else {
		if ((d16&0xFF00u)==0xFF00u) {
			return meta_event_t::unknown;
		} else {
			return meta_event_t::invalid;
		}
	}
}

std::string print(const meta_event_t& mt) {
	if (mt==meta_event_t::seqn) {
		return "seqn";
	} else if (mt==meta_event_t::text) {
		return "text";
	} else if (mt==meta_event_t::copyright) {
		return "copyright";
	} else if (mt==meta_event_t::trackname) {
		return "trackname";
	} else if (mt==meta_event_t::instname) {
		return "instname";
	} else if (mt==meta_event_t::lyric) {
		return "lyric";
	} else if (mt==meta_event_t::marker) {
		return "marker";
	} else if (mt==meta_event_t::cuepoint) {
		return "cuepoint";
	} else if (mt==meta_event_t::chprefix) {
		return "chprefix";
	} else if (mt==meta_event_t::eot) {
		return "eot";
	} else if (mt==meta_event_t::tempo) {
		return "tempo";
	} else if (mt==meta_event_t::smpteoffset) {
		return "smpteoffset";
	} else if (mt==meta_event_t::timesig) {
		return "timesig";
	} else if (mt==meta_event_t::keysig) {
		return "keysig";
	} else if (mt==meta_event_t::seqspecific) {
		return "seqspecific";
	} else if (mt==meta_event_t::invalid) {
		return "invalid";
	} else if (mt==meta_event_t::unknown) {
		return "unknown";
	} else {
		return "?";
	}
}

meta_event_t classify_meta_event(const mtrk_event_t& ev) {
	if (ev.type()!=smf_event_type::meta) {
		return meta_event_t::invalid;
	}
	auto it = ev.event_begin();
	//auto p = ev.data_skipdt();
	uint16_t d16 = dbk::be_2_native<uint16_t>(&*it); // (p);
	return classify_meta_event_impl(d16);
}
bool is_meta(const mtrk_event_t& ev) {
	return ev.type()==smf_event_type::meta;
}
bool is_meta(const mtrk_event_t& ev, const meta_event_t& mtype) {
	if (ev.type()==smf_event_type::meta) {
		auto it = ev.event_begin();
		return classify_meta_event_impl(dbk::be_2_native<uint16_t>(&*it))==mtype;
		//auto p = ev.data_skipdt();
		//return classify_meta_event_impl(dbk::be_2_native<uint16_t>(p))==mtype;
	}
	return false;
}
bool is_seqn(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::seqn);
}
bool is_text(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::text);
}
bool is_copyright(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::copyright);
}
bool is_trackname(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::trackname);
}
bool is_instname(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::instname);
}
bool is_lyric(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::lyric);
}
bool is_marker(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::marker);
}
bool is_cuepoint(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::cuepoint);
}
bool is_chprefix(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::chprefix);
}
bool is_eot(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::eot);
}
bool is_tempo(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::tempo);
}
bool is_smpteoffset(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::smpteoffset);
}
bool is_timesig(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::timesig);
}
bool is_keysig(const mtrk_event_t& ev) {
	return is_meta(ev, meta_event_t::keysig);
}

bool meta_has_text(const mtrk_event_t& ev) {
	auto mttype = classify_meta_event(ev);
	return (mttype==meta_event_t::text 
			|| mttype==meta_event_t::copyright
			|| mttype==meta_event_t::instname
			|| mttype==meta_event_t::trackname
			|| mttype==meta_event_t::lyric
			|| mttype==meta_event_t::marker
			|| mttype==meta_event_t::cuepoint);
}

std::string meta_generic_gettext(const mtrk_event_t& ev) {
	std::string s {};
	if (!meta_has_text(ev)) {
		return s;
	}
	s = ev.text_payload();
	return s;
}

uint32_t get_tempo(const mtrk_event_t& ev, uint32_t def) {
	if (!is_tempo(ev)) {
		return def;
	}
	return ev.uint32_payload();
}

midi_timesig_t get_timesig(const mtrk_event_t& ev, midi_timesig_t def) {
	if (!is_timesig(ev)) {
		return def;
	}
	auto it = ev.payload_begin();
	if ((ev.end()-it) != 4) {
		return def;
	}

	midi_timesig_t result {};
	result.num = *it++;
	result.log2denom = *it++;
	result.clckspclk = *it++;
	result.ntd32pq = *it++;
	return result;
}

mtrk_event_t make_tempo(const uint32_t& dt, const uint32_t& uspqn) {
	//;  // meta text event w/ payload size==0
	std::array<unsigned char,7> evdata {0xFFu,0x51u,0x03u,0x00u,0x00u,0x00u,0x00u};
	write_24bit_be(evdata.begin()+3,evdata.end(),uspqn);
	auto result = mtrk_event_t(dt,evdata.data(),evdata.size(),0x00u);
	//auto it = midi_write_vl_field(result.begin(),result.end(),dt);
	//std::copy(evdata.begin(),evdata.end(),it);
	return result;
}
/*
mtrk_event_t make_eot(const uint32_t&);
mtrk_event_t make_timesig(const uint32_t&, const midi_timesig_t&);
mtrk_event_t make_instname(const uint32_t&, const std::string&);
mtrk_event_t make_trackname(const uint32_t&, const std::string&);
*/






bool is_channel(const mtrk_event_t& ev) {  // voice or mode
	auto s = ev.status_byte();
	return ((s&0x80u)==0x80u && (s&0xF0u)!=0xF0u);
}
bool is_channel_voice(const mtrk_event_t& ev) {
	auto md = ev.midi_data();
	if (!md.is_valid || (md.status_nybble == 0xB0u)) {
		return false;
	}
	return (((md.p1>>3)&0x0Fu)!=0x0Fu);
}
bool is_channel_mode(const mtrk_event_t& ev) {
	auto md = ev.midi_data();
	if (!md.is_valid || (md.status_nybble != 0xB0u)) {
		return false;
	}
	return (((md.p1>>3)&0x0Fu)==0x0Fu);
}
bool is_note_on(const mtrk_event_t& ev) {
	auto md = ev.midi_data();
	return (md.is_valid 
		&& md.status_nybble==0x90u && (md.p2 > 0));
}
bool is_note_off(const mtrk_event_t& ev) {
	auto md = ev.midi_data();
	return (md.is_valid && 
		((md.status_nybble==0x80u)
			|| (md.status_nybble==0x90u && (md.p2==0))));
}
bool is_key_aftertouch(const mtrk_event_t& ev) {
	return ((ev.status_byte() & 0xF0u) == 0xA0u);
}
bool is_control_change(const mtrk_event_t& ev) {
	return (((ev.status_byte()&0xF0u)==0xB0u) 
		&& is_channel_voice(ev));
}
bool is_program_change(const mtrk_event_t& ev) {
	return ((ev.status_byte() & 0xF0u) == 0xC0u);
}
bool is_channel_aftertouch(const mtrk_event_t& ev) {
	return ((ev.status_byte() & 0xF0u) == 0xD0u);
}
bool is_pitch_bend(const mtrk_event_t& ev) {
	return ((ev.status_byte() & 0xF0u) == 0xE0u);
}
bool is_onoff_pair(const mtrk_event_t& on, const mtrk_event_t& off) {
	if (!is_note_on(on) || !is_note_off(off)) {
		return false;
	}
	auto on_md = on.midi_data();
	auto off_md = off.midi_data();
	return is_onoff_pair(on_md.ch,on_md.p1,off_md.ch,off_md.p1);
}
bool is_onoff_pair(int on_ch, int on_note, const mtrk_event_t& off) {
	if (!is_note_off(off)) {
		return false;
	}
	auto off_md = off.midi_data();
	return is_onoff_pair(on_ch,on_note,off_md.ch,off_md.p1);
}
bool is_onoff_pair(int on_ch, int on_note, int off_ch, int off_note) {
	return (on_ch==off_ch && on_note==off_note);
}

midi_ch_event_t get_channel_event(const mtrk_event_t& ev, midi_ch_event_t def) {
	if (!is_channel(ev)) {
		return def;
	}
	auto md = ev.midi_data();
	midi_ch_event_t result {};
	result.status_nybble = md.status_nybble;
	result.ch = md.ch;
	result.p1 = md.p1;
	result.p2 = md.p2;
	return result;
}


/*
//
// Meta events
//
text_event_t::text_event_t() {
	std::array<unsigned char,14> data {0x00u,0xFFu,0x01u,0x0A,
		't','e','x','t',' ','e','v','e','n','t'};
	d_ = mtrk_event_t(&(data[0]),data.size(),0x00u);
}
text_event_t::text_event_t(const std::string& s) {
	std::vector<unsigned char> data {0x00u,0xFFu,0x01u};

	uint32_t payload_sz = 0;
	if (s.size() > std::numeric_limits<uint32_t>::max()) {
		payload_sz = std::numeric_limits<uint32_t>::max();
	} else {
		payload_sz = static_cast<uint32_t>(s.size());
	}
	midi_write_vl_field(std::back_inserter(data),payload_sz);

	for (const auto& e: s) {
		data.push_back(e);
	};
	
	d_ = mtrk_event_t(&(data[0]),data.size(),0x00u);
}
std::string text_event_t::text() const {
	const unsigned char *p = this->d_.data();
	auto mt = mtrk_event_parse_meta_dtstart_unsafe(p);
	p += mt.payload_offset;
	std::string result(reinterpret_cast<const char*>(p),mt.length);
	return result;
}
uint32_t text_event_t::delta_time() const {
	return this->d_.delta_time();
}
uint32_t text_event_t::size() const {
	return this->d_.size();
}
uint32_t text_event_t::data_size() const {
	return this->d_.data_size();
}

bool text_event_t::set_text(const std::string& s) {
	text_event_t txtev(s);
	this->d_ = txtev.d_;
	return true;
}
*/


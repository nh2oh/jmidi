#include "mtrk_event_t.h"
#include "midi_raw.h"
#include "midi_vlq.h"
#include "dbklib\byte_manipulation.h"
#include <string>
#include <cstdint>
#include <iostream>
#include <cstring>  // std::memcpy()
#include <exception>
#include <algorithm>
#include <vector>
#include <iterator>
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
//
// For callers who have pre-computed the exact size of the event and who
// can also supply a midi status byte if applicible, ex, an mtrk_container_iterator_t.  
//
mtrk_event_t::mtrk_event_t(const unsigned char *p, uint32_t sz, unsigned char s) {
	if (sz<=static_cast<uint64_t>(offs::max_size_sbo)) {  // small
		this->set_flag_small();

		auto end = std::copy(p,p+sz,this->d_.begin());
		while (end!=this->d_.end()) {
			*end++ = 0x00u;
		}
		this->midi_status_ = mtrk_event_get_midi_status_byte_dtstart_unsafe(p,s);
	} else {  // big
		auto new_p = new unsigned char[sz];
		std::copy(p,p+sz,new_p);
		this->init_big(new_p,sz,sz,s);  // adopts new_p
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

const unsigned char *mtrk_event_t::payload() const {
	const unsigned char *result = this->data();
	result += midi_interpret_vl_field(result).N;

	if (this->type()==smf_event_type::channel_mode
				|| this->type()==smf_event_type::channel_voice) {
		//result is pointing at either an event-local status byte or
		// the first data byte of the midi msg. 
	} else if (this->type()==smf_event_type::meta) {
		// result is pointing at an 0xFF
		result += 2;  // Skip the 0xFF and the type byte
		result += midi_interpret_vl_field(result).N;  // Skip the length field
	} else if (this->type()==smf_event_type::sysex_f0
				|| this->type()==smf_event_type::sysex_f7) {
		result += 1; // Skips the 0xF0 or 0xF7
		result += midi_interpret_vl_field(result).N;  // Skip the length field
	} else {  // Invalid
		result = nullptr;
	}
	return result;
}
unsigned char mtrk_event_t::operator[](uint32_t i) const {
	return *(this->data()+i);
};
const unsigned char *mtrk_event_t::raw_data() const {
	return &(this->d_[0]);
}
// TODO:  If small, this returns a ptr to a stack-allocated object...  Bad?
unsigned char *mtrk_event_t::data() const {
	if (this->is_small()) {
		return this->small_ptr();
	} else {
		return this->big_ptr();
	}
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
smf_event_type mtrk_event_t::type() const {
	if (is_small()) {
		return detect_mtrk_event_type_dtstart_unsafe(this->data(),this->midi_status_);
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
mtrk_event_t::midi_data_t mtrk_event_t::midi_data() const {
	mtrk_event_t::midi_data_t result {};
	result.is_valid = false;

	if (this->type()==smf_event_type::channel_mode
				|| this->type()==smf_event_type::channel_voice) {
		result.is_valid = true;
		
		auto dt = midi_interpret_vl_field(this->data());
		auto p = this->data()+dt.N;
		result.is_running_status = *p!=this->midi_status_;

		result.status_nybble = this->midi_status_&0xF0u;
		result.ch = this->midi_status_&0x0Fu;
		result.p1 = mtrk_event_get_midi_p1_dtstart_unsafe(this->data(),this->midi_status_);
		result.p2 = mtrk_event_get_midi_p2_dtstart_unsafe(this->data(),this->midi_status_);
	}

	return result;
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

		auto type_loc = detect_mtrk_event_type_dtstart_unsafe(this->small_ptr(),this->midi_status_);
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
		auto type_remote = detect_mtrk_event_type_dtstart_unsafe(this->big_ptr(),this->midi_status_);
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
	if (this->type()==smf_event_type::channel_voice 
			|| this->type()==smf_event_type::channel_mode) {
		// Present event is a midi event
		auto p = this->data()+midi_vl_field_size(this->delta_time());
		if (is_midi_status_byte(*p)) {  // not running status
			// The event-local status byte must match this->midi_status_
			tf &= (*p==this->midi_status_);
		} else {  // running status
			// this->midi_status_ must accurately describe the # of midi
			// data bytes.  
			tf &= (is_midi_status_byte(this->midi_status_));
			auto nbytesmsg = midi_channel_event_n_bytes(this->midi_status_,0x00u)-1;
			tf &= (is_midi_data_byte(*p));
			if (nbytesmsg==2) {
				tf &= (is_midi_data_byte(*++p));
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
bool mtrk_event_t::init_big(unsigned char *p, uint32_t sz, uint32_t c, unsigned char s) {
	this->set_flag_big();

	// Set the 3 big-container parameters to point to the remote data
	this->set_big_ptr(p);
	this->set_big_size(sz);
	this->set_big_cap(c);
	
	// Set the local "cached" values of delta-time and event type from the
	// remote data.  These values are stored in the d_ array, so setters
	// that know the offsets and can do the serialization correctly are used.  
	this->set_big_cached_delta_t(midi_interpret_vl_field(p).val);
	this->set_big_cached_smf_event_type(detect_mtrk_event_type_dtstart_unsafe(p,s));

	// The two d_-external values to set are midi_status_ and flags
	this->midi_status_ = mtrk_event_get_midi_status_byte_dtstart_unsafe(p,s);

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
	s += ("delta_time == " + std::to_string(evnt.delta_time()) + ", ");
	s += ("type == " + print(evnt.type()) + ", ");
	s += ("data_size == " + std::to_string(evnt.data_size()) + ", ");
	s += ("size == " + std::to_string(evnt.size()) + "\n\t");

	auto ss = evnt.size();
	auto ds = evnt.data_size();
	auto n = ss-ds;

	s += ("[" + dbk::print_hexascii(evnt.data(), evnt.size()-evnt.data_size(), ' ') + "] ");
	s += dbk::print_hexascii(evnt.data()+n, evnt.data_size(), ' ');

	if (opts == mtrk_sbo_print_opts::debug) {
		s += "\n\t";
		if (evnt.is_small()) {
			s += "sbo=>small == ";
		} else {
			s += "sbo=>big   == ";
		}
		s += "{";
		s += dbk::print_hexascii(evnt.raw_data(), sizeof(mtrk_event_t), ' ');
		s += "}; \n";
		s += "\tbigsmall_flag==";
		s += dbk::print_hexascii(evnt.raw_flag(), 1, ' ');
	}
	return s;
}







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



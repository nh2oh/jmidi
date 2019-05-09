#include "mtrk_container_t.h"
#include "mtrk_event_t.h"
#include "midi_raw.h"
#include "dbklib\byte_manipulation.h"
#include <string>
#include <cstdint>
#include <iostream>
#include <cstring>  // std::memcpy()
#include <exception>
#include <algorithm>
#include <vector>
#include <iterator>


// Private ctor used by friend class mtrk_view_t.begin(),.end()
mtrk_iterator_t::mtrk_iterator_t(const unsigned char *p, unsigned char s) {
	this->p_ = p;
	this->s_ = s;
}
mtrk_event_t mtrk_iterator_t::operator*() const {
	// Note that this->s_ indicates the status of the event prior to this->p_.  
	auto sz = mtrk_event_get_size_dtstart_unsafe(this->p_,this->s_);
	return mtrk_event_t(this->p_,sz,this->s_);
}
mtrk_iterator_t& mtrk_iterator_t::operator++() {
	auto dt = midi_interpret_vl_field(this->p_);
	this->s_ = mtrk_event_get_midi_status_byte_unsafe(this->p_+dt.N,this->s_);
	auto sz = mtrk_event_get_size_dtstart_unsafe(this->p_,this->s_);
	this->p_+=sz;
	// Note that this->s_ now indicates the status of the *prior* event.  
	// Were I to attempt to update it to correspond to the present event(
	// that indicated by this->p_), ex:
	// this->s_ = mtrk_event_get_midi_status_byte_unsafe(this->p_,this->s_);
	// i will dereference an invalid this->p_ in the case that the prior event
	// was the last in the mtrk sequence and this->p_ is now pointing one past
	// the end of the valid range.  
	// One possible alternative design is to check for the end-of-sequence
	// mtrk event {0x00u,0xFFu,0x2Fu}
	return *this;
}
bool mtrk_iterator_t::operator==(const mtrk_iterator_t& rhs) const {
	// Note that this->s_ is not compared; it should be impossible for two
	// iterators w/ == p_ to have != s_, since for each, p_ must have been
	// set by an identical sequence of operations (operator++()...).  
	// However, the .end() method of mtrk_view_t constructs an iterator w/
	// p_ pointing one past the end of the seq and midi status byte 0x00u,
	// hence for the end iterator, s_ is not nec. valid.  
	return this->p_ == rhs.p_;
}
bool mtrk_iterator_t::operator!=(const mtrk_iterator_t& rhs) const {
	return !(*this==rhs);
}
mtrk_event_view_t mtrk_iterator_t::operator->() const {
	mtrk_event_view_t result;
	result.p_ = this->p_; result.s_ = this->s_;
	return result;
}





smf_event_type mtrk_event_view_t::type() const {
	return detect_mtrk_event_type_dtstart_unsafe(this->p_,this->s_);
}
uint32_t mtrk_event_view_t::size() const {
	return mtrk_event_get_size_dtstart_unsafe(this->p_,this->s_);
}
uint32_t mtrk_event_view_t::data_length() const {
	auto sz = mtrk_event_get_size_dtstart_unsafe(this->p_,this->s_);
	auto dl = midi_interpret_vl_field(this->p_).N;
	return sz-dl;
}
int32_t mtrk_event_view_t::delta_time() const {
	return midi_interpret_vl_field(this->p_).val;
}
// Only funtional for midi events
channel_msg_type mtrk_event_view_t::ch_msg_type() const {
	return mtrk_event_get_ch_msg_type_dtstart_unsafe(this->p_,this->s_);
}
uint8_t mtrk_event_view_t::velocity() const {
	return mtrk_event_get_midi_p1_dtstart_unsafe(this->p_,this->s_);
}
uint8_t mtrk_event_view_t::chn() const {
	auto s = mtrk_event_get_midi_status_byte_dtstart_unsafe(this->p_,this->s_);
	return channel_number_from_status_byte_unsafe(s);
}
uint8_t mtrk_event_view_t::note() const {
	return mtrk_event_get_midi_p1_dtstart_unsafe(this->p_,this->s_);
}
uint8_t mtrk_event_view_t::p1() const {
	return mtrk_event_get_midi_p1_dtstart_unsafe(this->p_,this->s_);
}
uint8_t mtrk_event_view_t::p2() const {
	return mtrk_event_get_midi_p2_dtstart_unsafe(this->p_,this->s_);
}






mtrk_view_t::mtrk_view_t(const validate_mtrk_chunk_result_t& mtrk) {
	if (!mtrk.is_valid) {
		std::abort();
	}

	this->p_ = mtrk.p;
	this->size_ = mtrk.size;
}
mtrk_view_t::mtrk_view_t(const unsigned char *p, uint32_t sz) {
	this->p_ = p;
	this->size_ = sz;
}
uint32_t mtrk_view_t::data_length() const {
	return this->size_-8;
	// Alternatively, could do:  return dbk::be_2_native<uint32_t>(this->p_+4);
	// However, *(p_+4) might not be in cache, but size_ will always be hot.  
}
uint32_t mtrk_view_t::size() const {
	return this->size_;
}
const unsigned char *mtrk_view_t::data() const {
	return this->p_;
}
mtrk_iterator_t mtrk_view_t::begin() const {
	// Note that in an mtrk_iterator_t, the member s_ is the status from the 
	// _prior_ event.  
	// See the std p.135:  "The first event in each MTrk chunk must specify status"
	// ...yet i have many midi files which begin w/ 0x00u,0xFFu,...
	return mtrk_iterator_t::mtrk_iterator_t(this->p_+8,0x00u);
}
mtrk_iterator_t mtrk_view_t::end() const {
	// Note that i am supplying an invalid midi status byte for this 
	// one-past-the-end iterator.  
	return mtrk_iterator_t::mtrk_iterator_t(this->p_+this->size(),0x00u);
}
bool mtrk_view_t::validate() const {
	bool tf = true;
	tf = tf && (this->size_-8 == dbk::be_2_native<uint32_t>(this->p_+4));

	auto mtrk = validate_mtrk_chunk(this->p_,this->size_);
	tf = tf && mtrk.is_valid;

	return tf;
}

// TODO:  This should not deref the ptr, it should call some sort of
// low level print(), perhaps one that works directly w/a unsigned char *;
// we have already resolved to work with a view type.  
std::string print(const mtrk_view_t& mtrk) {
	std::string s {};
	for (mtrk_iterator_t it=mtrk.begin(); it!=mtrk.end(); ++it) {
		auto ev = *it;
		s += print(ev,mtrk_sbo_print_opts::debug);
		s += "\n";
	}

	return s;
}



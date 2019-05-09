#include "mtrk_container_t.h"
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

std::string print(const mtrk_view_t& mtrk) {
	std::string s {};
	for (mtrk_iterator_t it=mtrk.begin(); it!=mtrk.end(); ++it) {
		auto ev = *it;
		s += print(ev,mtrk_sbo_print_opts::debug);
		s += "\n";
	}

	return s;
}


mtrk_event_t::mtrk_event_t() {
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
		this->set_flag_big();

		this->big_ptr(new unsigned char[sz]);
		this->big_size(sz);
		this->big_cap(sz);
		std::copy(p,p+sz,this->big_ptr());

		auto ev = parse_mtrk_event_type(p,s,sz);
		this->big_delta_t(ev.delta_t.val);
		this->big_smf_event_type(ev.type);
		this->midi_status_ = mtrk_event_get_midi_status_byte_dtstart_unsafe(p,s);
	}
}
//
// Copy ctor
//
mtrk_event_t::mtrk_event_t(const mtrk_event_t& rhs) {
	this->d_ = rhs.d_;
	this->midi_status_ = rhs.midi_status_;
	this->flags_ = rhs.flags_;
	if (rhs.is_big()) {
		// At this point, this is a clone of rhs; both rhs.big_ptr() and
		// this.big_ptr() are pointing at the same memory.  
		unsigned char *new_p = new unsigned char[rhs.big_cap()];
		std::copy(this->big_ptr(),this->big_ptr()+this->big_cap(),new_p);
		this->big_ptr(new_p);
	}
}
//
// Copy assignment; overwrites a pre-existing lhs 'this' w/ rhs
//
mtrk_event_t& mtrk_event_t::operator=(const mtrk_event_t& rhs) {
	if (this->is_big()) {
		delete this->big_ptr();
	}
	this->d_ = rhs.d_;
	this->midi_status_ = rhs.midi_status_;
	this->flags_ = rhs.flags_;

	if (this->is_big()) {
		// At this point, this is a clone of rhs; both rhs.big_ptr() and
		// this.big_ptr() are pointing at the same memory. 
		// Deep copy the pointed-at range
		unsigned char *new_p = new unsigned char[rhs.big_cap()];
		std::copy(this->big_ptr(),this->big_ptr()+this->big_cap(),new_p);
		this->big_ptr(new_p);
	}
	return *this;
}
//
// Move ctor
//
mtrk_event_t::mtrk_event_t(mtrk_event_t&& rhs) {
	this->d_ = rhs.d_;
	this->midi_status_ = rhs.midi_status_;
	this->flags_ = rhs.flags_;
	if (this->is_big()) {
		rhs.set_flag_small();  // prevents ~rhs() from freeing its memory
		// Note that this state of rhs is invalid as it almost certinally does
		// not contain a valid "small" mtrk event
	}
}
//
// Move assignment
//
mtrk_event_t& mtrk_event_t::operator=(mtrk_event_t&& rhs) {
	if (this->is_big()) {
		delete this->big_ptr();
	}

	this->d_ = rhs.d_;
	this->midi_status_ = rhs.midi_status_;
	this->flags_ = rhs.flags_;
	if (rhs.is_big()) {
		rhs.set_flag_small();  // prevents ~rhs() from freeing its memory
		// Note that this state of rhs is invalid as it almost certinally does
		// not contain a valid "small" mtrk event
	}
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
const unsigned char *mtrk_event_t::data() const {
	if (this->is_small()) {
		return &(this->d_[0]);
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
		auto sz = mtrk_event_get_size_dtstart_unsafe(this->data(),this->midi_status_);
		auto dt = midi_interpret_vl_field(this->data());
		if (dt.N >= sz) {
			std::abort();
		}
		return sz-dt.N;
}
uint32_t mtrk_event_t::size() const {  // Includes the contrib from delta-t
	return mtrk_event_get_size_dtstart_unsafe(this->data(),this->midi_status_);
}
bool mtrk_event_t::set_delta_time(uint32_t dt) {
	if (this->is_big()) {
		this->big_delta_t(dt);
		// TODO:  There is the possibility that the new dt is smaller than
		// the old dt, and this causes the object now to fit in the sbo.
	} else {  // small
		auto curr_dt_size = midi_interpret_vl_field(&(this->d_[0])).N;
		auto new_dt_size = midi_vl_field_size(dt);
		if (new_dt_size==curr_dt_size) {
			midi_write_vl_field(this->d_.begin(),this->d_.end(),dt);
		} else if (new_dt_size<curr_dt_size) {
			auto it_currdatastart = this->d_.begin()+curr_dt_size;
			auto it_newdatastart = midi_write_vl_field(this->d_.begin(),this->d_.end(),dt);
			auto it_newdataend = std::copy(it_currdatastart,this->d_.end(),it_newdatastart);
			std::fill(it_newdataend,this->d_.end(),0x00u);
		} else if (new_dt_size>curr_dt_size) {
			uint64_t sbo_sz = static_cast<uint64_t>(offs::max_size_sbo);
			if ((new_dt_size+this->data_size())<=sbo_sz) {  // Fits
				auto olddata = this->d_;
				auto olddata_size = this->data_size();
				auto it = midi_write_vl_field(this->d_.begin(),this->d_.end(),dt);
				std::copy(olddata.begin()+curr_dt_size,olddata.begin()+olddata_size,it);
			} else {  // Does not fit
				auto olddata = this->d_;
				auto new_size = new_dt_size+this->data_size();

				this->set_flag_big();
				auto p = this->big_ptr(new unsigned char[new_size]);
				this->big_size(new_size);
				this->big_cap(new_size);
				p = midi_write_vl_field(p,dt);
				std::copy(olddata.begin()+curr_dt_size,olddata.end(),p);

				auto ev = parse_mtrk_event_type(this->data(),this->midi_status_,this->size());
				this->big_delta_t(ev.delta_t.val);
				this->big_smf_event_type(ev.type);
				this->midi_status_ = 
					mtrk_event_get_midi_status_byte_dtstart_unsafe(this->data(),this->midi_status_);
			}
			auto olddata = this->d_;
		}
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

	auto dt_val = this->delta_time();
	tf &= dt_val>=0;
	if (this->is_small()) {
		auto dt_raw_interp = midi_interpret_vl_field(this->data());
		tf &= dt_raw_interp.N > 0 && dt_raw_interp.N <= 4;
		tf &= dt_raw_interp.val==dt_val;

		auto sz = mtrk_event_get_size_dtstart_unsafe(&(this->d_[0]),this->midi_status_);
		tf &= sz==this->size();
	} else {  // big
		tf &= this->big_delta_t()==dt_val;

		auto sz = mtrk_event_get_size_dtstart_unsafe(this->big_ptr(),this->midi_status_);
		tf &= sz==this->big_size();
		tf &= sz==this->size();
	}

	return tf;
}


bool mtrk_event_t::is_big() const {
	return (this->flags_&0x80u)==0x00u;
}
bool mtrk_event_t::is_small() const {
	return !(this->is_big());
}
void mtrk_event_t::set_flag_big() {
	this->flags_ &= 0x7Fu;
}
void mtrk_event_t::set_flag_small() {
	this->flags_ |= 0x80u;
}

unsigned char *mtrk_event_t::big_ptr() const {
	if (this->is_small()) {
		std::abort();
	}
	unsigned char *p {nullptr};
	uint64_t o = static_cast<uint64_t>(offs::ptr);
	std::memcpy(&p,&(this->d_[o]),sizeof(p));
	return p;
}
unsigned char *mtrk_event_t::big_ptr(unsigned char *p) {
	if (this->is_small()) {
		std::abort();
	}
	uint64_t o = static_cast<uint64_t>(offs::ptr);
	std::memcpy(&(this->d_[o]),&p,sizeof(p));
	return p;
}
uint32_t mtrk_event_t::big_size() const {
	if (this->is_small()) {
		std::abort();
	}
	uint32_t sz {0};
	uint64_t o = static_cast<uint64_t>(offs::size);
	std::memcpy(&sz,&(this->d_[o]),sizeof(uint32_t));
	return sz;
}
uint32_t mtrk_event_t::big_size(uint32_t sz) {
	if (this->is_small()) {
		std::abort();
	}
	uint64_t o = static_cast<uint64_t>(offs::size);
	std::memcpy(&(this->d_[o]),&sz,sizeof(uint32_t));
	return sz;
}
uint32_t mtrk_event_t::big_cap() const {
	if (this->is_small()) {
		std::abort();
	}
	uint32_t c {0};
	uint64_t o = static_cast<uint64_t>(offs::cap);
	std::memcpy(&c,&(this->d_[o]),sizeof(uint32_t));
	return c;
}
uint32_t mtrk_event_t::big_cap(uint32_t c) {
	if (this->is_small()) {
		std::abort();
	}
	uint64_t o = static_cast<uint64_t>(offs::cap);
	std::memcpy(&(this->d_[o]),&c,sizeof(uint32_t));
	return c;
}
uint32_t mtrk_event_t::big_delta_t() const {
	if (this->is_small()) {
		std::abort();
	}
	uint32_t dt {0};
	uint64_t o = static_cast<uint64_t>(offs::dt);
	std::memcpy(&dt,&(this->d_[o]),sizeof(uint32_t));
	return dt;
}
uint32_t mtrk_event_t::big_delta_t(uint32_t dt) {
	if (this->is_small()) {
		std::abort();
	}
	uint64_t o = static_cast<uint64_t>(offs::dt);
	std::memcpy(&(this->d_[o]),&dt,sizeof(uint32_t));
	return dt;
}
smf_event_type mtrk_event_t::big_smf_event_type() const {
	if (this->is_small()) {
		std::abort();
	}
	smf_event_type t {smf_event_type::invalid};
	uint64_t o = static_cast<uint64_t>(offs::type);
	std::memcpy(&t,&(this->d_[o]),sizeof(smf_event_type));
	return t;
}
smf_event_type mtrk_event_t::big_smf_event_type(smf_event_type t) {
	if (this->is_small()) {
		std::abort();
	}
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



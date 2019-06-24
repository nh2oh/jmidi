#include "mtrk_event_t2.h"
#include "mtrk_event_iterator_t2.h"
#include "midi_raw.h"
#include "midi_vlq.h"
#include "dbklib\byte_manipulation.h"
#include <string>
#include <cstdint>
#include <cstring>  // std::memcpy()
#include <exception>  // std::abort()
#include <algorithm>  
#include <utility>  // std::move()



unsigned char *mtrk_event_t2::small_t::begin() {
	return &(this->d_[0]);
}
const unsigned char *mtrk_event_t2::small_t::begin() const {
	return &(this->d_[0]);
}
unsigned char *mtrk_event_t2::small_t::end() {
	return &(this->d_[0]) + mtrk_event_get_size_dtstart_unsafe(&(this->d_[0]),0x00u);
}
const unsigned char *mtrk_event_t2::small_t::end() const {
	return &(this->d_[0]) + mtrk_event_get_size_dtstart_unsafe(&(this->d_[0]),0x00u);
}
uint64_t mtrk_event_t2::small_t::size() const {
	return mtrk_event_get_size_dtstart_unsafe(&(this->d_[0]),0x00u);
}
constexpr uint64_t mtrk_event_t2::small_t::capacity() const {
	static_assert(sizeof(this->d_) > 1);
	return sizeof(this->d_)-1;
}
uint64_t mtrk_event_t2::big_t::size() const {
	return this->sz_;
}
uint64_t mtrk_event_t2::big_t::capacity() const {
	return this->cap_;
}
unsigned char *mtrk_event_t2::big_t::begin() {
	return this->p_;
}
const unsigned char *mtrk_event_t2::big_t::begin() const {
	return this->p_;
}
unsigned char *mtrk_event_t2::big_t::end() {
	return this->p_ + this->sz_;
}
const unsigned char *mtrk_event_t2::big_t::end() const {
	return this->p_ + this->sz_;
}
bool mtrk_event_t2::sbo_t::is_small() const {
	return ((this->u_.s_.flags_)&0x80u)==0x80u;
}
bool mtrk_event_t2::sbo_t::is_big() const {
	return !(this->is_small());
}
void mtrk_event_t2::sbo_t::set_flag_small() {
	this->u_.s_.flags_ |= 0x80u;  // Note i always go through s_
}
void mtrk_event_t2::sbo_t::set_flag_big() {
	this->u_.s_.flags_ &= 0x7Fu;  // Note i always go through s_
}
void mtrk_event_t2::sbo_t::free_if_big() {
	if (this->is_big()) {
		delete this->u_.b_.p_;
		this->big_adopt(nullptr,0,0);  // sets flag big
	}
}
void mtrk_event_t2::sbo_t::big_adopt(unsigned char *p, uint32_t sz, uint32_t c) {
	this->set_flag_big();
	this->u_.b_.p_=p;
	this->u_.b_.sz_=sz;
	this->u_.b_.cap_=c;
}
void mtrk_event_t2::sbo_t::zero_object() {
	this->set_flag_small();
	for (auto it=this->u_.s_.begin(); it!=this->u_.s_.end(); ++it) {
		*it=0x00u;
	}
}
uint64_t mtrk_event_t2::sbo_t::size() const {
	if (this->is_small()) {
		return this->u_.s_.size();
	} else {
		return this->u_.b_.size();
	}
}
uint64_t mtrk_event_t2::sbo_t::capacity() const {
	if (this->is_small()) {
		return this->u_.s_.capacity();
	} else {
		return this->u_.b_.capacity();
	}
}
constexpr uint64_t mtrk_event_t2::sbo_t::small_capacity() {
	return this->u_.s_.capacity();
}
unsigned char *mtrk_event_t2::sbo_t::begin() {
	if (this->is_small()) {
		return this->u_.s_.begin();
	} else {
		return this->u_.b_.begin();
	}
}
const unsigned char *mtrk_event_t2::sbo_t::begin() const {
	if (this->is_small()) {
		return this->u_.s_.begin();
	} else {
		return this->u_.b_.begin();
	}
}
unsigned char *mtrk_event_t2::sbo_t::end() {
	if (this->is_small()) {
		return this->u_.s_.end();
	} else {
		return this->u_.b_.end();
	}
}
const unsigned char *mtrk_event_t2::sbo_t::end() const {
	if (this->is_small()) {
		return this->u_.s_.end();
	} else {
		return this->u_.b_.end();
	}
}

// Default ctor creates a meta text-event of length 0 
mtrk_event_t2::mtrk_event_t2() {  
	default_init();
}
//
// For callers who have pre-computed the exact size of the event and who
// can also supply a midi status byte if applicible, ex, an 
// mtrk_container_iterator_t.  
//
mtrk_event_t2::mtrk_event_t2(const unsigned char *p, uint32_t sz, unsigned char rs) {
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

	dest_end = std::copy(p,p+(sz-dt.N),dest_end);
	std::fill(dest_end,this->d_.end(),0x00u);
}
// TODO:  Clamp dt to the max allowable value
mtrk_event_t2::mtrk_event_t2(const uint32_t& dt, const unsigned char *p, 
										uint32_t sz, unsigned char rs) {
	auto cap = sz;
	auto dtN = midi_vl_field_size(dt);
	cap += dtN;
	auto s = get_status_byte(*p,rs);
	bool has_local_status = (s==*p);
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
	auto dest_end = std::copy(p,p+dtN,dest);
	p+=dtN;
	
	*dest_end++ = s;
	if (has_local_status) {
		++p;
	}

	dest_end = std::copy(p,p+(sz-dtN),dest_end);
	std::fill(dest_end,this->d_.end(),0x00u);
}
mtrk_event_t2::mtrk_event_t2(uint32_t dt, midi_ch_event_t md) {
	md = normalize(md);
	this->d_.set_flag_small();
	auto dest_end = midi_write_vl_field(this->d_.begin(),dt);
	unsigned char s = 0x80u|(md.status_nybble);
	s += 0x0Fu&(md.ch);
	*dest_end++ = s;
	*dest_end++ = 0x7Fu&(md.p1);
	if (channel_status_byte_n_data_bytes(s)==2) {
		*dest_end++ = 0x7Fu&(md.p2);
	}
	std::fill(dest_end,this->d_.end(),0x00u);
}
//
// Copy ctor
//
mtrk_event_t2::mtrk_event_t2(const mtrk_event_t2& rhs) {
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
mtrk_event_t2& mtrk_event_t2::operator=(const mtrk_event_t2& rhs) {
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
mtrk_event_t2::mtrk_event_t2(mtrk_event_t2&& rhs) noexcept {
	this->d_ = rhs.d_;
	rhs.default_init();
	// Prevents ~rhs() from freeing its memory.  Sets the 'small' flag and
	// zeros all elements of the data array.  
}
//
// Move assign
//
mtrk_event_t2& mtrk_event_t2::operator=(mtrk_event_t2&& rhs) noexcept {
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
mtrk_event_t2::~mtrk_event_t2() {
	this->d_.free_if_big();
}

uint64_t mtrk_event_t2::size() const {
	return this->d_.size();
}
uint64_t mtrk_event_t2::capacity() const {
	return this->d_.capacity();
}

unsigned char *mtrk_event_t2::data() {
	return this->d_.begin();
}
const unsigned char *mtrk_event_t2::data() const {
	return this->d_.begin();
}
mtrk_event_iterator_t2 mtrk_event_t2::begin() {
	return mtrk_event_iterator_t2(*this);
}
mtrk_event_const_iterator_t2 mtrk_event_t2::begin() const {
	return mtrk_event_const_iterator_t2(*this);
}
mtrk_event_iterator_t2 mtrk_event_t2::end() {
	return mtrk_event_iterator_t2(*this);
}
mtrk_event_const_iterator_t2 mtrk_event_t2::end() const {
	return mtrk_event_const_iterator_t2(*this);
}
mtrk_event_const_iterator_t2 mtrk_event_t2::dt_begin() const {
	return this->begin();
}
mtrk_event_iterator_t2 mtrk_event_t2::dt_begin() {
	return this->begin();
}
mtrk_event_const_iterator_t2 mtrk_event_t2::dt_end() const {
	return advance_to_vlq_end(this->begin());
}
mtrk_event_iterator_t2 mtrk_event_t2::dt_end() {
	return advance_to_vlq_end(this->begin());
}
mtrk_event_const_iterator_t2 mtrk_event_t2::event_begin() const {
	return advance_to_vlq_end(this->begin());
}
mtrk_event_iterator_t2 mtrk_event_t2::event_begin() {
	return advance_to_vlq_end(this->begin());
}
mtrk_event_const_iterator_t2 mtrk_event_t2::payload_begin() const {
	auto it = this->event_begin();
	if (this->type()==smf_event_type::meta) {
		it += 2;  // 0xFFu, type-byte
		it = advance_to_vlq_end(it);
	} else if (this->type()==smf_event_type::sysex_f0
					|| this->type()==smf_event_type::sysex_f7) {
		it += 1;  // 0xF0u or 0xF7u
		it = advance_to_vlq_end(it);
	} // else { smf_event_type::channel_voice, _mode, unknown, invalid...
	return it;
}
mtrk_event_iterator_t2 mtrk_event_t2::payload_begin() {
	auto it = this->event_begin();
	if (this->type()==smf_event_type::meta) {
		it += 2;  // 0xFFu, type-byte
		it = advance_to_vlq_end(it);
	} else if (this->type()==smf_event_type::sysex_f0
					|| this->type()==smf_event_type::sysex_f7) {
		it += 1;  // 0xF0u or 0xF7u
		it = advance_to_vlq_end(it);
	} // else { smf_event_type::channel_voice, _mode, unknown, invalid...
	return it;
}

smf_event_type mtrk_event_t2::type() const {
	auto p = advance_to_vlq_end(this->data());
	return classify_status_byte(*p);
	// Faster than classify_mtrk_event_dtstart_unsafe(s,rs), b/c the latter
	// calls classify_status_byte(get_status_byte(s,rs)); here, i do not 
	// need to worry about the possibility of the rs byte.  
}







void mtrk_event_t2::default_init() {
	this->d_.set_flag_small();
	std::array<unsigned char,4> evdata = {0x00,0xFF,0x01,0x00};
	auto end = std::copy(evdata.begin(),evdata.end(),this->d_.begin());
	std::fill(end,this->d_.end(),0x00u);
}




/*







mtrk_event_const_iterator_t mtrk_event_t2::end() const {
	return this->begin()+this->size();
}
mtrk_event_iterator_t mtrk_event_t2::end() {
	return this->begin()+this->size();
}
std::string mtrk_event_t2::text_payload() const {
	std::string s {};
	if (this->type()==smf_event_type::meta 
				|| this->type()==smf_event_type::sysex_f0
				|| this->type()==smf_event_type::sysex_f7) {
		std::copy(this->payload_begin(),this->end(),std::back_inserter(s));
	}
	return s;
}
uint32_t mtrk_event_t2::uint32_payload() const {
	return be_2_native<uint32_t>(this->payload_begin(),this->end());
}
mtrk_event_t2::channel_event_data_t mtrk_event_t2::midi_data() const {
	mtrk_event_t2::channel_event_data_t result {0x00u,0x80u,0x80u,0x80u};
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
const unsigned char& mtrk_event_t2::operator[](uint32_t i) const {
	const auto& r = *(this->data()+i);
	return r;
};
unsigned char& mtrk_event_t2::operator[](uint32_t i) {
	return *(this->data()+i);
};
const unsigned char *mtrk_event_t2::data() const {
	if (this->is_small()) {
		return this->small_ptr();
	} else {
		return this->big_ptr();
	}
}
unsigned char *mtrk_event_t2::data() {
	if (this->is_small()) {
		return this->small_ptr();
	} else {
		return this->big_ptr();
	}
}
const unsigned char *mtrk_event_t2::raw_data() const {
	return &(this->d_[0]);
}
const unsigned char *mtrk_event_t2::raw_flag() const {
	return &(this->flags_);
}
uint32_t mtrk_event_t2::delta_time() const {
	if (this->is_small()) {
		return midi_interpret_vl_field(this->data()).val;
	} else {
		return this->big_delta_t();
	}
}
unsigned char mtrk_event_t2::status_byte() const {
	// Since at present this->midi_status_ is == 0x00u for meta or sysex
	// events, have to check for its validity.  
	if ((this->midi_status_ & 0x80u) == 0x80u) {
		return this->midi_status_;
	} else {
		return *(this->event_begin());
	}
}
unsigned char mtrk_event_t2::running_status() const {
	// Since at present this->midi_status_ is == 0x00u for meta or sysex
	// events, have to check for its validity.  
	return get_running_status_byte(this->midi_status_,*(this->event_begin()));
}
smf_event_type mtrk_event_t2::type() const {
	if (is_small()) {
		// TODO:  Arbitary max_size==6
		return classify_mtrk_event_dtstart(this->data(),this->midi_status_,6);
	} else {
		return this->big_smf_event_type();
	}
}
uint32_t mtrk_event_t2::data_size() const {  // Not indluding delta-t
	if (this->is_small()) {
		return mtrk_event_get_data_size_dtstart_unsafe(this->small_ptr(),this->midi_status_);
	} else {
		return mtrk_event_get_data_size_dtstart_unsafe(this->big_ptr(),this->midi_status_);
	}
}
uint32_t mtrk_event_t2::size() const {  // Includes the contrib from delta-t
	if (this->is_small()) {
		return mtrk_event_get_size_dtstart_unsafe(this->small_ptr(),this->midi_status_);
	} else {
		return this->big_size();
	}
}

bool mtrk_event_t2::set_delta_time(uint32_t dt) {
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
bool mtrk_event_t2::operator==(const mtrk_event_t2& rhs) const {
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
bool mtrk_event_t2::operator!=(const mtrk_event_t2& rhs) const {
	return !(*this==rhs);
}
bool mtrk_event_t2::validate() const {
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


bool mtrk_event_t2::is_big() const {
	return (this->flags_&0x80u)==0x00u;
}
bool mtrk_event_t2::is_small() const {
	return !(this->is_big());
}

void mtrk_event_t2::clear_nofree() {
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
bool mtrk_event_t2::init_big(unsigned char *p, uint32_t sz, uint32_t c, unsigned char rs) {
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
bool mtrk_event_t2::small2big(uint32_t c) {
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
bool mtrk_event_t2::big_resize(uint32_t c) {
	if (!is_big()) {
		std::abort();
	}

	unsigned char *p = new unsigned char[c];
	std::copy(this->data(),this->data()+this->size(),p);
	delete this->big_ptr();
	init_big(p,this->size(),c,this->midi_status_);

	return true;
}


void mtrk_event_t2::set_flag_big() {
	this->flags_ &= 0x7Fu;
}
void mtrk_event_t2::set_flag_small() {
	this->flags_ |= 0x80u;
}

unsigned char *mtrk_event_t2::big_ptr() const {
	unsigned char *p {nullptr};
	uint64_t o = static_cast<uint64_t>(offs::ptr);
	std::memcpy(&p,&(this->d_[o]),sizeof(p));
	return p;
}
unsigned char *mtrk_event_t2::small_ptr() const {
	return const_cast<unsigned char*>(&(this->d_[0]));
}
unsigned char *mtrk_event_t2::set_big_ptr(unsigned char *p) {
	if (this->is_small()) {
		std::abort();
	}
	uint64_t o = static_cast<uint64_t>(offs::ptr);
	std::memcpy(&(this->d_[o]),&p,sizeof(p));
	return p;
}
uint32_t mtrk_event_t2::big_size() const {
	uint32_t sz {0};
	uint64_t o = static_cast<uint64_t>(offs::size);
	std::memcpy(&sz,&(this->d_[o]),sizeof(uint32_t));
	return sz;
}
uint32_t mtrk_event_t2::set_big_size(uint32_t sz) {
	if (this->is_small()) {
		std::abort();
	}
	uint64_t o = static_cast<uint64_t>(offs::size);
	std::memcpy(&(this->d_[o]),&sz,sizeof(uint32_t));
	return sz;
}
uint32_t mtrk_event_t2::big_cap() const {
	uint32_t c {0};
	uint64_t o = static_cast<uint64_t>(offs::cap);
	std::memcpy(&c,&(this->d_[o]),sizeof(uint32_t));
	return c;
}
uint32_t mtrk_event_t2::set_big_cap(uint32_t c) {
	uint64_t o = static_cast<uint64_t>(offs::cap);
	std::memcpy(&(this->d_[o]),&c,sizeof(uint32_t));
	return c;
}
uint32_t mtrk_event_t2::big_delta_t() const {
	uint32_t dt {0};
	uint64_t o = static_cast<uint64_t>(offs::dt);
	std::memcpy(&dt,&(this->d_[o]),sizeof(uint32_t));
	return dt;
}
uint32_t mtrk_event_t2::set_big_cached_delta_t(uint32_t dt) {
	// Sets the local "cached" dt value
	uint64_t o = static_cast<uint64_t>(offs::dt);
	std::memcpy(&(this->d_[o]),&dt,sizeof(uint32_t));
	return dt;
}
smf_event_type mtrk_event_t2::big_smf_event_type() const {
	smf_event_type t {smf_event_type::invalid};
	uint64_t o = static_cast<uint64_t>(offs::type);
	std::memcpy(&t,&(this->d_[o]),sizeof(smf_event_type));
	return t;
}
smf_event_type mtrk_event_t2::set_big_cached_smf_event_type(smf_event_type t) {
	uint64_t o = static_cast<uint64_t>(offs::type);
	std::memcpy(&(this->d_[o]),&t,sizeof(smf_event_type));
	return t;
}


std::string print(const mtrk_event_t2& evnt, mtrk_sbo_print_opts opts) {
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
		s += dbk::print_hexascii(evnt.raw_data(), sizeof(mtrk_event_t2), ' ');
		s += "}; \n";
		s += "\tbigsmall_flag = ";
		s += dbk::print_hexascii(evnt.raw_flag(), 1, ' ');
	}
	return s;
}
*/

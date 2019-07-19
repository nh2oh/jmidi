#include "mtrk_event_t_internal.h"
#include "midi_raw.h"
#include <cstdint>
#include <algorithm>  // std::clamp(), std::max()

namespace mtrk_event_t_internal {

unsigned char *small_t::begin() {
	return &(this->d_[0]);
}
const unsigned char *small_t::begin() const {
	return &(this->d_[0]);
}
unsigned char *small_t::end() {
	return &(this->d_[0]) + this->size();
}
const unsigned char *small_t::end() const {
	return &(this->d_[0]) + this->size();
}
int32_t small_t::size() const {
	return ((this->flags_)&0x7Fu);
}
constexpr int32_t small_t::capacity() const {
	return static_cast<int32_t>(small_t::size_max);
}
int32_t small_t::resize(int32_t sz) {
	sz = std::clamp(sz,0,this->capacity());
	return (this->flags_ |= sz);
}
//-----------------------------------------------------------------------------
int32_t big_t::size() const {
	return static_cast<int32_t>(this->sz_);
}
int32_t big_t::capacity() const {
	return static_cast<int32_t>(this->cap_);
}
int32_t big_t::resize(int32_t new_sz) {
	// Keeps the object 'big', even if the new size would allow it to
	// fit in a small_t.  big_t/small_t switching logic lives in sbo_t,
	// not here.  
	new_sz = std::clamp(new_sz,0,big_t::size_max);
	if (new_sz <= this->capacity()) {
		this->sz_ = static_cast<uint32_t>(new_sz);
	} else {  // resize to new_size > present capacity
		auto new_cap = new_sz;
		auto psrc = this->p_;

		unsigned char *pdest_beg = new unsigned char[static_cast<uint32_t>(new_cap)];
		auto pdest_data_end = std::copy(psrc,psrc+this->size(),pdest_beg);
		this->free_and_adpot_new(pdest_beg,new_sz,new_cap);
	}
	return new_sz;
}
int32_t big_t::reserve(int32_t new_cap) {
	new_cap = std::clamp(new_cap,this->capacity(),big_t::size_max);
	if (new_cap > this->capacity()) {
		auto psrc = this->p_;

		unsigned char *pdest_beg = new unsigned char[static_cast<uint32_t>(new_cap)];
		auto pdest_data_end = std::copy(psrc,psrc+this->size(),pdest_beg);
		this->free_and_adpot_new(pdest_beg,this->size(),new_cap);
	}
	return new_cap;
}
void big_t::free_and_adpot_new(unsigned char *p, int32_t sz, int32_t cap) {
	if (this->p_) {
		delete [] this->p_;
	}
	this->p_ = p;
	this->sz_ = static_cast<uint32_t>(sz);
	this->cap_ = static_cast<uint32_t>(cap);
}
unsigned char *big_t::begin() {
	return this->p_;
}
const unsigned char *big_t::begin() const {
	return this->p_;
}
unsigned char *big_t::end() {
	return this->p_ + this->cap_;
}
const unsigned char *big_t::end() const {
	return this->p_ + this->cap_;
}
//-----------------------------------------------------------------------------
int32_t sbo_t::size() const {
	if (this->is_small()) {
		return this->u_.s_.size();
	} else {
		return this->u_.b_.size();
	}
}
int32_t sbo_t::capacity() const {
	if (this->is_small()) {
		return this->u_.s_.capacity();
	} else {
		return this->u_.b_.capacity();
	}
}
int32_t sbo_t::resize(int32_t new_sz) {
	new_sz = std::clamp(new_sz,0,sbo_t::size_max);
	if (this->is_big()) {
		if (new_sz <= small_t::size_max) {  // Resize big->small
			// Note that even though the present object is big and the new
			// size is <= small_t::size_max, it is still possible that the
			// size of the present object is < new_sz.  
			auto n_bytes2copy = std::min(new_sz,this->u_.b_.size());
			auto big_ptr = this->u_.b_.p_;
			this->init_small_unsafe();
			this->u_.s_.resize(new_sz);
			std::copy(big_ptr,big_ptr+n_bytes2copy,this->u_.s_.begin());
			delete [] big_ptr;
		} else {  // new_sz > small_t::size_max;  keep object big
			return this->u_.b_.resize(new_sz);
		}
	} else {  // present object is small
		if (new_sz <= small_t::size_max) {  // Keep small
			return this->u_.s_.resize(new_sz);
		} else {  // new_sz > small_t::size_max; Resize small->big
			auto psrc_beg = this->u_.s_.begin();
			auto psrc_end = this->u_.s_.end();

			auto new_cap = new_sz;
			unsigned char *pdest_beg = new unsigned char[static_cast<uint32_t>(new_cap)];
			auto pdest_data_end = std::copy(psrc_beg,psrc_end,pdest_beg);
			
			this->init_big_unsafe();
			this->u_.b_.free_and_adpot_new(pdest_beg,new_sz,new_cap);
		}
	}
	return new_sz;
}
int32_t sbo_t::reserve(int32_t new_cap) {
	new_cap = std::clamp(new_cap,this->capacity(),sbo_t::size_max);
	if (this->is_big()) {
		new_cap = this->u_.b_.reserve(new_cap);
	} else {  // present object is small
		if (new_cap > small_t::size_max) {  // Resize small->big
			auto psrc_beg = this->u_.s_.begin();
			auto psrc_end = this->u_.s_.end();

			auto new_sz = this->u_.s_.size();
			unsigned char *pdest_beg = new unsigned char[static_cast<uint32_t>(new_cap)];
			auto pdest_data_end = std::copy(psrc_beg,psrc_end,pdest_beg);
			
			this->init_big_unsafe();
			this->u_.b_.free_and_adpot_new(pdest_beg,new_sz,new_cap);
		}  // else (new_cap <= small_t::size_max); do nothing
	}
	return new_cap;
}
unsigned char *sbo_t::begin() {
	if (this->is_small()) {
		return this->u_.s_.begin();
	} else {
		return this->u_.b_.begin();
	}
}
const unsigned char *sbo_t::begin() const {
	if (this->is_small()) {
		return this->u_.s_.begin();
	} else {
		return this->u_.b_.begin();
	}
}
unsigned char *sbo_t::end() {
	if (this->is_small()) {
		return this->u_.s_.end();
	} else {
		return this->u_.b_.end();
	}
}
const unsigned char *sbo_t::end() const {
	if (this->is_small()) {
		return this->u_.s_.end();
	} else {
		return this->u_.b_.end();
	}
}
unsigned char *sbo_t::raw_begin() {
	return &(this->u_.raw_[0]);
}
const unsigned char *sbo_t::raw_begin() const {
	return &(this->u_.raw_[0]);
}
unsigned char *sbo_t::raw_end() {
	return &(this->u_.raw_[0]) + this->u_.raw_.size();
}
const unsigned char *sbo_t::raw_end() const {
	return &(this->u_.raw_[0]) + this->u_.raw_.size();
}
bool sbo_t::is_big() const {  // private
	return !(this->is_small());
}
bool sbo_t::is_small() const {  // private
	return ((this->u_.s_.flags_)&0x80u)==0x80u;
}
void sbo_t::clear_flags_and_set_big() {  // private
	this->u_.s_.flags_ = 0x00u;  // Note i always go through s_
}
void sbo_t::clear_flags_and_set_small() {  // private
	this->u_.s_.flags_ = 0x00u;
	this->u_.s_.flags_ |= 0x80u;  // Note i always go through s_
}
void sbo_t::init_big_unsafe() {  // private
	for (auto it=this->u_.raw_.begin(); it!=this->u_.raw_.end(); ++it) {
		*it=0x00u;
	}
	this->clear_flags_and_set_big();
	this->u_.b_.p_=nullptr;
	this->u_.b_.sz_=0;
	this->u_.b_.cap_=0;
}
void sbo_t::init_small_unsafe() {  // private
	for (auto it=this->u_.raw_.begin(); it!=this->u_.raw_.end(); ++it) {
		*it=0x00u;
	}
	this->clear_flags_and_set_small();
}




};  // namespace mtrk_event_t_internal 





/*
uint32_t sbo_t::resize_old(uint32_t new_cap) {
	new_cap = std::max(static_cast<uint64_t>(new_cap),this->small_capacity());
	if (new_cap == this->small_capacity()) {
		// Resize the object to fit in a small_t.  If it's presently small,
		// do nothing.  
		if (this->is_big()) {
			auto p = this->u_.b_.p_;
			// Note that even though the present object is 'big', it is
			// still possible that the size of its allocated buffer is
			// _smaller_ than the capacity of a 'small' object.  
			auto src_copy_nbytes = std::min(this->u_.b_.cap_,new_cap);
			this->zero_local_object();
			this->set_flag_small();
			std::copy(p,p+src_copy_nbytes,this->u_.s_.begin());
			// No need to std::fill(...,0x00u); already called zero_object()
			delete p;
		}
	} else if (new_cap > this->small_capacity()) {
		// After resizing, the object will be held in a big_t
		if (this->is_big()) {  // presently big
			if (new_cap != this->u_.b_.cap_) {  // ... and are changing the cap
				// Note that new_cap may be > or < the present cap
				auto p = this->u_.b_.p_;
				auto src_copy_nbytes = std::min(this->u_.b_.cap_,new_cap);

				// Note that if new_cap is < sz, the data will be truncated
				// and the object will almost certainly be invalid.  
				unsigned char *new_p = new unsigned char[new_cap];
				auto new_end = std::copy(p,p+src_copy_nbytes,new_p);
				std::fill(new_end,new_p+new_cap,0x00u);
				delete p;
				this->big_adopt(new_p,0,new_cap);
				this->set_flag_big();
			}
			// if already big and new_cap==present cap;  do nothing
		} else {  // presently small 
			// Copy the present small object into a new big object.  The present
			// capacity of small_capacity() is < new_cap
			unsigned char *new_p = new unsigned char[new_cap];
			auto new_end = std::copy(this->u_.s_.begin(),this->u_.s_.end(),new_p);
			std::fill(new_end,new_p+new_cap,0x00u);
			this->zero_local_object();
			this->big_adopt(new_p,0,new_cap);
			this->set_flag_big();
		}
	}
	return new_cap;
}
*/




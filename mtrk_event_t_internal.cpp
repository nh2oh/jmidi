#include "mtrk_event_t_internal.h"
#include <cstdint>
#include <cstdlib>  // std::abort()
#include <algorithm>  // std::clamp(), std::max(), std::copy()

namespace mtrk_event_t_internal {

void small_t::init() noexcept {
	this->flags_ = 0x80u;
	this->d_.fill(0x00u);
}
int32_t small_t::size() const noexcept {
	this->abort_if_not_active();
	return ((this->flags_)&0x7Fu);
}
constexpr int32_t small_t::capacity() const noexcept {
	return small_t::size_max;
}
int32_t small_t::resize(int32_t sz) noexcept {
	this->abort_if_not_active();
	sz = std::clamp(sz,0,this->capacity());
	this->flags_ = (static_cast<unsigned char>(sz)|0x80u);
	return ((this->flags_)&0x7Fu);
}
void small_t::abort_if_not_active() const noexcept {
	if (!((this->flags_)&0x80u)) {
		std::abort();
	}
}
unsigned char *small_t::begin() noexcept {
	this->abort_if_not_active();
	return &(this->d_[0]);
}
const unsigned char *small_t::begin() const noexcept {
	this->abort_if_not_active();
	return &(this->d_[0]);
}
unsigned char *small_t::end() noexcept {
	this->abort_if_not_active();
	return &(this->d_[0]) + this->size();
}
const unsigned char *small_t::end() const noexcept {
	this->abort_if_not_active();
	return &(this->d_[0]) + this->size();
}
//-----------------------------------------------------------------------------
void big_t::init() noexcept {
	this->flags_=0x00u;
	this->pad_.fill(0x00u);
	this->p_ = nullptr;
	this->sz_ = 0u;
	this->cap_ = 0u;
}
void big_t::free_and_reinit() noexcept {
	this->abort_if_not_active();
	if (this->p_) {
		delete [] this->p_;
	}
	this->init();
}
int32_t big_t::size() const noexcept {
	this->abort_if_not_active();
	return static_cast<int32_t>(this->sz_);
}
int32_t big_t::capacity() const noexcept {
	this->abort_if_not_active();
	return static_cast<int32_t>(this->cap_);
}
void big_t::abort_if_not_active() const noexcept {
	if ((this->flags_)&0x80u) {
		std::abort();
	}
	if (this->p_ == nullptr) {
		if ((this->sz_!=0)||(this->cap_!=0)) {
			std::abort();
		}
	}
}
void big_t::adopt(const big_t::pad_t& pad, unsigned char *ptr, 
					int32_t sz, int32_t cap) noexcept {
	this->abort_if_not_active();

	this->pad_ = pad;
	if (this->p_) {
		delete [] this->p_;
	}
	this->p_ = ptr;
	this->sz_ = static_cast<uint32_t>(sz);
	this->cap_ = static_cast<uint32_t>(cap);
}
int32_t big_t::resize(int32_t new_sz) {
	this->abort_if_not_active();
	new_sz = std::clamp(new_sz,0,big_t::size_max);
	if (new_sz <= this->capacity()) {
		this->sz_ = static_cast<uint32_t>(new_sz);
	} else {  // resize to new_size > present capacity
		auto new_cap = new_sz;
		// For a freshly init()'d object, p_==nullptr, but sz_==cap_==0
		unsigned char *pdest = new unsigned char[static_cast<uint32_t>(new_cap)];
		std::copy(this->begin(),this->end(),pdest);
		this->adopt(this->pad_,pdest,new_sz,new_cap);  // Frees the current p_
	}
	return this->size();
}
int32_t big_t::resize_nocopy(int32_t new_sz) {
	this->abort_if_not_active();
	new_sz = std::clamp(new_sz,0,big_t::size_max);
	if (new_sz <= this->capacity()) {
		this->sz_ = static_cast<uint32_t>(new_sz);
	} else {  // resize to new_size > present capacity
		auto new_cap = new_sz;
		unsigned char *pdest = new unsigned char[static_cast<uint32_t>(new_cap)];
		this->adopt(this->pad_,pdest,new_sz,new_cap);  // Frees the current p_
	}
	return this->size();
}
int32_t big_t::reserve(int32_t new_cap) {
	this->abort_if_not_active();
	new_cap = std::clamp(new_cap,this->capacity(),big_t::size_max);
	if (new_cap > this->capacity()) {
		// For a freshly init()'d object, p_==nullptr, but sz_==cap_==0
		auto psrc = this->p_;
		unsigned char *pdest = new unsigned char[static_cast<uint32_t>(new_cap)];
		std::copy(this->begin(),this->end(),pdest);
		this->adopt(this->pad_,pdest,this->size(),new_cap);
	}
	return this->capacity();
}
unsigned char *big_t::begin() noexcept {
	this->abort_if_not_active();
	return this->p_;
}
const unsigned char *big_t::begin() const noexcept {
	this->abort_if_not_active();
	return this->p_;
}
unsigned char *big_t::end() noexcept {
	this->abort_if_not_active();
	return this->p_ + this->sz_;
}
const unsigned char *big_t::end() const noexcept {
	this->abort_if_not_active();
	return this->p_ + this->sz_;
}
//-----------------------------------------------------------------------------
small_bytevec_t::small_bytevec_t() noexcept {
	this->init_small();
}
void small_bytevec_t::init_small() noexcept {
	this->u_.s_.init();
}
void small_bytevec_t::init_big() noexcept {
	this->u_.b_.init();
}
small_bytevec_t::small_bytevec_t(const small_bytevec_t& rhs) {  // Copy ctor
	// This is a ctor; *this is in an uninitialized state
	if (rhs.is_big()) {
		// I *probably* need a big object, but rhs may be a small amount of
		// data in a big object.  
		this->init_big();
		this->resize_nocopy(rhs.u_.b_.size());
		// Note that calling this->resize_nocopy() is different from calling 
		// this->u_.b_.resize_nocopy(); the former will cause a big->small 
		// transition if possible.  Thus, although i began w/ a 
		// this->init_big(), after resize_nocopy()ing, this may be small, 
		// hence copying into this->begin() rather than this->u_.b_.begin().  
		std::copy(rhs.u_.b_.begin(),rhs.u_.b_.end(),this->begin());
	} else {
		this->init_small();
		this->resize(rhs.u_.s_.size());
		std::copy(rhs.u_.s_.begin(),rhs.u_.s_.end(),this->u_.s_.begin());
	}
}
small_bytevec_t& small_bytevec_t::operator=(const small_bytevec_t& rhs) {  // Copy assign
	this->resize_nocopy(rhs.size());
	std::copy(rhs.begin(),rhs.end(),this->begin());
	return *this;

	// TODO:  Can be optimized; resize() copies the old data when new[]'ing
	// a new buffer.  
	//this->resize(rhs.size());
	//std::copy(rhs.begin(),rhs.end(),this->begin());
	//return *this;
}
small_bytevec_t::small_bytevec_t(small_bytevec_t&& rhs) noexcept {  // Move ctor
	// This is a ctor; *this is in an uninitialized state
	if (rhs.is_big()) {
		this->init_big();
		this->u_.b_.adopt(rhs.u_.b_.pad_,rhs.u_.b_.begin(),rhs.u_.b_.size(),
							rhs.u_.b_.capacity());
		rhs.init_small();  // Does not delete [] rhs.u_.b_.p_
		// TODO:  Not really needed; need to null rhs->u_.b_.p_, but not zero 
		// it all out
	} else {  // rhs is 'small'
		this->init_small();
		this->resize(rhs.u_.s_.size());
		std::copy(rhs.u_.s_.begin(),rhs.u_.s_.end(),this->u_.s_.begin());
		rhs.init_small();  // TODO:  Not really needed
	}
}
small_bytevec_t& small_bytevec_t::operator=(small_bytevec_t&& rhs) noexcept {  // Move assign
	if (rhs.is_big()) {
		if (this->is_small()) {
			this->init_big();
		}
		// If this->is_big(), this->u_.b_.adopt() frees u_.b_.p_.  
		this->u_.b_.adopt(rhs.u_.b_.pad_,rhs.u_.b_.begin(),rhs.u_.b_.size(),
							rhs.u_.b_.capacity());
		rhs.init_small();  // Does not delete [] rhs.u_.b_.p_
		// TODO:  Not really needed; need to null rhs->u_.b_.p_, but not zero 
		// it all out
	} else {  // rhs is 'small'
		if (this->is_big()) {
			this->u_.b_.free_and_reinit();
		}
		this->init_small();
		this->u_.s_.resize(rhs.u_.s_.size());
		std::copy(rhs.u_.s_.begin(),rhs.u_.s_.end(),this->u_.s_.begin());
		rhs.init_small();  // TODO:  Not really needed
	}
	return *this;
}

small_bytevec_t::~small_bytevec_t() noexcept {  // Dtor
	if (this->is_small()) {
		this->init_small();
	} else {
		this->u_.b_.free_and_reinit();
	}
}
int32_t small_bytevec_t::size() const noexcept {
	if (this->is_small()) {
		return this->u_.s_.size();
	} else {
		return this->u_.b_.size();
	}
}
int32_t small_bytevec_t::capacity() const noexcept {
	if (this->is_small()) {
		return this->u_.s_.capacity();
	} else {
		return this->u_.b_.capacity();
	}
}
int32_t small_bytevec_t::resize(int32_t new_sz) {
	new_sz = std::clamp(new_sz,0,small_bytevec_t::size_max);
	if (this->is_big()) {
		if (new_sz <= small_t::size_max) {  // Resize big->small
			// Note that even though the present object is big and the new
			// size is <= small_t::size_max, it is still possible that the
			// size of the present object is < new_sz.  
			// TODO:  
			// small_bytevec_t yay=small_bytevec_t();
			// std::copy(this->begin(),this->begin()+n_bytes2copy,yay.begin());
			// *this=yay;
			// (ie, reuse the copy ctor); won't have this messy delete []
			// ??? 

			// If !psrc, expect n_bytes2copy == 0
			auto n_bytes2copy = std::min(new_sz,this->size());
			auto psrc = this->begin();
			this->init_small();  // Does not delete [] b_.p_
			this->u_.s_.resize(new_sz);
			std::copy(psrc,psrc+n_bytes2copy,this->u_.s_.begin());
			if (psrc) {
				delete [] psrc;
			}
		} else {  // new_sz > small_t::size_max;  keep object big
			return this->u_.b_.resize(new_sz);
		}
	} else {  // present object is small
		if (new_sz <= small_t::size_max) {  // Keep small
			return this->u_.s_.resize(new_sz);
		} else {  // new_sz > small_t::size_max; Resize small->big
			auto new_cap = new_sz;
			unsigned char *pdest = new unsigned char[static_cast<uint32_t>(new_cap)];
			std::copy(this->begin(),this->end(),pdest);
			this->init_big();
			this->u_.b_.adopt(this->u_.b_.pad_,pdest,new_sz,new_cap);
		}
	}
	return new_sz;
}
int32_t small_bytevec_t::resize_nocopy(int32_t new_sz) {
	new_sz = std::clamp(new_sz,0,small_bytevec_t::size_max);
	if (this->is_big()) {
		if (new_sz <= small_t::size_max) {  // Resize big->small
			if (this->u_.b_.p_) {
				delete [] this->u_.b_.p_;
			}
			this->init_small();  // Does not try to delete [] b_.p_
			return this->u_.s_.resize(new_sz);
		} else {  // new_sz > small_t::size_max;  keep object big
			return this->u_.b_.resize_nocopy(new_sz);
		}
	} else {  // present object is small
		if (new_sz <= small_t::size_max) {  // Keep small
			return this->u_.s_.resize(new_sz);
		} else {  // new_sz > small_t::size_max; Resize small->big
			auto new_cap = new_sz;
			unsigned char *p = new unsigned char[static_cast<uint32_t>(new_cap)];
			this->init_big();
			this->u_.b_.adopt(this->u_.b_.pad_,p,new_sz,new_cap);
		}
	}
	return new_sz;
}
int32_t small_bytevec_t::reserve(int32_t new_cap) {
	new_cap = std::clamp(new_cap,this->capacity(),small_bytevec_t::size_max);
	if (this->is_big()) {
		new_cap = this->u_.b_.reserve(new_cap);
	} else {  // present object is small
		if (new_cap > small_t::size_max) {  // Resize small->big
			auto new_sz = this->size();
			unsigned char *pdest = new unsigned char[static_cast<uint32_t>(new_cap)];
			std::copy(this->begin(),this->end(),pdest);
			
			this->init_big();
			this->u_.b_.adopt(this->u_.b_.pad_,pdest,new_sz,new_cap);
		}  // else (new_cap <= small_t::size_max); do nothing
	}
	return this->capacity();
}
unsigned char *small_bytevec_t::push_back(unsigned char c) {
	auto sz = this->size();
	if (sz==this->capacity()) {
		// Note that a consequence of this test is that if this->is_small(),
		// reserve() will not be called until the 'small' object is 
		// completely full, so the object remains small for as long as 
		// possible.  

		// if this->size()==0, don't want to reserve 2*sz==0; in theory
		// one could have a capacity 0 'big' object.  
		auto new_sz = std::max(2*sz,small_bytevec_t::capacity_small);
		this->reserve(new_sz);
	}
	this->resize(sz+1);
	*(this->end()-1) = c;
	return this->end()-1;
}

bool small_bytevec_t::debug_is_big() const noexcept {
	return this->is_big();
}
unsigned char *small_bytevec_t::begin() noexcept {
	if (this->is_small()) {
		return this->u_.s_.begin();
	} else {
		return this->u_.b_.begin();
	}
}
const unsigned char *small_bytevec_t::begin() const noexcept {
	if (this->is_small()) {
		return this->u_.s_.begin();
	} else {
		return this->u_.b_.begin();
	}
}
unsigned char *small_bytevec_t::end() noexcept {
	if (this->is_small()) {
		return this->u_.s_.end();
	} else {
		return this->u_.b_.end();
	}
}
const unsigned char *small_bytevec_t::end() const noexcept {
	if (this->is_small()) {
		return this->u_.s_.end();
	} else {
		return this->u_.b_.end();
	}
}
unsigned char *small_bytevec_t::raw_begin() noexcept {
	return &(this->u_.raw_[0]);
}
const unsigned char *small_bytevec_t::raw_begin() const noexcept {
	return &(this->u_.raw_[0]);
}
unsigned char *small_bytevec_t::raw_end() noexcept {
	return &(this->u_.raw_[0]) + this->u_.raw_.size();
}
const unsigned char *small_bytevec_t::raw_end() const noexcept{
	return &(this->u_.raw_[0]) + this->u_.raw_.size();
}
bool small_bytevec_t::is_big() const noexcept {  // private
	return !(this->is_small());
}
bool small_bytevec_t::is_small() const noexcept {  // private
	return ((this->u_.s_.flags_)&0x80u)==0x80u;
}


};  // namespace mtrk_event_t_internal 


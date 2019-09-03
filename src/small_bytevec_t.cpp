#include "small_bytevec_t.h"
#include <cstdint>
#include <cstdlib>  // std::abort()
#include <algorithm>  // std::clamp(), std::max(), std::copy()
#include <cstring>
#include <utility>  // std::move()



void jmid::internal::small_t::init() noexcept {
	this->flags_ = 0x80u;
}
int32_t jmid::internal::small_t::size() const noexcept {
	this->abort_if_not_active();
	return ((this->flags_)&0x7Fu);
}
constexpr std::int32_t jmid::internal::small_t::capacity() const noexcept {
	return jmid::internal::small_t::size_max;
}
std::int32_t jmid::internal::small_t::resize(std::int32_t sz) noexcept {
	this->abort_if_not_active();
	sz = std::clamp(sz,0,this->capacity());
	this->flags_ = (static_cast<unsigned char>(sz)|0x80u);
	return this->size();
}
std::int32_t jmid::internal::small_t::resize_unchecked(std::int32_t sz) noexcept {
	this->abort_if_not_active();
	this->flags_ = (static_cast<unsigned char>(sz)|0x80u);
	return this->size();
}
void jmid::internal::small_t::abort_if_not_active() const noexcept {
	if (!((this->flags_)&0x80u)) {
		std::abort();
	}
}
unsigned char *jmid::internal::small_t::begin() noexcept {
	this->abort_if_not_active();
	return &(this->d_[0]);
}
const unsigned char *jmid::internal::small_t::begin() const noexcept {
	this->abort_if_not_active();
	return &(this->d_[0]);
}
unsigned char *jmid::internal::small_t::end() noexcept {
	this->abort_if_not_active();
	return &(this->d_[0]) + this->size();
}
const unsigned char *jmid::internal::small_t::end() const noexcept {
	this->abort_if_not_active();
	return &(this->d_[0]) + this->size();
}
//-----------------------------------------------------------------------------
void jmid::internal::big_t::init() noexcept {
	this->flags_=0x00u;
	this->pad_.fill(0x00u);  // TODO:  Necessary?
	this->p_ = nullptr;
	this->sz_ = 0u;
	this->cap_ = 0u;
}
void jmid::internal::big_t::free_and_reinit() noexcept {
	this->abort_if_not_active();
	if (this->p_) {
		delete [] this->p_;
	}
	this->init();
}
void jmid::internal::big_t::adopt(const big_t::pad_t& pad, 
							unsigned char *ptr,  std::int32_t sz, 
							std::int32_t cap) noexcept {
	this->abort_if_not_active();

	this->pad_ = pad;
	if (this->p_) {
		delete [] this->p_;
	}
	this->p_ = ptr;
	this->sz_ = static_cast<uint32_t>(sz);
	this->cap_ = static_cast<uint32_t>(cap);
}
std::int32_t jmid::internal::big_t::size() const noexcept {
	this->abort_if_not_active();
	return static_cast<std::int32_t>(this->sz_);
}
std::int32_t jmid::internal::big_t::capacity() const noexcept {
	this->abort_if_not_active();
	return static_cast<std::int32_t>(this->cap_);
}


void jmid::internal::big_t::clear() noexcept {
	this->abort_if_not_active();
	this->sz_ = 0;
}
std::int32_t jmid::internal::big_t::resize(std::int32_t new_sz) {
	this->abort_if_not_active();
	new_sz = std::clamp(new_sz,0,big_t::size_max);
	if (new_sz <= this->capacity()) {
		this->sz_ = static_cast<std::uint32_t>(new_sz);
	} else {  // resize to new_size > present capacity
		auto new_cap = new_sz;
		// For a freshly init()'d object, p_==nullptr, but sz_==cap_==0
		unsigned char *pdest = new unsigned char[static_cast<uint32_t>(new_cap)];
		std::copy(this->begin(),this->end(),pdest);
		this->adopt(this->pad_,pdest,new_sz,new_cap);  // Frees the current p_
	}
	return this->size();
}
std::int32_t jmid::internal::big_t::resize_nocopy(std::int32_t new_sz) {
	this->abort_if_not_active();
	this->clear();
	return this->resize(new_sz);
}
std::int32_t jmid::internal::big_t::reserve(std::int32_t new_cap) {
	this->abort_if_not_active();
	new_cap = std::clamp(new_cap,this->capacity(),big_t::size_max);
	if (new_cap > this->capacity()) {
		// For a freshly init()'d object, p_==nullptr, but sz_==cap_==0
		unsigned char *pdest = new unsigned char[static_cast<std::uint32_t>(new_cap)];
		std::copy(this->begin(),this->end(),pdest);
		this->adopt(this->pad_,pdest,this->size(),new_cap);
	}
	return this->capacity();
}

void jmid::internal::big_t::abort_if_not_active() const noexcept {
	if ((this->flags_)&0x80u) {
		std::abort();
	}
	if (this->p_ == nullptr) {
		if ((this->sz_!=0)||(this->cap_!=0)) {
			std::abort();
		}
	}
}
unsigned char *jmid::internal::big_t::begin() noexcept {
	this->abort_if_not_active();
	return this->p_;
}
const unsigned char *jmid::internal::big_t::begin() const noexcept {
	this->abort_if_not_active();
	return this->p_;
}
unsigned char *jmid::internal::big_t::end() noexcept {
	this->abort_if_not_active();
	return this->p_ + this->sz_;
}
const unsigned char *jmid::internal::big_t::end() const noexcept {
	this->abort_if_not_active();
	return this->p_ + this->sz_;
}
//-----------------------------------------------------------------------------
jmid::internal::small_bytevec_t::small_bytevec_t() noexcept {
	this->init_small();
}
jmid::internal::small_bytevec_t::small_bytevec_t(std::int32_t sz) {
	sz = std::clamp(sz,0,jmid::internal::small_bytevec_t::size_max);
	if (sz > jmid::internal::small_t::size_max) {
		this->init_small();
		this->u_.s_.resize(sz);
	} else {
		this->init_big();
		this->u_.b_.resize_nocopy(sz);
	}
}
jmid::internal::small_bytevec_t::small_bytevec_t(const jmid::internal::small_bytevec_t& rhs) {
	// This is a ctor; *this is in an uninitialized state
	auto rhs_sz = rhs.size();
	auto p_rhs_beg = rhs.begin();
	if (rhs_sz <= jmid::internal::small_t::size_max) {
		// Note that rhs may be big or small; all that is known is that its 
		// data fits in the small object.  
		this->init_small();
		this->u_.s_.resize_unchecked(rhs_sz);
		std::copy(p_rhs_beg,p_rhs_beg+rhs_sz,this->u_.s_.begin());
	} else {
		this->init_big();
		this->u_.b_.resize_nocopy(rhs_sz);
		std::copy(p_rhs_beg,p_rhs_beg+rhs_sz,this->u_.b_.p_);
		this->u_.b_.pad_ = rhs.u_.b_.pad_;
	}
}
jmid::internal::small_bytevec_t& jmid::internal::small_bytevec_t::operator=(const jmid::internal::small_bytevec_t& rhs) {
	auto rhs_sz = rhs.size();
	auto p = this->resize_nocopy(rhs_sz);
	std::copy(rhs.begin(),rhs.begin()+rhs_sz,p);
	if (this->is_big() && rhs.is_big()) {
		this->u_.b_.pad_ = rhs.u_.b_.pad_;
	}
	return *this;
}
jmid::internal::small_bytevec_t::small_bytevec_t(jmid::internal::small_bytevec_t&& rhs) noexcept {  // Move ctor
	// This is a ctor; *this is in an uninitialized state
	// In principle i could replace w/
	// std::memcpy(&(this->u_.raw_[0]),&(rhs.u_.raw_[0]),sizeof(small_t));
	// however, i want to formally set the active member of the union.  
	if (rhs.is_big()) {
		this->init_big();
		this->u_.b_.adopt(rhs.u_.b_.pad_,rhs.u_.b_.p_,rhs.u_.b_.sz_,
							rhs.u_.b_.cap_);
	} else {  // rhs is 'small'
		this->init_small();
		this->u_.s_.resize(rhs.u_.s_.size());
		std::copy(rhs.u_.s_.begin(),rhs.u_.s_.end(),this->u_.s_.begin());
	}
	// Ensures rhs.~small_bytevec_t() will not delete [] the ptr.  Also has 
	// the effect of setting rhs.size()==0.  
	rhs.init_small();  
}
jmid::internal::small_bytevec_t& jmid::internal::small_bytevec_t::operator=(jmid::internal::small_bytevec_t&& rhs) noexcept {  // Move assign
	if (this->is_big()) {
		delete [] this->u_.b_.p_;
	}
	// In principle i could replace what is below w/
	// std::memcpy(&(this->u_.raw_[0]),&(rhs.u_.raw_[0]),sizeof(small_t));
	// however, i want to formally set the active member of the union.  
	if (rhs.is_big()) {
		this->init_big();
		this->u_.b_.adopt(rhs.u_.b_.pad_,rhs.u_.b_.p_,rhs.u_.b_.sz_,
							rhs.u_.b_.cap_);
	} else {  // rhs is 'small'
		this->init_small();
		this->u_.s_.resize(rhs.u_.s_.size());
		std::copy(rhs.u_.s_.begin(),rhs.u_.s_.end(),this->u_.s_.begin());
	}
	// Ensures rhs.~small_bytevec_t() will not delete [] the ptr.  Also has 
	// the effect of setting rhs.size()==0.  
	rhs.init_small();  
	return *this;
}
jmid::internal::small_bytevec_t::~small_bytevec_t() noexcept {  // Dtor
	if (this->is_big()) {
		delete [] this->u_.b_.p_;
	}
	this->init_small();
}


std::int32_t jmid::internal::small_bytevec_t::reserve(std::int32_t new_cap) {
	new_cap = std::clamp(new_cap,this->capacity(),jmid::internal::small_bytevec_t::size_max);
	if (this->is_big()) {
		new_cap = this->u_.b_.reserve(new_cap);
	} else {  // present object is small
		if (new_cap > small_t::size_max) {  // Resize small->big
			auto sz = this->u_.s_.size();
			unsigned char *pdest = new unsigned char[static_cast<uint32_t>(new_cap)];
			std::copy(this->u_.s_.begin(),this->u_.s_.end(),pdest);
			this->init_big();
			this->u_.b_.adopt(this->u_.b_.pad_,pdest,sz,new_cap);
		}  // else (new_cap <= small_t::size_max); do nothing
	}
	return this->capacity();
}
unsigned char *jmid::internal::small_bytevec_t::resize(std::uint64_t new_sz) {
	auto sz32 = static_cast<std::int32_t>(std::clamp(new_sz,std::uint64_t(0),
		static_cast<std::uint64_t>(jmid::internal::small_bytevec_t::size_max)));
	return this->resize(sz32);
}
unsigned char *jmid::internal::small_bytevec_t::resize(std::int64_t new_sz) {
	auto sz32 = static_cast<std::int32_t>(std::clamp(new_sz,std::int64_t(0),
		static_cast<std::int64_t>(jmid::internal::small_bytevec_t::size_max)));
	return this->resize(sz32);
}
unsigned char *jmid::internal::small_bytevec_t::resize(std::int32_t new_sz) {
	new_sz = std::clamp(new_sz,0,jmid::internal::small_bytevec_t::size_max);
	if (this->is_big()) {
		// Since this method does not cause unnecessary big <-> small 
		// transitions, an object that is big never has to worry about 
		// undergoing a transition, since a big object can accomodate 
		// any new_sz.  
		this->u_.b_.resize(new_sz);
		return this->u_.b_.begin();
	} else {  // this->is_small()
		if (new_sz > jmid::internal::small_t::size_max) {
			auto new_cap = new_sz;
			unsigned char *pdest = new unsigned char[static_cast<uint32_t>(new_cap)];
			std::copy(this->u_.s_.begin(),this->u_.s_.end(),pdest);
			this->init_big();
			this->u_.b_.adopt(this->u_.b_.pad_,pdest,new_sz,new_cap);
			return this->u_.b_.begin();
		} else {
			this->u_.s_.resize(new_sz);
			return this->u_.s_.begin();
		}
	}
}
unsigned char *jmid::internal::small_bytevec_t::resize_nocopy(std::int32_t new_sz) {
	new_sz = std::clamp(new_sz,0,jmid::internal::small_bytevec_t::size_max);
	if (this->is_big()) {
		// Since this method does not cause unnecessary big <-> small 
		// transitions, an object that is big never has to worry about 
		// undergoing a transition, since a big object can accomodate 
		// any new_sz.  
		this->u_.b_.resize_nocopy(new_sz);
		return this->u_.b_.begin();
	} else {  // this->is_small()
		if (new_sz > jmid::internal::small_t::size_max) {
			this->init_big();
			this->u_.b_.resize_nocopy(new_sz);
			return this->u_.b_.begin();
		} else {
			this->u_.s_.resize(new_sz);
			return this->u_.s_.begin();
		}
	}
}
unsigned char *jmid::internal::small_bytevec_t::push_back(unsigned char c) {
	auto sz = this->size();
	if (sz==this->capacity()) {
		// if this->size()==0, don't want to reserve 2*sz==0; in theory
		// one could have a capacity 0 'big' object.  
		auto new_sz = std::max(2*sz,
			jmid::internal::small_bytevec_t::capacity_small);
		this->reserve(new_sz);
	}
	
	this->resize(sz+1);
	auto pdest = this->end()-1;
	*pdest = c;
	return pdest;
}


std::int32_t jmid::internal::small_bytevec_t::size() const noexcept {
	if (this->is_small()) {
		return this->u_.s_.size();
	} else {
		return this->u_.b_.size();
	}
}
std::int32_t jmid::internal::small_bytevec_t::capacity() const noexcept {
	if (this->is_small()) {
		return this->u_.s_.capacity();
	} else {
		return this->u_.b_.capacity();
	}
}


bool jmid::internal::small_bytevec_t::debug_is_big() const noexcept {
	return this->is_big();
}
jmid::internal::small_bytevec_range_t jmid::internal::small_bytevec_t::data_range() noexcept {
	if (this->is_small()) {
		return {this->u_.s_.begin(),this->u_.s_.end()};
	} else {
		return {this->u_.b_.begin(),this->u_.b_.end()};
	}
}
jmid::internal::small_bytevec_const_range_t jmid::internal::small_bytevec_t::data_range() const noexcept {
	if (this->is_small()) {
		return {this->u_.s_.begin(),this->u_.s_.end()};
	} else {
		return {this->u_.b_.begin(),this->u_.b_.end()};
	}
}
jmid::internal::small_bytevec_range_t jmid::internal::small_bytevec_t::pad_or_data_range() noexcept {
	if (this->is_small()) {
		return {this->u_.s_.begin(),this->u_.s_.end()};
	} else {
		return {this->u_.b_.pad_.data(),
			this->u_.b_.pad_.data()+this->u_.b_.pad_.size()};
	}
}
jmid::internal::small_bytevec_const_range_t jmid::internal::small_bytevec_t::pad_or_data_range() const noexcept {
	if (this->is_small()) {
		return {this->u_.s_.begin(),this->u_.s_.end()};
	} else {
		return {this->u_.b_.pad_.data(),
			this->u_.b_.pad_.data()+this->u_.b_.pad_.size()};
	}
}
unsigned char *jmid::internal::small_bytevec_t::begin() noexcept {
	if (this->is_small()) {
		return this->u_.s_.begin();
	} else {
		return this->u_.b_.begin();
	}
}
const unsigned char *jmid::internal::small_bytevec_t::begin() const noexcept {
	if (this->is_small()) {
		return this->u_.s_.begin();
	} else {
		return this->u_.b_.begin();
	}
}
unsigned char *jmid::internal::small_bytevec_t::end() noexcept {
	if (this->is_small()) {
		return this->u_.s_.end();
	} else {
		return this->u_.b_.end();
	}
}
const unsigned char *jmid::internal::small_bytevec_t::end() const noexcept {
	if (this->is_small()) {
		return this->u_.s_.end();
	} else {
		return this->u_.b_.end();
	}
}
unsigned char *jmid::internal::small_bytevec_t::raw_begin() noexcept {
	return &(this->u_.raw_[0]);
}
const unsigned char *jmid::internal::small_bytevec_t::raw_begin() const noexcept {
	return &(this->u_.raw_[0]);
}
unsigned char *jmid::internal::small_bytevec_t::raw_end() noexcept {
	return &(this->u_.raw_[0]) + this->u_.raw_.size();
}
const unsigned char *jmid::internal::small_bytevec_t::raw_end() const noexcept{
	return &(this->u_.raw_[0]) + this->u_.raw_.size();
}
bool jmid::internal::small_bytevec_t::is_big() const noexcept {  // private
	return !(this->is_small());
}
bool jmid::internal::small_bytevec_t::is_small() const noexcept {  // private
	return ((this->u_.s_.flags_)&0x80u)==0x80u;
}

void jmid::internal::small_bytevec_t::init_small() noexcept {  // Private
	this->u_.s_.init();
}
void jmid::internal::small_bytevec_t::init_big() noexcept {  // Private
	this->u_.b_.init();
}

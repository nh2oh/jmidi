#include "small_bytevec_t.h"
#include <cstdint>
#include <cstdlib>  // std::abort()
#include <algorithm>  // std::clamp(), std::max(), std::copy()
#include <cstring>
#include <utility>  // std::move()



void jmid::internal::small_t::init() noexcept {
	this->flags_ = 0x80u;
	//this->d_.fill(0x00u);
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
	return ((this->flags_)&0x7Fu);
}
/*int32_t jmid::internal::small_t::resize(small_size_t sz) noexcept {
	this->abort_if_not_active();
	this->flags_ = (static_cast<unsigned char>(sz)|0x80u);
	return ((this->flags_)&0x7Fu);
}*/
std::int32_t jmid::internal::small_t::resize_unchecked(std::int32_t sz) noexcept {
	this->abort_if_not_active();
	this->flags_ = (static_cast<unsigned char>(sz)|0x80u);
	return ((this->flags_)&0x7Fu);
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
	this->pad_.fill(0x00u);
	this->p_ = nullptr;
	this->sz_ = 0u;
	this->cap_ = 0u;
}
void jmid::internal::big_t::free_and_reinit() noexcept {
	this->abort_if_not_active();
	if (this->p_) {
		delete [] this->p_;
		//small_bytevec_t::call_counts.calls_delete++;
	}
	this->init();
}
std::int32_t jmid::internal::big_t::size() const noexcept {
	this->abort_if_not_active();
	return static_cast<std::int32_t>(this->sz_);
}
std::int32_t jmid::internal::big_t::capacity() const noexcept {
	this->abort_if_not_active();
	return static_cast<std::int32_t>(this->cap_);
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
void jmid::internal::big_t::adopt(const big_t::pad_t& pad, unsigned char *ptr, 
					std::int32_t sz, std::int32_t cap) noexcept {
	this->abort_if_not_active();

	this->pad_ = pad;
	if (this->p_) {
		delete [] this->p_;
		//small_bytevec_t::call_counts.calls_delete++;
	}
	this->p_ = ptr;
	this->sz_ = static_cast<uint32_t>(sz);
	this->cap_ = static_cast<uint32_t>(cap);
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
		//small_bytevec_t::call_counts.calls_new++;
		std::copy(this->begin(),this->end(),pdest);
		this->adopt(this->pad_,pdest,new_sz,new_cap);  // Frees the current p_
	}
	return this->size();
}
std::int32_t jmid::internal::big_t::resize_nocopy(std::int32_t new_sz) {
	this->abort_if_not_active();
	new_sz = std::clamp(new_sz,0,big_t::size_max);
	if (new_sz <= this->capacity()) {
		this->sz_ = static_cast<std::uint32_t>(new_sz);
	} else {  // resize to new_size > present capacity
		auto new_cap = new_sz;
		unsigned char *pdest = new unsigned char[static_cast<std::uint32_t>(new_cap)];
		//small_bytevec_t::call_counts.calls_new++;
		this->adopt(this->pad_,pdest,new_sz,new_cap);  // Frees the current p_
	}
	return this->size();
}
std::int32_t jmid::internal::big_t::reserve(std::int32_t new_cap) {
	this->abort_if_not_active();
	new_cap = std::clamp(new_cap,this->capacity(),big_t::size_max);
	if (new_cap > this->capacity()) {
		// For a freshly init()'d object, p_==nullptr, but sz_==cap_==0
		//auto psrc = this->p_;
		unsigned char *pdest = new unsigned char[static_cast<std::uint32_t>(new_cap)];
		//small_bytevec_t::call_counts.calls_new++;
		std::copy(this->begin(),this->end(),pdest);
		this->adopt(this->pad_,pdest,this->size(),new_cap);
	}
	return this->capacity();
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
void jmid::internal::small_bytevec_t::init_small() noexcept {
	this->u_.s_.init();
}
void jmid::internal::small_bytevec_t::init_big() noexcept {
	this->u_.b_.init();
}
jmid::internal::small_bytevec_t::small_bytevec_t(const jmid::internal::small_bytevec_t& rhs) {  // Copy ctor
	// This is a ctor; *this is in an uninitialized state
	if (rhs.is_big()) {
		// I *probably* need a big object, but rhs may be a small amount of
		// data in a big object.  
		this->init_big();  // TODO:  Zeros this->u_.b_.pad_, which is not needed
		this->resize_nocopy(rhs.u_.b_.size());
		// Note that calling this->resize_nocopy() is different from calling 
		// this->u_.b_.resize_nocopy(); the former will cause a big->small 
		// transition if possible.  Thus, although i began w/ a 
		// this->init_big(), after resize_nocopy()ing, this may be small, 
		// hence copying into this->begin() rather than this->u_.b_.begin().  
		std::copy(rhs.u_.b_.begin(),rhs.u_.b_.end(),this->begin());
		if (this->is_big()) {
			this->u_.b_.pad_ = rhs.u_.b_.pad_;
		}
	} else {
		this->init_small();
		this->u_.s_.resize(rhs.u_.s_.size());
		std::copy(rhs.u_.s_.begin(),rhs.u_.s_.end(),this->u_.s_.begin());
	}
}
jmid::internal::small_bytevec_t& jmid::internal::small_bytevec_t::operator=(const jmid::internal::small_bytevec_t& rhs) {  // Copy assign
	this->resize_nocopy(rhs.size());
	std::copy(rhs.begin(),rhs.end(),this->begin());
	if (this->is_big()) {
		this->u_.b_.pad_ = rhs.u_.b_.pad_;
	}
	return *this;
}
jmid::internal::small_bytevec_t::small_bytevec_t(jmid::internal::small_bytevec_t&& rhs) noexcept {  // Move ctor
	// This is a ctor; *this is in an uninitialized state
	//std::memcpy(&(this->u_.raw_[0]),&(rhs.u_.raw_[0]),sizeof(small_t));
	//rhs.init_small();
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
jmid::internal::small_bytevec_t& jmid::internal::small_bytevec_t::operator=(jmid::internal::small_bytevec_t&& rhs) noexcept {  // Move assign
	//if (this->is_big()) {
	//	delete [] this->u_.b_.p_;
	//}
	//std::memcpy(&(this->u_.raw_[0]),&(rhs.u_.raw_[0]),sizeof(small_t));
	//rhs.init_small();
	//return *this;
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

jmid::internal::small_bytevec_t::~small_bytevec_t() noexcept {  // Dtor
	if (this->is_small()) {
		this->init_small();
	} else {
		this->u_.b_.free_and_reinit();
	}
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
unsigned char *jmid::internal::small_bytevec_t::resize(std::int32_t new_sz) {
	new_sz = std::clamp(new_sz,0,jmid::internal::small_bytevec_t::size_max);
	if (this->is_big()) {
		if (new_sz <= small_t::size_max) {  // Resize big->small
			// Note that even though the present object is big and the new
			// size is <= small_t::size_max, it is still possible that the
			// size of the present object is < new_sz.  
			// If !psrc, expect n_bytes2copy == 0
			auto n_bytes2copy = std::min(new_sz,this->u_.b_.size());
			auto psrc = this->u_.b_.begin();
			this->init_small();  // Does not delete [] b_.p_
			this->u_.s_.resize(new_sz);
			std::copy(psrc,psrc+n_bytes2copy,this->u_.s_.begin());
			if (psrc) {
				delete [] psrc;
				//small_bytevec_t::call_counts.calls_delete++;
			}
			return this->u_.s_.begin();
		} else {  // new_sz > small_t::size_max;  keep object big
			this->u_.b_.resize(new_sz);
			return this->u_.b_.begin();
		}
	} else {  // present object is small
		if (new_sz <= small_t::size_max) {  // Keep small
			this->u_.s_.resize(new_sz);
			return this->u_.s_.begin();
		} else {  // new_sz > small_t::size_max; Resize small->big
			auto new_cap = new_sz;
			unsigned char *pdest = new unsigned char[static_cast<uint32_t>(new_cap)];
			//small_bytevec_t::call_counts.calls_new++;
			std::copy(this->u_.s_.begin(),this->u_.s_.end(),pdest);
			this->init_big();
			this->u_.b_.adopt(this->u_.b_.pad_,pdest,new_sz,new_cap);
			return this->u_.b_.begin();
		}
	}
}
unsigned char *jmid::internal::small_bytevec_t::resize_nocopy(std::int32_t new_sz) {
	new_sz = std::clamp(new_sz,0,jmid::internal::small_bytevec_t::size_max);
	if (this->is_big()) {
		if (new_sz <= small_t::size_max) {  // Resize big->small
			if (this->u_.b_.p_) {
				delete [] this->u_.b_.p_;
				//small_bytevec_t::call_counts.calls_delete++;
			}
			this->init_small();  // Does not try to delete [] b_.p_
			this->u_.s_.resize(new_sz);
			return this->u_.s_.begin();
		} else {  // new_sz > small_t::size_max;  keep object big
			this->u_.b_.resize_nocopy(new_sz);
			return this->u_.b_.begin();
		}
	} else {  // present object is small
		if (new_sz <= small_t::size_max) {  // Keep small
			this->u_.s_.resize(new_sz);
			return this->u_.s_.begin();
		} else {  // new_sz > small_t::size_max; Resize small->big
			auto new_cap = new_sz;
			unsigned char *p = new unsigned char[static_cast<uint32_t>(new_cap)];
			//small_bytevec_t::call_counts.calls_new++;
			this->init_big();
			this->u_.b_.adopt(this->u_.b_.pad_,p,new_sz,new_cap);
			return this->u_.b_.begin();
		}
	}
}
unsigned char *jmid::internal::small_bytevec_t::resize_small2small_nocopy(std::int32_t new_sz) noexcept {
	this->u_.s_.resize(new_sz);
	return this->u_.s_.begin();
}
unsigned char *jmid::internal::small_bytevec_t::resize_preserve_cap(std::int32_t new_sz) {
	new_sz = std::clamp(new_sz,0,jmid::internal::small_bytevec_t::size_max);
	if (this->is_big()) {
		this->u_.b_.resize(new_sz);
		return this->u_.b_.begin();
	} else {  // present object is small
		if (new_sz <= small_t::size_max) {  // Keep small
			this->u_.s_.resize(new_sz);
			return this->u_.s_.begin();
		} else {  // new_sz > small_t::size_max; Resize small->big
			auto new_cap = new_sz;
			unsigned char *pdest = new unsigned char[static_cast<uint32_t>(new_cap)];
			std::copy(this->u_.s_.begin(),this->u_.s_.end(),pdest);
			this->init_big();
			this->u_.b_.adopt(this->u_.b_.pad_,pdest,new_sz,new_cap);
			return this->u_.b_.begin();
		}
	}
}
std::int32_t jmid::internal::small_bytevec_t::reserve(std::int32_t new_cap) {
	new_cap = std::clamp(new_cap,this->capacity(),jmid::internal::small_bytevec_t::size_max);
	if (this->is_big()) {
		new_cap = this->u_.b_.reserve(new_cap);
	} else {  // present object is small
		if (new_cap > small_t::size_max) {  // Resize small->big
			auto new_sz = this->u_.s_.size();
			unsigned char *pdest = new unsigned char[static_cast<uint32_t>(new_cap)];
			//small_bytevec_t::call_counts.calls_new++;
			std::copy(this->u_.s_.begin(),this->u_.s_.end(),pdest);
			
			this->init_big();
			this->u_.b_.adopt(this->u_.b_.pad_,pdest,new_sz,new_cap);
		}  // else (new_cap <= small_t::size_max); do nothing
	}
	return this->capacity();
}
unsigned char *jmid::internal::small_bytevec_t::push_back(unsigned char c) {
	// TODO:  Could optimize this by first determining if is_big/small(),
	// then delegating to this->u_.s/b_.push_back() functions
	auto sz = this->size();
	if (sz==this->capacity()) {
		// Note that a consequence of this test is that if this->is_small(),
		// reserve() will not be called until the 'small' object is 
		// completely full, so the object remains small for as long as 
		// possible.  

		// if this->size()==0, don't want to reserve 2*sz==0; in theory
		// one could have a capacity 0 'big' object.  
		auto new_sz = std::max(2*sz,jmid::internal::small_bytevec_t::capacity_small);
		this->reserve(new_sz);
	}
	this->resize(sz+1);
	*(this->end()-1) = c;
	return this->end()-1;
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



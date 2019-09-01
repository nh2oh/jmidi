#pragma once
#include <cstdint>
#include <array>
#include <algorithm>

namespace jmid {

namespace internal {


//
// Class small_t
// Invariants:  
// -> flags_&0x80u==0x80u
// -> flags_&0x7Fu==size(); size() <= capacity() <= small_t::size_max
// -> capacity()==small_t::size_max()
//
struct small_t {
	static constexpr std::int32_t size_max = 23;

	unsigned char flags_;  // small => flags_&0x80u==0x80u
	std::array<unsigned char,23> d_;
	
	void init() noexcept;
	std::int32_t size() const noexcept;
	constexpr std::int32_t capacity() const noexcept;
	// Can only resize to a value on [0,small_t::size_max]
	std::int32_t resize(std::int32_t) noexcept;
	//std::int32_t resize(small_size_t) noexcept;
	// Does not std::clamp(sz,0,this->capacity());
	std::int32_t resize_unchecked(std::int32_t) noexcept;

	void abort_if_not_active() const noexcept;

	unsigned char *begin() noexcept;
	const unsigned char *begin() const noexcept;
	unsigned char *end() noexcept;
	const unsigned char *end() const noexcept;
};
//
// Class big_t
// Invariants:  
// -> flags_&0x80u==0x00u
// -> p_ is a valid ptr or nullptr
// -> If p_==nullptr, sz_==cap_==0
//
struct big_t {
	static constexpr std::int32_t size_max = 0x0FFFFFFF;
	using pad_t = std::array<unsigned char,7>;

	unsigned char flags_;  // big => flags_&0x80u==0x00u
	pad_t pad_;
	unsigned char *p_;  // 16
	std::uint32_t sz_;  // 20
	std::uint32_t cap_;  // 24

	// init() is meant to be called from an _uninitialized_ state; it does
	// not and can not test and conditionally delete [] p_.  
	void init() noexcept;
	// free_and_reinit() checks p_ and calls delete []; it should only 
	// be called from an _initialized_ state.  
	void free_and_reinit() noexcept;
	// Deletes p_; object must be initialized before calling
	void adopt(const pad_t&, unsigned char*, std::int32_t, std::int32_t) noexcept;
	std::int32_t size() const noexcept;
	std::int32_t resize(std::int32_t);
	// If resizing to something bigger than the present capacity, does 
	// not copy the data into the new buffer; if resizing smaller, the 
	// effect is the same as a call to resize().  
	std::int32_t resize_nocopy(std::int32_t);
	std::int32_t reserve(std::int32_t);
	std::int32_t capacity() const noexcept;
	void abort_if_not_active() const noexcept;
	

	unsigned char *begin() noexcept;
	const unsigned char *begin() const noexcept;
	unsigned char *end() noexcept;
	const unsigned char *end() const noexcept;
};
// 
// class small_bytevec_t
//
// Very simple "'small' std::vector"-like class for managing an array 
// of unsigned char.  
//
struct small_bytevec_range_t {
	unsigned char *begin;
	unsigned char *end;
};
struct small_bytevec_const_range_t {
	const unsigned char *begin;
	const unsigned char *end;
};
class small_bytevec_t {
public:
	static constexpr std::int32_t size_max = big_t::size_max;
	static constexpr std::int32_t capacity_small = small_t::size_max;

	// small_bytevec_t() noexcept;
	// Constructs a 'small' object w/ size()==0
	small_bytevec_t() noexcept;
	// Constructs a 'small' or 'big' object w/ size() as specified
	small_bytevec_t(std::int32_t);
	// Copy ctor, copy assign; the new/destination object does not necessarily
	// inherit the same size-type as the the source.  If the source is 'big'
	// but its data fits in a small object, the destination object will be
	// constructed as, or transition to, a small object.  If the source is
	// small, the destination object is always small.  Contrast w/ the move
	// functions.  
	small_bytevec_t(const small_bytevec_t&);
	small_bytevec_t& operator=(const small_bytevec_t&);  
	// Move ctor, move assignment; the new/destination object inherits the 
	// same size-type as the source, even if the source data will fit in a 
	// small object.  Contrast w/the copy ctor/assign op.  
	small_bytevec_t(small_bytevec_t&&) noexcept;
	small_bytevec_t& operator=(small_bytevec_t&&) noexcept;
	~small_bytevec_t() noexcept;

	std::int32_t size() const noexcept;
	std::int32_t capacity() const noexcept;
	// int32_t resize(int32_t new_sz);
	// If 'big' and the new size is <= small_bytevec_t::capacity_small, 
	// will cause a big->small transition.  
	unsigned char *resize(std::int32_t);
	template<typename T>
	unsigned char *resize(T sz) {
		std::int32_t szi32 = static_cast<std::int32_t>(std::clamp(sz,T(0),
			static_cast<T>(small_bytevec_t::size_max)));
		return this->resize(szi32);
	};
	// If resizing causes either allocation of a new buffer or a 
	// big->small transition, the present object's data is not copied into
	// the new buffer.  Otherwise the effect is the same as a call to 
	// resize().  
	unsigned char *resize_nocopy(std::int32_t);
	// A wrapper for this->u_.s_.resize(new_sz); For a small->small resize
	// there is no allocation, thus the method can be noexcept
	unsigned char *resize_small2small_nocopy(std::int32_t) noexcept;
	// TODO:  unsigned char *resize_preserve_cap(std::int32_t) should be
	// the default behavior of reserve().  
	// If big, does _not_ cause a big->small transition
	unsigned char *resize_preserve_cap(std::int32_t);
	// int32_t reserve(int32_t new_cap);
	// Will cause big->small, but never small->big transitions.  
	std::int32_t reserve(std::int32_t);

	unsigned char *push_back(unsigned char);

	bool debug_is_big() const noexcept;

	small_bytevec_range_t data_range() noexcept;
	small_bytevec_const_range_t data_range() const noexcept;
	small_bytevec_range_t pad_or_data_range() noexcept;
	small_bytevec_const_range_t pad_or_data_range() const noexcept;

	unsigned char* begin() noexcept;
	const unsigned char* begin() const noexcept;
	unsigned char* end() noexcept;
	const unsigned char* end() const noexcept;
	unsigned char* raw_begin() noexcept;
	const unsigned char* raw_begin() const noexcept;
	unsigned char* raw_end() noexcept; 
	const unsigned char* raw_end() const noexcept;
private:
	union sbou_t {
		small_t s_;
		big_t b_;
		std::array<unsigned char,sizeof(small_t)> raw_;
	};
	sbou_t u_;

	bool is_big() const noexcept;
	bool is_small() const noexcept;
	// Call only on an unitialized object; calls the init() method of the 
	// corresponding type.  big_t::init() does _not_ free memory.  
	void init_small() noexcept;
	void init_big() noexcept;
};

static_assert(sizeof(small_t)==sizeof(big_t));
static_assert(sizeof(small_t)==sizeof(small_bytevec_t));


}  // namespace jmid
} // namespace internal

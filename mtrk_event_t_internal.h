#pragma once
#include <cstdint>
#include <array>


namespace mtrk_event_t_internal {

//
// Class small_t
// Invariants:  
// -> flags_&0x80u==0x80u
// -> flags_&0x7Fu==size(); size() <= capacity() <= small_t::size_max
// -> capacity()==small_t::size_max()
//
struct small_t {
	static constexpr int32_t size_max = 23;

	unsigned char flags_;  // small => flags_&0x80u==0x80u
	std::array<unsigned char,23> d_;
	
	void init() noexcept;
	int32_t size() const noexcept;
	constexpr int32_t capacity() const noexcept;
	// Can only resize to a value on [0,small_t::size_max]
	int32_t resize(int32_t) noexcept;
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
	static constexpr int32_t size_max = 0x0FFFFFFF;
	using pad_t = std::array<unsigned char,7>;

	unsigned char flags_;  // big => flags_&0x80u==0x00u
	pad_t pad_;
	unsigned char *p_;  // 16
	uint32_t sz_;  // 20
	uint32_t cap_;  // 24

	// init() is meant to be called from an _uninitialized_ state; it does
	// not and can not test and conditionally delete [] p_.  
	void init() noexcept;
	// free_and_reinit() checks p_ and calls delete []; it should only 
	// be called from an _initialized_ state.  
	void free_and_reinit() noexcept;
	int32_t size() const noexcept;
	int32_t resize(int32_t);
	int32_t reserve(int32_t);
	int32_t capacity() const noexcept;
	void abort_if_not_active() const noexcept;
	// Deletes p_; object must be initialized before calling
	void adopt(const pad_t&, unsigned char*, int32_t, int32_t) noexcept;

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
class small_bytevec_t {
public:
	static constexpr int32_t size_max = big_t::size_max;
	static constexpr int32_t capacity_small = small_t::size_max;

	// Constructs a 'small' object w/ size()==0
	small_bytevec_t() noexcept;
	small_bytevec_t(const small_bytevec_t&);  // Copy ctor
	small_bytevec_t& operator=(const small_bytevec_t&);  // Copy assign
	small_bytevec_t(small_bytevec_t&&) noexcept;  // Move ctor
	small_bytevec_t& operator=(small_bytevec_t&&) noexcept;  // Move assign
	~small_bytevec_t() noexcept;

	int32_t size() const noexcept;
	int32_t capacity() const noexcept;
	// int32_t resize(int32_t new_sz);
	// If 'big' and the new size is <= small_bytevec_t::capacity_small, 
	// will cause a big->small transition.  
	int32_t resize(int32_t);
	// int32_t reserve(int32_t new_cap);
	// Will cause big->small, but never small->big transitions.  
	int32_t reserve(int32_t);

	unsigned char *push_back(unsigned char);

	bool debug_is_big() const noexcept;

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

};  // namespace mtrk_event_t_internal


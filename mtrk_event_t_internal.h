#pragma once
#include <cstdint>
#include <array>


namespace mtrk_event_t_internal {

// 
// Very simple "'small' std::vector"-like class for managing an array 
// of unsigned char.  
//
struct small_t {
	static constexpr int32_t size_max = 23;

	unsigned char flags_;  // small => flags_&0x80u==0x80u
	std::array<unsigned char,23> d_;
	
	void init();
	int32_t size() const;
	constexpr int32_t capacity() const;  // Returns small_t::size_max
	// Can only resize to a value on [0,small_t::size_max]
	int32_t resize(int32_t);
	void abort_if_not_active() const;

	unsigned char *begin();
	const unsigned char *begin() const;
	unsigned char *end();
	const unsigned char *end() const;
};
//
// Class big_t
// Invariants:  
// -> If !(flags_&0x80u), p_ is a valid ptr or nullptr; it can always
//    be tested as if (this->p_) { ... .  
// -> If p_==nullptr, sz_==cap_==0
//
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
	void init();
	// free_and_reinit() checks p_ and calls delete []; it should only 
	// be called from an _initialized_ state.  
	void free_and_reinit();
	int32_t size() const;
	int32_t resize(int32_t);
	int32_t reserve(int32_t);
	int32_t capacity() const;
	void abort_if_not_active() const;
	// Deletes p_; object must be initialized before calling
	void adopt(const pad_t&, unsigned char*, int32_t, int32_t);

	unsigned char *begin();
	const unsigned char *begin() const;
	unsigned char *end();
	const unsigned char *end() const;
};
class sbo_t {
public:
	static constexpr int32_t size_max = big_t::size_max;
	static constexpr int32_t capacity_small = small_t::size_max;

	// Constructs a 'small' object w/ size()==0
	sbo_t();
	sbo_t(const sbo_t&);  // Copy ctor
	sbo_t& operator=(const sbo_t&);  // Copy assign
	sbo_t(sbo_t&&) noexcept;  // Move ctor
	sbo_t& operator=(sbo_t&&) noexcept;  // Move assign
	~sbo_t();

	int32_t size() const;
	int32_t capacity() const;
	// int32_t resize(int32_t new_sz);
	// If 'big' and the new size is <= sbo_t::capacity_small, will cause
	// a big->small transition.  
	int32_t resize(int32_t);
	int32_t reserve(int32_t);

	unsigned char *push_back(unsigned char);

	bool debug_is_big() const;

	unsigned char* begin();
	const unsigned char* begin() const;
	unsigned char* end();
	const unsigned char* end() const;
	unsigned char* raw_begin();
	const unsigned char* raw_begin() const;
	unsigned char* raw_end();
	const unsigned char* raw_end() const;
private:
	union sbou_t {
		small_t s_;
		big_t b_;
		std::array<unsigned char,sizeof(small_t)> raw_;
	};
	sbou_t u_;

	bool is_big() const;
	bool is_small() const;
	// Call only on an unitialized object; callas the init() method of the 
	// corresponding type.  big_t::init() does _not_ free memory.  
	void init_small();
	void init_big();
};

static_assert(sizeof(small_t)==sizeof(big_t));
static_assert(sizeof(small_t)==sizeof(sbo_t));

};  // namespace mtrk_event_t_internal


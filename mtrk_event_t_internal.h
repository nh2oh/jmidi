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

	unsigned char *begin();  // Returns &d[0]
	const unsigned char *begin() const;
	unsigned char *end();  // Returns &d[0]+this->size()
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

	// resize() keeps the object 'big', even if the new size would allow 
	// it to fit in a small_t.  big_t/small_t switching logic lives in 
	// sbo_t, not here.  
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

	unsigned char *begin();  // Returns p
	const unsigned char *begin() const;
	unsigned char *end();  // Returns begin() + sz_
	const unsigned char *end() const;
};
class sbo_t {
public:
	static constexpr int32_t size_max = big_t::size_max;
	static constexpr int32_t capacity_small = small_t::size_max;

	// Constructs a 'small' object w/ size()==0
	sbo_t();
	// Copy ctor
	sbo_t(const sbo_t&);
	// Copy assignment; overwrites a pre-existing lhs 'this' w/ rhs
	sbo_t& operator=(const sbo_t&);
	// Move ctor
	sbo_t(sbo_t&&) noexcept;
	// Move assignment
	sbo_t& operator=(sbo_t&&) noexcept;
	// Dtor
	~sbo_t();

	// Call only on an unitialized object; init() method of the  
	// corresponding type.  These do _not_ free memory.  
	void init_small();
	void init_big();

	int32_t size() const;
	int32_t capacity() const;
	// int32_t resize(int32_t new_sz);
	// If 'big' and the new size is <= small_t::size_max, will change the
	// object type to 'small'.  Memory is freed correctly.  If presently 
	// 'small', but the new_size is still <= small_t::size_max, leaves the 
	// object small and calls the small_t::resize() method; the object 
	// remains 'small'.  
	int32_t resize(int32_t);
	int32_t reserve(int32_t);
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
};

static_assert(sizeof(small_t)==sizeof(big_t));
static_assert(sizeof(small_t)==sizeof(sbo_t));

};  // namespace mtrk_event_t_internal


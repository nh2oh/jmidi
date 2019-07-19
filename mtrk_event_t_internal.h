#pragma once
#include <cstdint>
#include <array>

namespace mtrk_event_t_internal {

// 
// Very simple small-buffer class for managing an array of unsigned char.  
// The smallest value of capacity() allowed is the size of the data 
// region (data member d_) of the 'small' object (23 bytes).  
//
struct small_t {
	static constexpr int32_t size_max = 23;

	unsigned char flags_;  // small => flags_&0x80u==0x80u
	std::array<unsigned char,23> d_;
	
	int32_t size() const;
	constexpr int32_t capacity() const;  // Returns small_t::size_max
	// Can only resize to a value on [0,small_t::size_max]
	int32_t resize(int32_t);
	
	unsigned char *begin();  // Returns &d[0]
	const unsigned char *begin() const;
	unsigned char *end();  // Returns &d[0]+this->size()
	const unsigned char *end() const;
};
struct big_t {
	static constexpr int32_t size_max = 0x0FFFFFFF;

	unsigned char flags_;  // big => flags_&0x80u==0x00u
	std::array<unsigned char,7> pad_;
	unsigned char *p_;  // 16
	uint32_t sz_;  // 20
	uint32_t cap_;  // 24

	// Keeps the object 'big', even if the new size would allow it to
	// fit in a small_t.  big_t/small_t switching logic lives in sbo_t,
	// not here.  
	int32_t size() const;
	int32_t resize(int32_t);
	int32_t reserve(int32_t);
	int32_t capacity() const;
	// void free_and_adpot_new(unsigned char *p, int32_t sz, int32_t cap);
	// Calls delete [] on this->p_, replaces this->sz_ and this->cap_ 
	// w/ sz and cap.  Does not touch flags; it is assumed the parent 
	// sbo_t::is_big().  
	void free_and_adpot_new(unsigned char*, int32_t, int32_t);

	unsigned char *begin();  // Returns p
	const unsigned char *begin() const;  // Returns p
	unsigned char *end();  // Returns begin() + cap_
	const unsigned char *end() const;  // Returns begin() + cap_
};
class sbo_t {
public:
	static constexpr int32_t size_max = big_t::size_max;

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

	void clear_flags_and_set_big();
	void clear_flags_and_set_small();
	// void init_big_unsafe();
	// Accesses the union through raw_; sets all array elements (including
	// b_./s_.flags_) to 0x00u, then calls clear_flags_and_set_big().  
	// Sets b_.p_=nullptr, b_.sz_=0, b_.cap_=0.  
	// This has the effect of "initializing" the 'big' object member.  
	// If u_.is_big() at the time of the call, memory is not freed.  
	void init_big_unsafe();
	// void init_small_unsafe();
	// Accesses the union through raw_; sets all array elements (including
	// b_./s_.flags_) to 0x00u, then calls clear_flags_and_set_small().  
	// This has the effect of "initializing" the 'small' object member w/a 
	// size()==0 and all data == 0x00u.  
	// If u_.is_big() at the time of the call, memory is not freed.  
	void init_small_unsafe();
	
};

static_assert(sizeof(small_t)==sizeof(big_t));
static_assert(sizeof(small_t)==sizeof(sbo_t));

};  // namespace mtrk_event_t_internal


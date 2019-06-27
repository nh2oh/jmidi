#pragma once
#include <cstdint>
#include <array>

namespace mtrk_event_t_internal {

// TODO:   Need to implement the invariant that the capacity of a big_t
// is never smaller than small_capacity()

struct small_t {
	unsigned char flags_;  // small => flags_&0x80u==0x80u
	std::array<unsigned char,23> d_;

	//uint64_t size() const;
	constexpr uint64_t capacity() const;
	unsigned char *begin();  // Returns &d[0]
	const unsigned char *begin() const;  // Returns &d[0]
	unsigned char *end();  // Returns &d[0]+mtrk_event_get_data_size_unsafe(&d[0],0x00u)
	const unsigned char *end() const;
};
struct big_t {
	unsigned char flags_;  // big => flags_&0x80u==0x00u
	std::array<unsigned char,7> pad_;
	unsigned char* p_;  // 16
	uint32_t sz_;  // 20
	uint32_t cap_;  // 24

	//uint64_t size() const;
	uint64_t capacity() const;
	unsigned char *begin();  // Returns p
	const unsigned char *begin() const;  // Returns p
	unsigned char *end();  // Returns begin() + sz
	const unsigned char *end() const;  // Returns begin() + sz
};
class sbo_t {
public:
	constexpr uint64_t small_capacity() const;
	//uint64_t size() const;
	uint64_t capacity() const;
	// resize(uint32_t new_cap) Sets the _capacity_ to 
	// std::max(capacity(),new_cap).  Note that the smallest possible
	// capacity is small_capacity().  If new_cap is < size(), the event
	// will be truncated and become invalid in a way that almost 
	// certainly will violate the invariants of mtrk_event_t and in fact
	// also those of sbo_t (the size() calculations will not be correct).
	// Sets the big/small flag & allocates/frees memory as appropriate.  
	uint32_t resize(uint32_t);
	unsigned char* begin();
	const unsigned char* begin() const;
	unsigned char* end();
	const unsigned char* end() const;
	unsigned char* raw_begin();
	const unsigned char* raw_begin() const;
	unsigned char* raw_end();
	const unsigned char* raw_end() const;
	void set_flag_big();
	void set_flag_small();
	void free_if_big();  // preserves the initial big/small flag state
	// sets flag big.  If the object is big @ the time of the call, 
	// the old memory is not freed.  Call free_if_big() first.  
	void big_adopt(unsigned char*, uint32_t, uint32_t);  // ptr, size, capacity
	// Sets the small-object flag, then sets all non-flag data members 
	// of b_ or s_ (as appropriate) to 0; if u_.is_big(), the memory is 
	// not freed.  Used by the move operators.  
	void zero_object();
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


#pragma once
#include <cstdint>
#include <array>

namespace jmid {

namespace internal {

//
// TODO:  
// -Why not just store int32_t (rather than uint32_t) in big_t?
// -Reorder conditionals in resize()/reserve() etc to put the is_small() 
//  case first.  
//

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
	
	// Sets flag_==0x80u; this also has the effect of setting size == 0
	void init() noexcept;
	std::int32_t size() const noexcept;
	constexpr std::int32_t capacity() const noexcept;
	// Returns the new size.  Size must lie on [0,small_t::size_max]; the
	// input is std::clamp()ed to this range.  
	std::int32_t resize(std::int32_t) noexcept;
	// Does not std::clamp() the input to [0,small_t::size_max]
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

	// Sets p_==nullptr and zeros all members (including pad_).  
	// init() is meant to be called from an _uninitialized_ state; it does
	// not and can not test and conditionally delete [] p_.  
	void init() noexcept;
	// free_and_reinit() checks p_ and calls delete []; it should only 
	// be called from an _initialized_ state.  
	void free_and_reinit() noexcept;
	// Object must be initialized before calling.  If p_!=nullptr, 
	// deletes p_, then assigns pad_, sz_, cap_ to the values passed in
	void adopt(const pad_t&, unsigned char*, std::int32_t, std::int32_t) noexcept;
	std::int32_t size() const noexcept;
	// Sets size==0, does not alter capacity
	void clear() noexcept;
	// resize() and resize_nocopy() only change the capacity() when new_sz
	// is > the present capacity.  
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
	explicit small_bytevec_t(std::int32_t);

	// Copy ctor.  The new object has size-type appropriate to rhs.size(), 
	// and does _not_ necessarily inherit the size-type of rhs.  Big objects
	// are _only_ produced when rhs.size() > small_t::size_max.  
	small_bytevec_t(const small_bytevec_t&);
	// Copy assign.  The lhs object retains its size-type if sufficient to 
	// accomodate rhs.size().  This means that a big lhs remains big always, 
	// and a small lhs will undergo a small->big transition only if 
	// rhs.size() > small_t::size_max.  
	small_bytevec_t& operator=(const small_bytevec_t&);  

	// Move ctor, move assignment; the new/destination object inherits the 
	// same size-type as the source, even if the source data will fit in a 
	// small object.  Contrast w/the copy ctor/assign op.  The source object
	// is left 'small' w/a size()==0.  
	small_bytevec_t(small_bytevec_t&&) noexcept;
	small_bytevec_t& operator=(small_bytevec_t&&) noexcept;
	// Dtor;  the destructed state is a 'small 'object w/ size()==0.  
	~small_bytevec_t() noexcept;

	std::int32_t size() const noexcept;
	std::int32_t capacity() const noexcept;
	
	// int32_t reserve(int32_t new_cap);
	// If new_cap is > the present capacity(), the capacity is increased
	// (with a small->big transition if necessary); if new_cap <= the present
	// capacity, does nothing.  
	std::int32_t reserve(std::int32_t);
	// unsigned char *resize(std::int32_t new_sz);
	// Changes the size() of the object to the new value.  Does not cause 
	// unnecessary size transitions:  big objects /always/ remain big, small 
	// objects remain small if new_sz < small_t::size_max, otherwise they 
	// become big.  
	unsigned char *resize(std::uint64_t);
	unsigned char *resize(std::int64_t);
	unsigned char *resize(std::int32_t);
	// unsigned char *resize_nocopy(std::int32_t new_sz);
	// Same as for resize(), except that for values causing small->big 
	// transitions (for small objects) or new buffer allocation (for big 
	// objects), the old data is not copied into the new buffer.  
	unsigned char *resize_nocopy(std::int32_t);
	
	// unsigned char *push_back(unsigned char);
	// If this->is_small(), a small->big transition will not occur until the
	// small object is completely full.  If big, the capacity() is unchanged 
	// unless size()==capacity(), in which case the capacity() doubles.   
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
	// corresponding type (note that big_t::init(), and therefore init_big(),
	// does _not_ free memory).  These functions are how i formally change
	// the active member of the union.  
	void init_small() noexcept;
	void init_big() noexcept;
};

static_assert(sizeof(small_t)==sizeof(big_t));
static_assert(sizeof(small_t)==sizeof(small_bytevec_t));


}  // namespace internal
}  // namespace jmid


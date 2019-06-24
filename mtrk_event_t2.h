#pragma once
#include "midi_raw.h"  // declares smf_event_type
#include <string>
#include <cstdint>
#include <array>
#include <vector>
#include <memory>

class mtrk_event_t2;
class mtrk_event_iterator_t2;
class mtrk_event_const_iterator_t2;

enum class mtrk_sbo_print_opts {
	normal,
	detail,
	// Prints the value of flags_, midi_status_.  If big, prints the byte 
	// array d_ for the local container in addition to the heap-allocated
	// data.  
	debug  
};
std::string print(const mtrk_event_t2&,
			mtrk_sbo_print_opts=mtrk_sbo_print_opts::normal);

//
// mtrk_event_t2:  An sbo-featured container for mtrk events
//
// An mtrk_event_t2 is a container for storing and working with byte
// sequences representing mtrk events (including the event delta-time).  
// An mtrk_event_t2 stores and provides direct access (both read and 
// write) to the sequence of bytes (unsigned char) encoding the event.  
//
// The choice to store mtrk events directly as the byte array as specified by
// the MIDI spec (and not as some processed aggregate of native types, ex, 
// with a uint32_t for the delta-time, an enum for the sme_event_type, etc)
// is in keeping with the overall philosophy of the library to facilitate
// lightweight and straightforward manipulation of MIDI data.  All users 
// working with a MIDI library are presumed to know something about MIDI; 
// they are not presumed to be (or want to be) intimately fimilar with the 
// idiosyncratic details of an overly complicated library.  
// 
// Since write access to the underlying byte array is provided, it is hard for
// this container to maintain any invariants.  
//




// TODO:  Add typedefs so std::back_inserter() can be used.  
// TODO:  This exposes a lot of dangerous getters
// TODO:  Member enum class offs is absolutely disgusting
// TODO:  Error handling for some of the ctors; default uninit value?
// TODO:  Ctors for channel/meta/sysex data structs
// 
//
class mtrk_event_t2 {
public:
	// Default ctor; creates a "small" object representing a meta-text event
	// w/ a payload length of 0.  
	mtrk_event_t2();
	// Ctors for callers who have pre-computed the exact size of the event 
	// and who can also supply a midi status byte (if needed).  In the first
	// overload, parameter 1 is a pointer to the first byte of the delta-time
	// field for the event.  In the second overload, parameter 1 is the value
	// of the delta time, and parameter 2 is a pointer to the first byte 
	// immediately following the delta time field (the beginning of the 
	// "event" data).  
	mtrk_event_t2(const unsigned char*, uint32_t, unsigned char=0);
	mtrk_event_t2(const uint32_t&, const unsigned char*, uint32_t, unsigned char=0);
	// delta_time, raw midi channel event data
	// This ctor calls normalize() on the midi_ch_event_t parameter before
	// the data is copied into the event array; invalid values in this 
	// parameter may lead to an event with unexpected behavior.  
	mtrk_event_t2(uint32_t, midi_ch_event_t);
	// TODO:  the field validate_mtrk_event_result_t.running_status is obtained by
	// a call to get_running status() not get_status_byte().  The meaning is not
	// quite the same; this is probably a bug.  
	mtrk_event_t2(const unsigned char *p, 
		const validate_mtrk_event_result_t& ev)
		: mtrk_event_t2::mtrk_event_t2(p,ev.size,ev.running_status) {};
	// Copy ctor
	mtrk_event_t2(const mtrk_event_t2&);
	// Copy assignment; overwrites a pre-existing lhs 'this' w/ rhs
	mtrk_event_t2& operator=(const mtrk_event_t2&);
	// Move ctor
	mtrk_event_t2(mtrk_event_t2&&) noexcept;
	// Move assignment
	mtrk_event_t2& operator=(mtrk_event_t2&&) noexcept;	
	// Dtor
	~mtrk_event_t2();
	/*
	const unsigned char& operator[](uint32_t) const;
	unsigned char& operator[](uint32_t);

	uint32_t data_size() const;  // Not including the delta-t
	// TODO:  payload_size()
	// If is_small(), reports the size of the d_ array, which is the maximum
	// size of an event that the 'small' state can contain.  
	*/
	uint64_t size() const;
	uint64_t capacity() const;

	// Iterators allowing access to the underlying unsigned char array
	//
	// begin(), dt_begin() both return iterators to the first byte of the 
	// dt field.  The redundant dt_ versions are here for users who want
	// to be more explicit.  dt_end() returns an iterator to the first
	// byte following the dt field, which is the first byte of the "event," 
	// and is the same as is returned by event_begin().  
	// payload_begin() returns an iterator to the first byte following
	// the delta_t field for midi events, and to the first byte following
	// the vlq length field for sysex and meta events.  
	unsigned char *data();
	const unsigned char *data() const;
	mtrk_event_iterator_t2 begin();
	mtrk_event_const_iterator_t2 begin() const;
	mtrk_event_iterator_t2 end();
	mtrk_event_const_iterator_t2 end() const;
	mtrk_event_const_iterator_t2 dt_begin() const;
	mtrk_event_iterator_t2 dt_begin();
	mtrk_event_const_iterator_t2 dt_end() const;
	mtrk_event_iterator_t2 dt_end();
	mtrk_event_const_iterator_t2 event_begin() const;
	mtrk_event_iterator_t2 event_begin();
	// TODO:  These call type(), which advances past the delta_t to determine
	// the status byte, then they manually advance a local iterator past the 
	// delta_t... redundant...
	mtrk_event_const_iterator_t2 payload_begin() const;
	mtrk_event_iterator_t2 payload_begin();

	// Getters
	//
	smf_event_type type() const;

	/*
	// Getters
	unsigned char status_byte() const;
	// The value of the running-status _after_ this event has passed
	unsigned char running_status() const;
	
	uint32_t delta_time() const;
	bool set_delta_time(uint32_t);
	// For meta events w/ a text payload, copies the payload to a
	// std::string;  Returns an empty std::string otherwise.  
	std::string text_payload() const;
	uint32_t uint32_payload() const;
	// For channel events, gets the midi data, including.  For non-channel
	// events, .is_valid==false and the value of all other fields is 
	// undefined.  
	struct channel_event_data_t {
		uint8_t status_nybble {0x00u};  // most-significant nybble
		uint8_t ch {0x00u};
		uint8_t p1 {0x00u};
		uint8_t p2 {0x00u};
		bool is_valid {false};
		bool is_running_status {false};
	};
	channel_event_data_t midi_data() const;

	bool is_big() const;
	bool is_small() const;
	bool validate() const;

	bool operator==(const mtrk_event_t2&) const;
	bool operator!=(const mtrk_event_t2&) const;
	*/
private:
	struct small_t {
		unsigned char flags_;  // small => flags_&0x80u==0x80u
		std::array<unsigned char,23> d_;

		uint64_t size() const;
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

		uint64_t size() const;
		uint64_t capacity() const;
		unsigned char *begin();  // Returns p
		const unsigned char *begin() const;  // Returns p
		unsigned char *end();  // Returns begin() + sz
		const unsigned char *end() const;  // Returns begin() + sz
	};
	class sbo_t {
	public:
		constexpr uint64_t small_capacity();
		uint64_t size() const;
		uint64_t capacity() const;
		unsigned char* begin();
		const unsigned char* begin() const;
		unsigned char* end();
		const unsigned char* end() const;
		void set_flag_big();
		void set_flag_small();
		void free_if_big();  // preserves the initial big/small flag state
		void big_adopt(unsigned char*, uint32_t, uint32_t);  // ptr, size, capacity
		// Sets the small-object flag, then sets all non-flag data members 
		// of b_ or s_ (as appropriate) to 0; if u_.is_big(), the memory is 
		// not freed.  Used by the move operators.  
		void zero_object();
	private:
		union sbou_t {
			small_t s_;
			big_t b_;
		};
		sbou_t u_;

		bool is_big() const;
		bool is_small() const;
	};
	static_assert(sizeof(small_t)==sizeof(big_t));
	static_assert(sizeof(small_t)==sizeof(sbo_t));
	sbo_t d_;

	// Called by the default ctor mtrk_event_t2()
	// Overwrites *this w/ the default ctor'd value of a length 0 meta text
	// event.  Ignore the big/small flag of the union; do not free memory
	// if big.  
	void default_init();

	/*
	
	// Ptr to this->data_[0], w/o regard to this->is_small()
	const unsigned char *raw_data() const;
	// ptr to this->flags_
	const unsigned char *raw_flag() const;

	// Causes the container to "adopt" the data at p (writes p and the 
	// associated size, capacity into this->d_).  Note: data at p is _not_
	// copied into a new buffer.  Caller should not delete p after calling.  
	// The initial state of the container is completely overwritten w/o 
	// discretion; if is_big(), big_ptr() is _not_ freed.  Callers not wanting
	// to leak memory should first check is_big() and free big_ptr() as 
	// necessary.  This function does not rely in any way on the initial
	// state of the object; it is perfectly fine for the object to be 
	// initially in a completely invalid state.  
	// args:  ptr, size, cap, running status
	bool init_big(unsigned char *, uint32_t, uint32_t, unsigned char); 
	// If is_small(), make big with capacity as specified.  All data from the 
	// local d_ array is copied into the new remote buffer.  
	bool small2big(uint32_t);
	bool big_resize(uint32_t);

	// Zero all data members.  No attempt to query the state of the object
	// is made; if is_big() the memory will leak.  
	void clear_nofree();

	void set_flag_small();
	void set_flag_big();

	// Getters {big,small}_ptr() get a pointer to the first byte of
	// the underlying data array (the first byte of the dt field).  Neither
	// checks for the container size; the caller must be careful to call the
	// correct function.  These are the "unsafe" versions of .data().  
	unsigned char *big_ptr() const;  // getter
	unsigned char *small_ptr() const;  // getter
	unsigned char *set_big_ptr(unsigned char *p);  // setter
	// Getters to read the locally-cached size() field in the case of 
	// this->is_big().  Does not check if this->is_big(); must not be called
	// if this->is_small().  This is the unsafe version of .size().  
	uint32_t big_size() const;  // getter
	uint32_t set_big_size(uint32_t);  // setter
	uint32_t big_cap() const;  // getter
	uint32_t set_big_cap(uint32_t);  // setter
	uint32_t big_delta_t() const;  // shortcut to determining the ft if is_big()
	uint32_t set_big_cached_delta_t(uint32_t);
	smf_event_type big_smf_event_type() const;  // shortcut to determining the type if is_big()
	smf_event_type set_big_cached_smf_event_type(smf_event_type);
	*/
	friend std::string print(const mtrk_event_t2&,
			mtrk_sbo_print_opts);

};




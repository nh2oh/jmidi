#pragma once
#include "mtrk_event_t_internal.h"
#include "mtrk_event_iterator_t.h"
#include "midi_raw.h"  // declares smf_event_type
#include <string>  // For declaration of print()
#include <cstdint>


// #including mtrk_event_iterator_t.h rather than fwd declaring; a user
// who includes the container class should be able to construct the 
// iterators.  

// For friend dcln print(const mtrk_event_t&, mtrk_sbo_print_opts)
enum class mtrk_sbo_print_opts {
	normal,
	detail,
	// Prints the value of flags_, midi_status_.  If big, prints the byte 
	// array d_ for the local container in addition to the heap-allocated
	// data.  
	debug  
};


//
// mtrk_event_t:  An sbo-featured container for mtrk events
//
// An mtrk_event_t is a container for storing and working with byte
// sequences representing mtrk events (including the event delta-time).  
// An mtrk_event_t stores and provides direct access (both read and 
// write) to the sequence of bytes (unsigned char) encoding the event.  
//
// The choice to store mtrk events directly as the byte array as specified by
// the MIDI spec (and not as some processed aggregate of native types, ex, 
// with a uint32_t for the delta-time, an enum for the smf_event_type, etc)
// is in keeping with the overall philosophy of the library to facilitate
// lightweight and straightforward manipulation of MIDI data.  All users 
// working with a MIDI library are presumed to know something about MIDI; 
// they are not presumed to be (or want to be) intimately fimilar with the 
// idiosyncratic details of an overly complicated library.  
// 
// Since write access to the underlying byte array is provided, it is hard for
// this container to maintain any invariants.  
//


//
// TODO:  Error handling policy for some of the ctors
// TODO:  Safe resize()/reserve()
// TODO:  push_back()
//

class mtrk_event_t {
public:
	using value_type = unsigned char;
	using size_type = uint32_t;
	//using difference_type = std::ptrdiff_t;  // TODO:  Inconsistent
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	using iterator = mtrk_event_iterator_t;
	using const_iterator = mtrk_event_const_iterator_t;
	// TODO:  reverse_iterator, const_reverse_iterator

	// Default ctor; creates a "small" object representing a meta-text event
	// w/ a payload length of 0.  
	mtrk_event_t();
	// Ctors for callers who have pre-computed the exact size of the event 
	// and who can also supply a midi status byte (if needed).  In the first
	// overload, parameter 1 is a pointer to the first byte of the delta-time
	// field for the event.  In the second overload, parameter 1 is the value
	// of the delta time, and parameter 2 is a pointer to the first byte 
	// immediately following the delta time field (the beginning of the 
	// "event" data).  
	mtrk_event_t(const unsigned char*, uint32_t, unsigned char=0);
	mtrk_event_t(const uint32_t&, const unsigned char*, uint32_t, unsigned char=0);
	// delta_time, raw midi channel event data
	// This ctor blindly writes the values in the midi_ch_event_t struct into 
	// the object w/o any sort of validation whatsoever.  It is meant to be 
	// the fastest way to write a midi_ch_event_t into an mtrk_event_t.  May 
	// result in an invalid event w/ unexpected behavior.  
	mtrk_event_t(uint32_t, midi_ch_event_t);
	// TODO:  the field validate_mtrk_event_result_t.running_status is obtained by
	// a call to get_running status() not get_status_byte().  The meaning is not
	// quite the same; this is probably a bug.  
	mtrk_event_t(const unsigned char *p, 
		const validate_mtrk_event_result_t& ev)
		: mtrk_event_t::mtrk_event_t(p,ev.size,ev.running_status) {};
	// Copy ctor
	mtrk_event_t(const mtrk_event_t&);
	// Copy assignment; overwrites a pre-existing lhs 'this' w/ rhs
	mtrk_event_t& operator=(const mtrk_event_t&);
	// Move ctor
	mtrk_event_t(mtrk_event_t&&) noexcept;
	// Move assignment
	mtrk_event_t& operator=(mtrk_event_t&&) noexcept;
	// Dtor
	~mtrk_event_t();

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
	mtrk_event_iterator_t begin();
	mtrk_event_const_iterator_t begin() const;
	mtrk_event_iterator_t end();
	mtrk_event_const_iterator_t end() const;
	mtrk_event_const_iterator_t dt_begin() const;
	mtrk_event_iterator_t dt_begin();
	mtrk_event_const_iterator_t dt_end() const;
	mtrk_event_iterator_t dt_end();
	mtrk_event_const_iterator_t event_begin() const;
	mtrk_event_iterator_t event_begin();
	// TODO:  These call type(), which advances past the delta_t to determine
	// the status byte, then they manually advance a local iterator past the 
	// delta_t... redundant...
	mtrk_event_const_iterator_t payload_begin() const;
	mtrk_event_iterator_t payload_begin();
	iterator_range_t<mtrk_event_const_iterator_t> payload_range() const;
	iterator_range_t<mtrk_event_iterator_t> payload_range();
	const unsigned char& operator[](uint32_t) const;
	unsigned char& operator[](uint32_t);

	// Getters
	//
	smf_event_type type() const;
	uint32_t delta_time() const;
	unsigned char status_byte() const;
	// The value of the running-status _after_ this event has passed
	unsigned char running_status() const;
	uint32_t data_size() const;  // Not including the delta-t

	// Setters
	//
	uint32_t set_delta_time(uint32_t);

	bool operator==(const mtrk_event_t&) const;
	bool operator!=(const mtrk_event_t&) const;
private:
	mtrk_event_t_internal::sbo_t d_;

	// Called by the default ctor mtrk_event_t()
	// Overwrites *this w/ the default ctor'd value of a length 0 meta text
	// event.  Ignore the big/small flag of the union; do not free memory
	// if big.  
	void default_init();
	const unsigned char *raw_begin() const;
	const unsigned char *raw_end() const;
	unsigned char flags() const;
	bool is_big() const;
	bool is_small() const;

	friend class mtrk_event_unit_test_helper_t;
	friend std::string print(const mtrk_event_t&,
			mtrk_sbo_print_opts);
};

class mtrk_event_unit_test_helper_t {
public:
	mtrk_event_unit_test_helper_t(mtrk_event_t& ev) : p_(&ev) {};
	bool is_big();
	bool is_small();
	const unsigned char *raw_begin();
	const unsigned char *raw_end();
	unsigned char flags();
	mtrk_event_t *p_;
};

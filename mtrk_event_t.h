#pragma once
#include "mtrk_event_t_internal.h"
#include "..\..\generic_iterator.h"
#include "midi_raw.h"  // declares smf_event_type
#include <string>  // For declaration of print()
#include <cstdint>


// Needed for the friend dcln of 
// print(const mtrk_event_t&, mtrk_sbo_print_opts).  
// See mtrk_event_methods.h,.cpp
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
// An mtrk_event_t is a "container" for storing and working with byte
// sequences that represent MTrk events (for the purpose of this library,
// an MTrk event is defined to include the the leading delta-time vlq).  
// An mtrk_event_t stores and provides direct access (both read and 
// write) to the sequence of bytes (unsigned char) encoding the event (as 
// stipulated by the MIDI std).  
//
// Since write access to the underlying byte array is provided, it is hard 
// for this container to maintain any invariants.  
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


//
// TODO:  Error handling policy for some of the ctors
// TODO:  Safe resize()/reserve()
// TODO:  push_back()
//

struct mtrk_event_container_types_t {
	using value_type = unsigned char;
	using size_type = uint32_t;
	using difference_type = std::ptrdiff_t;  // TODO:  Inconsistent w/ size_type
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
};

class mtrk_event_t {
public:
	using value_type = mtrk_event_container_types_t::value_type;
	using size_type = mtrk_event_container_types_t::size_type;
	using difference_type = mtrk_event_container_types_t::difference_type;  // TODO:  Inconsistent
	using reference = mtrk_event_container_types_t::reference;
	using const_reference = mtrk_event_container_types_t::const_reference;
	using pointer = mtrk_event_container_types_t::pointer;
	using const_pointer = mtrk_event_container_types_t::const_pointer;
	using iterator = generic_ra_iterator<mtrk_event_container_types_t>;
	using const_iterator = generic_ra_const_iterator<mtrk_event_container_types_t>;
	// TODO:  reverse_iterator, const_reverse_iterator

	// Default ctor creates an small-capacity "empty" (zero-filled) event 
	mtrk_event_t();
	// "Empty" event consisting of a delta_time only
	mtrk_event_t(uint32_t);
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

	// Reads the delta-time, type-byte, and, as appropriate, any 
	// subsequent bytes (ex the length field in a sysex or meta event) to
	// determine the 'reported' size of the event in bytes.  Returns the 
	// _smaller_ of this quantity and capacity().  Thus, if the header for
	// a large event is written into an mtrk_event_t w/ insufficient 
	// capacity, users working with operator[] or raw pointers obtained from
	// data() will not have buffer overruns.  
	uint64_t size() const;
	uint64_t capacity() const;
	uint64_t resize(uint64_t);
	uint64_t reserve(uint64_t);

	// Iterators facilitating access to the underlying unsigned char array
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
	iterator begin();
	const_iterator begin() const;
	iterator end();
	const_iterator end() const;
	const_iterator dt_begin() const;
	iterator dt_begin();
	const_iterator dt_end() const;
	iterator dt_end();
	const_iterator event_begin() const;
	iterator event_begin();
	// TODO:  These call type(), which advances past the delta_t to determine
	// the status byte, then they manually advance a local iterator past the 
	// delta_t... redundant...
	const_iterator payload_begin() const;
	iterator payload_begin();
	iterator_range_t<const_iterator> payload_range() const;
	iterator_range_t<iterator> payload_range();
	const unsigned char& operator[](size_type) const;
	unsigned char& operator[](size_type);

	// Getters
	smf_event_type type() const;
	uint32_t delta_time() const;
	unsigned char status_byte() const;
	// The value of the running-status _after_ this event has passed
	unsigned char running_status() const;
	uint32_t data_size() const;  // Not including the delta-t
	bool verify() const;
	std::string verify_explain() const;

	// Setters
	uint32_t set_delta_time(uint32_t);

	bool operator==(const mtrk_event_t&) const;
	bool operator!=(const mtrk_event_t&) const;
private:
	mtrk_event_t_internal::sbo_t d_;

	// Called by the default ctor mtrk_event_t()
	// Overwrites *this w/ the default ctor'd value of a 'small' event
	// consisting of all zeros.  Ignores the big/small flag of the union;
	// does not free memory if big.  
	void zero_init();
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

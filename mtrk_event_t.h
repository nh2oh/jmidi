#pragma once
#include "mtrk_event_t_internal.h"
#include "midi_raw.h"  // declares smf_event_type
#include <string>  // For declaration of print()
#include <cstdint>



class mtrk_event_t;
class mtrk_event_iterator_t;
class mtrk_event_const_iterator_t;

enum class mtrk_sbo_print_opts {
	normal,
	detail,
	// Prints the value of flags_, midi_status_.  If big, prints the byte 
	// array d_ for the local container in addition to the heap-allocated
	// data.  
	debug  
};
std::string print(const mtrk_event_t&,
			mtrk_sbo_print_opts=mtrk_sbo_print_opts::normal);

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


// TODO:  If the sbo class prevented 'big' sbo's w/ capacity less than
// sizeof(small_t) - 1, i could implement mtrk_event_t.size() with
// mtrk_event_get_size_dtstart_unsafe() (instead of the 'safe' version)
// and return the lesser of the result and a call to sbo_t.capacity().  
//
// TODO:  The sbo types should not have size() methods, and big_t should
// not have a sz_ field (only uint64_t cap_).  size() is dynamic and
// should be implemented in the outter event container, not the sbo_t.  
//
// TODO:  Add typedefs so std::back_inserter() can be used.  
// TODO:  Error handling for some of the ctors
// TODO:  Ctors for channel/meta/sysex data structs
// TODO:  Safe resize()/reserve()
//
class mtrk_event_t {
public:
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
	// This ctor calls normalize() on the midi_ch_event_t parameter before
	// the data is copied into the event array; invalid values in this 
	// parameter may lead to an event with unexpected behavior.  
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


	// TODO:  All the crap in validate() should just be moved into 
	// indep. unit tests
	// bool validate() const;
	/*
	// Getters
	
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
	*/
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

	friend std::string print(const mtrk_event_t&,
			mtrk_sbo_print_opts);
};




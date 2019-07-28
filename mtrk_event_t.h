#pragma once
#include "mtrk_event_t_internal.h"
#include "..\..\generic_iterator.h"
#include "midi_raw.h"  // declares smf_event_type
#include <string>  // For declaration of print()
#include <cstdint>
#include <vector>

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



template<typename It>
struct iterator_range_t {
	It begin;
	It end;
};


//
// class mtrk_event_t
// A container for MTrk events.  
//
// An mtrk_event_t is a "container" for storing and working with byte
// sequences that represent MTrk events (for the purpose of this library,
// an MTrk event is defined to include the the leading delta-time vlq).  
// An mtrk_event_t stores and provides "direct" read-only access to the
// underlying sequence of bytes, and read/write access to event parameters
// (ex, the event's delta-time) through getter/setter methods.  
//
// All mtrk_event_t objects are valid MTrk events as stipulated by the
// MIDI std.  It is impossible to create an mtrk_event_t with an invalid
// value.  
//
// The rationale for storing MTrk events with the same byte representation
// as when serialized to a SMF, as opposed to some processed aggregate of
// platform-native types (ex, with an int32_t for the delta-time, an 
// enum for the smf_event_type, etc), is as follows:
// 1)  This is probably the most compact representation of MIDI event data
//     possible, and the manipulations needed to extract native quantites
//     are computationally trivial.  Processing a long sequence of 
//     mtrk_event_t objects is fast, since even for unusually large MIDI
//     files, the entire sequence will probaby fit in the CPU cache.  
// 2)  Most developers working with a MIDI library know something about 
//     MIDI encoding and are experienced with their own set of long-ago 
//     written (and extensively debugged) routines to read and write 
//     standard MIDI.  An mtrk_event_t presents as a simple array of 
//     unsigned char and is therefore amenable to analysis with 3rd party 
//     code.  
// 
// Invariants
// -> size() == mtrk_event_get_size_dtstart_unsafe(begin(),0x00u);
//
//

struct mtrk_event_error_t;
struct maybe_mtrk_event_t;
struct mtrk_event_debug_helper_t;/* {
	const unsigned char *raw_beg {nullptr};
	const unsigned char *raw_end {nullptr};
	unsigned char flags {0x00u};
	bool is_big {false};
};
*/
struct mtrk_event_container_types_t {
	using value_type = unsigned char;
	using size_type = int32_t;
	using difference_type = int32_t;
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

	static constexpr size_type size_max = 0x0FFFFFFF;

	// Default ctor creates Middle C (note-num==60) Note-on event
	// on channel "1" w/ velocity 60 and delta-time == 0.  
	mtrk_event_t();
	// Default-constructed value w/ the given delta-time.  
	mtrk_event_t(int32_t);
	mtrk_event_t(int32_t, midi_ch_event_t);
	mtrk_event_t(const mtrk_event_t&);
	mtrk_event_t& operator=(const mtrk_event_t&);
	mtrk_event_t(mtrk_event_t&&) noexcept;
	mtrk_event_t& operator=(mtrk_event_t&&) noexcept;
	~mtrk_event_t() noexcept;

	size_type size() const;
	size_type capacity() const;
	size_type reserve(size_type);

	const unsigned char *data() const;
	const unsigned char *data();
	const_iterator begin() const;
	const_iterator end() const;
	const_iterator begin();
	const_iterator end();
	const_iterator cbegin();
	const_iterator cend();
	const_iterator cbegin() const;
	const_iterator cend() const;
	const_iterator dt_begin() const;
	const_iterator dt_end() const;
	const_iterator event_begin() const;
	const_iterator payload_begin();
	const_iterator dt_begin();
	const_iterator dt_end();
	const_iterator event_begin();
	const_iterator payload_begin() const;
	iterator_range_t<const_iterator> payload_range() const;
	iterator_range_t<const_iterator> payload_range();
	const unsigned char operator[](size_type) const;
	unsigned char operator[](size_type);

	// Getters
	smf_event_type type() const;
	int32_t delta_time() const;
	unsigned char status_byte() const;
	// The value of the running-status _after_ this event has passed
	unsigned char running_status() const;
	size_type data_size() const;  // Not including the delta-t

	// Setters
	int32_t set_delta_time(int32_t);

	bool operator==(const mtrk_event_t&) const;
	bool operator!=(const mtrk_event_t&) const;
private:
	mtrk_event_t_internal::small_bytevec_t d_;
	
	// delta_time()==0, Note-on, channel==1, note==60 (0x3C), 
	// velocity==63 (0x3F).  
	// 63 is ~1/2 way between 0 and the max velocity of 127 (0x7F)
	// {0x00u,0x90u,0x3Cu,0x3Fu}
	void default_init(int32_t=0);

	unsigned char *push_back(unsigned char);
	iterator_range_t<const_iterator> payload_range_impl() const;

	const unsigned char *raw_begin() const;
	const unsigned char *raw_end() const;
	unsigned char flags() const;
	bool is_big() const;
	bool is_small() const;

	friend mtrk_event_t make_sysex_generic_impl(const int32_t&, unsigned char, 
					bool, const std::vector<unsigned char>&);

	friend maybe_mtrk_event_t make_mtrk_event(int32_t, const unsigned char*,
							const unsigned char*, unsigned char, 
							mtrk_event_error_t*);

	//friend std::string print(const mtrk_event_t&,
	//		mtrk_sbo_print_opts);

	friend mtrk_event_debug_helper_t debug_info(const mtrk_event_t&);
};
struct mtrk_event_debug_helper_t {
	const unsigned char *raw_beg {nullptr};
	const unsigned char *raw_end {nullptr};
	unsigned char flags {0x00u};
	bool is_big {false};
};
mtrk_event_debug_helper_t debug_info(const mtrk_event_t&);

struct mtrk_event_error_t {
	enum class errc : uint8_t {
		invalid_delta_time,
		zero_sized_input,  // beg==end
		invalid_status_byte,  // Can't determine, or ex, 0xF8, 0xFC,...
		channel_invalid_status_byte,
		channel_overflow,
		channel_calcd_length_exceeds_input,  // anticipated size() > (end-beg)
		channel_invalid_data_byte,  // non-data-byte in data section of channel event
		sysex_or_meta_overflow_in_header,
		sysex_or_meta_invalid_vlq_length,  // For meta,sysex type's w/ payload-length fields
		sysex_or_meta_calcd_length_exceeds_input,
		no_error,
		other
	};
	std::array<unsigned char,12> header;
	// The delta-time either passed in directly to overload 2, or 
	// processed from the input buffer for overload 1.  
	int32_t dt_input;
	// The running status passed in to the make_ function; the status
	// byte deduced for the event from rs and the input buffer
	unsigned char rs;
	unsigned char s;
	mtrk_event_error_t::errc code {mtrk_event_error_t::errc::other};

	void set(mtrk_event_error_t::errc, int32_t, unsigned char, unsigned char, 
			const unsigned char*, const unsigned char*);
};
struct maybe_mtrk_event_t {
	mtrk_event_t event;
	int32_t size;
	mtrk_event_error_t::errc error;// {mtrk_event_error_t::errc::other};
	operator bool() const;
};
maybe_mtrk_event_t make_mtrk_event(const unsigned char*, const unsigned char*,
					unsigned char, mtrk_event_error_t*);
maybe_mtrk_event_t make_mtrk_event(int32_t, const unsigned char*,
					const unsigned char*, unsigned char,mtrk_event_error_t*);



struct validate_channel_event_result_t {
	midi_ch_event_t data;
	int32_t size;
	mtrk_event_error_t::errc error {mtrk_event_error_t::errc::other};
	operator bool() const;
};
validate_channel_event_result_t
validate_channel_event(const unsigned char*, const unsigned char*,
						unsigned char);

struct validate_meta_event_result_t {
	const unsigned char *begin {nullptr};
	const unsigned char *end {nullptr};
	mtrk_event_error_t::errc error {mtrk_event_error_t::errc::other};
	operator bool() const;
};
validate_meta_event_result_t
validate_meta_event(const unsigned char*, const unsigned char*);

struct validate_sysex_event_result_t {
	const unsigned char *begin {nullptr};
	const unsigned char *end {nullptr};
	mtrk_event_error_t::errc error {mtrk_event_error_t::errc::other};
	operator bool() const;
};
validate_sysex_event_result_t
validate_sysex_event(const unsigned char*, const unsigned char*);





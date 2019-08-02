#pragma once
#include "mtrk_event_t_internal.h"
#include "..\..\generic_iterator.h"
#include "midi_raw.h"  // declares smf_event_type
#include "midi_delta_time.h"
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
	explicit mtrk_event_t(int32_t);
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

	template <typename InIt>
	friend maybe_mtrk_event_t *make_mtrk_event_impl(maybe_mtrk_event_t*, 
						unsigned char*, InIt, InIt, unsigned char, 
						mtrk_event_error_t *err);

	template <typename InIt>
	friend maybe_mtrk_event_t make_mtrk_event(InIt, InIt, unsigned char, 
									mtrk_event_error_t*);
	template <typename InIt>
	friend maybe_mtrk_event_t make_mtrk_event(int32_t, InIt, InIt, 
									unsigned char, mtrk_event_error_t*);

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
		no_data_following_delta_time,
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


// 
// maybe_mtrk_event_t *result has the delta-time field aready written into
// result->event.d_, dest points into result->event.d_ one past the end of 
// the dt field, and result->event.size==10.  
template <typename InIt>
maybe_mtrk_event_t *make_mtrk_event_impl(maybe_mtrk_event_t *result, 
			unsigned char *dest, InIt it, InIt end, unsigned char rs, 
			mtrk_event_error_t *err) {

	int i = 0;  // Counts the number of times 'it' has been incremented
	unsigned char uc = 0;  // The last byte extracted from 'it'

	auto set_error = [&err,&result,&rs,&i](mtrk_event_error_t::errc ec) -> void {
		result->error = ec;
		result->size = i;
		if (err) {
			err->rs = rs;
			err->code = ec;
		}
	};

	// On return, 'it' points one past the final byte of the field, and uc
	// is the final byte of the field.  This holds even for invalid fields
	// where the msb of the 4'th byte  == 1.  
	auto inl_read_vlq = [&it, &end, &i, &uc]()->int32_t {
		uint32_t uval = 0;
		int j = 0;
		while (it!=end) {
			uc = *it++;  ++i;  ++j;
			uval += uc&0x7Fu;
			if ((uc&0x80u) && (j<4)) {
				uval <<= 7;  // Note:  Not shifting on the final iteration
			} else {  // High bit not set => this is the final byte
				break;
			}
		}
		return static_cast<int32_t>(uval);
	};

	// The status byte
	if (it==end) {
		set_error(mtrk_event_error_t::errc::no_data_following_delta_time);
		return result;
	}
	uc = *it++;  ++i;
	auto s = get_status_byte(uc,rs);
	*dest++ = s;

	if (is_channel_status_byte(s)) {
		auto n = channel_status_byte_n_data_bytes(s);
		if (is_data_byte(uc)) {
			*dest++ = uc;
			n-=1;
		}
		for (int j=0; j<n; ++j) {
			if (it==end) {
				set_error(mtrk_event_error_t::errc::channel_calcd_length_exceeds_input);
				return result;
			}
			uc = *it++;  ++i;
			if (!is_data_byte(uc)) {
				set_error(mtrk_event_error_t::errc::channel_invalid_data_byte);
				return result;
			}
			*dest++ = uc;
		}
		result->event.d_.resize(dest-result->event.d_.begin());
	} else if (is_sysex_or_meta_status_byte(s)) {
		// s == 0xFF || 0xF7 || 0xF0
		if (is_meta_status_byte(s)) {
			if (it==end) {
				set_error(mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header);
				return result;
			}
			uc = *it++;  ++i;  // The Meta-type byte
			*dest++ = uc;
		}
		if (it==end) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header);
			return result;
		}
		auto len = inl_read_vlq();
		if (uc & 0x80u) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length);
			return result;
		}
		dest = write_vlq(static_cast<uint32_t>(len),dest);
		auto n_written = (dest-result->event.d_.begin());
		result->event.d_.resize(n_written+len);  // Resize may invalidate dest
		dest = result->event.d_.begin()+n_written;

		int j=0;
		for (true; (j<len && it!=end); ++j) {
			uc = *it++;  ++i;
			*dest++ = uc;
		}
		if (j!=len) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input);
			return result;
		}
	} else if (is_unrecognized_status_byte(s) || !is_status_byte(s)) {
		set_error(mtrk_event_error_t::errc::invalid_status_byte);
		return result;
	}

	result->size = (dest - result->event.d_.begin());
	result->event.d_.resize(result->size);
	result->error = mtrk_event_error_t::errc::no_error;
	return result;
};
//
// It, end point into a byte array _not_ beginning w/a delta-time.  
//
template <typename InIt>
maybe_mtrk_event_t make_mtrk_event(int32_t dt, InIt it, InIt end, 
								unsigned char rs, mtrk_event_error_t *err) {
	maybe_mtrk_event_t result;
	result.error = mtrk_event_error_t::errc::other;
	int i = 0;  // Counts the number of times 'it' has been incremented
	unsigned char uc = 0;  // The last byte extracted from 'it'

	auto set_error = [&err,&result,&rs,&i](mtrk_event_error_t::errc ec) -> void {
		result.error = ec;
		result.size = i;
		if (err) {
			err->rs = rs;
			err->code = ec;
		}
	};

	// The largest possible event "header" is 10 bytes, corresponding to
	// a meta event w/a 4-byte delta time and a 4 byte vlq length field:
	// 10 == 4-byte dt + 0xFF + type-byte + 4-byte len vlq
	result.event.d_.resize(10);
	auto dest = result.event.d_.begin();

	// The delta-time field
	if (!is_valid_delta_time(dt)) {
		set_error(mtrk_event_error_t::errc::invalid_delta_time);
		return result;
	}
	dest = write_delta_time(dt,dest);
	
	make_mtrk_event_impl(&result,dest,it,end,rs,err);
	return result;
};
//
// It, end point into a byte array beginning w/a delta-time.  
//
template <typename InIt>
maybe_mtrk_event_t make_mtrk_event(InIt it, InIt end, unsigned char rs, 
									mtrk_event_error_t *err) {
	maybe_mtrk_event_t result;
	result.error = mtrk_event_error_t::errc::other;
	int i = 0;  // Counts the number of times 'it' has been incremented
	unsigned char uc = 0;  // The last byte extracted from 'it'

	auto set_error = [&err,&result,&rs,&i](mtrk_event_error_t::errc ec) -> void {
		result.error = ec;
		result.size = i;
		if (err) {
			err->rs = rs;
			err->code = ec;
		}
	};
	// On return, 'it' points one past the final byte of the field, and uc
	// is the final byte of the field.  This holds even for invalid fields
	// where the msb of the 4'th byte  == 1.  
	auto inl_read_vlq = [&it, &end, &i, &uc]()->int32_t {
		uint32_t uval = 0;
		int j = 0;
		while (it!=end) {
			uc = *it++;  ++i;  ++j;
			uval += uc&0x7Fu;
			if ((uc&0x80u) && (j<4)) {
				uval <<= 7;  // Note:  Not shifting on the final iteration
			} else {  // High bit not set => this is the final byte
				break;
			}
		}
		return static_cast<int32_t>(uval);
	};

	// The largest possible event "header" is 10 bytes, corresponding to
	// a meta event w/a 4-byte delta time and a 4 byte vlq length field:
	// 10 == 4-byte dt + 0xFF + type-byte + 4-byte len vlq
	result.event.d_.resize(10);
	auto dest = result.event.d_.begin();

	// The delta-time field
	auto dt = inl_read_vlq();
	if (uc & 0x80u) {
		set_error(mtrk_event_error_t::errc::invalid_delta_time);
		return result;
	}
	dest = write_delta_time(dt,dest);
	
	make_mtrk_event_impl(&result, dest, it, end, rs, err);
	return result;
};




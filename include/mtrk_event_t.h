#pragma once
#include "small_bytevec_t.h"
#include "generic_iterator.h"
#include "midi_status_byte.h"
#include "midi_delta_time.h"
#include "aux_types.h"
#include <string>  // For declaration of print()
#include <cstdint>
#include <variant>
#include <optional>


namespace jmid {

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
struct mtrk_event_debug_helper_t;
class mtrk_event_t;

mtrk_event_t make_ch_event_generic_unsafe(std::int32_t, 
								const jmid::ch_event_data_t&) noexcept;

struct mtrk_event_container_types_t {
	using value_type = unsigned char;
	using size_type = int32_t;
	using difference_type = int32_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
};
struct mtrk_event_iterator_range_t {
	internal::generic_ra_const_iterator<mtrk_event_container_types_t> begin;
	internal::generic_ra_const_iterator<mtrk_event_container_types_t> end;
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
	using iterator = internal::generic_ra_iterator<mtrk_event_container_types_t>;
	using const_iterator = internal::generic_ra_const_iterator<mtrk_event_container_types_t>;
	// TODO:  reverse_iterator, const_reverse_iterator

	static constexpr size_type size_max = 0x0FFFFFFF;

	// Default ctor creates Middle C (note-num==60) Note-on event
	// on channel "1" w/ velocity 60 and delta-time == 0.  
	explicit mtrk_event_t() noexcept;
	// Default-constructed value w/ the given delta-time.  
	explicit mtrk_event_t(std::int32_t) noexcept;
	mtrk_event_t(std::int32_t, jmid::ch_event_data_t) noexcept;
	mtrk_event_t(const mtrk_event_t&);
	mtrk_event_t& operator=(const mtrk_event_t&);
	mtrk_event_t(mtrk_event_t&&) noexcept;
	mtrk_event_t& operator=(mtrk_event_t&&) noexcept;
	~mtrk_event_t() noexcept;

	size_type size() const noexcept;
	size_type capacity() const noexcept;
	size_type reserve(size_type);

	const unsigned char *data() const noexcept;
	const unsigned char *data() noexcept;
	const_iterator begin() const noexcept;
	const_iterator end() const noexcept;
	const_iterator begin() noexcept;
	const_iterator end() noexcept;
	const_iterator cbegin() noexcept;
	const_iterator cend() noexcept;
	const_iterator cbegin() const noexcept;
	const_iterator cend() const noexcept;
	const_iterator dt_begin() const noexcept;
	const_iterator dt_end() const noexcept;
	const_iterator event_begin() const noexcept;
	const_iterator payload_begin() noexcept;
	const_iterator dt_begin() noexcept;
	const_iterator dt_end() noexcept;
	const_iterator event_begin() noexcept;
	const_iterator payload_begin() const noexcept;
	mtrk_event_iterator_range_t payload_range() const noexcept;
	mtrk_event_iterator_range_t payload_range() noexcept;
	unsigned char operator[](size_type) const noexcept;
	unsigned char operator[](size_type) noexcept;

	// Getters
	//smf_event_type type() const noexcept;
	std::int32_t delta_time() const noexcept;
	unsigned char status_byte() const noexcept;
	// The value of the running-status _after_ this event has passed
	unsigned char running_status() const noexcept;
	size_type data_size() const noexcept;  // Not including the delta-t

	// If the object is not a channel event, the value that is returned 
	// will test invalid.  The exact value is unspecified.  
	jmid::ch_event_data_t get_channel_event_data() const noexcept;
	jmid::meta_header_t get_meta() const noexcept;
	jmid::sysex_header_t get_sysex() const noexcept;

	// Setters
	std::int32_t set_delta_time(std::int32_t);

	bool operator==(const mtrk_event_t&) const noexcept;
	bool operator!=(const mtrk_event_t&) const noexcept;
private:
	jmid::internal::small_bytevec_t d_;
	
	// delta_time()==0, Note-on, channel==1, note==60 (0x3C), 
	// velocity==63 (0x3F).  
	// 63 is ~1/2 way between 0 and the max velocity of 127 (0x7F)
	// {0x00u,0x90u,0x3Cu,0x3Fu}
	void default_init(std::int32_t=0) noexcept;

	// This ctor avoids calling default_init(), thus the newly ctor'd object
	// is in an invalid state (it's not a valid mtrk event).   
	struct init_small_w_size_0_t {};
	mtrk_event_t(init_small_w_size_0_t) noexcept;

	unsigned char *push_back(unsigned char);
	mtrk_event_iterator_range_t payload_range_impl() const noexcept;

	const unsigned char *raw_begin() const;
	const unsigned char *raw_end() const;
	unsigned char flags() const;
	bool is_big() const;
	bool is_small() const;

	friend mtrk_event_t jmid::make_ch_event_generic_unsafe(std::int32_t, 
								const jmid::ch_event_data_t&) noexcept;

	// delta-time, type (0x{FF,F0,F7}), meta-type, length, payload beg, payload end, 
	// add_f7_cap.  length must be consistent w/ end-beg && add_f7_cap.  
	friend mtrk_event_t make_meta_sysex_generic_unsafe(std::int32_t, unsigned char, 
		unsigned char, std::int32_t, const unsigned char *, 
		const unsigned char *, bool);

	template <typename InIt>
	friend InIt make_mtrk_event(InIt, InIt, std::int32_t, 
							unsigned char, maybe_mtrk_event_t*, 
							mtrk_event_error_t*,std::int32_t);

	template <typename InIt>
	friend InIt make_mtrk_event2(InIt, InIt, unsigned char, mtrk_event_t*, 
							mtrk_event_error_t*);

	friend mtrk_event_debug_helper_t debug_info(const mtrk_event_t&);
};
struct mtrk_event_debug_helper_t {
	const unsigned char *raw_beg {nullptr};
	const unsigned char *raw_end {nullptr};
	unsigned char flags {0x00u};
	bool is_big {false};
};
mtrk_event_debug_helper_t debug_info(const mtrk_event_t&);

mtrk_event_t make_meta_sysex_generic_unsafe(std::int32_t, unsigned char, 
		unsigned char, std::int32_t, const unsigned char *,
		const unsigned char *, bool);

struct mtrk_event_error_t {
	enum class errc : std::uint8_t {
		invalid_delta_time,
		no_data_following_delta_time,
		invalid_status_byte,  // Can't determine, or ex, 0xF8, 0xFC,...
		channel_calcd_length_exceeds_input,  // anticipated size() > (end-beg)
		channel_invalid_data_byte,  // non-data-byte in data section of channel event
		sysex_or_meta_overflow_in_header,
		sysex_or_meta_invalid_vlq_length,  // For meta,sysex type's w/ payload-length fields
		sysex_or_meta_calcd_length_exceeds_input,
		no_error,
		other
	};
	// The running status passed in to the make_ function; the status
	// byte deduced for the event from rs and the input buffer
	unsigned char rs {0x00u};
	unsigned char s {0x00u};
	mtrk_event_error_t::errc code {mtrk_event_error_t::errc::no_error};
};
// std::string print(mtrk_event_error_t::errc ec);
// If ec == mtrk_event_error_t::errc::no_error, returns an empty string
std::string print(mtrk_event_error_t::errc);
std::string explain(const mtrk_event_error_t&);
struct maybe_mtrk_event_t {
	mtrk_event_t event;
	std::ptrdiff_t nbytes_read;
	mtrk_event_error_t::errc error;
	operator bool() const;
};

struct validate_channel_event_result_t {
	jmid::ch_event_data_t data;
	std::int32_t size;
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
// make_mtrk_event() overloads
//
//
// template <typename InIt>
// InIt make_mtrk_event(InIt it, InIt end, unsigned char rs, 
//			maybe_mtrk_event_t *result, mtrk_event_error_t *err);
// template <typename InIt>
// maybe_mtrk_event_t make_mtrk_event(InIt it, InIt end, 
//							unsigned char rs, mtrk_event_error_t *err);
//
// 'it' points at the first byte of the delta-time field of an MTrk
// event.  *err may be null, *result must not be null.  
//
//
//template <typename InIt>
//InIt make_mtrk_event(InIt it, InIt end, int32_t dt, 
//						unsigned char rs, maybe_mtrk_event_t *result, 
//						mtrk_event_error_t *err, std::int32_t max_event_nbytes);
//template <typename InIt>
//maybe_mtrk_event_t make_mtrk_event(InIt it, InIt end, int32_t dt, 
//							unsigned char rs, mtrk_event_error_t *err,
//							std::int32_t max_event_nbytes);
// 
// 'it' points one past the final byte of the delta-time field of an
// MTrk event.  *err may be null, *result may not be null.  
//
// max_event_nbytes is the maximum number of bytes that result.event is 
// allowed to occupy.  This should usually be set to some large value, say
// ~ the size of the file being read in, to allow for the possibility of
// large sysex or meta text events.  If it is made too large, however, a 
// corrupted vlq length field in a meta or sysex event could cause a massive
// and unnecessary allocation for the event's payload.  
//
//
template <typename InIt>
InIt make_mtrk_event(InIt it, InIt end, unsigned char rs, 
			maybe_mtrk_event_t *result, mtrk_event_error_t *err,
			std::int32_t max_event_nbytes) {

	int i = 0;  // Counts the number of times 'it' has been incremented
	unsigned char uc = 0;  // The last byte extracted from 'it'

	// TODO:  Need to resize the event on error

	auto set_error = [&err,&result,&rs,&i](mtrk_event_error_t::errc ec) -> void {
		result->error = ec;
		result->nbytes_read = i;
		if (err) {
			err->s = 0;
			err->rs = rs;
			err->code = ec;
		}
	};

	jmid::dt_field_interpreted dtf;
	it = jmid::read_delta_time(it,end,dtf);
	i += dtf.N;
	if (!dtf.is_valid) {
		set_error(mtrk_event_error_t::errc::invalid_delta_time);
		return it;
	}

	// result->nbytes_read,event,size, etc is untouched; if the caller 
	// passed in a ptr to an uninitialized result, these values are 
	// garbage; this is ok!
	it = make_mtrk_event(it, end, dtf.val, rs, result, err, max_event_nbytes);// max_stream_bytes-i);
	result->nbytes_read += i;
	return it;
};


// 
// maybe_mtrk_event_t *result is empty; it points one past the end of a 
// dt field.  
//
// TODO:  If exiting due to error, result->event.size() is in general >
// the number of bytes written in to it; count resize() to 
// dest-event.begin() in set_error().  
//
template <typename InIt>
InIt make_mtrk_event(InIt it, InIt end, std::int32_t dt, 
						unsigned char rs, maybe_mtrk_event_t *result, 
						mtrk_event_error_t *err, std::int32_t max_event_nbytes) {
	// Initialize 'result'
	// The largest possible event "header" is 10 bytes, corresponding to
	// a meta event w/a 4-byte delta time and a 4 byte vlq length field:
	// 10 == 4-byte dt + 0xFF + type-byte + 4-byte len vlq
	// TODO:
	//if (result->event.size() < 10) {
		// If the caller has already allocated something bigger, don't
		// force a big->small transition
		result->event.d_.resize(10);
	//}
	auto dest = result->event.d_.begin();
	//result->nbytes_read = 0;
	result->error = mtrk_event_error_t::errc::other;
	int i = 0;  // Counts the number of times 'it' has been incremented
	unsigned char uc = 0;  // The last byte extracted from 'it'
	unsigned char s = 0; 

	auto set_error = [&err,&result,&rs,&s,&i](mtrk_event_error_t::errc ec) -> void {
		result->error = ec;
		result->nbytes_read = i;
		if (err) {
			err->s = s;
			err->rs = rs;
			err->code = ec;
		}
	};

	// The delta-time field
	if (!jmid::is_valid_delta_time(dt)) {
		set_error(mtrk_event_error_t::errc::invalid_delta_time);
		return it;
	}
	dest = jmid::write_delta_time(dt,dest);
	max_event_nbytes -= jmid::delta_time_field_size(dt);

	// The status byte
	if ((it==end) || (max_event_nbytes<=0)) {
		set_error(mtrk_event_error_t::errc::no_data_following_delta_time);
		return it;
	}
	uc = static_cast<unsigned char>(*it++);  ++i;
	s = jmid::get_status_byte(uc,rs);
	*dest++ = s;
	--max_event_nbytes;

	if (jmid::is_channel_status_byte(s)) {
		auto n = jmid::channel_status_byte_n_data_bytes(s);
		if (jmid::is_data_byte(uc)) {
			*dest++ = uc;
			--max_event_nbytes;
			n-=1;
		}
		for (int j=0; j<n; ++j) {
			if ((it==end) || (max_event_nbytes<=0)) {
				set_error(mtrk_event_error_t::errc::channel_calcd_length_exceeds_input);
				return it;
			}
			uc = static_cast<unsigned char>(*it++);  ++i;
			if (!jmid::is_data_byte(uc)) {
				set_error(mtrk_event_error_t::errc::channel_invalid_data_byte);
				return it;
			}
			*dest++ = uc;
			--max_event_nbytes;
		}
		result->event.d_.resize(dest-result->event.d_.begin());
	} else if (jmid::is_sysex_or_meta_status_byte(s)) {
		// s == 0xFF || 0xF7 || 0xF0
		if (jmid::is_meta_status_byte(s)) {
			if (it==end || (max_event_nbytes<=0)) {
				set_error(mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header);
				return it;
			}
			uc = static_cast<unsigned char>(*it++);  ++i;  // The Meta-type byte
			*dest++ = uc;
			--max_event_nbytes;
		}
		if ((it==end) || (max_event_nbytes<=0)) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header);
			return it;
		}
		jmid::vlq_field_interpreted lenf;
		it = jmid::read_vlq(it,end,lenf);
		i += lenf.N;
		if ((!lenf.is_valid) || (max_event_nbytes<0)) {
			// TODO:  The (max_event_nbytes<0) test is not needed
			set_error(mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length);
			return it;
		}
		/*auto len = inl_read_vlq();
		if (uc & 0x80u) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length);
			return it;
		}*/
		dest = jmid::write_vlq(static_cast<uint32_t>(lenf.val),dest);
		max_event_nbytes -= lenf.N;
		if (lenf.val > max_event_nbytes) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input);
			return it;
		}
		auto n_written = (dest-result->event.d_.begin());
		result->event.d_.resize(n_written+lenf.val);
		//std::int32_t expect_size = n_written+lenf.val;
		//result->event.d_.resize(std::min(expect_size,static_cast<std::int32_t>(max_stream_bytes+jmid::delta_time_field_size(dt))));
		dest = result->event.d_.begin()+n_written;
		for (int j=0; j<lenf.val; ++j) {
			if ((it==end) || (max_event_nbytes<=0)) {
				// TODO:  I should resize result->event to (dest-result->event.d_.begin())
				set_error(mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input);
				return it;
			}
			uc = static_cast<unsigned char>(*it++);  ++i;
			*dest++ = uc;
			--max_event_nbytes;
		}
	} else if (jmid::is_unrecognized_status_byte(s) || !jmid::is_status_byte(s)) {
		set_error(mtrk_event_error_t::errc::invalid_status_byte);
		return it;
	}

	result->event.d_.resize(dest - result->event.d_.begin());
	result->nbytes_read = i;
	result->error = mtrk_event_error_t::errc::no_error;
	return it;
};


// 
// it points at the first byte of a dt field.  
//
template <typename InIt>
maybe_mtrk_event_t make_mtrk_event(InIt it, InIt end, unsigned char rs, 
					mtrk_event_error_t *err, std::int32_t max_stream_bytes) {
	maybe_mtrk_event_t result;
	it = make_mtrk_event(it,end,rs,&result,err,max_stream_bytes);
	return result;
};
// 
// it points one past the end of a dt field.  
//
template <typename InIt>
maybe_mtrk_event_t make_mtrk_event(InIt it, InIt end, std::int32_t dt, 
							unsigned char rs, mtrk_event_error_t *err,
							std::int32_t max_stream_bytes) {
	maybe_mtrk_event_t result;
	it = make_mtrk_event(it,end,dt,rs,&result,err,max_stream_bytes);
	return result;
};


}  // namespace jmid

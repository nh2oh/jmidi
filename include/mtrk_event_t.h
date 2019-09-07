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

	mtrk_event_t(jmid::delta_time,jmid::ch_event) noexcept;
	mtrk_event_t(std::int32_t dt, jmid::ch_event_data_t md) noexcept 
		: mtrk_event_t(jmid::delta_time(dt), jmid::ch_event(md)) {};
	mtrk_event_t(jmid::delta_time, jmid::meta_header, 
					const unsigned char*, const unsigned char*);
	mtrk_event_t(jmid::delta_time, jmid::sysex_header, 
					const unsigned char*, const unsigned char*);

	mtrk_event_t(const mtrk_event_t&);
	mtrk_event_t& operator=(const mtrk_event_t&);
	mtrk_event_t(mtrk_event_t&&) noexcept;
	mtrk_event_t& operator=(mtrk_event_t&&) noexcept;
	~mtrk_event_t() noexcept;

	//
	// construct_unsafe_result construct_unsafe(...)
	//
	// Writes the delta-time and header data w/o performing any validity 
	// checks (UB if any of these values are invalid), then attempts to 
	// copy mt.length bytes from [beg,end) into the payload of the event.  
	// When copying from the range [beg,end), checks for a premature 
	// beg==end condition and stops if this occurs.  The event is _not_ 
	// resized to reflect the number of bytes actually copied.  This way, 
	// the vlq length field in the header correctly describes the number of
	// bytes in the container.  A consequence is that it is necessary to
	// return the number of bytes copied out of [beg,end) so the caller can
	// check to see that the expected number of bytes was actually copied
	// out.  There is no need to bother w/ the method conditionally writing
	// to error objects etc.  
	// 
	// TODO:  replace_unsafe()?  overwrite_unsafe()?  set_unsafe()?  
	// construct_unsafe()?
	template<typename InIt>
	struct construct_unsafe_result {
		InIt src_last;
		int32_t n_src_bytes_written;
	};
	template<typename InIt>
	construct_unsafe_result<InIt> replace_unsafe(std::int32_t dt, 
						jmid::meta_header_data mt, InIt beg, InIt end) {
		// 4 + 1 + 1 + 4 == 10; dt + 0xFF + mt-type + vlq-len
		auto dest_beg = this->d_.resize_nocopy(10);  // Probably oversized
		auto dest = jmid::write_delta_time_unsafe(dt,dest_beg);
		*dest++ = 0xFFu;
		*dest++ = mt.type;
		dest = jmid::write_vlq_unsafe(mt.length,dest);
		
		auto init_sz = dest - dest_beg;
		dest_beg = this->d_.resize(init_sz + mt.length);
		dest = dest_beg + init_sz;
		int i=0;
		while ((i<mt.length) && (beg!=end)) {
			*dest++ = *beg++;
			++i;
		}
		this->d_.resize(dest-dest_beg);
		return {beg,i};
	};
	template<typename InIt>
	construct_unsafe_result<InIt> replace_unsafe(std::int32_t dt, 
						jmid::sysex_header_data sx, InIt beg, InIt end) {
		// 4 + 1 + 4 == 10; dt + 0xF0/F7 + vlq-len
		auto dest_beg = this->d_.resize_nocopy(9);  // Probably oversized
		auto dest = jmid::write_delta_time_unsafe(dt,dest_beg);
		*dest++ = sx.type;
		dest = jmid::write_vlq_unsafe(sx.length,dest);
		
		auto init_sz = dest - dest_beg;
		dest_beg = this->d_.resize(init_sz + sx.length);
		dest = dest_beg + init_sz;
		int i=0;
		while ((i<sx.length) && (beg!=end)) {
			*dest++ = *beg++;
			++i;
		}
		this->d_.resize(dest-dest_beg);
		return {beg,i};
	};
	void replace_unsafe(std::int32_t, jmid::ch_event_data_t);
	void clear() noexcept;


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
	// is unspecified.  Note that while it will /probably/ test invalid
	// via its operator::bool(), this is not guaranteed!.  
	jmid::ch_event_data_t get_channel_event_data() const noexcept;
	jmid::meta_header_data get_meta() const noexcept;
	jmid::sysex_header_data get_sysex() const noexcept;

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

	unsigned char *push_back(unsigned char);
	mtrk_event_iterator_range_t payload_range_impl() const noexcept;

	const unsigned char *raw_begin() const;
	const unsigned char *raw_end() const;
	unsigned char flags() const;
	bool is_big() const;
	bool is_small() const;

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



}  // namespace jmid

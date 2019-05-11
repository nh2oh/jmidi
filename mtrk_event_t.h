#pragma once
#include "midi_raw.h"
#include <string>
#include <cstdint>
#include <vector>



//
// mtrk_event_t:  An sbo-featured container for mtrk events
//
// A side effect of always storing the midi-status byte applic. to the event
// in midi_status_ is that running-status events extracted from a file can 
// be stored elsewhere and interpreted correctly later.  For example, in 
// linking note pairs, pairs of corresponding on and off events are collected
// into a vector of linked events.  
//
//
//
class mtrk_event_t {
public:
	// Default ctor; creates a "small" object representing a meta-text event
	// w/ a payload length of 0.  
	mtrk_event_t();
	// Ctor for callers who have pre-computed the exact size of the event and who
	// can also supply a midi status byte if applicible, ex, an mtrk_container_iterator_t.  
	mtrk_event_t(const unsigned char*, uint32_t, unsigned char=0);
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

	// For midi events, ptr to first byte following the delta_time
	// For meta,sysex eveents, ptr to first byte following the length
	const unsigned char *payload() const;
	// Bytes of this->data()+i returned by value
	unsigned char operator[](uint32_t) const;
	// Ptr to this->data_[0] if this->is_small(), big_ptr() if is_big()
	unsigned char *data() const;
	// Ptr to this->data_[0], w/o regard to this->is_small()
	const unsigned char *raw_data() const;
	// ptr to this->flags_
	const unsigned char *raw_flag() const;
	uint32_t delta_time() const;
	smf_event_type type() const;
	uint32_t data_size() const;  // Not indluding the delta-t
	uint32_t size() const;  // Includes delta-t
	// If is_small(), reports the size of the d_ array, which is the maximum
	// size of an event that the 'small' state can contain.  
	uint32_t capacity() const;

	bool set_delta_time(uint32_t);

	struct midi_data_t {
		bool is_valid {false};
		bool is_running_status {false};
		uint8_t status_nybble {0x00u};  // most-significant nybble
		uint8_t ch {0x00u};
		uint8_t p1 {0x00u};
		uint8_t p2 {0x00u};
	};
	midi_data_t midi_data() const;
	
	bool validate() const;
	bool is_big() const;
	bool is_small() const;
private:
	enum class offs {
		ptr = 0,
		size = 0 + sizeof(unsigned char*),
		cap = 0 + sizeof(unsigned char*) + sizeof(uint32_t),
		dt = 0 + sizeof(unsigned char*) + 2*sizeof(uint32_t),
		type = 0 + sizeof(unsigned char*) + 3*sizeof(uint32_t),
		// Not really an "offset":
		max_size_sbo = sizeof(unsigned char*) + 3*sizeof(uint32_t) + 2
	};

	// 
	// Case big:  Probably meta, sysex_f0/f7, but _could_ be midi (rs or 
	// non-rs).  
	// d_ ~ { 
	//     unsigned char *;
	//     unit32_t size;
	//     uint32_t capacity;  // cum sizeof()==16
	//
	//     uint32_t delta_t.val;  // fixed size version of the vl quantity
	//     smf_event_type b21;  // 
	//     unsigned char b22;  // unused
	// };  // sizeof() => 22
	//
	//
	// Case small:  meta, sysex_f0/f7, midi (rs or non-rs), unknown.  
	// If a midi event and b1 is a valid status byte, b1 and midi_status must
	// match.  Otherwise, if a midi event and b1 is a data byte, midi_status
	// is the running-status value.  midi_status is always the value of the
	// midi status applic. to the present event, no matter what the state of
	// b1.  
	// d_ ~ {
	//     midi-vl-field delta_t;
	//     unsigned char b(delta_t.N+1);  // byte 1 following the vl dt field
	//     unsigned char b(delta_t.N+2);  // byte 2 following the vl dt field
	//     unsigned char b(delta_t.N+3);  // byte 3 following the vl dt field
	//     ...
	//     unsigned char b22;  // byte n-1 following the vl event start
	// };  // sizeof() => 22
	std::array<unsigned char,22> d_ {0x00u};
	unsigned char midi_status_ {0x00u};  // always the applic. midi status
	unsigned char flags_ {0x80u};  // 0x00u=>big; NB:  defaults to "small"
	static_assert(static_cast<uint32_t>(offs::max_size_sbo)==sizeof(d_));
	static_assert(sizeof(d_)==22);
	static_assert(static_cast<uint32_t>(offs::max_size_sbo)==22);
	// ptr, size, cap, running status
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

	unsigned char *big_ptr() const;  // getter
	unsigned char *small_ptr() const;  // getter
	unsigned char *set_big_ptr(unsigned char *p);  // setter
	uint32_t big_size() const;  // getter
	uint32_t set_big_size(uint32_t);  // setter
	uint32_t big_cap() const;  // getter
	uint32_t set_big_cap(uint32_t);  // setter
	uint32_t big_delta_t() const;  // shortcut to determining the ft if is_big()
	uint32_t set_big_cached_delta_t(uint32_t);
	smf_event_type big_smf_event_type() const;  // shortcut to determining the type if is_big()
	smf_event_type set_big_cached_smf_event_type(smf_event_type);
};

enum class mtrk_sbo_print_opts {
	normal,
	debug
};
std::string print(const mtrk_event_t&,
			mtrk_sbo_print_opts=mtrk_sbo_print_opts::normal);




//
// Meta events
//
class text_event_t {
public:
	// Creates an event w/ payload=="text event"
	text_event_t();
	explicit text_event_t(const std::string&);

	std::string text() const;
	uint32_t delta_time() const;
	uint32_t size() const;
	uint32_t data_size() const;

	// Setters return false upon failure (ex, if the caller attempts to
	// pass a value too large).  
	bool set_text(const std::string&);
	//bool set_delta_time(uint32_t);
private:
	mtrk_event_t d_;
};




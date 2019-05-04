#pragma once
#include "midi_raw.h"
#include <string>
#include <cstdint>
#include <vector>


class mecsbo2_t;
class mtrk_event_container_sbo_t;
class mtrk_view_t;
class mtrk_event_view_t;

//
// mtrk_iterator_t
//
// Obtained from the begin() && end() methods of class mtrk_view_t.  
//
// Dereferencing returns an mtrk_event_container_sbo_t, which may allocate
// if the underlying event is large enough (note that a range-for automatically
// derefs the iterator).
//
// TODO:  "Sticky" status bits w/ meta event FF 20 ???  May need modifications
// to the _unsafe_ lib funcs
//
class mtrk_iterator_t {
public:
	mecsbo2_t operator*() const;
	mtrk_iterator_t& operator++();
	//mtrk_iterator_t& operator++(int);
	bool operator==(const mtrk_iterator_t&) const;
	bool operator!=(const mtrk_iterator_t&) const;
	mtrk_event_view_t operator->() const;
private:
	// Ctor that takes a caller-specified p_ and midi status byte applicipable
	// to the _prior_ event in the sequence (arg 2); only for trusted callers 
	// (ex class mtrk_view_t)!
	mtrk_iterator_t(const unsigned char*, unsigned char);

	// ptr to first byte of the delta-time
	const unsigned char *p_ {nullptr};
	// midi status applicable to the event *prior* to this->p_;  0x00u if
	// that event was a meta or sysex_f0/f7.  See notes in operator++();
	unsigned char s_ {0x00u};

	friend class mtrk_view_t;
};

class mtrk_event_view_t {
public:
	smf_event_type type() const;
	uint32_t size() const;
	uint32_t data_length() const;
	int32_t delta_time() const;

	// Only funtional for midi events
	channel_msg_type ch_msg_type() const;
	uint8_t velocity() const;
	uint8_t chn() const;
	uint8_t note() const;
	uint8_t progn() const;
	uint8_t ctrln() const;
	uint8_t ctrlv() const;
	uint8_t p1() const;
	uint8_t p2() const;
private:
	// ptr to first byte of the delta-time
	const unsigned char *p_ {nullptr};
	// Possibly a valid midi status byte to be interpreted as the value of
	// the running-status should the event at p_ not have a status byte.  
	// _Not_ nec. the status byte applicable to the event at p_!
	unsigned char s_ {0x00u};

	friend class mtrk_iterator_t;
};







//
// mtrk_view_t
//
// An mtrk_view_t is a pointer,size pair into a valid MTrk event sequence.  
// p_ indicates the start of the MTrk chunk, so *p=='M', *++p=='T', etc.  It
// should not be possible to construct an mtrk_view_t to an underlying char 
// array that is not a valid MTrk chunk, so posession of an mtrk_view_t is an
// assurance that the underlying range is valid.  Internally, mtrk_view_t 
// takes advantage of this assurance by parsing the sequence using the fast
// but generally unsafe parse_*_usafe(const unsigned char*) family of 
// functions.  
//
// Events within MTrk event sequences are not random-accessible, since 
// events have variable size.  Even if this were not so, at each point in 
// the sequence there may be an implicit "running status" midi byte that 
// can be  determined only by parsing the the preceeding most recent midi 
// channel_voice/mode event containing a status byte.  In the worst case,
// this event may occur hundreds of bytes to the left of the MTrk event of
// interest.  Other types of implicit state infest the MTrk event sequence: 
// mid-sequence tempo and/or time-signature changes which change the meaning 
// of the delta-time fields are possible, control/program-change events 
// affect the way a given midi event should be sounded.  sysex_f0/f7 
// events may set opaque types of state.  In general, to usefully "process" 
// event n within an mtrk event sequence it is necessary to iterate through 
// the prior n-1 events and accumulate any state relevant to the procesing
// to be done.  
//
// Hence, mtrk_view_t.begin() and .end() return an iterator type defining 
// operators pre/post ++, but not +/- int, and not --.  This iterator keeps
// track of the most recent midi status byte, the smallest amount of 
// information required to determine the size of the pointed at MTrk event,
// and therefore locate the next MTrk event in the sequence.  
//
class mtrk_view_t {
public:
	mtrk_view_t(const validate_mtrk_chunk_result_t&);
	mtrk_view_t(const unsigned char*, uint32_t);
	
	uint32_t size() const;
	uint32_t data_length() const;
	
	// In one possible design, this returns a ptr to the first byte of the 
	// delta-time of the first mtrk event.  I reject this b/c for an empty 
	// mtrk (data_length()==0, this ptr is invalid.  
	// ctor mtrk_iterator_t(mtrk_view_t) relies on this behavior.  
	// TODO:  unsigned char or some sort of mtrk_event_t???
	const unsigned char *data() const;
	mtrk_iterator_t begin() const;
	mtrk_iterator_t end() const;

	bool validate() const;
private:
	const unsigned char *p_ {};  // Points at the 'M' of "MTrk..."
	uint32_t size_ {};
};
std::string print(const mtrk_view_t&);




//
//
//
class mtrk_event_container_sbo_t {  // sizeof() == 24
private:
	//
	// Of the smf_event_types, only sysex_f0/f7 & meta events can be large enough to
	// overflow the small buffer.  Hence, any midi message (channel_voice or 
	// channel_mode) will fit in the small buffer.  These messages may or may not be
	// in running_status so may or may not contain an explicit midi status byte.  
	//
	// For sysex_f0/f7 or meta events that overflow the small buffer, some of the
	// fields presently in big_t make no sense; these messges overwrite any 
	// running status, so the midi_status field makes no sense.  But, OTOH, see
	// the MIDI Channel Prefix meta event (p.138)
	// ->  big.capacity >= big.size
	//
	struct big_t {  // sizeof() == 24
		unsigned char *p;
		uint32_t size;
		uint32_t capacity;
		uint32_t dt_fixed;
		smf_event_type sevt;  // "smf_event_type"
		unsigned char midi_status;
		uint8_t unused;
		unsigned char bigsmall_flag;  // ==1u => small; ==0u => big
	};
	struct small_t {
		std::array<unsigned char, sizeof(big_t)> arry;
	};
	union bigsmall_t {
		big_t b;
		small_t s;
	};
	
	bigsmall_t d_;

	static_assert(sizeof(small_t)==sizeof(big_t),"sizeof(small_t)!=sizeof(big_t)");
	static_assert(sizeof(bigsmall_t)==sizeof(big_t),"sizeof(bigsmall_t)!=sizeof(big_t)");
	static_assert(sizeof(bigsmall_t)==24,"sizeof(bigsmall_t)!=24");

	void set_flag_big();
	void set_flag_small();
public:
	// Default ctor
	mtrk_event_container_sbo_t()=delete;
	//
	// Ctor for callers who have pre-computed the exact size of the event and who
	// can also supply a midi status byte if applicible, ex, an mtrk_container_iterator_t.  
	// 
	mtrk_event_container_sbo_t(const unsigned char*, uint32_t, unsigned char=0);
	// Copy ctor
	mtrk_event_container_sbo_t(const mtrk_event_container_sbo_t&);
	// Copy assignment; overwrites a pre-existing lhs 'this' w/ rhs
	mtrk_event_container_sbo_t& operator=(const mtrk_event_container_sbo_t&);
	// Move ctor
	mtrk_event_container_sbo_t(mtrk_event_container_sbo_t&&);
	// Move assignment
	mtrk_event_container_sbo_t& operator=(mtrk_event_container_sbo_t&&);
	// Dtor
	~mtrk_event_container_sbo_t();

	unsigned char operator[](int32_t) const;
	const unsigned char *data() const;
	bool is_small() const;
	bool is_big() const;
	// Ptr to this->data_[0], w/o regard to this->is_small()
	const unsigned char *raw_data() const;
	const unsigned char *raw_flag() const;
	int32_t delta_time() const;
	smf_event_type type() const;
	int32_t data_size() const;  // Not indluding delta-t
	int32_t size() const;  // Includes delta-t

	//uint8_t status_byte() const;
	//bool running_status() const;
};

enum class mtrk_sbo_print_opts {
	normal,
	debug
};
std::string print(const mtrk_event_container_sbo_t&,
			mtrk_sbo_print_opts=mtrk_sbo_print_opts::normal);


struct midi_extract_t {
	bool is_valid {false};
	uint8_t status_nybble {0x00u};  // MSBits of the status byte
	uint8_t ch {0xFFu};  // LSBits of the status byte
	uint8_t p1 {0xFFu};
	uint8_t p2 {0xFFu};
};
midi_extract_t midi_extract(const mtrk_event_container_sbo_t&);




















class mecsbo2_t {
public:
	// Default ctor; creates a "small" object that is essentially invalid 
	// (does not represent an mtrk event).  
	mecsbo2_t();
	// Ctor for callers who have pre-computed the exact size of the event and who
	// can also supply a midi status byte if applicible, ex, an mtrk_container_iterator_t.  
	mecsbo2_t(const unsigned char*, uint32_t, unsigned char=0);
	// Copy ctor
	mecsbo2_t(const mecsbo2_t&);
	// Copy assignment; overwrites a pre-existing lhs 'this' w/ rhs
	mecsbo2_t& operator=(const mecsbo2_t&);
	// Move ctor
	mecsbo2_t(mecsbo2_t&&);
	// Move assignment
	mecsbo2_t& operator=(mecsbo2_t&&);
	// Dtor
	~mecsbo2_t();

	// Bytes of this->data()+i returned by value
	unsigned char operator[](uint32_t) const;
	// Ptr to this->data_[0] if this->is_small(), big_ptr() if is_big()
	const unsigned char *data() const;
	// Ptr to this->data_[0], w/o regard to this->is_small()
	const unsigned char *raw_data() const;
	// ptr to this->flags_
	const unsigned char *raw_flag() const;
	uint32_t delta_time() const;
	smf_event_type type() const;
	uint32_t data_size() const;  // Not indluding delta-t
	uint32_t size() const;  // Includes delta-t

	struct midi_data_t {
		bool is_valid {false};
		bool is_running_status {false};
		uint8_t status_nybble {0x00u};  // most-significant nybble
		uint8_t ch {0x00u};
		uint8_t p1 {0x00u};
		uint8_t p2 {0x00u};
	};
	midi_data_t midi_data() const;

	bool is_big() const;
	bool is_small() const;
	bool validate() const;
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

	static_assert(static_cast<uint64_t>(offs::max_size_sbo)==sizeof(d_));
	
	void set_flag_small();
	void set_flag_big();

	unsigned char *big_ptr() const;  // getter
	unsigned char *big_ptr(unsigned char *p);  // setter
	uint32_t big_size() const;  // getter
	uint32_t big_size(uint32_t);  // setter
	uint32_t big_cap() const;  // getter
	uint32_t big_cap(uint32_t);  // setter
	uint32_t big_delta_t() const;  // shortcut to determining the ft if is_big()
	uint32_t big_delta_t(uint32_t);
	smf_event_type big_smf_event_type() const;  // shortcut to determining the type if is_big()
	smf_event_type big_smf_event_type(smf_event_type);
};

std::string print(const mecsbo2_t&,
			mtrk_sbo_print_opts=mtrk_sbo_print_opts::normal);




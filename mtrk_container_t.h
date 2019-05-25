#pragma once
#include "midi_raw.h"
#include <string>
#include <cstdint>
#include <vector>


class mtrk_event_t;
class mtrk_view_t;
class mtrk_event_view_t;

//
// raw_mtrk_iterator_t
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
class raw_mtrk_iterator_t {
public:
	mtrk_event_t operator*() const;
	raw_mtrk_iterator_t& operator++();
	//raw_mtrk_iterator_t& operator++(int);
	bool operator==(const raw_mtrk_iterator_t&) const;
	bool operator!=(const raw_mtrk_iterator_t&) const;
	mtrk_event_view_t operator->() const;
private:
	// Ctor that takes a caller-specified p_ and midi status byte applicipable
	// to the _prior_ event in the sequence (arg 2); only for trusted callers 
	// (ex class mtrk_view_t)!
	raw_mtrk_iterator_t(const unsigned char*, unsigned char);

	// ptr to first byte of the delta-time
	const unsigned char *p_ {nullptr};
	// midi status applicable to the event *prior* to this->p_;  0x00u if
	// that event was a meta or sysex_f0/f7.  See notes in operator++();
	unsigned char s_ {0x00u};

	friend class mtrk_view_t;
};


//
// Not used at present.  One idea is to make a ctor from a pair of 
// mtrk_event_iterator_t's.  This way a user processing an mtrk can inspect
// event properties w/o derefing the iterator (and possibly causing
// allocation) to get an mtrk_event_t owning-type.  OTOH, maybe a user
// who wants to do this should instead iterate over an mtrk_view_t type,
// and a user wanting the other behavior should opt to iterate over some
// type of "owning" mtrk container type:  
//
// *(mtrk_view_t::iterator == mtrk_view_iterator_t) == mtrk_event_view_t
// *(mtrk_t::iterator == mtrk_iterator_t) == mtrk_event_t
//
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

	friend class raw_mtrk_iterator_t;
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
	raw_mtrk_iterator_t begin() const;
	raw_mtrk_iterator_t end() const;

	bool validate() const;
private:
	const unsigned char *p_ {};  // Points at the 'M' of "MTrk..."
	uint32_t size_ {};
};
std::string print(const mtrk_view_t&);



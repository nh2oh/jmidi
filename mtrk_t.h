#pragma once
#include "midi_raw.h"  // midi_time_t
#include "generic_chunk_low_level.h"
#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"  // is_eot()
#include "..\..\generic_iterator.h"
#include <string>
#include <cstdint>
#include <vector>
#include <array>  // For method .get_header()
#include <type_traits>  // std::is_same<>
#include <algorithm>  // std::rotate()
#include <iterator>  // std::back_inserter

struct mtrk_error_t;
struct maybe_mtrk_t;

// Intended to represent an event occuring in an mtrk at a tk value given
// by .tk.  The onset tk of the event is .tk + .it->delta_time().  The onset
// tk of the event(s) prior to .it is tk.  
template <typename It>
struct event_tk_t {
	static_assert(std::is_same<It,typename mtrk_t::iterator>::value
		|| std::is_same<It,typename mtrk_t::const_iterator>::value);
	It it;
	int64_t tk;
};

//
// mtrk_t
// Container adapter around std::vector<mtrk_event_t>
//
// Holds a sequence of mtrk_event_t's as a std::vector<mtrk_event_t>; owns 
// the underlying data.  Provides certain convienience functions for 
// obtaining iterators into the sequence (at a specific tick number, etc).  
//
// TODO:  Check for max_size() type of overflow?  Maximum data_size
// == 0xFFFFFFFFu (?)
//
// TODO:  Use the internal type aliases for member function arg, return
// types
//
struct mtrk_container_types_t {
	using value_type = mtrk_event_t;
	using size_type = int32_t;
	using difference_type = int32_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
};

class mtrk_t {
public:
	using value_type = mtrk_container_types_t::value_type;
	using size_type = mtrk_container_types_t::size_type;
	using difference_type = mtrk_container_types_t::difference_type;
	using reference = mtrk_container_types_t::reference;
	using const_reference = mtrk_container_types_t::const_reference;
	using pointer = mtrk_container_types_t::pointer;
	using const_pointer = mtrk_container_types_t::const_pointer;
	using iterator = generic_ra_iterator<mtrk_container_types_t>;
	using const_iterator = generic_ra_const_iterator<mtrk_container_types_t>;

	// The max. allowed value of the 'length' field in the chunk header;
	// _not_ the same as the max allowed number of events.  
	static constexpr size_type length_max = std::numeric_limits<size_type>::max()-8;

	// Creates an empty MTrk event sequence:
	// nbytes() == 8, data_nbytes() == 0;
	// This will classify as invalid (!.validate()), because an MTrk 
	// sequence must terminate w/ an EOT meta event.  
	mtrk_t()=default;
	// Calls validate_chunk_header(p,max_sz) on the input, then iterates 
	// through the array building mtrk_event_t's by calling 
	// validate_mtrk_event_dtstart(curr_p,rs,curr_max_sz) and 
	// the ctor mtrk_event_t(const unsigned char*, unsigned char, uint32_t)
	// Iteration stops when the number of bytes reported in the chunk
	// header (required to be <= max_size) have been processed or when an 
	// invalid smf event is encountered.  No error checking other than
	// that provided by validate_mtrk_event_dtstart() is implemented.  
	// The resulting mtrk_t may be invalid as an MTrk event sequence.  For
	// example, it may contain multiple internal EOT events, orphan 
	// note-on events, etc.  Note that the sequence may be only partially
	// read-in if an error is encountered before the number of bytes 
	// indicated by the chunk header have been read.  
	//
	// TODO:  This should be deprecated
	mtrk_t(const unsigned char*, int32_t);
	mtrk_t(const_iterator,const_iterator);

	// The number of events in the track
	size_type size() const;
	size_type capacity() const;
	// The size in bytes occupied by the container written out (serialized)
	// as an MTrk chunk, including the 8-bytes occupied by the header.  
	// This is an O(n) operation
	size_type nbytes() const;
	// Same as .nbytes(), but excluding the chunk header
	size_type data_nbytes() const;
	// Cumulative number of midi ticks occupied by the entire sequence
	int64_t nticks() const;

	// Writes out the literal chunk header:
	// {'M','T','r','k',_,_,_,_}
	std::array<unsigned char,8> get_header() const;

	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;
	mtrk_event_t& operator[](uint32_t);
	const mtrk_event_t& operator[](uint32_t) const;
	mtrk_event_t& back();
	const mtrk_event_t& back() const;
	mtrk_event_t& front();
	const mtrk_event_t& front() const;
	// at_cumtk() returns an iterator to the first event with
	// cumtk >= the number provided, and the exact cumtk for that event.  
	// The onset tk for the event pointed to by .it is:
	// .tk + .it->delta_time();
	event_tk_t<iterator> at_cumtk(int64_t);
	event_tk_t<const_iterator> at_cumtk(int64_t) const;
	// at_tkonset() returns an iterator to the first event with onset
	// tk >= the number provided, and the exact onset tk for that event.  
	// The cumtk for the event pointed to by .it is:
	// .tk - .it->delta_time();
	event_tk_t<iterator> at_tkonset(int64_t);
	event_tk_t<const_iterator> at_tkonset(int64_t) const;

	// Returns a ref to the event just added
	mtrk_event_t& push_back(const mtrk_event_t&);
	void pop_back();
	// Inserts arg2 _before_ arg1 and returns an iterator to the newly
	// inserted event.  Note that if the new event has a nonzero delta_time
	// insertion will timeshift all downstream events by that number of 
	// ticks.  
	iterator insert(iterator, const mtrk_event_t&);
	// Insert the provided event into the sequence such that its onset tick
	// is == the cumtk at arg1 + arg2.delta_time() and such that the onset 
	// tick of all downstream events is unchanged.  The delta time of the
	// event and/or of the event immediately downstream (after insertion) 
	// may be changed (reduced).  
	// If the onset tick intended for the new event (cumtk at arg1 + 
	// arg2.delta_time()) is <= nticks() prior to insertion, nticks() is 
	// unchanged following insertion.  
	iterator insert_no_tkshift(iterator, mtrk_event_t);
	// Insert the provided event into the sequence such that its onset tick
	// is == arg1 + arg2.delta_time()
	// TODO:  Unit tests
	iterator insert(int64_t, mtrk_event_t);	
	
	// Erase the event pointed to by the iterator.  Returns an iterator to 
	// the event immediately following the erased event.  
	iterator erase(iterator);
	const_iterator erase(const_iterator);
	// Erase the event indicated by the iterator.  If the event has a delta
	// time > 0, it is added to the event immediately following the deleted
	// event.  
	iterator erase_no_tkshift(iterator);


	// Note that calling clear will cause !this.validate(), since there is
	// no longer an EOT meta event at the end of the sequence.  
	void clear();
	void resize(size_type);
	void reserve(size_type);

	// TODO:  This substantially duplicates the functionality of 
	// make_mtrk(const unsigned char*, uint32_t);
	// Could have make_mtrk() just call push_back() "blindly" on the
	// sequence then call validate() on the object.  
	struct validate_t {
		std::string error {};
		std::string warning {};
		operator bool() const;
	};
	validate_t validate() const;
private:
	// The maximum number of events allowed... should probably be 
	// something like length_max/4, not what i have here...
	static constexpr size_type capacity_max = 0x0FFFFFFF;

	std::vector<mtrk_event_t> evnts_ {};

	template <typename InIt>
	friend InIt make_mtrk(InIt, InIt, maybe_mtrk_t*, mtrk_error_t*);

};
std::string print(const mtrk_t&);
// Prints each mtrk event as hexascii (using dbk::print_hexascii()) along
// with its cumtk and onset tick.  The output is valid syntax to brace-init 
// a c++ array.  
std::string print_event_arrays(mtrk_t::const_iterator,mtrk_t::const_iterator);
std::string print_event_arrays(const mtrk_t&);

// Returns true if the track qualifies as a tempo map.  Only a certain
// subset of meta events are permitted in a tempo_map.  Does not 
// validate the mtrk.  
bool is_tempo_map(const mtrk_t&);
// bool is_equivalent_permutation_ignore_dt(...)
// bool is_equivalent_permutation(...)
// Returns true if events in the range [beg1,end1) are a permutation
// of the events in range [beg2,end2) (for every event in range 1 there
// is exactly one event in range 2 for which operator == returns true).  
bool is_equivalent_permutation_ignore_dt(mtrk_t::const_iterator,
	mtrk_t::const_iterator,mtrk_t::const_iterator,mtrk_t::const_iterator);
bool is_equivalent_permutation(mtrk_t::const_iterator,mtrk_t::const_iterator,
	mtrk_t::const_iterator,mtrk_t::const_iterator);

// Get the duration in seconds.  A midi_time_t _must_ be provided, since
// a naked MTrk object does not inherit the tpq field from the MThd chunk
// of an smf_t, and there is no standardized default value for this 
// quantity.  The value midi_time_t.uspq is updated as meta tempo events 
// are encountered in the mtrk_t.  
double duration(const mtrk_t&, const midi_time_t&);
double duration(mtrk_t::const_iterator&, mtrk_t::const_iterator&, const midi_time_t&);

// maybe_mtrk_t make_mtrk(const unsigned char*, uint32_t);
// maybe_mtrk_t make_mtrk(const unsigned char*, const unsigned char*);
// maybe_mtrk_t make_mtrk_permissive(const unsigned char*, const unsigned char*);
// If the maybe_mtrk_t object returned is invalid, the .mtrk field may
// contain a partial MTrk, probably lacking an end-of-track meta event,
// containing orphan note-on events, etc.  
struct mtrk_error_t {
	enum class errc : uint8_t {
		header_overflow,
		valid_but_non_mtrk_id,
		invalid_id,
		length_gt_mtrk_max,  // length > ...limits<int32_t>::max()-8
		invalid_event,
		no_eot_event,  // it==end but no eot encountered yet
		no_error,
		other
	};
	// This is the chunk header copied verbatim.  For a chunk w/a valid
	// ID != 'MTrk' (ie, an "unknown" chunk), this can be examined by a
	// caller of make_mtrk() to optionally read in a UChk.  
	std::array<unsigned char,8> header;
	mtrk_event_error_t event_error;
	unsigned char rs;
	mtrk_error_t::errc code;
};
std::string explain(const mtrk_error_t&);
struct maybe_mtrk_t {
	mtrk_t mtrk;
	std::ptrdiff_t nbytes_read;
	mtrk_error_t::errc error;
	operator bool() const;
};

//
// This ignores the 'length' field in the MTrk header, and only stops 
// collecting mtrk events into the array upon encountering and EOT event
// or the end of the input stream.  A 'length' value not matching the
// actual event sequence is not reported as an error.  
// 
//
template <typename InIt>
InIt make_mtrk(InIt it, InIt end, maybe_mtrk_t *result, mtrk_error_t *err) {
	std::ptrdiff_t i = 0;  // The number of bytes read from the stream
	
	std::array<unsigned char, 8> header;

	// The main loop calls make_mtrk_event(..., p_mtrk_event_error), which
	// can write directly into the error object, if approproate.  
	mtrk_event_error_t* p_mtrk_event_error = nullptr;
	if (err) {
		p_mtrk_event_error = &(err->event_error);
	}
	auto set_error = [&err,&result,&i,&header](mtrk_error_t::errc ec, 
						unsigned char rs) -> void {
		result->nbytes_read = i;
		result->error = ec;
		if (err) {
			err->header.fill(0x00u);
			err->header = header;
			err->rs = rs;
			err->code = ec;
		}
	};
	
	auto dest = header.begin();
	
	// Copy the {'M','T','r','k', a,b,c,d}
	while ((it!=end)&&(i<8)) {
		*dest++ = static_cast<unsigned char>(*it++);  ++i;
	}
	if (i!=8) {
		set_error(mtrk_error_t::errc::header_overflow,0x00u);
		return it;
	}
	
	if (!is_mtrk_header_id(header.data(),header.data()+header.size())) {
		if (is_valid_header_id(header.data(),header.data()+header.size())) {
			set_error(mtrk_error_t::errc::valid_but_non_mtrk_id,0x00u);
		} else {
			set_error(mtrk_error_t::errc::invalid_id,0x00u);
		}
		return it;
	}
	auto ulen = read_be<uint32_t>(header.data()+4,header.data()+header.size());
	if (ulen > mtrk_t::length_max) {
		set_error(mtrk_error_t::errc::length_gt_mtrk_max,0x00u);
		return it;
	}
	auto len = static_cast<int32_t>(ulen);

	// From experience with thousands of midi files encountered in the wild,
	// the average number of bytes per MTrk event is about 3.  
	auto approx_nevents = static_cast<double>(len/3.0);
	result->mtrk.evnts_.reserve(static_cast<mtrk_t::size_type>(approx_nevents));

	bool found_eot = false;
	unsigned char rs = 0x00u;  // value of the running-status
	maybe_mtrk_event_t curr_event;
	while (it!=end && !found_eot) {
		it = make_mtrk_event(it,end,rs,&curr_event,p_mtrk_event_error);
		i += curr_event.nbytes_read;
		if (!curr_event) {
			set_error(mtrk_error_t::errc::invalid_event,rs);
			return it;
		}
		
		if (!found_eot) {
			found_eot = is_eot(curr_event.event);
		}
		rs = curr_event.event.running_status();
		result->mtrk.evnts_.push_back(std::move(curr_event.event));
	}
	if (!found_eot) {
		set_error(mtrk_error_t::errc::no_eot_event,rs);
		return it;
	}

	result->error = mtrk_error_t::errc::no_error;
	result->nbytes_read = i;
	return it;
};

template <typename InIt>
maybe_mtrk_t make_mtrk(InIt it, InIt end, mtrk_error_t *err) {
	maybe_mtrk_t result;
	it = make_mtrk(it,end,&result,err);
	return result;
};

template<typename OIt>
OIt write_mtrk(const mtrk_t& mtrk, OIt it) {
	std::array<char,4> h {'M','T','r','k'};
	it = std::copy(h.begin(),h.end(),it);
	it = write_32bit_be(static_cast<uint32_t>(mtrk.data_nbytes()), it);
	for (const auto& ev : mtrk) {
		for (const auto& b : ev) {
			*it++ = b;
		}
	}
	return it;
};



/*
maybe_mtrk_t make_mtrk_permissive(const unsigned char*, const unsigned char*,
									mtrk_error_t*);
maybe_mtrk_t make_mtrk_permissive(const unsigned char*, const unsigned char*);
*/


//
// mtrk_from_event_seq_result_t
// make_mtrk_from_event_seq_array(const unsigned char *beg, 
//						const unsigned char *end, mtrk_t *dest);
// beg points at the first byte of an mtrk event (_not_ at the first byte
// of an MTrk chunk header) and end points one byte past the end of the last
// event in the sequence.  Returns p pointing one byte past the end of the
// last byte pushed_back() into dest, along w/ the value of the 
// running-status byte.  
// Calls curr_event = make_mtrk_event(p,...) on each event.  
// If no error is indicated, calls 
// dest->push_back(mtrk_event_t(p,curr_event));
// Terminates when any one of the following occurs:
// -> if validate_mtrk_event_dtstart() indicates an error
// -> when the end of the input range has been reached
// -> immediately after an EOT event is pushed_back()
//
struct mtrk_from_event_seq_result_t {
	const unsigned char *p {nullptr};
	unsigned char rs {0x00u};
};
mtrk_from_event_seq_result_t make_mtrk_from_event_seq_array(const unsigned char*, 
						const unsigned char*,mtrk_t*);

//
// get_simultaneous_events(mtrk_const_iterator_t beg,
//							mtrk_const_iterator_tend);
//
// Returns an iterator to one past the last event with onset tick == that
// of beg.  
// TODO:  Another possible meaning of "simultaneous" is all events w/
// tk onset < the tk onset of the off-event matching beg.  
mtrk_t::const_iterator get_simultaneous_events(mtrk_t::const_iterator, mtrk_t::const_iterator);
mtrk_t::iterator get_simultaneous_events(mtrk_t::iterator, mtrk_t::iterator);

//
// find_linked_off(mtrk_const_iterator_t beg, mtrk_const_iterator_t end,
//						mtrk_event_t on);
// Find the first off event on [beg,end) matching the mtrk_event_t "on" 
// event arg mtrk_event_t on.  
// 
// Returns an event_tk_t<mtrk_const_iterator_t> where member .it is an 
// iterator to the corresponding off mtrk_event_t, and .tk is the cumulative
// number of ticks occuring on the interval [beg,.it).  It is the cumulative 
// number of ticks starting from beg and and continuing to the event
// immediately _prior_ to event .it (the onset tick of event .it is thus
// .tk + .it->delta_time()).  
// If on is not a note-on event, .it==end and .tk==0.  
// If no corresponding off event can be found, .it==end and .tk has the
// same interpretation as before.  
//
event_tk_t<mtrk_t::const_iterator> find_linked_off(mtrk_t::const_iterator, 
					mtrk_t::const_iterator, const mtrk_event_t&);

//
// get_linked_onoff_pairs()
// Find all the linked note-on/off event pairs on [beg,end)
//
// For each linked pair {on,off}, the field cumtk (on.cumtk, off.cumtk) is 
// the cumulative number of ticks immediately prior to the event pointed 
// at by the iterator in field ev (on.ev, off.ev).  For a 
// linked_onoff_pair_t p,
// the onset tick for the on event is p.on.cumtk + p.on.ev->delta_time(),
// and similarly for the onset tick of the off event.  The duration of
// the note is:
// uint32_t duration = (p.on.cumtk+p.on.ev->delta_time()) 
//                     - (p.off.cumtk+p.off.ev->delta_time());
//
// Orphan note-on events are not included in the results.  
//
struct linked_onoff_pair_t {
	event_tk_t<mtrk_t::const_iterator> on;
	event_tk_t<mtrk_t::const_iterator> off;
};
std::vector<linked_onoff_pair_t>
	get_linked_onoff_pairs(mtrk_t::const_iterator,mtrk_t::const_iterator);

//
// Print a table of linked note-on/off event pairs in the input mtrk_t.  
// Orphan note-on and note-off events are skipped (same behavior as 
// get_linked_onoff_pairs().  
std::string print_linked_onoff_pairs(const mtrk_t&);


//
// OIt split_copy_if(InIt beg, InIt end, OIt dest, UPred pred)
//
// Copy events on [beg,end) for which pred() into dest.  Adjust the delta
// time of each copied event such that in the new track the onset tk of 
// each event is the same as in the original.  Note that the cumtk of the
// events in the new track will in general differ from their values in the
// original track.  
//
//
template<typename InIt, typename OIt, typename UPred>
OIt split_copy_if(InIt beg, InIt end, OIt dest, UPred pred) {
	uint64_t cumtk_src = 0;
	uint64_t cumtk_dest = 0;
	for (auto curr=beg; curr!=end; ++curr) {
		if (pred(*curr)) {
			auto curr_cpy = *curr;
			curr_cpy.set_delta_time(curr_cpy.delta_time() + (cumtk_src-cumtk_dest));
			*dest++ = curr_cpy;
			cumtk_dest += curr_cpy.delta_time();
		}
		cumtk_src += curr->delta_time();
	}
	return dest;
};
template <typename UPred>
mtrk_t split_copy_if(const mtrk_t& mtrk, UPred pred) {
	auto result = mtrk_t();
	result.reserve(mtrk.size()/2);
	split_copy_if(mtrk.begin(),mtrk.end(),std::back_inserter(result),pred);
	return result;
};

//
// FwIt split_if(FwIt beg, FwIt end, UPred pred)
//
// Partitions the mtrk events in [beg,end) such that events matching pred
// appear at the beginning of the range, preserving relative order of the 
// events.  
// Returns an iterator to one-past the last event matching pred.  The delta 
// times for events on both of the resulting ranges [beg,result_it), 
// [result_it,end) are adjusted so that each event has the same onset tk as 
// in the original range.  
//
// This is similar to std::remove_if(), but adjusts event delta times.  
//
template<typename FwIt, typename UPred>
FwIt split_if(FwIt beg, FwIt end, UPred pred) {
	uint64_t cumtk_src = 0;
	uint64_t cumtk_dest = 0;
	for (auto curr=beg; curr!=end; ++curr) {
		auto curr_dt = curr->delta_time();
		if (pred(*curr)) {
			curr->set_delta_time(curr_dt + (cumtk_src-cumtk_dest));
			cumtk_dest += curr->delta_time();

			auto next = curr+1;
			beg = std::rotate(beg,curr,next);
			if (next != end) {
				next->set_delta_time(next->delta_time()+curr_dt);
			}
		} else {
			// Event *curr has been removed, so no longer contributes to the 
			// src cumtk; curr_dt was added to the delta time for the next 
			// (curr+1) event & will be accounted for in the next iteration.  
			cumtk_src += curr_dt;
		}
	}
	return beg;
};
template <typename UPred>
mtrk_t split_if(mtrk_t& mtrk, UPred pred) {
	auto it = split_if(mtrk.begin(),mtrk.end(),pred);

	auto result = mtrk_t(mtrk.begin(),it);
	mtrk = mtrk_t(it,mtrk.end());
	return result;
};

//
// OIt merge(InIt beg1, InIt end1, InIt beg2, InIt end2, OIt dest)
//
// Merges the sequence of mtrk events on [beg1,end1) with the sequence on
// [beg2,end2) into dest.  Delta times are adjusted so that the onset tk for 
// each event in the merged sequence is the same as in the input 
// (un-merged) sequence.  Events are merged in "blocks" of events with the
// same onset tk.  This ensures that sets of simultaneous events from each 
// range are kept adjacent, uninterrupted by events from the other range 
// which nonetheless occur simultaneously (have the same onset tick) in the 
// stream.  If an event block in range 1 has the same onset tk as an event 
// range in range 2, the block in range 1 is merged first.  
//
template<typename InIt, typename OIt>
OIt merge(InIt beg1, InIt end1, InIt beg2, InIt end2, OIt dest) {
	uint64_t ontk_1 = 0;
	if (beg1 != end1) {
		ontk_1 = beg1->delta_time();
	}
	uint64_t ontk_2 = 0;
	if (beg2 != end2) {
		ontk_2 = beg2->delta_time();
	}
	uint64_t cumtk_dest = 0;
	auto curr1 = beg1;  auto curr2 = beg2;
	uint64_t ontk_curr = 0;
	InIt curr_beg = beg1;  InIt curr_end = beg1;
	while ((curr1!=end1) || (curr2!=end2)) {
		// Set curr_beg, curr_end, ontk_curr to the appropriate values for
		// the appropriate stream based on curr1/2 and ontk_1/2.  
		// Prior to this conditional block,
		// -> [curr_beg, curr_end) point to the range in either stream 1 or
		//    2 that has just been written into dest.
		// -> ontk_curr is the onset tk of the event block 
		//    [curr_beg,curr_end).  
		// -> curr1/2 and ontk_1/2 point at the _next_ (yet to be written 
		//    into dest) event in the corresponding stream.  
		// After this conditional block, 
		// -> curr_beg points at the next event intended for dest; its 
		//    desired onset tk is ontk_curr.  
		// -> curr_end points one past the last event (in the same input
		//    stream as curr_beg) w/ the same onset tk as curr_beg.  
		// -> curr1/2 and ontk_1/2 have been advanced/incremented to indicate  
		//    the _next_ event in the respective input stream (==curr_end),
		//    ie, the event to follow the block [curr_beg,curr_end).  
		if (curr1!=end1 && (ontk_1<=ontk_2 || curr2==end2)) {
			ontk_curr = ontk_1;  curr_beg = curr1;
			curr_end = get_simultaneous_events(curr1,end1);
			curr1 = curr_end;
			if (curr1 != end1) {
				ontk_1 += curr1->delta_time();
			}
		} else if (curr2!=end2 && (ontk_1>ontk_2 || curr1==end1)) {
			ontk_curr = ontk_2;  curr_beg = curr2;
			curr_end = get_simultaneous_events(curr2,end2);
			curr2 = curr_end;
			if (curr2 != end2) {
				ontk_2 += curr2->delta_time();
			}
		}
		while (curr_beg!=curr_end) {
			auto curr_ev_cpy = *curr_beg;
			curr_ev_cpy.set_delta_time(ontk_curr - cumtk_dest);
			*dest = curr_ev_cpy;
			cumtk_dest += (ontk_curr - cumtk_dest);
			++dest;
			++curr_beg;
		}
	}
	return dest;
};




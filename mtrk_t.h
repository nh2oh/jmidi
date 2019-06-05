#pragma once
#include "mtrk_event_t.h"
#include "mtrk_iterator_t.h"
#include <string>
#include <cstdint>
#include <vector>
#include <array>

struct maybe_mtrk_t;
class mtrk_t;

//
// mtrk_t
//
// Holds a sequence of mtrk_event_t's; owns the underlying data.  Stores 
// the event sequence as a std::vector<mtrk_event_t>.  Also caches certain
// properties of the sequence to allow faster access, ex the total number
// of ticks spanned by the sequence (member cumdt_).  
//
// Because the conditions on a valid MTrk event sequence are complex and 
// expensive to maintain for operations such as push_back(), insert() etc,
// an mtrk_t may not serialize to a valid MTrk chunk.  For example, there
// may be multiple EOT events within the event sequence (or a terminating
// EOT event may be missing), there may be orphan note-on events, etc.  
// The method validate() returns a summary of the problems w/ the object.  
// Disallowing "invalid" states would make it prohibitively cumbersome to
// provide straightforward, low-overhead editing functionality to users.
// A user _has_ to be able to, for example, call push_back() to add note-on
// and note_off events one at a time.  If I were to require the object to
// always be serializable to a valid MTrk, this would be impossible since 
// adding a note-on event before the corresponsing note-off creates an
// orphan on-event situation (as well as a "missing EOT" error).  
//
// Note that a default-constructed mtrk_t is empty (nevents()==0, 
// data_size()==0) and .validate() will return false (no EOT event);
// empty mtrk_t's do not represent serializable MTrk chunks.  
//
// Maintains the following invariants:
// -> No events in the sequence have .type()==smf_event_type::invalid
// -> cumtk_, data_size_ match the event sequence evnts_.  
//
// TODO:  Need some sort of private fn to convert to/from std::vector 
// iterators.  
// TODO:  Method size() returns a size in bytes not a number-of-events,
// which might be confusing, esp since capacity()
// TODO:  Check for max_size() type of overflow?  Maximum data_size
// == 0xFFFFFFFFu (?)
// TODO:  Things like non-const operator[], begin(), end() make it
// impossible to maintain the invariants.  
// These methods could set a "maybe_dirty_" bit, which could force
// reverification of the invariants when needed.  
// TODO:  The zillion is_linked_off() lambda's implemented everywhere
// need to be made into a free function on mtrk_event_t.  
//
//
class mtrk_t {
public:
	// Creates an empty MTrk event sequence:
	// size() == 8, data_size() == 0;
	// This will classify as invalid (!.validate()), because an MTrk 
	// sequence must terminate w/ an EOT meta event.  
	mtrk_t()=default;
	// Calls validate_chunk_header(p,max_sz) on the input, then iterates 
	// through the array building mtrk_event_t's by calling 
	// validate_mtrk_event_dtstart(curr_p,rs,curr_max_sz) and 
	// the ctor mtrk_event_t(const unsigned char*, unsigned char, uint32_t)
	// Iteration stops when arg2 bytes have been processed or when an 
	// invalid smf event is encountered.  No error checking other than
	// that provided by validate_mtrk_event_dtstart() is implemented.  
	// The resulting mtrk_t may be invalid as an MTrk event sequence.  For
	// example, it may contain multiple internal EOT events, orphan 
	// note-on events, etc.  this->data_size_ is _not_ blindly taken from
	// the chunk header; it will reflect the actual size occupied by the
	// event sequence, ex, if an invalid event is encountered before 
	// processing arg2 bytes.  
	mtrk_t(const unsigned char*, uint32_t);

	// The size in bytes occupied by the container written out (serialized)
	// as an MTrk chunk, including the 8-bytes occupied by the header.  
	uint32_t size() const;
	uint32_t data_size() const;
	uint32_t nevents() const;
	// Writes out the literal chunk header:
	// {'M','T','r','k',_,_,_,_}
	std::array<unsigned char,8> get_header() const;

	mtrk_iterator_t begin();
	mtrk_iterator_t end();
	mtrk_const_iterator_t begin() const;
	mtrk_const_iterator_t end() const;
	mtrk_event_t& operator[](uint32_t);
	const mtrk_event_t& operator[](uint32_t) const;

	// Returns false if the event could not be added, ex if it's an
	// smf_event_type::invalid.  
	bool push_back(const mtrk_event_t&);
	void pop_back();
	// Inserts arg2 _before_ arg1 and returns an iterator to the newly
	// inserted event.  If arg2.type()==smf_event_type::invalid, does
	// not insert, and returns this->end().  
	mtrk_iterator_t insert(mtrk_iterator_t, const mtrk_event_t&);
	// Note that calling clear will cause !this.validate(), since there is
	// no longer an EOT meta event at the end of the sequence.  
	void clear();
	
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

	friend maybe_mtrk_t make_mtrk(const unsigned char*, uint32_t);
private:
	uint32_t data_size_ {0};
	uint64_t cumtk_ {0};
	std::vector<mtrk_event_t> evnts_ {};
};
std::string print(const mtrk_t&);


// Declaration matches the in-class friend declaration to make the 
// name visible for lookup outside the class.  
maybe_mtrk_t make_mtrk(const unsigned char*, uint32_t);
struct maybe_mtrk_t {
	std::string error {"No error"};
	mtrk_t mtrk;
	operator bool() const;
};

// Returns an iterator to one past the last simultanious event 
// following beg.  
mtrk_iterator_t get_simultanious_events(mtrk_iterator_t, mtrk_iterator_t);


//
// find_linked_off(mtrk_const_iterator_t beg, mtrk_const_iterator_t end,
//						mtrk_event_t on);
// Find an off event on [beg,end) matching the mtrk_event_t "on" event 
// given by arg mtrk_event_t on.  
// 
// Returns an mtrk_event_cumtk_t where member .ev is an iterator to the
// corresponding off mtrk_event_t, and .cumtk is the cumulative number
// of ticks occuring on the interval [beg,.ev).  It is the cumulative 
// number of ticks starting from beg and and continuing to the event
// immediately _prior_ to event .ev (the onset tick of event .ev is thus
// .cumtk + .ev->delta_time()).  
// If on is not a note-on event, .ev==end and .cumtk==0.  
// If no corresponding off event can be found, .ev==end and .cumtk has the
// same interpretation as before.  
//
struct mtrk_event_cumtk_t {
	uint32_t cumtk;  // cumtk _before_ event ev
	mtrk_const_iterator_t ev;
};
mtrk_event_cumtk_t find_linked_off(mtrk_const_iterator_t, 
					mtrk_const_iterator_t, const mtrk_event_t&);

//
// get_linked_onoff_pairs()
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
	mtrk_event_cumtk_t on;
	mtrk_event_cumtk_t off;
};
std::vector<linked_onoff_pair_t>
	get_linked_onoff_pairs(mtrk_const_iterator_t,mtrk_const_iterator_t);
std::string print_linked_onoff_pairs(const mtrk_t&);



// First draft
template<typename InIt, typename UPred, typename FIntegrate>
std::pair<InIt,typename FIntegrate::value_type> accumulate_to(InIt beg,
								InIt end, UPred pred, FIntegrate func) {
	std::pair<InIt,typename FIntegrate::value_type> res;
	for (res.ev=beg; (res.ev!=end && !pred(res.ev)); ++res.ev) {
		res.first = func(res.first,res.ev);
	}
	return res;
};




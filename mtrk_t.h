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
// Holds an mtrk; owns the underlying data.  Stores the event sequence as
// a std::vector<mtrk_event_t>.  Also caches certain properties of the
// sequence to allow faster access, ex the total number of ticks (member
// cumdt_).  
// Maintains the following invariants:
// -> No events in the sequence have .type()==smf_event_type::invalid
// -> cumtk_, data_size_ matches the event sequence evnts_.  
//
// TODO:  Need some sort of private fn to convert to/from std::vector 
// iterators.  
// TODO:  Method size() returns a size in bytes not a number-of-events,
// which might be confusing, esp since capacity()
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

	bool push_back(const mtrk_event_t&);
	void pop_back();
	// Insert arg2 _before_ arg1 and returns an iterator to the newly
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

// TODO:  Really just an mtrk_event_t w/ an integrated delta-time
struct orphan_onoff_t {
	uint32_t cumtk;
	mtrk_event_t ev;
};
struct linked_onoff_pair_t {
	uint32_t cumtk_on;
	mtrk_event_t on;
	uint32_t cumtk_off;
	mtrk_event_t off;
};
struct linked_and_orphan_onoff_pairs_t {
	std::vector<linked_onoff_pair_t> linked {};
	std::vector<orphan_onoff_t> orphan_on {};
	std::vector<orphan_onoff_t> orphan_off {};
};
linked_and_orphan_onoff_pairs_t get_linked_onoff_pairs(mtrk_const_iterator_t,
					mtrk_const_iterator_t);
std::string print(const linked_and_orphan_onoff_pairs_t&);

//
// An alternate design, possibly more general, & w/o question
// more lightweight
//
// Args:  Iterator to the first event of the sequence, iterator to one past
// the end of the sequence, the on-event of interest.  
// Returns an mtrk_event_cumtk_t where member .ev points to the linked off 
// event and .cumtk is the cumulative number of ticks occuring between 
// [beg,.ev).  It is the cumulative number if ticks past immediately _before_
// event .ev (the onset tick of event .ev is .cumtk + .ev->delta_time()).  
// If no corresponding off event can be found, .ev==end and .cumtk has the
// same interpretation as before.  
struct mtrk_event_cumtk_t {
	uint32_t cumtk;  // cumtk _before_ event ev
	mtrk_const_iterator_t ev;
};
mtrk_event_cumtk_t find_linked_off(mtrk_const_iterator_t, mtrk_const_iterator_t,
			const mtrk_event_t&);
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




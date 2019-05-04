#pragma once
#include "midi_raw.h"
#include "mthd_container_t.h"
#include "mtrk_container_t.h"
#include <string>
#include <cstdint>
#include <vector>


//
// smf_t
//
// Holds an smf; owns the underlying data.  Splits each chunk into its
// own std::vector<unsigned char>
//
//
// Alternate design:  Can hold mthd_view_t, mtrk_view_t objects as members.
//
class smf_chrono_iterator_t;

class smf_t {
public:
	smf_t(const validate_smf_result_t&);

	uint32_t size() const;
	uint16_t nchunks() const;
	uint16_t format() const;  // mthd alias
	uint16_t division() const;  // mthd alias
	uint32_t mthd_size() const;  // mthd alias
	uint32_t mthd_data_length() const;  // mthd alias
	uint16_t ntrks() const;
	std::string fname() const;

	mtrk_view_t get_track_view(int) const;
	mthd_view_t get_header_view() const;

	// Returns mtrk events in order by tonset
	smf_chrono_iterator_t event_iterator_begin() const;  // int track number
	smf_chrono_iterator_t event_iterator_end() const;  // int track number
private:
	std::string fname_ {};
	std::vector<std::vector<unsigned char>> d_ {};  // All chunks, incl ::unknown
};
std::string print(const smf_t&);

// Draft of the the chrono_iterator
std::string print_events_chrono(const smf_t&);
// Uses the iterator
std::string print_events_chrono2(const smf_t&);





//
// smf_chrono_iterator_t
//
// Returned from smf_t.event_iterator_begin().  Returns all the mtrk_events
// in the smf_t in chronological order, ie, w/o regard to the track number
// in which the event is encoded (format 1 files).  Internally, holds a 
// std::vector<track_state_t>, which, for each track in the collection, 
// holds an end iterator, an iterator to the _next_ event scheduled on that
// track, and the cumulative tick value, which is the onset time (tick) 
// corresponding to the _previous_ event in the track (one event prior to 
// the next-event iterator).  The event indicated by an iterator object, ie,
// the mtrk event that the iterator is currently "pointing to" is encoded 
// implictly:  it is the event for which the quantity 
// tonset = trks_[i].cumtk + (*trks_[i].next).delta_time()
// is minimum.  
// This has to me calculated for all i on every call to operator++() and
// operator*().  
//
// In an alternate design, which is probably simpler, trks_[i].cumtk holds 
// the tonset for trks_[i].next.  When trks_[i].next==trks_[i].end, however,
// trks_[i].cumtk has a different meaning.  
//
// TODO:  MIDI tracks are indeiendent midi streams; events on diff mtrks do
// not crosstalk.  
//
// TODO:  Need to incorporate an understanding of tick units and respond
// to meta events that change those units.  
//
// TODO:  Don't intersperse events for format 2 files
//
// How "attached" should this be to class smf_t?  As written, will work to 
// intersperse events from any collection of mtrk_view_t's.  
//
//
//


struct track_state_t {
	mtrk_iterator_t next;
	mtrk_iterator_t end;
	int32_t cumtk;  // The time at which the _previous_ event on the track initiated
};
struct smf_event_t {
	uint16_t trackn;
	int32_t tick_onset;  // cumulative (TODO: ?)
	mecsbo2_t event;
};
class smf_chrono_iterator_t {
public:
	smf_event_t operator*() const;
	smf_chrono_iterator_t& operator++();
	//smf_chrono_iterator_t& operator++(int);
	bool operator==(const smf_chrono_iterator_t&) const;
	bool operator!=(const smf_chrono_iterator_t&) const;
private:
	smf_chrono_iterator_t(const std::vector<track_state_t>&);  // used by smf_t

	struct present_event_t {
		uint16_t trackn;
		int32_t tick_onset;
	};
	present_event_t present_event() const;
	std::vector<track_state_t> trks_;

	friend class smf_t;
};









//
// Operations
//
// Print a list of on and off times for each note in each track in the file 
// in order of tick-onset:
//
//
//
std::string print_tied_events(const smf_t&);


struct linked_event_t {
	// -1 is used to signify "empty" or "not found" for the orphan_on/off
	// members of linked_events_result_t.  
	uint16_t trackn {0};
	int32_t tk_on {-1};  // cumulative
	int32_t tk_off {-1};
	mecsbo2_t event_on {};
	mecsbo2_t event_off {};
};
struct linked_note_events_result_t {
	std::vector<linked_event_t> linked {};
	std::vector<linked_event_t> orphan_on {};
	std::vector<linked_event_t> orphan_off {};
};
bool example_is_on_event(const mecsbo2_t&);
bool example_is_off_event(const mecsbo2_t&);
bool default_is_linked_pair(const linked_event_t&, const linked_event_t&);
template <typename F_is_on_event, typename F_is_off_event, typename F_is_linked_pair>
linked_note_events_result_t link_note_events(const smf_t& smf, F_is_on_event is_on, F_is_off_event is_off) {
	struct unlinked_event_t {
		uint16_t trackn {0};
		int32_t tk {0};
		mecsbo2_t event;
	};
	// Holds data for all note-on midi events for which a corresponing note-off
	// event has not yet occured.  
	std::vector<linked_event_t> orphan_on {};
	// Holds midi data for any note-off events unable to be matched w/a 
	// corresponding note-on event.  
	std::vector<linked_event_t> orphan_off {};
	// Holds all the linked events
	std::vector<linked_event_t> linked {};

	//mecsbo2_t curr_off_ev;
	smf_event_t curr_smf_event;
	auto matching_on_event = [&curr_smf_event](const linked_event_t& on_ev)->bool {
		// NB:  If there is >1 note-on event (perhaps @ different dt) for a single
		// ch,note pair w/o any intervening note-off events, followed at some point 
		// by 1 or more matching note-off events, this function will match the
		// _first_ note-off event w/ the _first_ note-on event and the _second_ note-on
		// w/ the _second_ note off and so on.  It is ambiguous what the user intends in
		// such a situation.  
		return (on_ev.trackn==curr_smf_event.trackn
			&& F_is_linked_pair(curr_off_ev,on_ev);
	};

	auto smf_end = smf.event_iterator_end();
	for (auto smf_it=smf.event_iterator_begin(); smf_it!=smf_end; ++smf_it) {
		curr_smf_event = *smf_it;

		if (F_is_on_event(curr_smf_event.event)) {
			// curr_smf_event is an "on" event; add it to orphan_on.  
			orphan_on.push_back(curr_onoff_ev);
		} else if (F_is_off_event(curr_smf_event.event)) {
			// curr_smf_event is an "off" event; try to find a matching "on"
			// event in orphan_on.  Lambda matching_on_event captures 
			// curr_smf_event by ref and takes a const linked_event_t& as
			// its only arg.  
			auto it_matched_on_ev = 
				std::find_if(orphan_on.begin(),orphan_on.end(),matching_on_event);
			if (it_matched_on_ev==orphan_on.end()) {
				// curr_smf_event is an "off" event but does not match any
				// entry in orphan_on.  Weird.  
				orphan_off.push_back({curr_smf_event.trackn,
					curr_smf_event.tick_onset,curr_smf_event.event});
			} else {
				// The event in orphan_on pointed to by it_matched_on_ev matches
				// the off-event curr_smf_event.  Add the event pair to linked and 
				// delete the entry in orphan_on pointed to by it_matched_on_ev.
				linked.push_back({curr_smf_event.trackn,
					*it_matched_on_ev.tk,  // tk_on
					curr_smf_event.tick_onset,  // tk_off
					*it_matched_on_ev.event,  // event_on
					curr_smf_event.event});  // event_off

				orphan_on.erase(it_matched_on_ev);
			}
		} else if (curr_ev_midi_data.status_nybble==0xB0u 
			&& curr_ev_midi_data.p1==0x7Du) {  // p1==0x7D => all notes off
			// Find all entries in sounding for which trackn==curr_sounding.trackn;
			// print the event w/ curr_sounding as the value for the note-off
			// event, then delete the entry in sounding.  The complexity here arises
			// b/c calling sounding.erase(it-to-on-event) invalidates the iterator 
			// it-to-on-event pointing to the on event i just printed and now wish
			// to delete.  
			while (true) {
				std::vector<onoff_event_t>::iterator it;
				bool found_event = false;
				for (it=sounding.begin(); it!=sounding.end(); ++it) {
					if ((*it).trackn==curr_onoff_ev.trackn) {
						found_event = true;
						linked.push_back(make_linked(*it,curr_onoff_ev));
						sounding.erase(it);  // Invalidates it
						break;
					}
				}
				if (!found_event) {
					break;
					// Don't do:  if (it==sounding.end());
					// calling erase() may => true even if events still exist
				}
			}  // while (true)
		}  // else if (curr_ev_midi_data.status_nybble==0xB0u)  {  // all notes off
	}  // To next smf-event

	//
	// Printing
	//
	std::stringstream s {};

	auto print_linked_ev = [&s](const linked_t& ev)->void {
		bool missing_ev_on = (ev.tk_on==-1);
		s << std::setw(12) << std::to_string(ev.tk_on);
		s << std::setw(12) << std::to_string(ev.tk_off);
		s << std::setw(12) << std::to_string(ev.tk_off-ev.tk_on);
		s << std::setw(12) << std::to_string(ev.ch);
		s << std::setw(12) << std::to_string(ev.note);  // p1
		s << std::setw(12) << std::to_string(ev.velocity_on);  // p2
		s << std::setw(12) << std::to_string(ev.velocity_off);  // p2
		s << std::setw(12) << std::to_string(ev.trackn);
	};

	
	s << smf.fname() << "\n";
	s << std::left;
	s << std::setw(12) << "Tick on";
	s << std::setw(12) << "Tick off";
	s << std::setw(12) << "Duration";
	s << std::setw(12) << "Ch (off)";
	s << std::setw(12) << "p1 (off)";
	s << std::setw(12) << "p2 (on)";
	s << std::setw(12) << "p2 (off)";
	s << std::setw(12) << "Trk (off)";
	s << "\n";

	if (linked.size()>0) {
		std::sort(linked.begin(),linked.end(),
			[](const linked_t& r, const linked_t& l)->bool {
				return r.tk_on<l.tk_on;
			});
		s << "LINKED EVENTS:\n";
		for (const auto& e : linked) {
			print_linked_ev(e);
			/*s << std::setw(12) << std::to_string(e.tk_on);
			s << std::setw(12) << std::to_string(e.tk_off);
			s << std::setw(12) << std::to_string(e.duration);
			s << std::setw(12) << std::to_string(e.ch);
			s << std::setw(12) << std::to_string(e.note);
			s << std::setw(12) << std::to_string(e.velocity_on);
			s << std::setw(12) << std::to_string(e.velocity_off);
			s << std::setw(12) << std::to_string(e.trackn);*/
			s << "\n";
		}
	}
	if (sounding.size()>0) {
		s << "FILE CONTAINS ORPHAN NOTE-ON EVENTS:\n";
		for (const auto& e : sounding) {
			linked_t curr_unmatched_link;
			curr_unmatched_link = make_linked(e,onoff_event_t {});
			print_linked_ev(curr_unmatched_link);
			/*s << std::setw(12) << std::to_string(e.tk);
			s << std::setw(12) << "?";
			s << std::setw(12) << "?";
			s << std::setw(12) << std::to_string(e.ch);
			s << std::setw(12) << std::to_string(e.note);
			s << std::setw(12) << std::to_string(e.trackn);*/
			s << "\n";
		}
	}
	if (orphan_off_events.size()>0) {
		s << "FILE CONTAINS ORPHAN NOTE-OFF EVENTS:\n";
		for (const auto& e : orphan_off_events) {
			//print_sounding_ev({0,0,0,0},e);
			linked_t curr_unmatched_link = make_linked(e,onoff_event_t {});
			print_linked_ev(curr_unmatched_link);
			s << "\n";
		}
	}

	return s.str();
};






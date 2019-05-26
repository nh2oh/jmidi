#include "smf_container_t.h"
#include "midi_raw.h"
#include "mthd_t.h"
#include "mtrk_container_t.h"
#include <string>
#include <cstdint>
#include <vector>
#include <algorithm>  // std::copy() in smf_t::smf_t(const validate_smf_result_t& maybe_smf)
#include <iostream>
#include <exception>
#include <iomanip>  // std::setw()
#include <ios>  // std::left
#include <sstream>
#include <iostream>

smf_t::smf_t(const validate_smf_result_t& maybe_smf) {
	if (!maybe_smf.is_valid) {
		std::abort();
	}

	this->fname_ = maybe_smf.fname;

	const unsigned char *p = maybe_smf.p;
	for (const auto& e : maybe_smf.chunk_idxs) {
		std::vector<unsigned char> temp {};
		std::copy(p+e.offset,p+e.offset+e.size,std::back_inserter(temp));
		this->d_.push_back(temp);
		temp.clear();
	}
}
uint16_t smf_t::ntrks() const {
	uint16_t ntracks {0};
	for (const auto& e : this->d_) {
		if (chunk_type_from_id(e.data())==chunk_type::track) {
			++ntracks;
		}
	}
	return ntracks;
}
uint16_t smf_t::nchunks() const {
	return static_cast<uint16_t>(this->d_.size());
}
uint16_t smf_t::format() const {
	return this->get_header_view().format();
}
uint16_t smf_t::division() const {
	return this->get_header_view().division();
}
uint32_t smf_t::mthd_size() const {
	return this->get_header_view().size();
}
uint32_t smf_t::mthd_data_length() const {
	return this->get_header_view().data_size();
}
mtrk_view_t smf_t::get_track_view(int n) const {
	int curr_trackn {0};
	for (const auto& e : this->d_) {
		if (chunk_type_from_id(e.data())==chunk_type::track) {
			if (n == curr_trackn) {
				return mtrk_view_t(e.data(), e.size());
			}
			++curr_trackn;
		}
	}
	std::abort();
}
mthd_view_t smf_t::get_header_view() const {
	for (const auto& e : this->d_) {
		if (chunk_type_from_id(e.data())==chunk_type::header) {
			return mthd_view_t(e.data(),e.size());
		}
	}
	std::abort();
}
std::string smf_t::fname() const {
	return this->fname_;
}
// TODO:
// -Creating mtrk_iterator_t's to _temporary_ mtrk_view_t's works but is horrible
//
smf_chrono_iterator_t smf_t::event_iterator_begin() const {
	std::vector<track_state_t> trks {};
	for (int i=0; i<this->ntrks(); ++i) {
		auto curr_trk = this->get_track_view(i);
		trks.push_back({curr_trk.begin(),curr_trk.end(),0});
	}
	return smf_chrono_iterator_t(trks);
}
smf_chrono_iterator_t smf_t::event_iterator_end() const {
	std::vector<track_state_t> trks {};
	for (int i=0; i<this->ntrks(); ++i) {
		auto curr_trk = this->get_track_view(i);
		trks.push_back({curr_trk.end(),curr_trk.end(),0});
	}
	return smf_chrono_iterator_t(trks);
}

std::string print(const smf_t& smf) {
	std::string s {};

	s += smf.fname();
	s += "\n";

	s += "Header (MThd) \t(data_size = ";
	//s += " smf_t has no get_header()-like method yet :(\n";
	s += std::to_string(smf.mthd_data_length()) ;
	s += ", size = ";
	s += std::to_string(smf.mthd_size());
	s += "):\n";
	auto mthd = smf.get_header_view();
	s += print(mthd);
	s += "\n";

	// where smf.get_track_view(i) => mtrk_view_t
	for (int i=0; i<smf.ntrks(); ++i) {
		auto curr_trk = smf.get_track_view(i);
		s += ("Track (MTrk) " + std::to_string(i) 
			+ "\t(data_size = " + std::to_string(curr_trk.data_length())
			+ ", size = " + std::to_string(curr_trk.size()) + "):\n");
		s += print(curr_trk);
		s += "\n";
	}

	return s;
}

/*maybe_smf_t::operator bool() const {
	return this->error == "No error";
}*/


std::string print_events_chrono2(const smf_t& smf) {
	std::vector<int32_t> onset_prior(smf.ntrks(),0);
	std::string s {};
	auto end = smf.event_iterator_end();
	for (auto curr=smf.event_iterator_begin(); curr!=end; ++curr) {
		auto curr_event = *curr;
		std::cout << "Tick onset == " + std::to_string(curr_event.tick_onset) + "; "
			<< "Track == " + std::to_string(curr_event.trackn) + "; "
			<< "Onset prior track event == " + std::to_string(onset_prior[curr_event.trackn]) + "\n\t"
			<< print(curr_event.event,mtrk_sbo_print_opts::debug) + "\n";
		onset_prior[curr_event.trackn] = curr_event.tick_onset;
	}

	return s;
}

//
// Draft of the the chrono_iterator
//
// TODO:
// -Creating mtrk_iterator_t's to _temporary_ mtrk_view_t's works but is horrible
//
std::string print_events_chrono(const smf_t& smf) {
	std::vector<track_state_t> its;
	for (int i=0; i<smf.ntrks(); ++i) {
		auto curr_trk = smf.get_track_view(i);
		its.push_back({curr_trk.begin(),curr_trk.end(),0});
	}
	
	auto finished = [&its]()->bool { 
		bool result {true};
		for (const auto& e : its) {
			result &= (e.next==e.end);
		}
		return result;
	};
	auto idx_next = [](const std::vector<track_state_t>& its)->int {
		// its[i].cumtk is the time at which the _previous_ event on the track w/
		// idx i _initiated_ (one event prior to its[i].next).  The time at which the
		// next event on track i should begin is:  
		// its[i].cumtk + (*its[i].next).delta_time()
		bool first {true};
		int32_t curr_min_cumtk = -1;
		int track_min_cumtk = -1;
		for (int i=0; i<its.size(); ++i) {
			if (its[i].next==its[i].end) { continue; }
			auto curr_cumtk_start = its[i].cumtk + (*its[i].next).delta_time();
			if (first) {
				curr_min_cumtk = curr_cumtk_start;
				track_min_cumtk = i;
				first = false;
			} else {
				if (curr_cumtk_start<curr_min_cumtk) {
					curr_min_cumtk = curr_cumtk_start;
					track_min_cumtk = i;
				}
			}
		}
		return track_min_cumtk;
	};

	std::string s {};
	while (!finished()) {
		int curr_trackn = idx_next(its);
		auto curr_event = *(its[curr_trackn].next);
		s += "Tick onset == " + std::to_string(its[curr_trackn].cumtk+curr_event.delta_time()) + "; "
			+ "Track == " + std::to_string(curr_trackn) + "; "
			+ "Onset prior track event == " + std::to_string(its[curr_trackn].cumtk) + "\n\t"
			+ print(curr_event,mtrk_sbo_print_opts::debug) + "\n";
		//std::cout << "Tick onset == " << std::to_string(its[curr_trackn].cumtk+curr_event.delta_time()) << "; "
		//	<< "Track == " << std::to_string(curr_trackn) << "; "
		//	<< "Onset prior track event == " << std::to_string(its[curr_trackn].cumtk) << "\n\t"
		//	<< print(curr_event,mtrk_sbo_print_opts::debug) << std::endl;
		its[curr_trackn].cumtk += curr_event.delta_time();
		++(its[curr_trackn].next);
	}

	return s;
}


smf_chrono_iterator_t::smf_chrono_iterator_t(const std::vector<track_state_t>& trks) {  // private ctor
	this->trks_=trks;
}
smf_event_t smf_chrono_iterator_t::operator*() const {
	//smf_event_t result;  // TODO:  Deleted function error
	auto ev = this->present_event();
	//result.trackn = ev.trackn;
	//result.tick_onset = ev.tick_onset;
	//result.event = *(this->trks_[ev.trackn].next);
	//return result;
	return smf_event_t {ev.trackn,ev.tick_onset,*(this->trks_[ev.trackn].next)};
}
smf_chrono_iterator_t& smf_chrono_iterator_t::operator++() {
	auto present = this->present_event();
	this->trks_[present.trackn].cumtk = present.tick_onset;
	++(this->trks_[present.trackn].next);
	return *this;
}
bool smf_chrono_iterator_t::operator==(const smf_chrono_iterator_t& rhs) const {
	if (this->trks_.size() != rhs.trks_.size()) {
		return false;
	}
	bool result {true};
	for (int i=0; i<this->trks_.size(); ++i) {
		result &= (rhs.trks_[i].next==this->trks_[i].next);
	}
	return result;
}

bool smf_chrono_iterator_t::operator!=(const smf_chrono_iterator_t& rhs) const {
	return !(*this==rhs);
}


smf_chrono_iterator_t::present_event_t smf_chrono_iterator_t::present_event() const {
	smf_chrono_iterator_t::present_event_t result;
	// trks_[i].cumtk is the time at which the _previous_ event on the track w/
	// idx i _initiated_ (one event prior to trks_[i].next).  The time at which the
	// next event on track i should begin is:  
	// trks_[i].cumtk + (*trks_[i].next).delta_time()
	//
	// The next track to fire an event is the track i for which the quantity
	// this->trks_[i].cumtk + (*this->trks_[i].next).delta_time();
	// is minimum
	//
	bool first {true};
	//result.tick_onset = -1; //int32_t curr_min_cumtk = -1;
	//result.trackn = -1;  //int track_min_cumtk = -1;
	for (int i=0; i<this->trks_.size(); ++i) {
		if (this->trks_[i].next==this->trks_[i].end) { continue; }
		auto curr_cumtk_start = this->trks_[i].cumtk + (*this->trks_[i].next).delta_time();
		if (first) {
			result.tick_onset = curr_cumtk_start;
			result.trackn = i;
			first = false;
		} else {
			if (curr_cumtk_start<result.tick_onset) {
				result.tick_onset = curr_cumtk_start;
				result.trackn = i;
			}
		}
	}

	// first => someone has incremented to, or beyond the end.  
	// result is uninitialized
	return result;
}

//
// TODO:  It is possible (?) that two note-on events for the same 
// track/ch/note occur w/ only one note-off event.  In this case, only
// one of the note-on events will be deleted from vector sounding
//
std::string print_tied_events(const smf_t& smf) {
	struct onoff_event_t {
		// Needed to prevent cross-track communication.  A track 2 note-off event
		// matching the ch- and note-number of a presently "on" track-1 note should
		// not shut off the track 1 note.  
		uint16_t trackn {0};
		int32_t tk {-1};  // cumulative
		uint8_t ch {0};
		uint8_t note {0};  // p1
		uint8_t velocity {0};  // p2
	};
	struct linked_t {  // -1 used to signify "empty" or "not found"
		uint16_t trackn {0};
		int32_t tk_on {-1};  // cumulative
		int32_t tk_off {-1};
		int32_t duration {-1};
		uint8_t ch {0};
		uint8_t note {0};  // p1
		uint8_t velocity_on {0};  // p2
		uint8_t velocity_off {0};  // p2
	};

	// Holds data fro all note-on midi events for which a corresponing note-off
	// event has not yet occured.  When the corresponding note-off event is
	// encountered, the on-event is erased() from the vector.  
	std::vector<onoff_event_t> sounding {};
	// Holds midi data for any note-off events unable to be matched w/a 
	// corresponding note-on event.  
	std::vector<onoff_event_t> orphan_off_events {};
	// Holds all the linked events
	std::vector<linked_t> linked {};

	onoff_event_t curr_onoff_ev;
	auto matching_noteon = [&curr_onoff_ev](const onoff_event_t& rhs)->bool {
		// NB:  If there is >1 note-on event (perhaps @ different dt) for a single
		// ch,note pair w/o any intervening note-off events, followed at some point 
		// by 1 or more matching note-off events, this function will match the
		// _first_ note-off event w/ the _first_ note-on event and the _second_ note-on
		// w/ the second note off and so on.  It is ambiguous what the user intends in
		// such a situation.  
		return (rhs.trackn==curr_onoff_ev.trackn
			&& rhs.ch==curr_onoff_ev.ch
			&& rhs.note==curr_onoff_ev.note);
	};
	auto make_linked = [](const onoff_event_t& ev_on, const onoff_event_t& ev_off)->linked_t {
		linked_t res;
		bool missing_ev_on = (ev_on.trackn==0 && ev_on.tk==0 && ev_on.ch==0 && ev_on.note==0);
		missing_ev_on ? res.tk_on=-1 : res.tk_on=ev_on.tk;
		res.tk_off = ev_off.tk;
		missing_ev_on ? res.duration=-1 : res.duration=ev_off.tk-ev_on.tk;
		res.ch=ev_off.ch;
		res.note = ev_off.note;
		res.velocity_on = ev_on.velocity;
		res.velocity_off = ev_off.velocity;
		res.trackn=ev_off.trackn;

		return res;
	};

	auto smf_end = smf.event_iterator_end();
	for (auto smf_it=smf.event_iterator_begin(); smf_it!=smf_end; ++smf_it) {
		mtrk_event_t::midi_data_t curr_ev_midi_data;
		{	// Prevent curr_smf_event from entering the main loop scope; 
			// is slightly cleaner
			auto curr_smf_event = *smf_it;  // {trackn, tick_onset, event}
			curr_ev_midi_data = curr_smf_event.event.midi_data();
			curr_onoff_ev.trackn=curr_smf_event.trackn;
			curr_onoff_ev.tk=curr_smf_event.tick_onset;
			curr_onoff_ev.ch=curr_ev_midi_data.ch;
			curr_onoff_ev.note=curr_ev_midi_data.p1;
			curr_onoff_ev.velocity=curr_ev_midi_data.p2;
		}
		if (!curr_ev_midi_data.is_valid) {
				continue;  
		}

		if (curr_ev_midi_data.status_nybble==0x90u && curr_ev_midi_data.p2!=0) {  // curr_onoff_ev is a note-on
			sounding.push_back(curr_onoff_ev);
		} else if (curr_ev_midi_data.status_nybble==0x80u
			|| (curr_ev_midi_data.status_nybble==0x90u 
			&& curr_ev_midi_data.p2==0)) {  // curr_onoff_ev is a note-off
			auto matching_on_ev = std::find_if(sounding.begin(),sounding.end(),matching_noteon);
			if (matching_on_ev==sounding.end()) {  // no corresponding note-on event (weird)
				orphan_off_events.push_back(curr_onoff_ev);
			} else {
				linked.push_back(make_linked(*matching_on_ev,curr_onoff_ev));
				sounding.erase(matching_on_ev);
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
}





bool is_linked_pair(const orphan_event_t& ev_on, const orphan_event_t& ev_off) {
	auto md_on = ev_on.event.midi_data();
	auto md_off = ev_off.event.midi_data();
	// TODO:  Supplement w/ additional conditions to match meta events such as
	// all notes off, system reset, etc.  Some such events can have effects
	// across tracks.  
	return (ev_on.trackn==ev_off.trackn
			&& ev_on.tk<=ev_off.tk
			&& md_on.ch==md_off.ch
			&& ((md_on.p1==md_off.p1)
				|| (md_off.status_nybble==0xB0u && md_off.p1==0x7Du)));
}

linked_note_events_result_t link_note_events(const smf_t& smf) {
	std::vector<orphan_event_t> orphans_on {}; 
	std::vector<orphan_event_t> orphans_off {};
	std::vector<linked_event_t> linked {};

	orphan_event_t curr_event;
	auto matching_on_event = [&curr_event](const orphan_event_t& on_ev)->bool {
		// Defined purely for convenience.  Captures curr_event for use in
		// std::find_if(orphans_on.begin(),orphans_on.end(),...)
		// to find a matching on-event when curr_event is an off/multioff
		// event.  
		// Do not put any complicated logic in here (trackn matching etc);
		// leave that for is_linked_pair().  
		return is_linked_pair(on_ev,curr_event);
	};
	
	auto smf_end = smf.event_iterator_end();
	for (auto smf_it=smf.event_iterator_begin(); smf_it!=smf_end; ++smf_it) {
		auto curr_smf_event = *smf_it;
		curr_event.trackn=curr_smf_event.trackn;
		curr_event.tk=curr_smf_event.tick_onset,
		curr_event.event=curr_smf_event.event;

		if (is_on_event(curr_event.event)) {
			orphans_on.push_back(curr_event);
		} else if (is_off_event(curr_event.event)) {
			// curr_event is an "off" event; try to find a matching "on"
			// event in orphan_on.  
			auto it_matching_on_ev = 
				std::find_if(orphans_on.begin(),orphans_on.end(),matching_on_event);
			if (it_matching_on_ev==orphans_on.end()) {
				// curr_event does not match anything in orphans_on; weird.  
				orphans_off.push_back(curr_event);
			} else {
				// The event in orphans_on pointed to by it_matching_on_ev matches
				// the off-event curr_event.  
				linked.push_back({*it_matching_on_ev,curr_event});
				orphans_on.erase(it_matching_on_ev);
			}
		} else if (is_multioff_event(curr_event.event)) {
			// Find all entries in orphans_on matching the multioff event 
			// curr_event.  
			// Initially, orphans_on==
			// [.begin()==ev1,2,...,.end())
			// after std::remove_if(), orphans_on==
			// [.begin()==!matchingev1,2,...,it_matching_on_ev==matchingev1,2,...,.end())
			// orphans_on events [it_matching_on_ev,...,orphans_on.end())
			// match curr_event.  
			auto it_matching_on_ev =
				std::remove_if(orphans_on.begin(),orphans_on.end(),matching_on_event);
			if (it_matching_on_ev==orphans_on.end()) {
				// curr_event does not match anything in orphans_on.
				orphans_off.push_back(curr_event);
			} else {
				auto it_new_end = it_matching_on_ev;
				while (it_matching_on_ev!=orphans_on.end()) {
					linked.push_back({*it_matching_on_ev,curr_event});
					++it_matching_on_ev;
				}
				orphans_on.erase(it_new_end,orphans_on.end());
			}
		}  // else if (is_multioff_event(curr_event.event)) {
	}  // To next smf-event

	auto lt_oe = [](const orphan_event_t& rhs,const orphan_event_t& lhs)->bool {
		return rhs.tk<lhs.tk;
	};
	auto lt_le = [](const linked_event_t& rhs,const linked_event_t& lhs)->bool {
		return rhs.on.tk<lhs.on.tk;
	};
	std::sort(orphans_on.begin(),orphans_on.end(),lt_oe);
	std::sort(orphans_on.begin(),orphans_on.end(),lt_oe);
	std::sort(linked.begin(),linked.end(),lt_le);

	return linked_note_events_result_t {linked, orphans_on, orphans_off};
}


std::string print(const linked_note_events_result_t& evs) {
	struct width_t {
		int def {12};  // "default"
		int p1p2 {10};
		int ch {10};
		int sep {3};
		int trk {10};
	};
	width_t w {};

	std::stringstream ss {};
	ss << std::left;
	ss << std::setw(w.ch) << "Ch (on)";
	ss << std::setw(w.p1p2) << "p1 (on)";
	ss << std::setw(w.p1p2) << "p2 (on)";
	ss << std::setw(w.def) << "Tick (on)";
	ss << std::setw(w.trk) << "Trk (on)";
	ss << std::setw(w.sep) << " ";
	ss << std::setw(w.ch) << "Ch (off)";
	ss << std::setw(w.p1p2) << "p1 (off)";
	ss << std::setw(w.p1p2) << "p2 (off)";
	ss << std::setw(w.def) << "Tick off";
	ss << std::setw(w.trk) << "Trk (off)";
	ss << std::setw(w.sep) << " ";
	ss << std::setw(w.def) << "Duration";
	ss << "\n";

	auto half = [&ss,&w](const orphan_event_t& ev)->void {
		auto md = ev.event.midi_data();
		ss << std::setw(w.ch) << std::to_string(md.ch);
		ss << std::setw(w.p1p2) << std::to_string(md.p1);
		ss << std::setw(w.p1p2) << std::to_string(md.p2);
		ss << std::setw(w.def) << std::to_string(ev.tk);
		ss << std::setw(w.trk) << std::to_string(ev.trackn);
	};
	
	for (const auto& e : evs.linked) {
		half(e.on);
		ss << std::setw(w.sep) << " ";
		half(e.off);
		ss << std::setw(w.sep) << " ";
		ss << std::to_string(e.off.tk-e.on.tk);
		ss << "\n";
	}

	if (evs.orphan_on.size()>0) {
		ss << "FILE CONTAINS ORPHAN NOTE-ON EVENTS:\n";
		for (const auto& e : evs.orphan_on) {
			half(e);
			ss << std::setw(w.sep) << "\n";
		}
	}
	if (evs.orphan_off.size()>0) {
		ss << "FILE CONTAINS ORPHAN NOTE-OFF EVENTS:\n";
		for (const auto& e : evs.orphan_off) {
			half(e);
			ss << std::setw(w.sep) << "\n";
		}
	}

	return ss.str();
}



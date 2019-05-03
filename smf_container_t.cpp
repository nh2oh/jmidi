#include "smf_container_t.h"
#include "midi_raw.h"
#include "mthd_container_t.h"
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
		if (detect_chunk_type_unsafe(e.data())==chunk_type::track) {
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
	return this->get_header_view().data_length();
}
mtrk_view_t smf_t::get_track_view(int n) const {
	int curr_trackn {0};
	for (const auto& e : this->d_) {
		if (detect_chunk_type_unsafe(e.data())==chunk_type::track) {
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
		if (detect_chunk_type_unsafe(e.data())==chunk_type::header) {
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
	struct sounding_t {
		// Needed to prevent cross-track communication.  A track 2 note-off event
		// matching the ch- and note-number of a presently "on" track-1 note should
		// not shut off the track 1 note.  
		uint16_t trackn;
		int32_t tk;  // cumulative
		uint8_t ch;
		uint8_t note;  // p1
	};

	std::stringstream s {};
	s << smf.fname() << "\n";
	s << std::left;
	s << std::setw(12) << "Tick on";
	s << std::setw(12) << "Tick off";
	s << std::setw(12) << "Duration";
	s << std::setw(12) << "Ch (off)";
	s << std::setw(12) << "p1 (off)";
	s << std::setw(12) << "Trk (off)";
	s << "\n";

	// Holds data fro all note-on midi events for which a corresponing note-off
	// event has not yet occured.  When the corresponding note-off event is
	// encountered, the on-event is erased() from the vector.  
	std::vector<sounding_t> sounding {};
	// Holds midi data for any note-off events unable to be matched w/a 
	// corresponding note-on event.  
	std::vector<sounding_t> orphan_off_events {};

	midi_extract_t curr_ev_midi_data;
	sounding_t curr_sounding;
	auto matching_noteon = [&curr_sounding](const sounding_t& rhs)->bool {
		return (rhs.trackn==curr_sounding.trackn
			&& rhs.ch==curr_sounding.ch
			&& rhs.note==curr_sounding.note);
	};
	auto print_sounding_ev = [&s](const sounding_t& ev_on, const sounding_t& ev_off)->void {
		bool missing_ev_on = (ev_on.trackn==0 && ev_on.tk==0 && ev_on.ch==0 && ev_on.note==0);
		if (missing_ev_on) {
			s << std::setw(12) << "?";  // tk note-on
		} else {
			s << std::setw(12) << std::to_string(ev_on.tk);  // tk note-on
		}
		s << std::setw(12) << std::to_string(ev_off.tk);  // tk note-off
		if (missing_ev_on) {
			s << std::setw(12) << "?";  // tk note-on
		} else {
			s << std::setw(12) << std::to_string(ev_off.tk-ev_on.tk);
		}
		s << std::setw(12) << std::to_string(ev_off.ch);
		s << std::setw(12) << std::to_string(ev_off.note);
		s << std::setw(12) << std::to_string(ev_off.trackn);
		s << "\n";
	};
	
	auto end = smf.event_iterator_end();
	for (auto curr=smf.event_iterator_begin(); curr!=end; ++curr) {
		auto curr_smf_event = *curr;  // {trackn, tick_onset, event}
		curr_ev_midi_data = midi_extract(curr_smf_event.event);
		if (!curr_ev_midi_data.is_valid) {
			continue;  // Not a midi-event (maybe a meta event, sysex_f0/f7...)
		}

		curr_sounding.trackn = curr_smf_event.trackn;
		curr_sounding.tk = curr_smf_event.tick_onset;
		curr_sounding.ch = curr_ev_midi_data.ch;

		if (curr_ev_midi_data.status_nybble==0x90u) {  // note on
			curr_sounding.note = curr_ev_midi_data.p1;
			sounding.push_back(curr_sounding);
		} else if (curr_ev_midi_data.status_nybble==0x80u) {  // note off
			curr_sounding.note = curr_ev_midi_data.p1;
			auto on_ev = std::find_if(sounding.begin(),sounding.end(),matching_noteon);
			if (on_ev==sounding.end()) {  // no corresponding note-on event (weird)
				orphan_off_events.push_back(curr_sounding);
			} else {
				print_sounding_ev(*on_ev,curr_sounding);
				sounding.erase(on_ev);
			}
		} else if (curr_ev_midi_data.status_nybble==0xB0u) {  // all notes off
			// Find all entries in sounding for which trackn==curr_sounding.trackn;
			// print the event w/ curr_sounding as the value for the note-off
			// event, then delete the entry in sounding.  The complexity here arises
			// b/c calling sounding.erase(it-to-on-event) invalidates the iterator 
			// it-to-on-event pointing to the on event i just printed and now wish
			// to delete.  
			while (true) {
				std::vector<sounding_t>::iterator it;
				bool found_event = false;
				for (it=sounding.begin(); it!=sounding.end(); ++it) {
					if ((*it).trackn==curr_sounding.trackn) {
						found_event = true;
						print_sounding_ev(*it,curr_sounding);
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

	if (sounding.size()>0) {
		s << "FILE CONTAINS ORPHAN NOTE-ON EVENTS:\n";
		for (const auto& e : sounding) {
			s << std::setw(12) << std::to_string(e.tk);
			s << std::setw(12) << "?";
			s << std::setw(12) << "?";
			s << std::setw(12) << std::to_string(e.ch);
			s << std::setw(12) << std::to_string(e.note);
			s << std::setw(12) << std::to_string(e.trackn);
			s << "\n";
		}
	}
	if (orphan_off_events.size()>0) {
		s << "FILE CONTAINS ORPHAN NOTE-OFF EVENTS:\n";
		for (const auto& e : orphan_off_events) {
			print_sounding_ev({0,0,0,0},e);
		}
	}

	return s.str();
}


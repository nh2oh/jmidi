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
#include <iomanip>



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


std::string print_tied_events(const smf_t& smf) {
	struct sounding_t {
		int32_t trackn;  // TODO:   Needed?  Do events communicate cross-track?
		int32_t ch;
		int32_t note;
		int32_t tkon;
		//mtrk_event_container_sbo_t ev_on;
		//mtrk_event_container_sbo_t ev_off;
	};
	std::string s {};
	std::vector<sounding_t> sounding {};
	std::cout << std::setw(12) << "Tick on";
	std::cout << std::setw(12) << "Tick off";
	std::cout << std::setw(12) << "Ch (off)";
	std::cout << std::setw(12) << "p1 (off)";
	std::cout << std::setw(12) << "p2 (off)";
	std::cout << std::setw(12) << "Trk (on)";
	std::cout << std::setw(12) << "Trk (off)";
	std::cout << "\n";

	auto end = smf.event_iterator_end();
	for (auto curr=smf.event_iterator_begin(); curr!=end; ++curr) {
		auto curr_event = *curr;
		auto curr_ev_midi_data = midi_extract(curr_event.event);
		if (curr_ev_midi_data.is_valid) {
			if (curr_ev_midi_data.status_nybble==0x90) {  // note on
				sounding.push_back({curr_event.trackn,curr_ev_midi_data.ch,
					curr_ev_midi_data.p1,curr_event.tick_onset});
			} else if (curr_ev_midi_data.status_nybble==0x80) {  // note off
				auto on_ev = std::find_if(sounding.begin(),sounding.end(),
					[&curr_ev_midi_data](const sounding_t& rhs)->bool {
						return (rhs.ch==curr_ev_midi_data.ch && rhs.note==curr_ev_midi_data.p1);
					});
				if (on_ev==sounding.end()) {  // no corresponding on event (weird)
					continue;
				}
				std::cout << std::setw(12) << std::to_string((*on_ev).tkon);
				std::cout << std::setw(12) << std::to_string(curr_event.tick_onset);
				std::cout << std::setw(12) << std::to_string(curr_ev_midi_data.ch);
				std::cout << std::setw(12) << std::to_string(curr_ev_midi_data.p1);
				std::cout << std::setw(12) << std::to_string(curr_ev_midi_data.p2);
				std::cout << std::setw(12) << std::to_string((*on_ev).trackn);
				std::cout << std::setw(12) << std::to_string(curr_event.trackn);
				std::cout << "\n";

				sounding.erase(on_ev);
			}
		}
	}
	return s;
}




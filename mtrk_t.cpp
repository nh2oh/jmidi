#include "mtrk_t.h"
#include "mtrk_event_t.h"
#include "mtrk_iterator_t.h"
#include <string>
#include <cstdint>
#include <vector>
#include <algorithm>  // std::find_if() in linked-pair finding functions
#include <iomanip>  // std::setw()
#include <ios>  // std::left
#include <sstream>



uint32_t mtrk_t::size() const {
	return this->data_size_+8;
}
uint32_t mtrk_t::data_size() const {
	return this->data_size_;
}
uint32_t mtrk_t::nevents() const {
	return this->evnts_.size();
}
mtrk_iterator_t mtrk_t::begin() {
	return mtrk_iterator_t(&(this->evnts_[0]));
}
mtrk_iterator_t mtrk_t::end() {
	auto p_last = &(this->evnts_.back());
	return mtrk_iterator_t(++p_last);
}
mtrk_const_iterator_t mtrk_t::begin() const {
	return mtrk_const_iterator_t(&(this->evnts_[0]));
}
mtrk_const_iterator_t mtrk_t::end() const {
	auto p_last = &(this->evnts_.back());
	return mtrk_const_iterator_t(++p_last);
}
bool mtrk_t::push_back(const mtrk_event_t& ev) {
	if (ev.type() == smf_event_type::invalid) {
		return false;
	}
	this->evnts_.push_back(ev);
	this->cumtk_ += ev.delta_time();
	return true;
}
mtrk_t::validate_t mtrk_t::validate() const {
	mtrk_t::validate_t r {};

	// vector sounding holds a list of note-on events for which a 
	// corresponding note-off event has not yet been encountered.  When the
	// corresponding note-off event is found, the on event is cleared from 
	// the list.  
	// -> Multiple "on" events w/the same note and ch number are not
	//    considered errors; the first off event will match with the first
	//    on event only.  
	// -> Orphan note-off events are not considered errors
	struct sounding_notes_t {
		int idx;
		int ch {0};
		int note {0};  // note number
	};
	std::vector<sounding_notes_t> sounding;
	sounding.reserve(10);  // Expect <= 10 simultaniously sounding notes???
	mtrk_event_t::channel_event_data_t curr_chev_data;
	auto is_matching_off = [&curr_chev_data](const sounding_notes_t& sev)->bool {
		return (curr_chev_data.ch==sev.ch
			&& curr_chev_data.p1==sev.note);
	};

	// Certain meta events are not allowed to occur after the first channel
	// event, hence found_ch_event is set to true once a ch event has been
	// encountered.  
	bool found_ch_ev = false;
	uint64_t cumtk = 0;  // Cumulative delta-time
	for (int i=0; i<this->evnts_.size(); ++i) {
		auto curr_event_type = this->evnts_[i].type();
		if (curr_event_type == smf_event_type::invalid) {
			r.msg = ("this->evnts_[i].type() == smf_event_type::invalid; i = "
				+ std::to_string(i) + "\n\t");
			return r;
		}
		
		cumtk += this->evnts_[i].delta_time();
		found_ch_ev = (found_ch_ev || (curr_event_type == smf_event_type::channel));
		
		// Test for note-on/note-off event
		if (is_note_on(this->evnts_[i])) {
			curr_chev_data = this->evnts_[i].midi_data();
			sounding.push_back({i,curr_chev_data.ch,curr_chev_data.p1});
		} else if (is_note_off(this->evnts_[i])) {
			curr_chev_data = this->evnts_[i].midi_data();
			auto it = std::find_if(sounding.begin(),sounding.end(),
				is_matching_off);
			if (it!=sounding.end()) {
				sounding.erase(it);
			}  //it==sounding.end() => orphan "off" event
		} else if (is_eot(this->evnts_[i]) && (i!=(this->evnts_.size()-1))) {
			r.msg = ("Illegal \"End-of-track\" (EOT) meta-event found "
				"at idx i = " + std::to_string(i) + " and cumulative "
				"delta-time = " + std::to_string(cumtk) + ".  EOT "
				"events are only valid as the very last event in an MTrk "
				"event sequence.  ");
			return r;
		} else if (is_seqn(this->evnts_[i]) && ((cumtk>0) || found_ch_ev)) {
			// Test for illegal sequence-number meta event occuring after 
			// a midi channel event or at t>0.  
			r.msg = ("Illegal \"Sequence number\" meta-event found "
				"at idx i = " + std::to_string(i) + " and cumulative "
				"delta-time = " + std::to_string(cumtk) + ".  Sequence "
				"number events must occur before any non-zero delta times "
				"and before any MIDI [channel] events.  ");
			return r;
		}
	}

	// The final event must be a meta-end-of-track msg.  
	if (!is_eot(this->evnts_.back())) {
		r.msg = "!is_eot(this->evnts_.back()):  "
			"The final event in an MTrk eent sequence must be an"
			"\"end-of-track\" meta event.  ";
		return r;
	}

	// std::vector<sounding_notes_t> sounding must be empty
	if (sounding.size() > 0) {
		r.msg = "Orphan note-on events found (sounding.size() > 0):  \n";
		r.msg += "\tidx\tch\tnote\n";
			for (int i=0; i<sounding.size(); ++i) {
				r.msg += ("\t" + std::to_string(sounding[i].idx));
				r.msg += ("\t" + std::to_string(sounding[i].ch));
				r.msg += ("\t" + std::to_string(sounding[i].note) + "\n");
			}
		return r;
	}

	if (this->cumtk_ != cumtk) {
		r.msg = ("Inconsistent values of cumtk (==" + std::to_string(cumtk)
			+ "), this->cumtk_ (==" + std::to_string(this->cumtk_) + ")\n");
		return r;
	}

	return r;
}
mtrk_t::validate_t::operator bool() const {
	return this->msg.size()==0;
}

std::string print(const mtrk_t& mtrk) {
	std::string s {};
	s.reserve(mtrk.nevents()*100);  // TODO:  Magic constant 100
	for (auto it=mtrk.begin(); it!=mtrk.end(); ++it) {
		//s += print(*it);
		s += print(*it,mtrk_sbo_print_opts::detail);
		s += "\n";
	}
	return s;
}


maybe_mtrk_t::operator bool() const {
	return this->error=="No error";
}

maybe_mtrk_t make_mtrk(const unsigned char *p, uint32_t max_sz) {
	maybe_mtrk_t result {};
	auto chunk_detect = validate_chunk_header(p,max_sz);
	if (chunk_detect.type != chunk_type::track) {
		result.error = ("Error validating MTrk chunk:  "
			"validate_chunk_header() reported:\n\t"
			+ print_error(chunk_detect));
		return result;
	}
	
	// From experience with thousands of midi files encountered in the wild,
	// the average number of bytes per MTrk event is about 3.  
	std::vector<mtrk_event_t> evec;  // "event vector"
	auto approx_nevents = static_cast<double>(chunk_detect.data_size)/3.0;
	evec.reserve(approx_nevents);
	// vector sounding holds a list of note-on events for which a 
	// corresponding note-off event has not yet been encountered.  When the
	// corresponding note-off event is found, the on event is cleared from 
	// the list.  
	// -> Multiple "on" events w/the same note and ch number are not
	//    considered errors; the first off event will match with the first
	//    on event only.  
	// -> Orphan note-off events are not considered errors
	struct sounding_notes_t {
		uint32_t offset {0};
		int ch {0};
		int note {0};  // note number
	};
	std::vector<sounding_notes_t> sounding;
	sounding.reserve(10);  // Expect <= 10 simultaniously-sounding notes ???
	mtrk_event_t::channel_event_data_t curr_chev_data;
	auto is_matching_off = [&curr_chev_data](const sounding_notes_t& sev)->bool {
		return (curr_chev_data.ch==sev.ch
			&& curr_chev_data.p1==sev.note);
	};

	// Process the data section of the mtrk chunk until the number of bytes
	// processed is as indicated by chunk_detect.size, or an end-of-track 
	// meta event is encountered.  
	// Certain meta events are not allowed to occur after the first channel
	// event, hence found_ch_event is set to true once a ch event has been
	// encountered.  
	bool found_eot = false;  bool found_ch_ev = false;
	uint32_t o = 8;  // offset; skip "MTrk" & the 4-byte length
	unsigned char rs {0};  // value of the running-status
	uint64_t cumtk = 0;  // Cumulative delta-time
	validate_mtrk_event_result_t curr_event;
	while ((o<chunk_detect.size) && !found_eot) {
		const unsigned char *curr_p = p+o;  // ptr to start of present event
		uint32_t curr_max_sz = chunk_detect.size-o;
		
		curr_event = validate_mtrk_event_dtstart(curr_p,rs,curr_max_sz);
		if (curr_event.error!=mtrk_event_validation_error::no_error) {
			result.error = ("Error validating the MTrk event at offset o = "
				+ std::to_string(o) + "\n\t" + print(curr_event.error));
			return result;
		}
		
		rs = curr_event.running_status;
		cumtk += curr_event.delta_t;
		found_ch_ev = (found_ch_ev || (curr_event.type == smf_event_type::channel));
		auto curr_mtrk_event = mtrk_event_t(curr_p,curr_event);
		
		// Test for note-on/note-off event
		if (is_note_on(curr_mtrk_event)) {
			curr_chev_data = curr_mtrk_event.midi_data();
			sounding.push_back({o,curr_chev_data.ch,curr_chev_data.p1});
		} else if (is_note_off(curr_mtrk_event)) {
			curr_chev_data = curr_mtrk_event.midi_data();
			auto it = std::find_if(sounding.begin(),sounding.end(),
				is_matching_off);
			if (it!=sounding.end()) {
				sounding.erase(it);
			}  //it==sounding.end() => orphan "off" event
		}

		// Test for end-of-track msg; finding the eot will cause the loop to
		// exit, so, if there is an "illegal" eot in the middle of the track,
		// it will be caught below the loop when o is compared to 
		// chunk_detect.size.  
		found_eot = (found_eot || (is_eot(curr_mtrk_event)));
		// Test for illegal sequence-number meta event occuring after a midi
		// channel event or at t>0.  
		if (is_seqn(curr_mtrk_event) && ((cumtk>0) || found_ch_ev)) {
			result.error = ("Illegal \"Sequence number\" meta-event found "
				"at offset o = " + std::to_string(o) + " and cumulative "
				"delta-time = " + std::to_string(cumtk) + ".  Sequence "
				"number events must occur before any non-zero delta times "
				"and before any MIDI [channel] events.  ");
			return result;
		}

		evec.push_back(curr_mtrk_event);
		o += curr_event.size;
	}

	// The final event must be a meta-end-of-track msg.  The loop above
	// exits when the eot event is encountered _or_ when 
	// o>=chunk_detect.size, hence, it is possible at this point that 
	// found_eot==false if o>=chunk_detect.size (clearly illegal).  
	// I am also making it illegal for an mtrk header to report a length 
	// > the number of bytes spanned by the actual events (which could 
	// perhaps => that the track is 0-padded after the end-of-track msg).  
	if (o!=chunk_detect.size || !found_eot) {
		result.error = "o!=chunk_detect.size || !found_eot:  "
			"o==" + std::to_string(o) + "; "
			"chunk_detect.size==" + std::to_string(chunk_detect.size) + "; "
			"found_eot===" + std::to_string(found_eot);
		return result;
	}

	// std::vector<sounding_notes_t> sounding must be empty
	if (sounding.size() > 0) {
		result.error = "Orphan note-on events found (sounding.size() > 0):  \n";
		result.error += "\tch\tnote\n";
			for (int i=0; i<sounding.size(); ++i) {
				result.error += ("\t" + std::to_string(sounding[i].ch));
				result.error += ("\t" + std::to_string(sounding[i].note));
				result.error += ("\toffset== " + std::to_string(sounding[i].offset)+"\n");
			}
		return result;
	}

	result.mtrk.evnts_ = evec;
	result.mtrk.data_size_ = chunk_detect.data_size;
	result.mtrk.cumtk_ = cumtk;

	return result;
}




mtrk_iterator_t get_simultanious_events(mtrk_iterator_t beg, 
					mtrk_iterator_t end) {
	mtrk_iterator_t range_end = beg;
	if (range_end==end) {
		return range_end;
	}
	++range_end;
	while ((range_end!=end) && (range_end->delta_time()==0)) {
		++range_end;
	}
	return range_end;
}

linked_and_orphan_onoff_pairs_t get_linked_onoff_pairs(mtrk_const_iterator_t beg,
					mtrk_const_iterator_t end) {
	// TODO:  It might be faster to pull all on,off events into a single
	// "orphans" vector, then iterate over this collection pairing up 
	// the events and moving them into a linked-events vector.  
	linked_and_orphan_onoff_pairs_t result {};
	//result.linked.reserve(end-beg);

	uint32_t cumtk = 0;
	for (auto it=beg; it!=end; ++it) {
		const auto& curr_ev = *it;
		cumtk += curr_ev.delta_time();

		auto matching_on_event = [&curr_ev,&cumtk](const orphan_onoff_t& on_ev)->bool {
			// Captures curr_ev for use in std::find_if() to find
			// the matching on-event when curr_ev is an off event.  
			auto md_on = on_ev.ev.midi_data();
			auto md_off = curr_ev.midi_data();
			return ((on_ev.cumtk<=cumtk) 
				&& (md_on.ch==md_off.ch) 
				&& (md_on.p1==md_off.p1));
		};

		if (is_note_on(curr_ev)) {
			result.orphan_on.push_back({cumtk,*it});
		} else if (is_note_off(curr_ev)) {
			auto it_linked_on = 
				std::find_if(result.orphan_on.begin(),result.orphan_on.end(),
					matching_on_event);
			if (it_linked_on==result.orphan_on.end()) {
				// curr_ev does not match anything in orphans_on; weird.  
				result.orphan_off.push_back({cumtk,curr_ev});
			} else {
				// The event in orphans_on pointed to by it_linked_on matches
				// the off-event curr_ev.  
				result.linked.push_back({(*it_linked_on).cumtk,
					(*it_linked_on).ev,cumtk,curr_ev});
				result.orphan_on.erase(it_linked_on);
			}
		}  // else { // not an on or off event
	}  // To the next event on [beg,end)

	// "less-than" functions for orphans and linked pairs
	// "lt_oe" => "less than for orphan events"
	// "lt_le" => "less than for linked events"
	auto lt_oe = [](const orphan_onoff_t& rhs,const orphan_onoff_t& lhs)->bool {
		return rhs.cumtk<lhs.cumtk;
	};
	auto lt_le = [](const linked_onoff_pair_t& rhs,const linked_onoff_pair_t& lhs)->bool {
		return rhs.cumtk_on<lhs.cumtk_on;
	};
	std::sort(result.orphan_on.begin(),result.orphan_on.end(),lt_oe);
	std::sort(result.orphan_off.begin(),result.orphan_off.end(),lt_oe);
	std::sort(result.linked.begin(),result.linked.end(),lt_le);

	return result;
}

std::string print(const linked_and_orphan_onoff_pairs_t& evs) {
	std::string s {};
	struct width_t {
		int def {12};  // "default"
		int p1p2 {10};
		int ch {10};
		int sep {3};
	};
	width_t w {};

	std::stringstream ss {};
	ss << std::left;
	ss << std::setw(w.ch) << "Ch (on)";
	ss << std::setw(w.p1p2) << "p1 (on)";
	ss << std::setw(w.p1p2) << "p2 (on)";
	ss << std::setw(w.def) << "Tick (on)";
	ss << std::setw(w.sep) << " ";
	ss << std::setw(w.ch) << "Ch (off)";
	ss << std::setw(w.p1p2) << "p1 (off)";
	ss << std::setw(w.p1p2) << "p2 (off)";
	ss << std::setw(w.def) << "Tick off";
	ss << std::setw(w.sep) << " ";
	ss << std::setw(w.def) << "Duration";
	ss << "\n";

	auto half = [&ss,&w](uint32_t cumtk, const mtrk_event_t& onoff)->void {
		auto md = onoff.midi_data();
		ss << std::setw(w.ch) << std::to_string(md.ch);
		ss << std::setw(w.p1p2) << std::to_string(md.p1);
		ss << std::setw(w.p1p2) << std::to_string(md.p2);
		ss << std::setw(w.def) << std::to_string(cumtk);
	};
	
	for (const auto& e : evs.linked) {
		half(e.cumtk_on,e.on);
		ss << std::setw(w.sep) << " ";
		half(e.cumtk_off,e.off);
		ss << std::setw(w.sep) << " ";
		ss << std::to_string(e.cumtk_off-e.cumtk_on);
		ss << "\n";
	}

	if (evs.orphan_on.size()>0) {
		ss << "FILE CONTAINS ORPHAN NOTE-ON EVENTS:\n";
		for (const auto& e : evs.orphan_on) {
			half(e.cumtk,e.ev);
			ss << std::setw(w.sep) << "\n";
		}
	}
	if (evs.orphan_off.size()>0) {
		ss << "FILE CONTAINS ORPHAN NOTE-OFF EVENTS:\n";
		for (const auto& e : evs.orphan_off) {
			half(e.cumtk,e.ev);
			ss << std::setw(w.sep) << "\n";
		}
	}

	return ss.str();
}





mtrk_event_cumtk_t find_linked_off(mtrk_const_iterator_t beg,
					mtrk_const_iterator_t end, const mtrk_event_t& on) {
	mtrk_event_cumtk_t res {0, end};
	if (beg==end || !is_note_on(on)) {
		return res;
	}

	auto mdata_on = on.midi_data();
	auto is_linked_off = [&mdata_on](const mtrk_const_iterator_t& evit)->bool {
		if (!is_note_off(*evit)) {
			return false;
		}
		auto mdata_ev = evit->midi_data();
		return ((mdata_ev.ch==mdata_on.ch) 
			&& (mdata_ev.p1==mdata_on.p1));
	};

	for (res.ev=beg; (res.ev!=end && !is_linked_off(res.ev)); ++res.ev) {
		res.cumtk += res.ev->delta_time();
	}

	return res;
}

std::string print_linked_onoff_pairs(const mtrk_t& mtrk) {
	std::string s;

	struct width_t {
		int def {12};  // "default"
		int p1p2 {10};
		int ch {10};
		int sep {3};
	};
	width_t w {};

	std::stringstream ss {};
	ss << std::left;
	ss << std::setw(w.ch) << "Ch (on)";
	ss << std::setw(w.p1p2) << "p1 (on)";
	ss << std::setw(w.p1p2) << "p2 (on)";
	ss << std::setw(w.def) << "Tick (on)";
	ss << std::setw(w.sep) << " ";
	ss << std::setw(w.ch) << "Ch (off)";
	ss << std::setw(w.p1p2) << "p1 (off)";
	ss << std::setw(w.p1p2) << "p2 (off)";
	ss << std::setw(w.def) << "Tick off";
	ss << std::setw(w.sep) << " ";
	ss << std::setw(w.def) << "Duration";
	ss << "\n";

	auto print_half = [&ss,&w](uint32_t cumtk, const mtrk_event_t& onoff)->void {
		auto md = onoff.midi_data();
		ss << std::setw(w.ch) << std::to_string(md.ch);
		ss << std::setw(w.p1p2) << std::to_string(md.p1);
		ss << std::setw(w.p1p2) << std::to_string(md.p2);
		ss << std::setw(w.def) << std::to_string(cumtk);
	};

	uint32_t cumtk_on = 0;
	for (auto curr=mtrk.begin(); curr!=mtrk.end(); ++curr) {
		cumtk_on += curr->delta_time();
		if (!is_note_on(*curr)) {
			continue;
		}
		auto cumtk_prior = (cumtk_on - curr->delta_time());

		print_half(cumtk_on,*curr);
		ss << std::setw(w.sep) << " ";

		// Note:  Not starting @ curr+1
		auto it_off = find_linked_off(curr,mtrk.end(),*curr);
		if (it_off.ev==mtrk.end()) {
			ss << "NO OFF EVENT\n";
			continue;
		}

		auto cumtk_off = cumtk_prior + it_off.cumtk + it_off.ev->delta_time();
		print_half(cumtk_off,*it_off.ev);
		ss << std::setw(w.sep) << " ";
		ss << std::to_string(cumtk_off-cumtk_on);
		ss << "\n";
	}
	return ss.str();
}




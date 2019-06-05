#include "mtrk_t.h"
#include "mtrk_event_t.h"
#include "mtrk_iterator_t.h"
#include <string>
#include <cstdint>
#include <vector>
#include <array>  // get_header()
#include <algorithm>  // std::find_if() in linked-pair finding functions
#include <iomanip>  // std::setw()
#include <ios>  // std::left
#include <sstream>


mtrk_t::mtrk_t(const unsigned char *p, uint32_t sz) {
	auto chunk_detect = validate_chunk_header(p,sz);
	if (chunk_detect.type != chunk_type::track) {
		return;
	}
	if (chunk_detect.size != sz) {
		return;
	}

	// From experience with thousands of midi files encountered in the wild,
	// the average number of bytes per MTrk event is about 3.  
	auto approx_nevents = static_cast<double>(chunk_detect.data_size)/3.0;
	this->evnts_.reserve(approx_nevents);
	
	// Loop continues until o==sz (==chunk_detect.size) or a invalid 
	// smf_event_type is encountered.  Note that it does _not_ break 
	// upon encountering an EOT meta event.  If an invalid event is
	// hit before o==sz, the actual data_size of the track (==o-8) is
	// written for this->data_size_.  
	uint32_t o = 8;  // offset; skip "MTrk" & the 4-byte length
	unsigned char rs {0};  // value of the running-status
	uint64_t cumtk = 0;  // Cumulative delta-time
	validate_mtrk_event_result_t curr_event;
	while (o<sz) {
		const unsigned char *curr_p = p+o;  // ptr to start of present event
		uint32_t curr_max_sz = chunk_detect.size-o;
		curr_event = validate_mtrk_event_dtstart(curr_p,rs,curr_max_sz);
		if (curr_event.error!=mtrk_event_validation_error::no_error) {
			break;
		}
		rs = curr_event.running_status;
		this->cumtk_ += curr_event.delta_t;
		this->evnts_.push_back(mtrk_event_t(curr_p,curr_event));
		o += curr_event.size;
	}
	this->data_size_ = (o-8);
}
uint32_t mtrk_t::size() const {
	return this->data_size_+8;
}
uint32_t mtrk_t::data_size() const {
	return this->data_size_;
}
uint32_t mtrk_t::nevents() const {
	return this->evnts_.size();
}
std::array<unsigned char,8> mtrk_t::get_header() const {
	std::array<unsigned char,8> r {'M','T','r','k',0,0,0,0};
	unsigned char *p_dest = &(r[4]);
	uint32_t src = this->data_size_;
	uint32_t mask = 0xFF000000u;
	for (int i=3; i>=0; --i) {
		*p_dest = static_cast<unsigned char>((mask&src)>>(i*8));
		mask>>=8;
		++p_dest;
	}
	return r;
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
mtrk_event_t& mtrk_t::operator[](uint32_t idx) {
	return this->evnts_[idx];
}
const mtrk_event_t& mtrk_t::operator[](uint32_t idx) const {
	return this->evnts_[idx];
}
bool mtrk_t::push_back(const mtrk_event_t& ev) {
	if (ev.type() == smf_event_type::invalid) {
		return false;
	}
	this->evnts_.push_back(ev);
	this->cumtk_ += ev.delta_time();
	this->data_size_ += ev.size();
	return true;
}
void mtrk_t::pop_back() {
	this->data_size_ -= this->evnts_.back().size();
	this->cumtk_ -= this->evnts_.back().delta_time();
	this->evnts_.pop_back();
}
mtrk_iterator_t mtrk_t::insert(mtrk_iterator_t it, const mtrk_event_t& ev) {
	if (ev.type() == smf_event_type::invalid) {
		return this->end();
	}
	auto vit = this->evnts_.insert(this->evnts_.begin()+(it-this->begin()),ev);
	this->data_size_ += vit->size();
	this->cumtk_ += vit->delta_time();
	return this->begin() + (vit-this->evnts_.begin());
}
void mtrk_t::clear() {
	this->evnts_.clear();
	this->data_size_ = 0;
	this->cumtk_ = 0;
}
mtrk_t::validate_t mtrk_t::validate() const {
	mtrk_t::validate_t r {};

	// vector sounding holds a list of note-on events for which a 
	// corresponding note-off event has not yet been encountered.  When the
	// corresponding note-off event is found, the on event is cleared from 
	// the list.  
	// -> Multiple "on" events w/the same note and ch number are considered 
	//    warnings, not errors; the first off event will match with the 
	//    first on event only.  
	// -> Orphan note-off events are considered warnings, not errors
	struct sounding_notes_t {
		int idx;
		int ch {0};
		int note {0};  // note number
	};
	std::vector<sounding_notes_t> sounding;
	sounding.reserve(10);  // Expect <= 10 simultaniously sounding notes???
	mtrk_event_t::channel_event_data_t curr_chev_data;
	auto is_matching_onoff = [&curr_chev_data](const sounding_notes_t& sev)->bool {
		return (curr_chev_data.ch==sev.ch
			&& curr_chev_data.p1==sev.note);
	};

	// Certain meta events are not allowed to occur after the first channel
	// event, hence found_ch_event is set to true once a ch event has been
	// encountered.  
	bool found_ch_ev = false;
	uint64_t cumtk = 0;  // Cumulative delta-time
	uint64_t cum_data_size = 0;
	for (int i=0; i<this->evnts_.size(); ++i) {
		auto curr_event_type = this->evnts_[i].type();
		if (curr_event_type == smf_event_type::invalid) {
			r.error = ("this->evnts_[i].type() == smf_event_type::invalid; i = "
				+ std::to_string(i) + "\n\t");
			return r;
		}
		
		cumtk += this->evnts_[i].delta_time();
		cum_data_size += this->evnts_[i].size();
		found_ch_ev = (found_ch_ev || (curr_event_type == smf_event_type::channel));
		
		if (is_note_on(this->evnts_[i])) {
			curr_chev_data = this->evnts_[i].midi_data();
			auto it = std::find_if(sounding.begin(),sounding.end(),
				is_matching_onoff);
			if (it!=sounding.end()) {
				r.warning += ("Multiple on-events found for note "
					+ std::to_string(curr_chev_data.p1) + " on channel "
					+ std::to_string(curr_chev_data.ch) + "; events "
					+ std::to_string(it->idx) + " and " + std::to_string(i)
					+ ".  \n");
			}  //it==sounding.end() => orphan "off" event
			sounding.push_back({i,curr_chev_data.ch,curr_chev_data.p1});
		} else if (is_note_off(this->evnts_[i])) {
			curr_chev_data = this->evnts_[i].midi_data();
			auto it = std::find_if(sounding.begin(),sounding.end(),
				is_matching_onoff);
			if (it!=sounding.end()) {
				sounding.erase(it);
			} else {
				r.warning += ("Orphan note-off event found for note "
					+ std::to_string(curr_chev_data.p1) + " on channel "
					+ std::to_string(curr_chev_data.ch) + "; event number "
					+ std::to_string(it->idx) + ".  \n");
			}
		} else if (is_eot(this->evnts_[i]) && (i!=(this->evnts_.size()-1))) {
			r.error = ("Illegal \"End-of-track\" (EOT) meta-event found "
				"at idx i = " + std::to_string(i) + " and cumulative "
				"delta-time = " + std::to_string(cumtk) + ".  EOT "
				"events are only valid as the very last event in an MTrk "
				"event sequence.  ");
			return r;
		} else if (is_seqn(this->evnts_[i]) && ((cumtk>0) || found_ch_ev)) {
			// Test for illegal sequence-number meta event occuring after 
			// a midi channel event or at t>0.  
			r.error = ("Illegal \"Sequence number\" meta-event found "
				"at idx i = " + std::to_string(i) + " and cumulative "
				"delta-time = " + std::to_string(cumtk) + ".  Sequence "
				"number events must occur before any non-zero delta times "
				"and before any MIDI [channel] events.  ");
			return r;
		}
	}

	// The final event must be a meta-end-of-track msg.  
	if ((this->evnts_.size()==0) || !is_eot(this->evnts_.back())) {
		r.error = "this->evnts_.size()==0 || !is_eot(this->evnts_.back()):  "
			"The final event in an MTrk event sequence must be an"
			"\"end-of-track\" meta event.  ";
		return r;
	}

	// std::vector<sounding_notes_t> sounding must be empty
	if (sounding.size() > 0) {
		r.error = "Orphan note-on events found (sounding.size() > 0):  \n";
		r.error += "\tidx\tch\tnote\n";
			for (int i=0; i<sounding.size(); ++i) {
				r.error += ("\t" + std::to_string(sounding[i].idx));
				r.error += ("\t" + std::to_string(sounding[i].ch));
				r.error += ("\t" + std::to_string(sounding[i].note) + "\n");
			}
		return r;
	}

	if (this->cumtk_ != cumtk) {
		r.error = ("Inconsistent values of cumtk (==" + std::to_string(cumtk)
			+ "), this->cumtk_ (==" + std::to_string(this->cumtk_) + ")\n");
		return r;
	}

	if (this->data_size_ != cum_data_size) {
		r.error = ("Inconsistent values of cum_data_size (==" 
			+ std::to_string(cum_data_size) + "), this->data_size_ (==" 
			+ std::to_string(this->data_size_) + ")\n");
		return r;
	}

	return r;
}
mtrk_t::validate_t::operator bool() const {
	return this->error.size()==0;
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

	// Note that when res.ev points at the event for which 
	// is_linked_off(res.ev), res.ev->delta_time() will _not_ be added
	// to res.cumtk.  
	for (res.ev=beg; (res.ev!=end && !is_linked_off(res.ev)); ++res.ev) {
		res.cumtk += res.ev->delta_time();
	}

	return res;
}

std::vector<linked_onoff_pair_t>
	get_linked_onoff_pairs(mtrk_const_iterator_t beg,
							mtrk_const_iterator_t end) {
	std::vector<linked_onoff_pair_t> result;

	uint32_t cumtk = 0;
	for (auto curr=beg; curr!=end; ++curr) {
		cumtk += curr->delta_time();
		if (!is_note_on(*curr)) {
			continue;
		}

		auto off = find_linked_off(curr,end,*curr);  // Note:  Not starting @ curr+1
		if (off.ev==end) {  // Orphan on event
			continue;
		}
		// Event *curr is the on-event
		auto cumtk_on = (cumtk - curr->delta_time());
		auto cumtk_off = cumtk_on + off.cumtk;
		result.push_back({{cumtk_on,curr},{cumtk_off,off.ev}});
	}

	return result;
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
	ss << std::setw(w.def) << "Tick (off)";
	ss << std::setw(w.sep) << " ";
	ss << std::setw(w.def) << "Duration";
	ss << "\n";

	auto print_half = [&ss,&w](uint32_t cumtk, const mtrk_event_t& onoff)->void {
		auto md = onoff.midi_data();
		ss << std::setw(w.ch) << std::to_string(md.ch);
		ss << std::setw(w.p1p2) << std::to_string(md.p1);
		ss << std::setw(w.p1p2) << std::to_string(md.p2);
		ss << std::setw(w.def) << std::to_string(cumtk+onoff.delta_time());
	};

	uint32_t cumtk = 0;
	for (auto curr=mtrk.begin(); curr!=mtrk.end(); ++curr) {
		cumtk += curr->delta_time();
		if (!is_note_on(*curr)) {
			continue;
		}

		// Event *curr is the present on-event
		auto cumtk_on = cumtk - curr->delta_time();
		print_half(cumtk_on,*curr);
		ss << std::setw(w.sep) << " ";
		auto off = find_linked_off(curr,mtrk.end(),*curr);
		if (off.ev==mtrk.end()) {  // Orphan on event
			ss << "NO CORRESPONDING OFF EVENT\n";
			continue;
		}
		// off.cumtk includes curr->delta_time(), but does not include
		// off.ev->delta_time().  
		auto cumtk_off = cumtk_on + off.cumtk;
		print_half(cumtk_off,*off.ev);
		ss << std::setw(w.sep) << " ";
		auto duration = cumtk_off + off.ev->delta_time()
			- (cumtk_on + curr->delta_time());
		ss << std::to_string(duration);
		ss << "\n";
	}
	return ss.str();
}




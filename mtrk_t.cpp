#include "mtrk_t.h"
#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include "midi_vlq.h"
#include <string>
#include <cstdint>
#include <vector>
#include <array>  // get_header()
#include <algorithm>  // std::find_if() in linked-pair finding functions
#include <iomanip>  // std::setw()
#include <ios>  // std::left
#include <sstream>


mtrk_t::mtrk_t(const unsigned char *p, uint32_t max_sz) {
	//auto chunk_detect = validate_chunk_header(p,max_sz);
	auto chunk_detect = read_chunk_header(p,p+max_sz);
	if (chunk_detect.id != chunk_id::mtrk) {
		return;
	}
	if (chunk_detect.length > max_sz) {
		return;
	}
	uint32_t sz = chunk_detect.length+8;

	// From experience with thousands of midi files encountered in the wild,
	// the average number of bytes per MTrk event is about 3.  
	auto approx_nevents = static_cast<double>(chunk_detect.length)/3.0;
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
		uint32_t curr_max_sz = sz-o;
		
		auto curr_event = make_mtrk_event(curr_p,curr_p+curr_max_sz,rs,nullptr);
		if (!curr_event) {
			break;
		}
		rs = curr_event.event.running_status();
		this->evnts_.push_back(curr_event.event);
		o += curr_event.size;
		/*curr_event = validate_mtrk_event_dtstart(curr_p,rs,curr_max_sz);
		if (curr_event.error!=mtrk_event_validation_error::no_error) {
			break;
		}
		rs = curr_event.running_status;
		this->evnts_.push_back(mtrk_event_t(curr_p,curr_event));
		o += curr_event.size;*/
	}
}
mtrk_t::mtrk_t(mtrk_t::const_iterator beg, mtrk_t::const_iterator end) {
	for (auto it=beg; it!=end; ++it) {
		this->push_back(*it);
	}
}
uint32_t mtrk_t::size() const {
	return this->evnts_.size();
}
uint32_t mtrk_t::capacity() const {
	return this->evnts_.capacity();
}
uint32_t mtrk_t::nbytes() const {
	uint32_t sz = 8;  // MTrk+4-byte length field
	for (const auto& e : this->evnts_) {
		sz += e.size();
	}
	return sz;
}
uint32_t mtrk_t::data_nbytes() const {
	return (this->nbytes()-8);
}
uint64_t mtrk_t::nticks() const {
	uint64_t cumtk = 0;
	for (const auto& e : this->evnts_) {
		cumtk += e.delta_time();
	}
	return cumtk;
}
std::array<unsigned char,8> mtrk_t::get_header() const {
	std::array<unsigned char,8> r {'M','T','r','k',0,0,0,0};
	unsigned char *p_dest = &(r[4]);
	uint32_t src = this->data_nbytes();
	uint32_t mask = 0xFF000000u;
	for (int i=3; i>=0; --i) {
		*p_dest = static_cast<unsigned char>((mask&src)>>(i*8));
		mask>>=8;
		++p_dest;
	}
	return r;
}
mtrk_t::iterator mtrk_t::begin() {
	if (this->evnts_.size()==0) {
		return mtrk_t::iterator(nullptr);
	}
	return mtrk_t::iterator(&(this->evnts_[0]));
}
mtrk_t::iterator mtrk_t::end() {
	if (this->evnts_.size()==0) {
		return mtrk_t::iterator(nullptr);
	}
	return mtrk_t::iterator(&(this->evnts_[0]) + this->evnts_.size());
}
mtrk_t::const_iterator mtrk_t::begin() const {
	if (this->evnts_.size()==0) {
		return mtrk_t::iterator(nullptr);
	}
	return mtrk_t::const_iterator(&(this->evnts_[0]));
}
mtrk_t::const_iterator mtrk_t::end() const {
	if (this->evnts_.size()==0) {
		return mtrk_t::iterator(nullptr);
	}
	return mtrk_t::const_iterator(&(this->evnts_[0]) + this->evnts_.size());
}
mtrk_event_t& mtrk_t::operator[](uint32_t idx) {
	return this->evnts_[idx];
}
const mtrk_event_t& mtrk_t::operator[](uint32_t idx) const {
	return this->evnts_[idx];
}
mtrk_event_t& mtrk_t::back() {
	return this->evnts_.back();
}
const mtrk_event_t& mtrk_t::back() const {
	return this->evnts_.back();
}
mtrk_event_t& mtrk_t::front() {
	return this->evnts_.front();
}
const mtrk_event_t& mtrk_t::front() const {
	return this->evnts_.front();
}
event_tk_t<mtrk_t::iterator> mtrk_t::at_cumtk(uint64_t cumtk_on) {
	event_tk_t<mtrk_t::iterator> res {this->begin(),0};
	while (res.it!=this->end() && res.tk<cumtk_on) {
		res.tk += res.it->delta_time();
		++(res.it);
	}
	return res;
}
event_tk_t<mtrk_t::const_iterator>
						mtrk_t::at_cumtk(uint64_t cumtk_on) const {
	event_tk_t<mtrk_t::const_iterator> res {this->begin(),0};
	while (res.it!=this->end() && res.tk<cumtk_on) {
		res.tk += res.it->delta_time();
		++(res.it);
	}
	return res;
}
event_tk_t<mtrk_t::iterator> mtrk_t::at_tkonset(uint64_t tk_on) {
	event_tk_t<mtrk_t::iterator> 
		res {this->begin(),0};
	while (res.it!=this->end()) {
		res.tk += res.it->delta_time();
		if (res.tk >= tk_on) {
			break;
		}
		++(res.it);
	}
	return res;
}
event_tk_t<mtrk_t::const_iterator> mtrk_t::at_tkonset(uint64_t tk_on) const {
	event_tk_t<mtrk_t::const_iterator> 
		res {this->begin(),this->begin()->delta_time()};
	while (res.it!=this->end()) {
		res.tk += res.it->delta_time();
		if (res.tk >= tk_on) {
			break;
		}
		++(res.it);
	}
	return res;
}
mtrk_event_t& mtrk_t::push_back(const mtrk_event_t& ev) {
	this->evnts_.push_back(ev);
	return this->back();
}
void mtrk_t::pop_back() {
	this->evnts_.pop_back();
}
mtrk_t::iterator mtrk_t::insert(mtrk_t::iterator it, const mtrk_event_t& ev) {
	auto vit = this->evnts_.insert(this->evnts_.begin()+(it-this->begin()),ev);
	return this->begin() + (vit-this->evnts_.begin());
}
mtrk_t::iterator mtrk_t::insert_no_tkshift(mtrk_t::iterator it, mtrk_event_t ev) {
	uint64_t new_dt = ev.delta_time();
	while ((it != this->end()) && (it->delta_time() < new_dt)) {
		new_dt -= it->delta_time();
		++it;
	}
	if (it != this->end()) {
		it->set_delta_time(it->delta_time()-new_dt);
	}
	ev.set_delta_time(new_dt);
	return this->insert(it,ev);
}
// Insert the provided event into the sequence such that its onset tick
// is == arg1 + arg2.delta_time()
mtrk_t::iterator mtrk_t::insert(uint64_t cumtk_pos, mtrk_event_t ev) {
	auto new_tk_onset = cumtk_pos+ev.delta_time();
	auto where = this->at_tkonset(cumtk_pos);
	// Insertion before where.it guarantees insertion at cumtk < cumtk_pos
	auto where_cumtk = where.tk;
	if (where.it != this->end()) {
		// TODO:  Gross behavior to have to check for the end iterator
		where_cumtk -= where.it->delta_time();
	}
	// The tkonset would be where_cumtk + ev.delta_time()
	ev.set_delta_time(new_tk_onset - where_cumtk);
	return this->insert(where.it,ev);
}
mtrk_t::iterator mtrk_t::erase(mtrk_t::iterator it) {
	auto idx = it-this->begin();
	auto vit = this->evnts_.erase(this->evnts_.begin()+idx);
	return this->begin()+(vit-this->evnts_.begin());
}
mtrk_t::const_iterator mtrk_t::erase(mtrk_t::const_iterator it) {
	auto idx = it-this->begin();
	auto vit = this->evnts_.erase(this->evnts_.begin()+idx);
	return this->begin()+(vit-this->evnts_.begin());
}
mtrk_t::iterator mtrk_t::erase_no_tkshift(mtrk_t::iterator it) {
	auto dt = it->delta_time();
	it = this->erase(it);
	if (it!=this->end()) {
		it->set_delta_time(it->delta_time()+dt);
	}
	return it;
}
void mtrk_t::clear() {
	this->evnts_.clear();
}
void mtrk_t::resize(uint32_t n) {
	this->evnts_.resize(n);
}
void mtrk_t::reserve(mtrk_t::size_type n) {
	this->evnts_.reserve(n);
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
	// is_matching_onoff(const sounding_notes_t& sev) also serves to check 
	// for duplicate note-on events.  
	struct sounding_notes_t {
		int idx;
		int ch {0};
		int note {0};  // note number
	};
	std::vector<sounding_notes_t> sounding;
	sounding.reserve(10);  // Expect <= 10 simultaneously sounding notes???
	midi_ch_event_t curr_chev_data;  //mtrk_event_t::channel_event_data_t curr_chev_data;
	auto is_matching_onoff 
		= [&curr_chev_data](const sounding_notes_t& sev) -> bool {
		return is_onoff_pair(sev.ch,sev.note,
						curr_chev_data.ch,curr_chev_data.p1);
	};

	// Certain meta events are not allowed to occur after the first channel
	// event, or after t=0.  found_ch_event is set to true once a ch event 
	// has been encountered.  
	bool found_ch_ev = false;
	uint64_t cumtk = 0;  // Cumulative delta-time
	for (int i=0; i<this->evnts_.size(); ++i) {
		auto curr_event_type = this->evnts_[i].type();
		if (curr_event_type == smf_event_type::invalid) {
			r.error = ("this->evnts_[i].type() == smf_event_type::invalid; i = "
				+ std::to_string(i) + "\n\t");
			return r;
		}
		
		cumtk += this->evnts_[i].delta_time();
		found_ch_ev = (found_ch_ev || (curr_event_type == smf_event_type::channel));
		
		if (is_note_on(this->evnts_[i])) {
			curr_chev_data = get_channel_event(this->evnts_[i]);  //curr_chev_data = this->evnts_[i].midi_data();
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
			curr_chev_data = get_channel_event(this->evnts_[i]);  //curr_chev_data = this->evnts_[i].midi_data();
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

	return r;
}
mtrk_t::validate_t::operator bool() const {
	return this->error.size()==0;
}



std::string print(const mtrk_t& mtrk) {
	std::string s {};
	s.reserve(mtrk.nbytes()*100);  // TODO:  Magic constant 100
	for (auto it=mtrk.begin(); it!=mtrk.end(); ++it) {
		//s += print(*it);
		s += print(*it,mtrk_sbo_print_opts::detail);
		s += "\n";
	}
	return s;
}
std::string print_event_arrays(mtrk_t::const_iterator beg, mtrk_t::const_iterator end) {
	std::string s {};

	sep_t sep {};
	sep.byte_pfx = "0x";
	sep.byte_sfx = "u";
	sep.elem_sep = ",";

	uint64_t cumtk = 0;
	for (auto it=beg; it!=end; ++it) { //(const auto& e : mtrk) {
		s += "{{";
		print_hexascii(it->begin(),it->end(),std::back_inserter(s),sep);
		s += ("}, " + std::to_string(cumtk) + ", "
			+ std::to_string(cumtk+it->delta_time()) + "},\n");
		cumtk += it->delta_time();
	}
	return s;
}
std::string print_event_arrays(const mtrk_t& mtrk) {
	std::string s {};

	sep_t sep {};
	sep.byte_pfx = "0x";
	sep.byte_sfx = "u";
	sep.elem_sep = ",";

	uint64_t cumtk = 0;
	for (const auto& e : mtrk) {
		s += "{{";
		print_hexascii(e.begin(),e.end(),std::back_inserter(s),sep);
		s += ("}, " + std::to_string(cumtk) + ", "
			+ std::to_string(cumtk+e.delta_time()) + "},\n");
		cumtk += e.delta_time();
	}
	return s;
}
bool is_tempo_map(const mtrk_t& trk) {
	auto found_not_allowed = [](const mtrk_event_t& ev) -> bool {
		if (ev.type()!=smf_event_type::meta) {
			return true;
		}
		auto mt_type = classify_meta_event(ev);
		return (mt_type!=meta_event_t::tempo
			&& mt_type!=meta_event_t::timesig
			&& mt_type!=meta_event_t::eot
			&& mt_type!=meta_event_t::seqn
			&& mt_type!=meta_event_t::trackname
			&& mt_type!=meta_event_t::smpteoffset
			&& mt_type!=meta_event_t::marker);
	};

	return std::find_if(trk.begin(),trk.end(),found_not_allowed)==trk.end();
}

bool is_equivalent_permutation_ignore_dt(mtrk_t::const_iterator beg1, 
				mtrk_t::const_iterator end1, mtrk_t::const_iterator beg2, 
				mtrk_t::const_iterator end2) {
	if ((end1-beg1) != (end2-beg2)) {
		return false;
	}
	// For each unique element on [beg1,end1), count the number n2 of equiv
	// elements on [beg2,end2).  If there are 0, or if n2 != the number of
	// equiv elements on [beg1,end1), return false.  
	for (auto it=beg1; it!=end1; ++it) {
		auto pred = [&it](const mtrk_event_t& rhs) -> bool {
			return is_eq_ignore_dt(*it,rhs);
		};

		if (it != std::find_if(beg1,it,pred)) {
			// An event equiv to the current event it has already been
			// encountered in the first sequence (occuring prior to it)
			// and the correct count has been verified in sequence 2.  
			continue;
		}
		auto n2 = std::count_if(beg2,end2,pred);
		if ((n2==0) || (n2 != std::count_if(it,end1,pred))) {
			return false;
		}
	}
	return true;
}
bool is_equivalent_permutation(mtrk_t::const_iterator beg1, 
				mtrk_t::const_iterator end1, mtrk_t::const_iterator beg2, 
				mtrk_t::const_iterator end2) {
	if ((end1-beg1) != (end2-beg2)) {
		return false;
	}
	auto it1 = beg1;  uint64_t ontk1 = 0;
	auto it2 = beg2;  uint64_t ontk2 = 0;
	while ((it1!=end1) && (it2!=end2)) {
		ontk1 += it1->delta_time();
		ontk2 += it2->delta_time();
		if (ontk1 != ontk2) {
			return false;
		}

		auto curr_end1 = get_simultaneous_events(it1,end1);
		auto curr_end2 = get_simultaneous_events(it2,end2);
		if (!is_equivalent_permutation_ignore_dt(it1,curr_end1,it2,curr_end2)) {
			return false;
		}
		it1 = curr_end1;
		it2 = curr_end2;
	}
	if ((it1!=end1) || (it2!=end2)) {
		return false;  // Prior loop exits if only one reaches the end
	}
	return true;
}

double duration(const mtrk_t& mtrk, const midi_time_t& t) {
	auto beg = mtrk.begin();
	auto end = mtrk.end();
	return duration(beg,end,t);
}

double duration(mtrk_t::const_iterator& beg, mtrk_t::const_iterator& end,
				const midi_time_t& t) {
	if (t.tpq == 0) {
		return -1.0;
	}
	auto curr_uspq = t.uspq;
	double curr_usptk = (1.0/t.tpq)*curr_uspq;
	double cum_us = 0.0;  // "cumulative usec"
	for (auto it=beg; it!=end; ++it) {
		cum_us += curr_usptk*(it->delta_time());
		// A tempo meta event updates the current value for us-per-q-note;
		// the value for ticks-per-q-note is constant for the track and is
		// set in the MThd chunk for the corresponding smf_t.  
		curr_uspq = get_tempo(*it,curr_uspq);
		curr_usptk = (1.0/t.tpq)*curr_uspq;
	}
	return cum_us/1000000.0;  // usec->sec
}

maybe_mtrk_t::operator bool() const {
	return this->is_valid;
}
/*maybe_mtrk_t make_mtrk(const unsigned char *p, uint32_t max_sz) {
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
	auto approx_nevents = static_cast<double>(chunk_detect.data_size)/3.0;
	result.mtrk.reserve(static_cast<mtrk_t::size_type>(approx_nevents));
	// vector sounding holds a list of note-on events for which a 
	// corresponding note-off event has not yet been encountered.  When the
	// corresponding note-off event is found, the on event is cleared from 
	// the list.  
	// -> Multiple "on" events w/the same note and ch number are not
	//    considered errors; the first off event will match with the first
	//    on event only.  
	// -> Orphan note-off events are not considered errors
	struct sounding_notes_t {
		uint32_t offset {0};  // Offset from start of the MTrk header
		int ch {0};
		int note {0};  // note number == p1
	};
	std::vector<sounding_notes_t> sounding;
	sounding.reserve(10);  // Expect <= 10 simultaneously-sounding notes ???
	midi_ch_event_t curr_chev_data;
	auto is_matching_off = [&curr_chev_data](const sounding_notes_t& sev)->bool {
		return is_onoff_pair(sev.ch,sev.note,curr_chev_data.ch,curr_chev_data.p1);
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
				+ std::to_string(o) + " (from the MTrk header):  \n"
				+ print(curr_event.error));
			return result;
		}
		
		rs = curr_event.running_status;
		cumtk += curr_event.delta_t;
		found_ch_ev = (found_ch_ev || (curr_event.type == smf_event_type::channel));
		auto curr_mtrk_event = mtrk_event_t(curr_p,curr_event);
		
		// Test for note-on/note-off event
		if (is_note_on(curr_mtrk_event)) {
			curr_chev_data = get_channel_event(curr_mtrk_event);
			sounding.push_back({o,curr_chev_data.ch,curr_chev_data.p1});
		} else if (is_note_off(curr_mtrk_event)) {
			curr_chev_data = get_channel_event(curr_mtrk_event);
			auto it = std::find_if(sounding.begin(),sounding.end(),
				is_matching_off);
			if (it!=sounding.end()) {
				sounding.erase(it);
			}
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

		result.mtrk.push_back(curr_mtrk_event);
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
		result.error = "Chunk-size error; either (1) encountered an end-of-track "
			"event at an offset lower than expected from the value of length "
			"field in the chunk header, or (2) processed the chunk header-specified "
			"number of bytes but did not encounter an end-of-tack event.  \n"
			"\toffset = " + std::to_string(o) + " (from start of the MTrk header)\n"
			"\texpected chunk size = " + std::to_string(chunk_detect.size) + "\n"
			"\tfound_eot? = " + std::to_string(found_eot);
		return result;
	}

	if (sounding.size() > 0) {
		result.error = "Orphan note-on events found (sounding.size() > 0):  \n";
		result.error += "\tch\tnote\toffset (from MTrk start)\n";
			for (int i=0; i<sounding.size(); ++i) {
				result.error += ("\t" + std::to_string(sounding[i].ch));
				result.error += ("\t" + std::to_string(sounding[i].note));
				result.error += ("\t " + std::to_string(sounding[i].offset)+"\n");
			}
		return result;
	}

	return result;
}  // make_mtrk()*/

maybe_mtrk_t make_mtrk_permissive(const unsigned char *beg, 
									const unsigned char *end) {
	return make_mtrk_permissive(beg,end,nullptr);
}
maybe_mtrk_t make_mtrk_permissive(const unsigned char *beg, 
								const unsigned char *end, mtrk_error_t *err) {
	maybe_mtrk_t result {};
	result.is_valid = false;

	auto header = read_chunk_header(beg,end);
	if (!header) {
		if (err) {
			chunk_header_error_t header_error {};
			read_chunk_header(beg,end,&header_error);
			err->code  = mtrk_error_t::errc::header_error;
			err->hdr_error = header_error;
		}
		return result;
	}
	if (header.id != chunk_id::mtrk) {
		if (err) {
			err->code = mtrk_error_t::errc::invalid_id;
			err->length = header.length;
		}
		return result;
	}
	if (header.length > mtrk_t::length_max) {
		if (err) {
			err->code = mtrk_error_t::errc::length_gt_mtrk_max;
			err->length = header.length;
		}
		return result;
	}
	if (header.length > (end-(beg+8))) {
		if (err) {
			err->code = mtrk_error_t::errc::overflow;
			err->length = header.length;
		}
		return result;
	}
	auto p = beg;
	p += 8;

	// From experience with thousands of midi files encountered in the wild,
	// the average number of bytes per MTrk event is about 3.  
	auto approx_nevents = static_cast<double>(header.length/3.0);
	result.mtrk.reserve(static_cast<mtrk_t::size_type>(approx_nevents));

	// Terminates on encountering an invalid event, an EOT, or when p==end
	auto end_state = make_mtrk_from_event_seq_array(p,end,&(result.mtrk));
	p = end_state.p;
	bool found_eot_event = ((result.mtrk.size()>0) && is_eot(result.mtrk.back()));
	auto next_event = validate_mtrk_event_dtstart(p,end_state.rs,(end-p));
	// Premature termination w/o an EOT due to having hit an invalid event
	if ((p<end) && !found_eot_event 
				&& (next_event.error!=mtrk_event_validation_error::no_error)) {
		if (err) {
			err->code = mtrk_error_t::errc::invalid_event;
			err->length = header.length;
			err->termination_offset = p-beg;
		}
		return result;
	}
	// In any event, lack of an EOT is always an error
	if (!found_eot_event) {
		if (err) {
			err->code = mtrk_error_t::errc::no_eot_event;
			err->length = header.length;
			err->termination_offset = p-beg;
		}
		return result;
	}

	result.is_valid = true;
	result.nbytes_read = p-beg;
	return result;
}

mtrk_from_event_seq_result_t make_mtrk_from_event_seq_array(const unsigned char *beg, 
						const unsigned char *end, mtrk_t *dest) {
	auto p = beg;

	bool found_eot = false;
	unsigned char rs {0x00u};  // value of the running-status
	while ((p<end) && !found_eot) {

		auto curr_event = make_mtrk_event(p,end,rs,nullptr);
		if (!curr_event) {
			break;
		}
		rs = curr_event.event.running_status();
		dest->push_back(curr_event.event);


		/*
		auto curr_event = validate_mtrk_event_dtstart(p,rs,(end-p));
		if (curr_event.error!=mtrk_event_validation_error::no_error) {
			return mtrk_from_event_seq_result_t {p,rs};
		}
		rs = curr_event.running_status;
		dest->push_back(mtrk_event_t(p,curr_event));
		*/
		if (!found_eot) {
			found_eot = is_eot(dest->back());
		}

		p += curr_event.size;
	}

	return mtrk_from_event_seq_result_t {p,rs};
}

std::string explain(const mtrk_error_t& err) {
	std::string s;  
	if (err.code==mtrk_error_t::errc::no_error) {
		return s;
	}
	s = "Invalid MTrk chunk:  ";

	if (err.code==mtrk_error_t::errc::header_error) {
		s += explain(err.hdr_error);
	} else if (err.code==mtrk_error_t::errc::overflow) {
		s += "The input range is not large enough to accommodate "
			"the number of bytes specified by the 'length' field.  "
			"length == ";
		s += std::to_string(err.length);
		s += ".  ";
	} else if (err.code==mtrk_error_t::errc::invalid_id) {
		s += "Invalid MTrk ID field; expected the first 4 bytes to be "
			"'MThd' (0x4D,54,72,6B).  ";
	} else if (err.code==mtrk_error_t::errc::length_gt_mtrk_max) {
		s += "The length field encodes a value that is too large.  "
			"This library enforces a maximum MTrk chunk length of "
			"mtrk_t::length_max.  length == ";
		s += std::to_string(err.length);
		s += ", mtrk_t::length_max == ";
		s += std::to_string(mtrk_t::length_max);
		s += ".  ";
	} else if (err.code==mtrk_error_t::errc::no_eot_event) {
		s += "No end-of-track meta event found.  Terminated scanning at offset == ";
		s += std::to_string(err.termination_offset);
		s += ".  ";
	} else if (err.code==mtrk_error_t::errc::invalid_event) {
		s += "Terminated scanning at offset == ";
		s += std::to_string(err.termination_offset);
		s += " due to an invalid mtrk event.  ";
	} else if (err.code==mtrk_error_t::errc::other) {
		s += "Error code mtrk_error_t::errc::other.  ";
	} else {
		s += "Unknown error.  ";
	}
	return s;
}



mtrk_t::iterator get_simultaneous_events(mtrk_t::iterator beg, 
					mtrk_t::iterator end) {
	auto it = get_simultaneous_events(mtrk_t::const_iterator(beg),
				mtrk_t::const_iterator(end));
	return beg+(it-beg);
}
mtrk_t::const_iterator get_simultaneous_events(mtrk_t::const_iterator beg, 
					mtrk_t::const_iterator end) {
	mtrk_t::const_iterator range_end = beg;
	if (range_end==end) {
		return range_end;
	}
	++range_end;
	while ((range_end!=end) && (range_end->delta_time()==0)) {
		++range_end;
	}
	return range_end;
}

event_tk_t<mtrk_t::const_iterator> find_linked_off(mtrk_t::const_iterator beg,
					mtrk_t::const_iterator end, const mtrk_event_t& on) {
	event_tk_t<mtrk_t::const_iterator> res {end,0};
	if (!is_note_on(on)) {
		return res;
	}
	auto md_on = get_channel_event(on);
	// Note that when beg points at the linked-off event, beg->delta_time()
	// is _not_ added to res.tk.  
	while (beg!=end && !is_onoff_pair(md_on.ch,md_on.p1,*beg)) {
		res.tk += beg->delta_time();
		++beg;
	}
	res.it=beg;
	return res;
}

std::vector<linked_onoff_pair_t>
	get_linked_onoff_pairs(mtrk_t::const_iterator beg,
							mtrk_t::const_iterator end) {
	std::vector<linked_onoff_pair_t> result;

	uint32_t tkonset = 0;
	for (auto curr=beg; curr!=end; ++curr) {
		tkonset += curr->delta_time();
		if (!is_note_on(*curr)) {
			continue;
		}

		auto off = find_linked_off(curr,end,*curr);  // Note:  Not starting @ curr+1
		if (off.it==end) {  // Orphan on event; not included in the results
			continue;
		}
		// Event *curr is the on-event
		auto cumtk_on = (tkonset - curr->delta_time());
		auto cumtk_off = cumtk_on + off.tk;
		result.push_back({{curr,cumtk_on},{off.it,cumtk_off}});
	}

	return result;
}

std::string print_linked_onoff_pairs(const mtrk_t& mtrk) {
	struct width_t {
		int def {12};  // "default"
		int p1p2 {10};
		int ch {10};
		int sep {3};
	};
	width_t w {};

	std::stringstream ss {};
	ss << std::left;
	ss << std::setw(w.ch) << "Ch";
	ss << std::setw(w.p1p2) << "p1 (on)";
	ss << std::setw(w.p1p2) << "p2 (on)";
	ss << std::setw(w.def) << "Tick (on)";
	ss << std::setw(w.sep) << " ";
	//ss << std::setw(w.ch) << "Ch (off)";
	ss << std::setw(w.p1p2) << "p1 (off)";
	ss << std::setw(w.p1p2) << "p2 (off)";
	ss << std::setw(w.def) << "Tick (off)";
	ss << std::setw(w.sep) << " ";
	ss << std::setw(w.def) << "Duration";
	ss << "\n";

	auto print_half = [&ss,&w](uint32_t cumtk, const mtrk_event_t& onoff)->void {
		auto md = get_channel_event(onoff); //.midi_data();
		//ss << std::setw(w.ch) << std::to_string(md.ch);
		ss << std::setw(w.p1p2) << std::to_string(md.p1);
		ss << std::setw(w.p1p2) << std::to_string(md.p2);
		ss << std::setw(w.def) << std::to_string(cumtk+onoff.delta_time());
	};

	auto pairs = get_linked_onoff_pairs(mtrk.begin(),mtrk.end());
	for (const auto e : pairs) {
		ss << std::setw(w.ch) << std::to_string(get_channel_event(*e.on.it).ch); //->midi_data().ch);
		print_half(e.on.tk,*e.on.it);
		ss << std::setw(w.sep) << " ";
		print_half(e.off.tk,*e.off.it);
		ss << std::setw(w.sep) << " ";
		auto duration = e.off.tk + e.off.it->delta_time() 
			- (e.on.tk + e.on.it->delta_time());
		ss << std::to_string(duration);
		ss << "\n";
	}
	return ss.str();
}




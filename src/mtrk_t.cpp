#include "mtrk_t.h"
#include "midi_time.h"
#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include "midi_status_byte.h"
#include "midi_vlq.h"
#include "print_hexascii.h"
#include <string>
#include <cstdint>
#include <utility>
#include <vector>
#include <array>  // get_header()
#include <algorithm>  // std::copy(), std::find_if() in linked-pair finding functions
#include <iomanip>  // std::setw()
#include <ios>  // std::left
#include <sstream>

jmid::mtrk_t::mtrk_t() noexcept {
	//...
}
jmid::mtrk_t::mtrk_t(jmid::mtrk_t::const_iterator beg, jmid::mtrk_t::const_iterator end) {
	this->evnts_.resize(end-beg);
	std::copy(beg,end,this->evnts_.begin());
}
jmid::mtrk_t::mtrk_t(const jmid::mtrk_t& rhs) {
	this->evnts_ = rhs.evnts_;
}
jmid::mtrk_t::mtrk_t(jmid::mtrk_t&& rhs) noexcept {
	this->evnts_ = std::move(rhs.evnts_);
}
jmid::mtrk_t& jmid::mtrk_t::operator=(const jmid::mtrk_t& rhs) {
	this->evnts_ = rhs.evnts_;
	return *this;
}
jmid::mtrk_t& jmid::mtrk_t::operator=(jmid::mtrk_t&& rhs) noexcept {
	this->evnts_ = std::move(rhs.evnts_);
	return *this;
}
jmid::mtrk_t::~mtrk_t() noexcept {
	//...
}

jmid::mtrk_t::size_type jmid::mtrk_t::size() const {
	return static_cast<jmid::mtrk_t::size_type>(this->evnts_.size());
}
jmid::mtrk_t::size_type jmid::mtrk_t::capacity() const {
	auto cap = this->evnts_.capacity();
	if (cap > jmid::mtrk_t::capacity_max) {
		return jmid::mtrk_t::capacity_max;
	}
	return static_cast<jmid::mtrk_t::size_type>(cap);
}
jmid::mtrk_t::size_type jmid::mtrk_t::nbytes() const {
	jmid::mtrk_t::size_type sz = 8;  // MTrk+4-byte length field
	for (const auto& e : this->evnts_) {
		sz += e.size();
	}
	return sz;
}
jmid::mtrk_t::size_type jmid::mtrk_t::data_nbytes() const {
	return (this->nbytes()-8);
}
std::int32_t jmid::mtrk_t::nticks() const {
	std::int32_t cumtk = 0;
	for (const auto& e : this->evnts_) {
		cumtk += e.delta_time();
	}
	return cumtk;
}
jmid::mtrk_t::iterator jmid::mtrk_t::begin() {
	if (this->evnts_.size()==0) {
		return jmid::mtrk_t::iterator(nullptr);
	}
	return jmid::mtrk_t::iterator(&(this->evnts_[0]));
}
jmid::mtrk_t::iterator jmid::mtrk_t::end() {
	if (this->evnts_.size()==0) {
		return jmid::mtrk_t::iterator(nullptr);
	}
	return jmid::mtrk_t::iterator(&(this->evnts_[0]) + this->evnts_.size());
}
jmid::mtrk_t::const_iterator jmid::mtrk_t::begin() const {
	if (this->evnts_.size()==0) {
		return jmid::mtrk_t::iterator(nullptr);
	}
	return jmid::mtrk_t::const_iterator(&(this->evnts_[0]));
}
jmid::mtrk_t::const_iterator jmid::mtrk_t::end() const {
	if (this->evnts_.size()==0) {
		return jmid::mtrk_t::iterator(nullptr);
	}
	return jmid::mtrk_t::const_iterator(&(this->evnts_[0]) + this->evnts_.size());
}
mtrk_event_t& jmid::mtrk_t::operator[](jmid::mtrk_t::size_type idx) {
	return this->evnts_[idx];
}
const mtrk_event_t& jmid::mtrk_t::operator[](jmid::mtrk_t::size_type idx) const {
	return this->evnts_[idx];
}
mtrk_event_t& jmid::mtrk_t::back() {
	return this->evnts_.back();
}
const mtrk_event_t& jmid::mtrk_t::back() const {
	return this->evnts_.back();
}
mtrk_event_t& jmid::mtrk_t::front() {
	return this->evnts_.front();
}
const mtrk_event_t& jmid::mtrk_t::front() const {
	return this->evnts_.front();
}
jmid::event_tk_t<jmid::mtrk_t::iterator> jmid::mtrk_t::at_cumtk(int32_t cumtk_on) {
	event_tk_t<mtrk_t::iterator> res {this->begin(),0};
	while (res.it!=this->end() && res.tk<cumtk_on) {
		res.tk += res.it->delta_time();
		++(res.it);
	}
	return res;
}
jmid::event_tk_t<jmid::mtrk_t::const_iterator>
						jmid::mtrk_t::at_cumtk(std::int32_t cumtk_on) const {
	jmid::event_tk_t<jmid::mtrk_t::const_iterator> res {this->begin(),0};
	while (res.it!=this->end() && res.tk<cumtk_on) {
		res.tk += res.it->delta_time();
		++(res.it);
	}
	return res;
}
jmid::event_tk_t<jmid::mtrk_t::iterator> jmid::mtrk_t::at_tkonset(std::int32_t tk_on) {
	jmid::event_tk_t<mtrk_t::iterator> 
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
jmid::event_tk_t<jmid::mtrk_t::const_iterator> jmid::mtrk_t::at_tkonset(std::int32_t tk_on) const {
	jmid::event_tk_t<jmid::mtrk_t::const_iterator> 
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
mtrk_event_t& jmid::mtrk_t::push_back(const mtrk_event_t& ev) {
	if (this->evnts_.size() < jmid::mtrk_t::capacity_max) {
		this->evnts_.push_back(ev);
	}
	return this->back();
}
void jmid::mtrk_t::pop_back() {
	this->evnts_.pop_back();
}
jmid::mtrk_t::iterator jmid::mtrk_t::insert(jmid::mtrk_t::iterator it, const mtrk_event_t& ev) {
	if (this->evnts_.size() >= jmid::mtrk_t::capacity_max) {
		return it;
	}
	auto vit = this->evnts_.insert(this->evnts_.begin()+(it-this->begin()),ev);
	return this->begin() + static_cast<mtrk_t::size_type>(vit-this->evnts_.begin());
}
jmid::mtrk_t::iterator jmid::mtrk_t::insert_no_tkshift(jmid::mtrk_t::iterator it, mtrk_event_t ev) {
	std::int32_t new_dt = ev.delta_time();
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
jmid::mtrk_t::iterator jmid::mtrk_t::insert(std::int32_t cumtk_pos, mtrk_event_t ev) {
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
jmid::mtrk_t::iterator jmid::mtrk_t::erase(jmid::mtrk_t::iterator it) {
	auto idx = it-this->begin();
	auto vit = this->evnts_.erase(this->evnts_.begin()+idx);
	return this->begin()+static_cast<jmid::mtrk_t::size_type>(vit-this->evnts_.begin());
}
jmid::mtrk_t::const_iterator jmid::mtrk_t::erase(jmid::mtrk_t::const_iterator it) {
	auto idx = it-this->begin();
	auto vit = this->evnts_.erase(this->evnts_.begin()+idx);
	return this->begin()+static_cast<mtrk_t::size_type>(vit-this->evnts_.begin());
}
jmid::mtrk_t::iterator jmid::mtrk_t::erase_no_tkshift(jmid::mtrk_t::iterator it) {
	auto dt = it->delta_time();
	it = this->erase(it);
	if (it!=this->end()) {
		it->set_delta_time(it->delta_time()+dt);
	}
	return it;
}
void jmid::mtrk_t::clear() {
	this->evnts_.clear();
}
void jmid::mtrk_t::resize(jmid::mtrk_t::size_type n) {
	n = std::clamp(n,0,jmid::mtrk_t::capacity_max);
	this->evnts_.resize(n);
}
void jmid::mtrk_t::reserve(jmid::mtrk_t::size_type n) {
	if (n > jmid::mtrk_t::capacity_max) {
		n = jmid::mtrk_t::capacity_max;
	}
	this->evnts_.reserve(n);
}
jmid::mtrk_t::validate_t jmid::mtrk_t::validate() const {
	jmid::mtrk_t::validate_t r {};

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
	jmid::ch_event_data_t curr_chev_data;  //mtrk_event_t::channel_event_data_t curr_chev_data;
	auto is_matching_onoff 
		= [&curr_chev_data](const sounding_notes_t& sev) -> bool {
		return jmid::is_onoff_pair(sev.ch,sev.note,
						curr_chev_data.ch,curr_chev_data.p1);
	};

	// Certain meta events are not allowed to occur after the first channel
	// event, or after t=0.  found_ch_event is set to true once a ch event 
	// has been encountered.  
	bool found_ch_ev = false;
	std::int32_t cumtk = 0;  // Cumulative delta-time
	for (int i=0; i<this->evnts_.size(); ++i) {
		auto s = this->evnts_[i].status_byte();
		if (!jmid::is_status_byte(s) || jmid::is_unrecognized_status_byte(s)) {
			r.error = "this->evnts_[i].status_byte() == " + std::to_string(s)
				+ " is invalid\n\t";
			return r;
		}
		
		cumtk += this->evnts_[i].delta_time();
		found_ch_ev = (found_ch_ev || jmid::is_channel_status_byte(s));
		
		if (jmid::is_note_on(this->evnts_[i])) {
			curr_chev_data = jmid::get_channel_event(this->evnts_[i]);
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
		} else if (jmid::is_note_off(this->evnts_[i])) {
			curr_chev_data = jmid::get_channel_event(this->evnts_[i]);  //curr_chev_data = this->evnts_[i].midi_data();
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
		} else if (jmid::is_eot(this->evnts_[i]) && (i!=(this->evnts_.size()-1))) {
			r.error = ("Illegal \"End-of-track\" (EOT) meta-event found "
				"at idx i = " + std::to_string(i) + " and cumulative "
				"delta-time = " + std::to_string(cumtk) + ".  EOT "
				"events are only valid as the very last event in an MTrk "
				"event sequence.  ");
			return r;
		} else if (jmid::is_seqn(this->evnts_[i]) && ((cumtk>0) || found_ch_ev)) {
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
	if ((this->evnts_.size()==0) || !jmid::is_eot(this->evnts_.back())) {
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
jmid::mtrk_t::validate_t::operator bool() const {
	return this->error.size()==0;
}



std::string jmid::print(const jmid::mtrk_t& mtrk) {
	std::string s {};
	s.reserve(mtrk.nbytes()*100);  // TODO:  Magic constant 100
	for (auto it=mtrk.begin(); it!=mtrk.end(); ++it) {
		//s += print(*it);
		s += jmid::print(*it,jmid::mtrk_sbo_print_opts::detail);
		s += "\n";
	}
	return s;
}
std::string jmid::print_event_arrays(jmid::mtrk_t::const_iterator beg, jmid::mtrk_t::const_iterator end) {
	std::string s {};

	jmid::sep_t sep {};
	sep.byte_pfx = "0x";
	sep.byte_sfx = "u";
	sep.elem_sep = ",";

	std::int32_t cumtk = 0;
	for (auto it=beg; it!=end; ++it) { //(const auto& e : mtrk) {
		s += "{{";
		jmid::print_hexascii(it->begin(),it->end(),std::back_inserter(s),sep);
		s += ("}, " + std::to_string(cumtk) + ", "
			+ std::to_string(cumtk+it->delta_time()) + "},\n");
		cumtk += it->delta_time();
	}
	return s;
}
std::string jmid::print_event_arrays(const jmid::mtrk_t& mtrk) {
	std::string s {};

	jmid::sep_t sep {};
	sep.byte_pfx = "0x";
	sep.byte_sfx = "u";
	sep.elem_sep = ",";

	std::int32_t cumtk = 0;
	for (const auto& e : mtrk) {
		s += "{{";
		jmid::print_hexascii(e.begin(),e.end(),std::back_inserter(s),sep);
		s += ("}, " + std::to_string(cumtk) + ", "
			+ std::to_string(cumtk+e.delta_time()) + "},\n");
		cumtk += e.delta_time();
	}
	return s;
}
bool jmid::is_tempo_map(const jmid::mtrk_t& trk) {
	auto found_not_allowed = [](const mtrk_event_t& ev) -> bool {
		if (!jmid::is_meta(ev)) {
			return true;
		}
		auto mt_type = jmid::classify_meta_event(ev);
		return (mt_type!=jmid::meta_event_t::tempo
			&& mt_type!=jmid::meta_event_t::timesig
			&& mt_type!=jmid::meta_event_t::eot
			&& mt_type!=jmid::meta_event_t::seqn
			&& mt_type!=jmid::meta_event_t::trackname
			&& mt_type!=jmid::meta_event_t::smpteoffset
			&& mt_type!=jmid::meta_event_t::marker);
	};

	return std::find_if(trk.begin(),trk.end(),found_not_allowed)==trk.end();
}

bool jmid::is_equivalent_permutation_ignore_dt(jmid::mtrk_t::const_iterator beg1, 
				jmid::mtrk_t::const_iterator end1, jmid::mtrk_t::const_iterator beg2, 
				jmid::mtrk_t::const_iterator end2) {
	if ((end1-beg1) != (end2-beg2)) {
		return false;
	}
	// For each unique element on [beg1,end1), count the number n2 of equiv
	// elements on [beg2,end2).  If there are 0, or if n2 != the number of
	// equiv elements on [beg1,end1), return false.  
	for (auto it=beg1; it!=end1; ++it) {
		auto pred = [&it](const mtrk_event_t& rhs) -> bool {
			return jmid::is_eq_ignore_dt(*it,rhs);
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
bool jmid::is_equivalent_permutation(jmid::mtrk_t::const_iterator beg1, 
				jmid::mtrk_t::const_iterator end1, jmid::mtrk_t::const_iterator beg2, 
				jmid::mtrk_t::const_iterator end2) {
	if ((end1-beg1) != (end2-beg2)) {
		return false;
	}
	auto it1 = beg1;  std::int32_t ontk1 = 0;
	auto it2 = beg2;  std::int32_t ontk2 = 0;
	while ((it1!=end1) && (it2!=end2)) {
		ontk1 += it1->delta_time();
		ontk2 += it2->delta_time();
		if (ontk1 != ontk2) {
			return false;
		}

		auto curr_end1 = jmid::get_simultaneous_events(it1,end1);
		auto curr_end2 = jmid::get_simultaneous_events(it2,end2);
		if (!jmid::is_equivalent_permutation_ignore_dt(it1,curr_end1,it2,curr_end2)) {
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

double jmid::duration(const jmid::mtrk_t& mtrk, const jmid::time_division_t& tdiv,
				std::int32_t tempo) {
	auto beg = mtrk.begin();
	auto end = mtrk.end();
	return jmid::duration(beg,end,tdiv,tempo);
}
double jmid::duration(jmid::mtrk_t::const_iterator beg, jmid::mtrk_t::const_iterator end,
				const jmid::time_division_t& tdiv, std::int32_t tempo) {
	double s = 0.0;  // cumulative number of seconds
	for (auto it=beg; it!=end; ++it) {
		s += jmid::ticks2sec(it->delta_time(),tdiv,tempo);
		// A tempo meta event can change the tempo:
		tempo = jmid::get_tempo(*it,tempo);
	}
	return s;
}

jmid::maybe_mtrk_t::operator bool() const {
	return (this->error == jmid::mtrk_error_t::errc::no_error);
}

std::string jmid::explain(const jmid::mtrk_error_t& err) {
	std::string s;  
	if (err.code==jmid::mtrk_error_t::errc::no_error) {
		return s;
	}
	s = "Invalid MTrk chunk:  ";

	if (err.code==jmid::mtrk_error_t::errc::header_overflow) {
		s += "Encountered end-of-input after reading < 8 bytes.  ";
	} else if (err.code==jmid::mtrk_error_t::errc::valid_but_non_mtrk_id) {
		s += "The header contains a valid, but non-MTrk chunk ID:  "
			"{ '" + std::to_string(err.header[0]) + "', " 
			+ std::to_string(err.header[1]) + "', '" 
			+ std::to_string(err.header[2]) + "', '" 
			+ std::to_string(err.header[3]) + "' }.  ";
	} else if (err.code==jmid::mtrk_error_t::errc::invalid_id) {
		s += "The header begins with an invalid SMF chunk ID.  ";
	} else if (err.code==jmid::mtrk_error_t::errc::length_gt_mtrk_max) {
		s += "The length field in the chunk header encodes the value ";
		s += std::to_string(jmid::read_be<std::uint32_t>(err.header.data()+4,err.header.data()+8));
		s += ".  This library enforces a maximum MTrk chunk length of "
			"mtrk_t::length_max == ";
		s += std::to_string(mtrk_t::length_max);
		s += ".  ";
	} else if (err.code==jmid::mtrk_error_t::errc::no_eot_event) {
		s += "No end-of-track meta event found.  ";
	} else if (err.code==jmid::mtrk_error_t::errc::invalid_event) {
		s += explain(err.event_error);
	} else if (err.code==jmid::mtrk_error_t::errc::other) {
		s += "mtrk_error_t::errc::other.  ";
	} else {
		s += "Unknown error.  ";
	}
	return s;
}



jmid::mtrk_t::iterator jmid::get_simultaneous_events(jmid::mtrk_t::iterator beg, 
					jmid::mtrk_t::iterator end) {
	auto it = jmid::get_simultaneous_events(jmid::mtrk_t::const_iterator(beg),
				jmid::mtrk_t::const_iterator(end));
	return beg+(it-beg);
}
jmid::mtrk_t::const_iterator jmid::get_simultaneous_events(jmid::mtrk_t::const_iterator beg, 
					jmid::mtrk_t::const_iterator end) {
	jmid::mtrk_t::const_iterator range_end = beg;
	if (range_end==end) {
		return range_end;
	}
	++range_end;
	while ((range_end!=end) && (range_end->delta_time()==0)) {
		++range_end;
	}
	return range_end;
}

jmid::event_tk_t<jmid::mtrk_t::const_iterator> jmid::find_linked_off(jmid::mtrk_t::const_iterator beg,
					jmid::mtrk_t::const_iterator end, const mtrk_event_t& on) {
	jmid::event_tk_t<jmid::mtrk_t::const_iterator> res {end,0};
	if (!jmid::is_note_on(on)) {
		return res;
	}
	auto md_on = jmid::get_channel_event(on);
	// Note that when beg points at the linked-off event, beg->delta_time()
	// is _not_ added to res.tk.  
	while (beg!=end && !jmid::is_onoff_pair(md_on.ch,md_on.p1,*beg)) {
		res.tk += beg->delta_time();
		++beg;
	}
	res.it=beg;
	return res;
}

std::vector<jmid::linked_onoff_pair_t>
	jmid::get_linked_onoff_pairs(jmid::mtrk_t::const_iterator beg,
							jmid::mtrk_t::const_iterator end) {
	std::vector<jmid::linked_onoff_pair_t> result;

	std::int32_t tkonset = 0;
	for (auto curr=beg; curr!=end; ++curr) {
		tkonset += curr->delta_time();
		if (!jmid::is_note_on(*curr)) {
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

std::string jmid::print_linked_onoff_pairs(const jmid::mtrk_t& mtrk) {
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

	auto print_half = [&ss,&w](std::int32_t cumtk, const mtrk_event_t& onoff)->void {
		auto md = jmid::get_channel_event(onoff); //.midi_data();
		//ss << std::setw(w.ch) << std::to_string(md.ch);
		ss << std::setw(w.p1p2) << std::to_string(md.p1);
		ss << std::setw(w.p1p2) << std::to_string(md.p2);
		ss << std::setw(w.def) << std::to_string(cumtk+onoff.delta_time());
	};

	auto pairs = jmid::get_linked_onoff_pairs(mtrk.begin(),mtrk.end());
	for (const auto e : pairs) {
		ss << std::setw(w.ch) << std::to_string(jmid::get_channel_event(*e.on.it).ch); //->midi_data().ch);
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




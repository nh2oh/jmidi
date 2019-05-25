#include "mtrk_t.h"
#include "dbklib\byte_manipulation.h"
#include <string>
#include <cstdint>
#include <vector>


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
void mtrk_t::push_back(const mtrk_event_t& ev) {
	this->evnts_.push_back(ev);
	this->data_size_ += ev.data_size();
}
// Private methods
void mtrk_t::set_data_size(uint32_t data_size) {
	this->data_size_=data_size;
}
void mtrk_t::set_events(const std::vector<mtrk_event_t>& evec) {
	this->evnts_=evec;
}

std::string print(const mtrk_t& mtrk) {
	std::string s {};
	s.reserve(mtrk.nevents()*100);  // TODO:  Magic constant 100
	for (auto it=mtrk.begin(); it!=mtrk.end(); ++it) {
		s += print(*it);
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
		result.error = "chunk_detect.type != chunk_type::track";
		return result;
	}
	
	// Process the data section of the mtrk chunk until the number of bytes
	// processed is as indicated by chunk_detect.size, or an end-of-track 
	// meta event is encountered.  
	std::vector<mtrk_event_t> evec;  // "event vector"
	evec.reserve(static_cast<double>(chunk_detect.data_size)/3.0);
	bool found_eot = false;
	uint32_t o = 8;  // offset; skip "MTrk" & the 4-byte length
	unsigned char rs {0};  // value of the running-status
	validate_mtrk_event_result_t curr_event;
	while ((o<chunk_detect.size) && !found_eot) {
		const unsigned char *curr_p = p+o;  // ptr to start of present event
		uint32_t curr_max_sz = chunk_detect.size-o;  // max excursion for present event beyond p
		
		curr_event = validate_mtrk_event_dtstart(curr_p,rs,curr_max_sz);
		if (curr_event.error!=mtrk_event_validation_error::no_error) {
			result.error = "curr_event.error!=mtrk_event_validation_error::no_error";
			return result;
		}

		rs = curr_event.running_status;
		auto curr_mtrk_event = mtrk_event_t(curr_p,curr_event.size,rs);
		evec.push_back(curr_mtrk_event);

		// Test for end-of-track msg
		if (curr_event.type==smf_event_type::meta && curr_event.size>=4) {
			uint32_t last_four = dbk::be_2_native<uint32_t>(curr_p+(curr_event.size-4));
			found_eot = ((last_four&0x00FFFFFFu)==0x00FF2F00u);
		}
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

	result.mtrk.set_events(evec);
	result.mtrk.set_data_size(chunk_detect.data_size);

	return result;
}



// Private ctor used by friend class mtrk_view_t.begin(),.end()
mtrk_iterator_t::mtrk_iterator_t(mtrk_event_t *p) {
	this->p_ = p;
}
mtrk_event_t& mtrk_iterator_t::operator*() const {
	return *(this->p_);
}
mtrk_event_t* mtrk_iterator_t::operator->() const {
	return this->p_;
}
mtrk_iterator_t& mtrk_iterator_t::operator++() {
	++(this->p_);
	return *this;
}
mtrk_iterator_t& mtrk_iterator_t::operator+=(int n) {
	this->p_ += n;
	return *this;
}
mtrk_iterator_t mtrk_iterator_t::operator+(int n) {
	mtrk_iterator_t temp = *this;
	return temp += n;
}
bool mtrk_iterator_t::operator==(const mtrk_iterator_t& rhs) const {
	return this->p_ == rhs.p_;
}
bool mtrk_iterator_t::operator!=(const mtrk_iterator_t& rhs) const {
	return this->p_ != rhs.p_;
}

// Private ctor used by friend class mtrk_view_t.begin(),.end()
mtrk_const_iterator_t::mtrk_const_iterator_t(const mtrk_event_t *p) {
	this->p_ = p;
}
const mtrk_event_t& mtrk_const_iterator_t::operator*() const {
	return *(this->p_);
}
const mtrk_event_t *mtrk_const_iterator_t::operator->() const {
	return this->p_;
}
mtrk_const_iterator_t& mtrk_const_iterator_t::operator++() {
	++(this->p_);
	return *this;
}
mtrk_const_iterator_t& mtrk_const_iterator_t::operator+=(int n) {
	this->p_ += n;
	return *this;
}
mtrk_const_iterator_t mtrk_const_iterator_t::operator+(int n) {
	mtrk_const_iterator_t temp = *this;
	return temp += n;
}
bool mtrk_const_iterator_t::operator==(const mtrk_const_iterator_t& rhs) const {
	return this->p_ == rhs.p_;
}
bool mtrk_const_iterator_t::operator!=(const mtrk_const_iterator_t& rhs) const {
	return !(*this==rhs);
}


simultanious_event_range_t 
make_simultanious_event_range(mtrk_iterator_t beg, mtrk_iterator_t end) {
	simultanious_event_range_t result {beg,end};
	if (beg==end) {
		return result;
	}

	++beg;
	while ((beg!=end) && (beg->delta_time()==0)) {
		++beg;
	}
	result.end=beg;
}


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
std::vector<mtrk_event_t>::iterator mtrk_t::begin() {
	return this->evnts_.begin();
}
std::vector<mtrk_event_t>::iterator mtrk_t::end() {
	return this->evnts_.end();
}
std::vector<mtrk_event_t>::const_iterator mtrk_t::cbegin() const {
	return this->evnts_.cbegin();
}
std::vector<mtrk_event_t>::const_iterator mtrk_t::cend() const {
	return this->evnts_.cend();
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
	for (auto it=mtrk.cbegin(); it!=mtrk.cend(); ++it) {
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


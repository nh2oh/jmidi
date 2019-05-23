#include "mtrk_t.h"
#include <string>
#include <cstdint>
#include <vector>


uint32_t mtrk_t::size() const {
	return this->hdr_data_size_+8;
}
uint32_t mtrk_t::data_size() const {
	return this->hdr_data_size_;
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
}

void mtrk_t::set_data_size(uint32_t data_size) {
	this->hdr_data_size_=data_size;
}
void mtrk_t::set_events(const std::vector<mtrk_event_t>& evec) {
	this->evnts_=evec;
}

std::string print(const mtrk_t& mtrk) {
	std::string s {};
	for (auto it=mtrk.cbegin(); it!=mtrk.cend(); ++it) {
		s += print(*it);
		s += "\n";
	}
	return s;
}


#pragma once
#include "mtrk_event_t.h"
#include <string>
#include <cstdint>
#include <vector>


//
// mtrk_t
//
// Holds an mtrk; owns the underlying data.  Stores the eent sequence as
// a std::vector<mtrk_event_t>.  
//
class mtrk_t {
public:
	uint32_t size() const;
	uint32_t data_size() const;
	uint32_t nevents() const;

	std::vector<mtrk_event_t>::iterator begin();
	std::vector<mtrk_event_t>::iterator end();
	std::vector<mtrk_event_t>::const_iterator cbegin() const;
	std::vector<mtrk_event_t>::const_iterator cend() const;

	void push_back(const mtrk_event_t&);

	void set_events(const std::vector<mtrk_event_t>&);
	void set_data_size(uint32_t);
private:
	uint32_t hdr_data_size_ {};
	std::vector<mtrk_event_t> evnts_ {};
};
std::string print(const mtrk_t&);




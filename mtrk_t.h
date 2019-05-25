#pragma once
#include "mtrk_event_t.h"
#include <string>
#include <cstdint>
#include <vector>


struct maybe_mtrk_t;
class mtrk_t;

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

	friend maybe_mtrk_t make_mtrk(const unsigned char*, uint32_t);
private:
	uint32_t data_size_ {};
	std::vector<mtrk_event_t> evnts_ {};

	// "Unsafe" setters; set_events() does not update this->data_size_
	void set_events(const std::vector<mtrk_event_t>&);
	void set_data_size(uint32_t);
};
std::string print(const mtrk_t&);

maybe_mtrk_t make_mtrk(const unsigned char*, uint32_t);
struct maybe_mtrk_t {
	std::string error {"No error"};
	mtrk_t mtrk;
	operator bool() const;
};



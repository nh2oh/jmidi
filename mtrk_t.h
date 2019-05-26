#pragma once
#include "mtrk_event_t.h"
#include <string>
#include <cstdint>
#include <vector>


struct maybe_mtrk_t;
class mtrk_t;
class mtrk_iterator_t;
class mtrk_const_iterator_t;

//
// mtrk_t
//
// Holds an mtrk; owns the underlying data.  Stores the event sequence as
// a std::vector<mtrk_event_t>.  
//
class mtrk_t {
public:
	uint32_t size() const;
	uint32_t data_size() const;
	uint32_t nevents() const;

	mtrk_iterator_t begin();
	mtrk_iterator_t end();
	mtrk_const_iterator_t begin() const;
	mtrk_const_iterator_t end() const;

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

// Declaration matches the in-class friend declaration to make the 
// name visible for lookup outside the class.  
maybe_mtrk_t make_mtrk(const unsigned char*, uint32_t);
struct maybe_mtrk_t {
	std::string error {"No error"};
	mtrk_t mtrk;
	operator bool() const;
};


// TODO:  Template this & use inheritence so it isn't so repetitive
// TODO:  The const_iterator should be obtainable from the non-const
// version.  
// TODO:  Need a - operator so can do things like:  
// result.linked.reserve(end-beg);
class mtrk_iterator_t {
public:
	mtrk_event_t& operator*() const;
	mtrk_event_t *operator->() const;
	mtrk_iterator_t& operator++();  // preincrement
	mtrk_iterator_t& operator+=(int);
	mtrk_iterator_t operator+(int);
	bool operator==(const mtrk_iterator_t&) const;
	bool operator!=(const mtrk_iterator_t&) const;
private:
	mtrk_iterator_t(mtrk_event_t*);
	mtrk_event_t *p_;
	friend class mtrk_t;
};
class mtrk_const_iterator_t {
public:
	const mtrk_event_t& operator*() const;
	const mtrk_event_t *operator->() const;
	mtrk_const_iterator_t& operator++();  // preincrement
	mtrk_const_iterator_t& operator+=(int);
	mtrk_const_iterator_t operator+(int);
	bool operator==(const mtrk_const_iterator_t&) const;
	bool operator!=(const mtrk_const_iterator_t&) const;
private:
	mtrk_const_iterator_t(const mtrk_event_t*);
	const mtrk_event_t *p_;
	friend class mtrk_t;
};



// Returns an iterator to one past the last simultanious event 
// following beg.  
mtrk_iterator_t get_simultanious_events(mtrk_iterator_t, mtrk_iterator_t);

// TODO:  Really just an mtrk_event_t w/ an integrated delta-time
struct orphan_onoff_t {
	uint32_t cumtk;
	mtrk_event_t ev;
};
struct linked_onoff_pair_t {
	uint32_t cumtk_on;
	mtrk_event_t on;
	uint32_t cumtk_off;
	mtrk_event_t off;
};
struct linked_and_orphan_onoff_pairs_t {
	std::vector<linked_onoff_pair_t> linked {};
	std::vector<orphan_onoff_t> orphan_on {};
	std::vector<orphan_onoff_t> orphan_off {};
};
linked_and_orphan_onoff_pairs_t get_linked_onoff_pairs(mtrk_iterator_t, mtrk_iterator_t);





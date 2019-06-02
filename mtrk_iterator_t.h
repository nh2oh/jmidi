#pragma once
#include "mtrk_event_t.h"
#include <cstdint>
#include <cstddef>  // std::ptrdiff_t


// TODO:  Template this so it isn't so repetitive
class mtrk_iterator_t {
public:
	mtrk_iterator_t(mtrk_event_t*);
	mtrk_event_t& operator*() const;
	mtrk_event_t *operator->() const;
	mtrk_iterator_t& operator++();  // preincrement
	mtrk_iterator_t operator++(int);  // postincrement
	mtrk_iterator_t& operator--();  // pre
	mtrk_iterator_t operator--(int);  // post
	mtrk_iterator_t& operator+=(int);
	mtrk_iterator_t operator+(int);
	std::ptrdiff_t operator-(const mtrk_iterator_t&) const;
	bool operator==(const mtrk_iterator_t&) const;
	bool operator!=(const mtrk_iterator_t&) const;
private:
	mtrk_event_t *p_;
};
class mtrk_const_iterator_t {
public:
	mtrk_const_iterator_t(mtrk_event_t*);
	mtrk_const_iterator_t(const mtrk_event_t*);
	mtrk_const_iterator_t(const mtrk_iterator_t&);
	const mtrk_event_t& operator*() const;
	const mtrk_event_t *operator->() const;
	mtrk_const_iterator_t& operator++();  // preincrement
	mtrk_const_iterator_t operator++(int);  // postincrement
	mtrk_const_iterator_t& operator--();  // pre
	mtrk_const_iterator_t operator--(int);  // post
	mtrk_const_iterator_t& operator+=(int);
	mtrk_const_iterator_t operator+(int);
	std::ptrdiff_t operator-(const mtrk_const_iterator_t& rhs) const;
	bool operator==(const mtrk_const_iterator_t&) const;
	bool operator!=(const mtrk_const_iterator_t&) const;
private:
	const mtrk_event_t *p_;
};



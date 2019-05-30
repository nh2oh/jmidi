#pragma once
#include <cstdint>
#include <iterator>  // std::random_access_iterator_tag;

class mtrk_event_t;

//
// TODO:  difference type -> std::ptrfiff_t ? 
// TODO:  Use the member typedefs in the declarations
//
class mtrk_event_iterator_t {
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = unsigned char;
	using difference_type = int;
	using pointer = unsigned char *;
	using reference = unsigned char&;

	mtrk_event_iterator_t(mtrk_event_t&);
	mtrk_event_iterator_t(mtrk_event_t*);
	unsigned char& operator*() const;
	unsigned char *operator->() const;
	mtrk_event_iterator_t& operator++();  // preincrement
	mtrk_event_iterator_t operator++(int);  // postincrement
	mtrk_event_iterator_t& operator--();  // pre
	mtrk_event_iterator_t operator--(int);  // post
	mtrk_event_iterator_t& operator+=(int);
	mtrk_event_iterator_t operator+(int);
	mtrk_event_iterator_t& operator-=(int);
	int operator-(const mtrk_event_iterator_t&) const;
	bool operator==(const mtrk_event_iterator_t&) const;
	bool operator!=(const mtrk_event_iterator_t&) const;
	bool operator<(const mtrk_event_iterator_t&) const;
	bool operator>(const mtrk_event_iterator_t&) const;
	bool operator<=(const mtrk_event_iterator_t&) const;
	bool operator>=(const mtrk_event_iterator_t&) const;
private:
	unsigned char *p_;
};

class mtrk_event_const_iterator_t {
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = unsigned char;
	using difference_type = int;
	using pointer = const unsigned char *;
	using reference = const unsigned char&;

	mtrk_event_const_iterator_t(mtrk_event_t*);
	mtrk_event_const_iterator_t(const mtrk_event_t*);
	mtrk_event_const_iterator_t(const mtrk_event_t&);
	mtrk_event_const_iterator_t(const mtrk_event_iterator_t&);
	const unsigned char& operator*() const;
	const unsigned char *operator->() const;
	mtrk_event_const_iterator_t& operator++();  // preincrement
	mtrk_event_const_iterator_t operator++(int);  // postincrement
	mtrk_event_const_iterator_t& operator--();  // pre
	mtrk_event_const_iterator_t operator--(int);  // post
	mtrk_event_const_iterator_t& operator+=(int);
	mtrk_event_const_iterator_t operator+(int);
	mtrk_event_const_iterator_t& operator-=(int);
	int operator-(const mtrk_event_const_iterator_t& rhs) const;
	bool operator==(const mtrk_event_const_iterator_t&) const;
	bool operator!=(const mtrk_event_const_iterator_t&) const;
	bool operator<(const mtrk_event_const_iterator_t&) const;
	bool operator>(const mtrk_event_const_iterator_t&) const;
	bool operator<=(const mtrk_event_const_iterator_t&) const;
	bool operator>=(const mtrk_event_const_iterator_t&) const;
private:
	const unsigned char *p_;
};


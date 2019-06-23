#pragma once
#include <cstdint>
#include <iterator>  // std::random_access_iterator_tag;

class mtrk_event_t2;

//
// TODO:  difference type -> std::ptrfiff_t ? 
// TODO:  Use the member typedefs in the declarations
//
class mtrk_event_iterator_t2 {
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = unsigned char;
	using difference_type = int;
	using pointer = unsigned char *;
	using reference = unsigned char&;

	mtrk_event_iterator_t2(mtrk_event_t2&);
	mtrk_event_iterator_t2(mtrk_event_t2*);
	unsigned char& operator*() const;
	unsigned char *operator->() const;
	mtrk_event_iterator_t2& operator++();  // preincrement
	mtrk_event_iterator_t2 operator++(int);  // postincrement
	mtrk_event_iterator_t2& operator--();  // pre
	mtrk_event_iterator_t2 operator--(int);  // post
	mtrk_event_iterator_t2& operator+=(int);
	mtrk_event_iterator_t2 operator+(int);
	mtrk_event_iterator_t2& operator-=(int);
	int operator-(const mtrk_event_iterator_t2&) const;
	bool operator==(const mtrk_event_iterator_t2&) const;
	bool operator!=(const mtrk_event_iterator_t2&) const;
	bool operator<(const mtrk_event_iterator_t2&) const;
	bool operator>(const mtrk_event_iterator_t2&) const;
	bool operator<=(const mtrk_event_iterator_t2&) const;
	bool operator>=(const mtrk_event_iterator_t2&) const;
private:
	unsigned char *p_;
};

class mtrk_event_const_iterator_t2 {
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = unsigned char;
	using difference_type = int;
	using pointer = const unsigned char *;
	using reference = const unsigned char&;

	mtrk_event_const_iterator_t2(mtrk_event_t2*);
	mtrk_event_const_iterator_t2(const mtrk_event_t2*);
	mtrk_event_const_iterator_t2(const mtrk_event_t2&);
	mtrk_event_const_iterator_t2(const mtrk_event_iterator_t2&);
	const unsigned char& operator*() const;
	const unsigned char *operator->() const;
	mtrk_event_const_iterator_t2& operator++();  // preincrement
	mtrk_event_const_iterator_t2 operator++(int);  // postincrement
	mtrk_event_const_iterator_t2& operator--();  // pre
	mtrk_event_const_iterator_t2 operator--(int);  // post
	mtrk_event_const_iterator_t2& operator+=(int);
	mtrk_event_const_iterator_t2 operator+(int);
	mtrk_event_const_iterator_t2& operator-=(int);
	int operator-(const mtrk_event_const_iterator_t2& rhs) const;
	bool operator==(const mtrk_event_const_iterator_t2&) const;
	bool operator!=(const mtrk_event_const_iterator_t2&) const;
	bool operator<(const mtrk_event_const_iterator_t2&) const;
	bool operator>(const mtrk_event_const_iterator_t2&) const;
	bool operator<=(const mtrk_event_const_iterator_t2&) const;
	bool operator>=(const mtrk_event_const_iterator_t2&) const;
private:
	const unsigned char *p_;
};


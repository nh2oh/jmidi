#pragma once
#include <cstdint>
#include <cstddef>  // std::ptrdiff_t
#include <iterator>  // std::random_access_iterator_tag;

class mtrk_event_t;

class mtrk_iterator_t {
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = mtrk_event_t;
	using difference_type = std::ptrdiff_t;
	using pointer = mtrk_event_t *;
	using reference = mtrk_event_t&;

	mtrk_iterator_t(mtrk_event_t&);
	mtrk_iterator_t(mtrk_event_t*);
	reference operator*() const;
	pointer operator->() const;
	mtrk_iterator_t& operator++();  // preincrement
	mtrk_iterator_t operator++(int);  // postincrement
	mtrk_iterator_t& operator--();  // pre
	mtrk_iterator_t operator--(int);  // post
	mtrk_iterator_t& operator+=(int);
	mtrk_iterator_t operator+(int);
	mtrk_iterator_t& operator-=(int);
	difference_type operator-(const mtrk_iterator_t&) const;
	bool operator==(const mtrk_iterator_t&) const;
	bool operator!=(const mtrk_iterator_t&) const;
	bool operator<(const mtrk_iterator_t&) const;
	bool operator>(const mtrk_iterator_t&) const;
	bool operator<=(const mtrk_iterator_t&) const;
	bool operator>=(const mtrk_iterator_t&) const;
private:
	pointer p_;
};

class mtrk_const_iterator_t {
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = mtrk_event_t;
	using difference_type = std::ptrdiff_t;
	using pointer = const mtrk_event_t *;
	using reference = const mtrk_event_t&;

	mtrk_const_iterator_t(mtrk_event_t*);
	mtrk_const_iterator_t(const mtrk_event_t*);
	mtrk_const_iterator_t(const mtrk_event_t&);
	mtrk_const_iterator_t(const mtrk_iterator_t&);
	reference operator*() const;
	pointer operator->() const;
	mtrk_const_iterator_t& operator++();  // preincrement
	mtrk_const_iterator_t operator++(int);  // postincrement
	mtrk_const_iterator_t& operator--();  // pre
	mtrk_const_iterator_t operator--(int);  // post
	mtrk_const_iterator_t& operator+=(int);
	mtrk_const_iterator_t operator+(int);
	mtrk_const_iterator_t& operator-=(int);
	difference_type operator-(const mtrk_const_iterator_t& rhs) const;
	bool operator==(const mtrk_const_iterator_t&) const;
	bool operator!=(const mtrk_const_iterator_t&) const;
	bool operator<(const mtrk_const_iterator_t&) const;
	bool operator>(const mtrk_const_iterator_t&) const;
	bool operator<=(const mtrk_const_iterator_t&) const;
	bool operator>=(const mtrk_const_iterator_t&) const;
private:
	pointer p_;
};



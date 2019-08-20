#pragma once
#include <iterator>  // std::random_access_iterator_tag;


//
// TODO
// -> Binary operators ==, !=, <, >, ... for pairs of const and non-const
//    iterators are not defined.  
//    const_it==it works b/c 'it' goes through the converting ctor 
//    generic_ra_const_iterator(it), but it==const_it fails.  
//
//
template<typename C>
class generic_ra_iterator {
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = typename C::value_type;
	using difference_type = typename C::difference_type;
	using pointer = typename C::pointer;
	using reference = typename C::reference;

	generic_ra_iterator(pointer p) : p_(p) {};

	reference operator*() const {
		return *(this->p_);
	};
	pointer operator->() const {
		return this->p_;
	};
	generic_ra_iterator& operator++() {  // pre
		++(this->p_);
		return *this;
	};
	generic_ra_iterator operator++(int) {  // post
		generic_ra_iterator temp = *this;
		++(this->p_);
		return temp;
	};
	generic_ra_iterator& operator--() {  // pre
		--(this->p_);
		return *this;
	};
	generic_ra_iterator operator--(int) {  // post
		generic_ra_iterator temp = *this;
		--(this->p_);
		return temp;
	};
	generic_ra_iterator& operator+=(const difference_type n) {
		this->p_ += n;
		return *this;
	};
	generic_ra_iterator operator+(const difference_type n) const {
		generic_ra_iterator temp = *this;
		return temp += n;
	};
	generic_ra_iterator& operator-=(const difference_type n) {
		this->p_ -= n;
		return *this;
	};
	generic_ra_iterator operator-(const difference_type n) const {
		generic_ra_iterator temp = *this;
		return temp -= n;
	};
	difference_type operator-(const generic_ra_iterator rhs) const {
		return static_cast<difference_type>(this->p_ - rhs.p_);
	};
	bool operator==(const generic_ra_iterator& rhs) const {
		return this->p_ == rhs.p_;
	};
	bool operator!=(const generic_ra_iterator& rhs) const {
		return !(*this == rhs);
	};
	bool operator<(const generic_ra_iterator& rhs) const {
		return this->p_ < rhs.p_;
	};
	bool operator>(const generic_ra_iterator& rhs) const {
		return rhs < *this;  // NB:  Impl in terms of <
	};
	bool operator<=(const generic_ra_iterator& rhs) const {
		return !(rhs < *this);  // NB:  Impl in terms of <
	};
	bool operator>=(const generic_ra_iterator& rhs) const {
		return !(*this < rhs);  // NB:  Impl in terms of <
	};

	pointer p_;
};

template <typename C>
generic_ra_iterator<C> operator+(typename generic_ra_iterator<C>::difference_type n,
				generic_ra_iterator<C> it) {
	return it += n;
};

//template <typename C>
//bool operator==(const generic_ra_iterator<C>& lhs,
//				const generic_ra_const_iterator<C>& rhs) {
//	return generic_ra_const_iterator<C>(lhs)==rhs;
//};

template<typename C>
class generic_ra_const_iterator {
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = typename C::value_type;
	using difference_type = typename C::difference_type;
	using pointer = typename C::const_pointer;
	using reference = typename C::const_reference;

	// Converting ctor from a non-const iterator to a const_iterator.  
	generic_ra_const_iterator(const generic_ra_iterator<C>& it) : p_(it.p_) {};

	generic_ra_const_iterator(pointer p) : p_(p) {};

	reference operator*() const {
		return *(this->p_);
	};
	pointer operator->() const {
		return this->p_;
	};
	generic_ra_const_iterator& operator++() {  // pre
		++(this->p_);
		return *this;
	};
	generic_ra_const_iterator operator++(int) {  // post
		generic_ra_const_iterator temp = *this;
		++(this->p_);
		return temp;
	};
	generic_ra_const_iterator& operator--() {  // pre
		--(this->p_);
		return *this;
	};
	generic_ra_const_iterator operator--(int) {  // post
		generic_ra_const_iterator temp = *this;
		--(this->p_);
		return temp;
	};
	generic_ra_const_iterator& operator+=(const difference_type n) {
		this->p_ += n;
		return *this;
	};
	generic_ra_const_iterator operator+(const difference_type n) const {
		generic_ra_const_iterator temp = *this;
		return temp += n;
	};
	generic_ra_const_iterator& operator-=(const difference_type n) {
		this->p_ -= n;
		return *this;
	};
	generic_ra_const_iterator operator-(const difference_type n) const {
		generic_ra_const_iterator temp = *this;
		return temp -= n;
	};
	difference_type operator-(const generic_ra_const_iterator rhs) const {
		return static_cast<difference_type>(this->p_ - rhs.p_);
	};
	bool operator==(const generic_ra_const_iterator& rhs) const {
		return this->p_ == rhs.p_;
	};
	bool operator!=(const generic_ra_const_iterator& rhs) const {
		return !(*this == rhs);
	};
	bool operator<(const generic_ra_const_iterator& rhs) const {
		return this->p_ < rhs.p_;
	};
	bool operator>(const generic_ra_const_iterator& rhs) const {
		return rhs < *this;  // NB:  Impl in terms of <
	};
	bool operator<=(const generic_ra_const_iterator& rhs) const {
		return !(rhs < *this);  // NB:  Impl in terms of <
	};
	bool operator>=(const generic_ra_const_iterator& rhs) const {
		return !(*this < rhs);  // NB:  Impl in terms of <
	};

	pointer p_;
};


template <typename C>
generic_ra_const_iterator<C> operator+(typename generic_ra_const_iterator<C>::difference_type n,
				generic_ra_const_iterator<C> it) {
	return it += n;
};


void generic_iterator_tests();



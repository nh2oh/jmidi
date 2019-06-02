#include "mtrk_iterator_t.h"
#include "mtrk_event_t.h"
#include <cstdint>
#include <cstddef>  // std::ptrdiff_t


// Private ctor used by friend class mtrk_view_t.begin(),.end()
mtrk_iterator_t::mtrk_iterator_t(mtrk_event_t *p) {
	this->p_ = p;
}
mtrk_event_t& mtrk_iterator_t::operator*() const {
	return *(this->p_);
}
mtrk_event_t* mtrk_iterator_t::operator->() const {
	return this->p_;
}
mtrk_iterator_t& mtrk_iterator_t::operator++() {  // preincrement
	++(this->p_);
	return *this;
}
mtrk_iterator_t mtrk_iterator_t::operator++(int) {  // postincrement
	mtrk_iterator_t temp = *this;
	++this->p_;
	return temp;
}
mtrk_iterator_t& mtrk_iterator_t::operator--() {  // pre
	--(this->p_);
	return *this;
}
mtrk_iterator_t mtrk_iterator_t::operator--(int) {  // post
	mtrk_iterator_t temp = *this;
	--this->p_;
	return temp;
}
mtrk_iterator_t& mtrk_iterator_t::operator+=(int n) {
	this->p_ += n;
	return *this;
}
mtrk_iterator_t mtrk_iterator_t::operator+(int n) {
	mtrk_iterator_t temp = *this;
	return temp += n;
}
std::ptrdiff_t mtrk_iterator_t::operator-(const mtrk_iterator_t& rhs) const {
	return this->p_-rhs.p_;
}
bool mtrk_iterator_t::operator==(const mtrk_iterator_t& rhs) const {
	return this->p_ == rhs.p_;
}
bool mtrk_iterator_t::operator!=(const mtrk_iterator_t& rhs) const {
	return this->p_ != rhs.p_;
}

mtrk_const_iterator_t::mtrk_const_iterator_t(const mtrk_iterator_t& it) {
	this->p_ = it.operator->();//p_;;
}
mtrk_const_iterator_t::mtrk_const_iterator_t(mtrk_event_t *p) {
	this->p_=p;
}
mtrk_const_iterator_t::mtrk_const_iterator_t(const mtrk_event_t *p) {
	this->p_ = p;
}
const mtrk_event_t& mtrk_const_iterator_t::operator*() const {
	return *(this->p_);
}
const mtrk_event_t *mtrk_const_iterator_t::operator->() const {
	return this->p_;
}
mtrk_const_iterator_t& mtrk_const_iterator_t::operator++() {  // preincrement
	++(this->p_);
	return *this;
}
mtrk_const_iterator_t mtrk_const_iterator_t::operator++(int) {  // postincrement
	mtrk_const_iterator_t temp = *this;
	++this->p_;
	return temp;
}
mtrk_const_iterator_t& mtrk_const_iterator_t::operator--() {  // pre
	--(this->p_);
	return *this;
}
mtrk_const_iterator_t mtrk_const_iterator_t::operator--(int) {  // post
	mtrk_const_iterator_t temp = *this;
	--this->p_;
	return temp;
}
mtrk_const_iterator_t& mtrk_const_iterator_t::operator+=(int n) {
	this->p_ += n;
	return *this;
}
mtrk_const_iterator_t mtrk_const_iterator_t::operator+(int n) {
	mtrk_const_iterator_t temp = *this;
	return temp += n;
}
std::ptrdiff_t mtrk_const_iterator_t::operator-(const mtrk_const_iterator_t& rhs) const {
	return this->p_-rhs.p_;
}
bool mtrk_const_iterator_t::operator==(const mtrk_const_iterator_t& rhs) const {
	return this->p_ == rhs.p_;
}
bool mtrk_const_iterator_t::operator!=(const mtrk_const_iterator_t& rhs) const {
	return !(*this==rhs);
}



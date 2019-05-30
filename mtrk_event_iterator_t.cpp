#include "mtrk_event_iterator_t.h"
#include "mtrk_event_t.h"
#include <cstdint>



mtrk_event_iterator_t::mtrk_event_iterator_t(mtrk_event_t& ev) {
	this->p_ = ev.data();
}
mtrk_event_iterator_t::mtrk_event_iterator_t(mtrk_event_t *p) {
	this->p_ = p->data();
}
unsigned char& mtrk_event_iterator_t::operator*() const {
	return *(this->p_);
}
unsigned char *mtrk_event_iterator_t::operator->() const {
	return this->p_;
}
mtrk_event_iterator_t& mtrk_event_iterator_t::operator++() {  // preincrement
	++(this->p_);
	return *this;
}
mtrk_event_iterator_t mtrk_event_iterator_t::operator++(int) {  // postincrement
	mtrk_event_iterator_t temp = *this;
	++this->p_;
	return temp;
}
mtrk_event_iterator_t& mtrk_event_iterator_t::operator--() {  // pre
	--(this->p_);
	return *this;
}
mtrk_event_iterator_t mtrk_event_iterator_t::operator--(int) {  // post
	mtrk_event_iterator_t temp = *this;
	--this->p_;
	return temp;
}
mtrk_event_iterator_t& mtrk_event_iterator_t::operator+=(int n) {
	this->p_ += n;
	return *this;
}
mtrk_event_iterator_t mtrk_event_iterator_t::operator+(int n) {
	mtrk_event_iterator_t temp = *this;
	return temp += n;
}
mtrk_event_iterator_t& mtrk_event_iterator_t::operator-=(int n) {
	this->p_ -= n;
	return *this;
}
int mtrk_event_iterator_t::operator-(const mtrk_event_iterator_t& rhs) const {
	return this->p_-rhs.p_;
}
bool mtrk_event_iterator_t::operator==(const mtrk_event_iterator_t& rhs) const {
	return this->p_ == rhs.p_;
}
bool mtrk_event_iterator_t::operator!=(const mtrk_event_iterator_t& rhs) const {
	return this->p_ != rhs.p_;
}
bool mtrk_event_iterator_t::operator<(const mtrk_event_iterator_t& rhs) const {
	return this->p_ < rhs.p_;
}
bool mtrk_event_iterator_t::operator>(const mtrk_event_iterator_t& rhs) const {
	return this->p_ > rhs.p_;
}
bool mtrk_event_iterator_t::operator<=(const mtrk_event_iterator_t& rhs) const {
	return this->p_ <= rhs.p_;
}
bool mtrk_event_iterator_t::operator>=(const mtrk_event_iterator_t& rhs) const {
	return this->p_ >= rhs.p_;
}








mtrk_event_const_iterator_t::mtrk_event_const_iterator_t(const mtrk_event_iterator_t& it) {
	this->p_ = it.operator->();//p_;;
}
mtrk_event_const_iterator_t::mtrk_event_const_iterator_t(mtrk_event_t *p) {
	this->p_ = p->data();
}
mtrk_event_const_iterator_t::mtrk_event_const_iterator_t(const mtrk_event_t& ev) {
	this->p_ = ev.data();
}
const unsigned char& mtrk_event_const_iterator_t::operator*() const {
	return *(this->p_);
}
const unsigned char *mtrk_event_const_iterator_t::operator->() const {
	return this->p_;
}
mtrk_event_const_iterator_t& mtrk_event_const_iterator_t::operator++() {  // preincrement
	++(this->p_);
	return *this;
}
mtrk_event_const_iterator_t mtrk_event_const_iterator_t::operator++(int) {  // postincrement
	mtrk_event_const_iterator_t temp = *this;
	++this->p_;
	return temp;
}
mtrk_event_const_iterator_t& mtrk_event_const_iterator_t::operator--() {  // pre
	--(this->p_);
	return *this;
}
mtrk_event_const_iterator_t mtrk_event_const_iterator_t::operator--(int) {  // post
	mtrk_event_const_iterator_t temp = *this;
	--this->p_;
	return temp;
}
mtrk_event_const_iterator_t& mtrk_event_const_iterator_t::operator+=(int n) {
	this->p_ += n;
	return *this;
}
mtrk_event_const_iterator_t mtrk_event_const_iterator_t::operator+(int n) {
	mtrk_event_const_iterator_t temp = *this;
	return temp += n;
}
mtrk_event_const_iterator_t& mtrk_event_const_iterator_t::operator-=(int n) {
	this->p_ -= n;
	return *this;
}
int mtrk_event_const_iterator_t::operator-(const mtrk_event_const_iterator_t& rhs) const {
	return this->p_-rhs.p_;
}
bool mtrk_event_const_iterator_t::operator==(const mtrk_event_const_iterator_t& rhs) const {
	return this->p_ == rhs.p_;
}
bool mtrk_event_const_iterator_t::operator!=(const mtrk_event_const_iterator_t& rhs) const {
	return !(*this==rhs);
}
bool mtrk_event_const_iterator_t::operator<(const mtrk_event_const_iterator_t& rhs) const {
	return this->p_ < rhs.p_;
}
bool mtrk_event_const_iterator_t::operator>(const mtrk_event_const_iterator_t& rhs) const {
	return this->p_ > rhs.p_;
}
bool mtrk_event_const_iterator_t::operator<=(const mtrk_event_const_iterator_t& rhs) const {
	return this->p_ <= rhs.p_;
}
bool mtrk_event_const_iterator_t::operator>=(const mtrk_event_const_iterator_t& rhs) const {
	return this->p_ >= rhs.p_;
}


#include "mtrk_event_iterator_t2.h"
#include "mtrk_event_t2.h"
#include <cstdint>



mtrk_event_iterator_t2::mtrk_event_iterator_t2(mtrk_event_t2& ev) {
	this->p_ = ev.data();
}
mtrk_event_iterator_t2::mtrk_event_iterator_t2(mtrk_event_t2 *p) {
	this->p_ = p->data();
}
unsigned char& mtrk_event_iterator_t2::operator*() const {
	return *(this->p_);
}
unsigned char *mtrk_event_iterator_t2::operator->() const {
	return this->p_;
}
mtrk_event_iterator_t2& mtrk_event_iterator_t2::operator++() {  // preincrement
	++(this->p_);
	return *this;
}
mtrk_event_iterator_t2 mtrk_event_iterator_t2::operator++(int) {  // postincrement
	mtrk_event_iterator_t2 temp = *this;
	++this->p_;
	return temp;
}
mtrk_event_iterator_t2& mtrk_event_iterator_t2::operator--() {  // pre
	--(this->p_);
	return *this;
}
mtrk_event_iterator_t2 mtrk_event_iterator_t2::operator--(int) {  // post
	mtrk_event_iterator_t2 temp = *this;
	--this->p_;
	return temp;
}
mtrk_event_iterator_t2& mtrk_event_iterator_t2::operator+=(int n) {
	this->p_ += n;
	return *this;
}
mtrk_event_iterator_t2 mtrk_event_iterator_t2::operator+(int n) {
	mtrk_event_iterator_t2 temp = *this;
	return temp += n;
}
mtrk_event_iterator_t2& mtrk_event_iterator_t2::operator-=(int n) {
	this->p_ -= n;
	return *this;
}
int mtrk_event_iterator_t2::operator-(const mtrk_event_iterator_t2& rhs) const {
	return this->p_-rhs.p_;
}
bool mtrk_event_iterator_t2::operator==(const mtrk_event_iterator_t2& rhs) const {
	return this->p_ == rhs.p_;
}
bool mtrk_event_iterator_t2::operator!=(const mtrk_event_iterator_t2& rhs) const {
	return this->p_ != rhs.p_;
}
bool mtrk_event_iterator_t2::operator<(const mtrk_event_iterator_t2& rhs) const {
	return this->p_ < rhs.p_;
}
bool mtrk_event_iterator_t2::operator>(const mtrk_event_iterator_t2& rhs) const {
	return this->p_ > rhs.p_;
}
bool mtrk_event_iterator_t2::operator<=(const mtrk_event_iterator_t2& rhs) const {
	return this->p_ <= rhs.p_;
}
bool mtrk_event_iterator_t2::operator>=(const mtrk_event_iterator_t2& rhs) const {
	return this->p_ >= rhs.p_;
}








mtrk_event_const_iterator_t2::mtrk_event_const_iterator_t2(const mtrk_event_iterator_t2& it) {
	this->p_ = it.operator->();//p_;;
}
mtrk_event_const_iterator_t2::mtrk_event_const_iterator_t2(mtrk_event_t2 *p) {
	this->p_ = p->data();
}
mtrk_event_const_iterator_t2::mtrk_event_const_iterator_t2(const mtrk_event_t2& ev) {
	this->p_ = ev.data();
}
const unsigned char& mtrk_event_const_iterator_t2::operator*() const {
	return *(this->p_);
}
const unsigned char *mtrk_event_const_iterator_t2::operator->() const {
	return this->p_;
}
mtrk_event_const_iterator_t2& mtrk_event_const_iterator_t2::operator++() {  // preincrement
	++(this->p_);
	return *this;
}
mtrk_event_const_iterator_t2 mtrk_event_const_iterator_t2::operator++(int) {  // postincrement
	mtrk_event_const_iterator_t2 temp = *this;
	++this->p_;
	return temp;
}
mtrk_event_const_iterator_t2& mtrk_event_const_iterator_t2::operator--() {  // pre
	--(this->p_);
	return *this;
}
mtrk_event_const_iterator_t2 mtrk_event_const_iterator_t2::operator--(int) {  // post
	mtrk_event_const_iterator_t2 temp = *this;
	--this->p_;
	return temp;
}
mtrk_event_const_iterator_t2& mtrk_event_const_iterator_t2::operator+=(int n) {
	this->p_ += n;
	return *this;
}
mtrk_event_const_iterator_t2 mtrk_event_const_iterator_t2::operator+(int n) {
	mtrk_event_const_iterator_t2 temp = *this;
	return temp += n;
}
mtrk_event_const_iterator_t2& mtrk_event_const_iterator_t2::operator-=(int n) {
	this->p_ -= n;
	return *this;
}
int mtrk_event_const_iterator_t2::operator-(const mtrk_event_const_iterator_t2& rhs) const {
	return this->p_-rhs.p_;
}
bool mtrk_event_const_iterator_t2::operator==(const mtrk_event_const_iterator_t2& rhs) const {
	return this->p_ == rhs.p_;
}
bool mtrk_event_const_iterator_t2::operator!=(const mtrk_event_const_iterator_t2& rhs) const {
	return !(*this==rhs);
}
bool mtrk_event_const_iterator_t2::operator<(const mtrk_event_const_iterator_t2& rhs) const {
	return this->p_ < rhs.p_;
}
bool mtrk_event_const_iterator_t2::operator>(const mtrk_event_const_iterator_t2& rhs) const {
	return this->p_ > rhs.p_;
}
bool mtrk_event_const_iterator_t2::operator<=(const mtrk_event_const_iterator_t2& rhs) const {
	return this->p_ <= rhs.p_;
}
bool mtrk_event_const_iterator_t2::operator>=(const mtrk_event_const_iterator_t2& rhs) const {
	return this->p_ >= rhs.p_;
}


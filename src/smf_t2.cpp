#include "smf_t2.h"





jmid::smf_t2::smf_t2() {
	//...
}

jmid::smf_t2::smf_t2(std::int32_t ntrks, std::int32_t fmt, jmid::time_division_t tdiv) {
	if (ntrks < 0) {
		ntrks = 0;
	}
	this->mtrks_.resize(ntrks);
	if ((fmt != 0) && (fmt != 1) && (fmt != 2)) {
		fmt = 0;
	}
	this->fmt_ = fmt;
	this->tdiv_ = tdiv;
}






std::int64_t jmid::smf_t2::size() const {
	return this->ntrks();
}
std::int64_t jmid::smf_t2::nchunks() const {
	return this->chunkorder_.size();
}
std::int64_t jmid::smf_t2::ntrks() const {
	return this->mtrks_.size();
}
std::int64_t jmid::smf_t2::nuchks() const {
	return this->uchks_.size();
}
std::int64_t jmid::smf_t2::nbytes() const {
	return 0;  // TODO
}





std::int32_t jmid::smf_t2::format() const {
	return this->fmt_;
}
jmid::time_division_t jmid::smf_t2::time_division() const {
	return this->tdiv_;
}





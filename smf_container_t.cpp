#include "smf_container_t.h"
#include "midi_raw.h"
#include "mthd_container_t.h"
#include "mtrk_container_t.h"
#include <string>
#include <cstdint>

smf_container_t::smf_container_t(const validate_smf_result_t& maybe_smf) {
	if (!maybe_smf.is_valid) {
		std::abort();
	}

	this->fname_ = maybe_smf.fname;
	this ->chunk_idxs_ = maybe_smf.chunk_idxs;
	this->n_mtrk_ = maybe_smf.n_mtrk;
	this->n_unknown_ = maybe_smf.n_unknown;
	this->p_ = maybe_smf.p;
	this->size_ = maybe_smf.size;
}

int smf_container_t::n_tracks() const {
	return this->n_mtrk_;
}
int smf_container_t::n_chunks() const {
	return this->n_mtrk_ + this->n_unknown_ + 1;  // +1 => MThd
}
mthd_container_t smf_container_t::get_header() const {
	// I am assuming this->chunk_idxs_[0] has .type == chunk_type::header
	// and offser == 0
	return mthd_container_t {this->p_,this->chunk_idxs_[0].size};
}
mtrk_container_t smf_container_t::get_track(int n) const {
	int curr_trackn {0};
	for (const auto& e : this->chunk_idxs_) {
		if (e.type == chunk_type::track) {
			if (n == curr_trackn) {
				return mtrk_container_t {this->p_ + e.offset, e.size};
			}
			++curr_trackn;
		}
	}

	std::abort();
}
bool smf_container_t::get_chunk(int n) const {
	// don't yet have a generic chunk container

	return false;
}

std::string smf_container_t::fname() const {
	return this->fname_;
}

std::string print(const smf_container_t& smf) {
	std::string s {};

	s += smf.fname();
	s += "\n";

	auto mthd = smf.get_header();
	s += "Header (MThd) \t(data_size = ";
	s += std::to_string(mthd.data_size()) ;
	s += ", size = ";
	s += std::to_string(mthd.size());
	s += "):\n";
	s += print(mthd);
	s += "\n";

	for (int i=0; i<smf.n_tracks(); ++i) {
		auto curr_trk = smf.get_track(i);
		s += ("Track (MTrk) " + std::to_string(i) 
			+ "\t(data_size = " + std::to_string(curr_trk.data_size())
			+ ", size = " + std::to_string(curr_trk.size()) + "):\n");
		s += print(curr_trk);
		s += "\n";
	}

	return s;
}

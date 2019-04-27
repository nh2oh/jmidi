#include "smf_container_t.h"
#include "midi_raw.h"
#include "mthd_container_t.h"
#include "mtrk_container_t.h"
#include <string>
#include <cstdint>
#include <vector>
#include <algorithm>







smf_t::smf_t(const validate_smf_result_t& maybe_smf) {
	if (!maybe_smf.is_valid) {
		std::abort();
	}

	this->fname_ = maybe_smf.fname;

	const unsigned char *p = maybe_smf.p;
	for (const auto& e : maybe_smf.chunk_idxs) {
		std::vector<unsigned char> temp {};
		std::copy(p+e.offset,p+e.offset+e.size,std::back_inserter(temp));
		this->d_.push_back(temp);
		temp.clear();
	}
}
uint16_t smf_t::ntrks() const {
	uint16_t ntracks {0};
	for (const auto& e : this->d_) {
		if (detect_chunk_type_unsafe(e.data())==chunk_type::track) {
			++ntracks;
		}
	}
	return ntracks;
}
uint16_t smf_t::nchunks() const {
	return static_cast<uint16_t>(this->d_.size());
}
uint16_t smf_t::format() const {
	return this->get_header_view().format();
}
uint16_t smf_t::division() const {
	return this->get_header_view().division();
}
uint32_t smf_t::mthd_size() const {
	return this->get_header_view().size();
}
uint32_t smf_t::mthd_data_length() const {
	return this->get_header_view().data_size();
}
mtrk_view_t smf_t::get_track_view(int n) const {
	int curr_trackn {0};
	for (const auto& e : this->d_) {
		if (detect_chunk_type_unsafe(e.data())==chunk_type::track) {
			if (n == curr_trackn) {
				return mtrk_view_t(e.data(), e.size());
			}
			++curr_trackn;
		}
	}

	std::abort();
}
mthd_container_t smf_t::get_header_view() const {
	for (const auto& e : this->d_) {
		if (detect_chunk_type_unsafe(e.data())==chunk_type::header) {
			return mthd_container_t(e.data(),e.size());
		}
	}

	std::abort();
}
std::string smf_t::fname() const {
	return this->fname_;
}

std::string print(const smf_t& smf) {
	std::string s {};

	s += smf.fname();
	s += "\n";

	s += "Header (MThd) \t(data_size = ";
	//s += " smf_t has no get_header()-like method yet :(\n";
	s += std::to_string(smf.mthd_data_length()) ;
	s += ", size = ";
	s += std::to_string(smf.mthd_size());
	s += "):\n";
	auto mthd = smf.get_header_view();
	s += print(mthd);
	s += "\n";

	// where smf.get_track_view(i) => mtrk_view_t
	for (int i=0; i<smf.ntrks(); ++i) {
		auto curr_trk = smf.get_track_view(i);
		s += ("Track (MTrk) " + std::to_string(i) 
			+ "\t(data_size = " + std::to_string(curr_trk.data_length())
			+ ", size = " + std::to_string(curr_trk.size()) + "):\n");
		s += print(curr_trk);
		s += "\n";
	}

	return s;
}







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
//mtrk_container_t smf_container_t::get_track(int n) const {
mtrk_view_t smf_container_t::get_track(int n) const {
	int curr_trackn {0};
	for (const auto& e : this->chunk_idxs_) {
		if (e.type == chunk_type::track) {
			if (n == curr_trackn) {
				//return mtrk_container_t {this->p_ + e.offset, e.size};
				return mtrk_view_t(this->p_ + e.offset, e.size);
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


	// where smf.get_track(i) => mtrk_view_t
	for (int i=0; i<smf.n_tracks(); ++i) {
		auto curr_trk = smf.get_track(i);
		s += ("Track (MTrk) " + std::to_string(i) 
			+ "\t(data_size = " + std::to_string(curr_trk.data_length())
			+ ", size = " + std::to_string(curr_trk.size()) + "):\n");
		s += print(curr_trk);
		s += "\n";
	}

	// where smf.get_track(i) => mtrk_container_t
	//for (int i=0; i<smf.n_tracks(); ++i) {
	//	auto curr_trk = smf.get_track(i);
	//	s += ("Track (MTrk) " + std::to_string(i) 
	//		+ "\t(data_size = " + std::to_string(curr_trk.data_size())
	//		+ ", size = " + std::to_string(curr_trk.size()) + "):\n");
	//	s += print(curr_trk);
	//	s += "\n";
	//}

	return s;
}

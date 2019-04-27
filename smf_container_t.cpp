#include "smf_container_t.h"
#include "midi_raw.h"
#include "mthd_container_t.h"
#include "mtrk_container_t.h"
#include <string>
#include <cstdint>
#include <vector>
#include <algorithm>  // std::copy() in smf_t::smf_t(const validate_smf_result_t& maybe_smf)
#include <iostream>



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
	return this->get_header_view().data_length();
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
mthd_view_t smf_t::get_header_view() const {
	for (const auto& e : this->d_) {
		if (detect_chunk_type_unsafe(e.data())==chunk_type::header) {
			return mthd_view_t(e.data(),e.size());
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


//
// Draft of the the chrono_iterator
//
// TODO:
// -Creating mtrk_iterator_t's to _temporary_ mtrk_view_t's works but is horrible
//
std::string print_events_chrono(const smf_t& smf) {
	std::string s {};
	struct range_t {
		int traknum;
		mtrk_iterator_t beg;
		mtrk_iterator_t end;
		uint32_t cumtk;  // cumulative tick for the track
	};
	std::vector<range_t> its;
	for (int i=0; i<smf.ntrks(); ++i) {
		auto curr_trk = smf.get_track_view(i);
		its.push_back({i,curr_trk.begin(),curr_trk.end(),0});
	}
	
	auto finished = [](const std::vector<range_t>& its)->bool {
		bool result {true};
		for (const auto& e : its) {
			result &= (e.beg==e.end);
		}
		return result;
	};
	auto idx_min_range = [](const std::vector<range_t>& its, uint32_t ticks_min)->int {
		for (int i=0; i<its.size(); ++i) {
			if (its[i].beg==its[i].end) {
				continue;
			}
			if (its[i].cumtk<=ticks_min) {
				return i;
			}
		}
		return -1;
	};

	int evnum {0};
	int curr_range_idx = -1;
	while (!finished(its)) {
		curr_range_idx = idx_min_range(its,curr_range_idx);
		if (curr_range_idx==-1) {
			std::cout << "wait";
		}
		if (its[curr_range_idx].beg==its[curr_range_idx].end) {
			continue;
		}
		auto curr_mtrk_event = *(its[curr_range_idx].beg);
		
		if (curr_mtrk_event.delta_time() > 10000) {
			s += "what";
		}
		std::cout << "TRACK = " << std::to_string(its[curr_range_idx].traknum) << "; "
			 << "CUM_TICK = " << std::to_string(its[curr_range_idx].cumtk) << "\n";
		std::cout << print(curr_mtrk_event,mtrk_sbo_print_opts::debug) << std::endl;

		its[curr_range_idx].cumtk += curr_mtrk_event.delta_time();
		s += "TRACK = " + std::to_string(its[curr_range_idx].traknum) + "; "
			 + "CUM_TICK = " + std::to_string(its[curr_range_idx].cumtk) + "\n";
		s += print(curr_mtrk_event);
		s += "\n";

		++(its[curr_range_idx].beg);
		++evnum;
		if (evnum >= 400) {
			break;
		}
	}


	return s;
}




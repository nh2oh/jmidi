#include "smf_t.h"
#include "midi_raw.h"
#include "mthd_t.h"
#include "mtrk_container_t.h"
#include "dbklib\byte_manipulation.h"
#include <string>
#include <cstdint>
#include <vector>
#include <algorithm>  // std::copy() in smf2_t::smf2_t(const validate_smf_result_t& maybe_smf)
#include <iostream>
#include <exception>
#include <iomanip>  // std::setw()
#include <ios>  // std::left
#include <sstream>
#include <iostream>
#include <filesystem>
#include <fstream>



uint32_t smf2_t::size() const {
	uint32_t n = 0;
	n = this->mthd_.size();
	for (const auto& e : this->mtrks_) {
		n += e.size();
	}
	for (const auto& e : this->uchks_) {
		n += e.size();
	}
	return n;
}
uint16_t smf2_t::ntrks() const {
	return this->mtrks_.size();
}
uint16_t smf2_t::nchunks() const {
	return static_cast<uint16_t>(this->mtrks_.size()+this->uchks_.size()+1);
}
uint16_t smf2_t::format() const {
	return this->mthd_.format();
}
uint16_t smf2_t::division() const {
	return this->mthd_.division();
}
uint32_t smf2_t::mthd_size() const {
	return this->mthd_.size();
}
uint32_t smf2_t::mthd_data_size() const {
	return this->mthd_.data_size();
}
std::string smf2_t::fname() const {
	return this->fname_;
}
uint32_t smf2_t::track_size(int trackn) const {
	return this->mtrks_[trackn].size();
}
uint32_t smf2_t::track_data_size(int trackn) const {
	return this->mtrks_[trackn].data_size();
}
mthd_view_t smf2_t::get_header_view() const {
	return this->mthd_.get_view();
}
const mthd_t& smf2_t::get_header() const {
	return this->mthd_;
}
const mtrk_t& smf2_t::get_track(int trackn) const {
	return this->mtrks_[trackn];
}
mtrk_t& smf2_t::get_track(int trackn) {
	return this->mtrks_[trackn];
}

void smf2_t::set_fname(const std::string& fname) {
	this->fname_ = fname;
}

void smf2_t::set_mthd(const validate_mthd_chunk_result_t& val_mthd) {
	this->mthd_.set(val_mthd);
}
void smf2_t::append_mtrk(const mtrk_t& mtrk) {
	this->mtrks_.push_back(mtrk);
}
void smf2_t::append_uchk(const std::vector<unsigned char>& uchk) {
	this->uchks_.push_back(uchk);
}

std::string print(const smf2_t& smf) {
	std::string s {};
	s.reserve(20*smf.size());  // TODO: Magic constant 20

	s += smf.fname();
	s += "\nsize = " + std::to_string(smf.size()) + ", "
		"num chunks = " + std::to_string(smf.nchunks()) + ", "
		"num tracks = " + std::to_string(smf.ntrks());
	s += "\n\n";

	s += print(smf.get_header_view());
	s += "\n\n";

	for (int i=0; i<smf.ntrks(); ++i) {
		const mtrk_t& curr_trk = smf.get_track(i);
		s += ("Track (MTrk) " + std::to_string(i) 
				+ "\t(data_size = " + std::to_string(curr_trk.data_size())
				+ ", size = " + std::to_string(curr_trk.size()) + "):\n");
		s += print(curr_trk);
		s += "\n";
	}

	return s;
}







maybe_smf2_t::operator bool() const {
	return this->error == "No error";
}

maybe_smf2_t read_smf2(const std::string& fn) {
	maybe_smf2_t result {};
	
	// Read the file into fdata, close the file
	std::vector<unsigned char> fdata {};
	std::filesystem::path fp(fn);
	std::basic_ifstream<unsigned char> f(fp,std::ios_base::in|std::ios_base::binary);
	if (!f.is_open() || !f.good()) {
		result.error = "(!f.is_open() || !f.good())";
		return result;
	}
	f.seekg(0,std::ios::end);
	auto fsize = f.tellg();
	f.seekg(0,std::ios::beg);
	fdata.resize(fsize);
	f.read(fdata.data(),fsize);
	f.close();

	result.smf.set_fname(fn);

	uint32_t o {0};  // Global offset into the fdata vector
	const unsigned char *p = fdata.data();
	auto curr_chunk = validate_chunk_header(p,fdata.size());
	if (curr_chunk.type != chunk_type::header) {
		result.error += "curr_chunk.type != chunk_type::header at offset 0.  ";
		result.error += "A valid midi file must begin w/ an MThd chunk.  \n";
		return result;
	}
	auto val_mthd = validate_mthd_chunk(p,curr_chunk.size-o);
	if (val_mthd.error!=mthd_validation_error::no_error) {
		result.error += "val_mthd.error!=mthd_validation_error::no_error\n";
		return result;
	}
	result.smf.set_mthd(val_mthd);

	o += curr_chunk.size;
	// Note: Loop terminates based on fdata.size(), not on the number of 
	// chunks read; mthd_.ntrks() reports the number of track chunks, but not
	// the number of other chunk types.  
	while (o<fdata.size()) {  
		const unsigned char *curr_p = p+o;
		uint32_t curr_max_sz = fdata.size()-o;
		auto curr_chunk = validate_chunk_header(curr_p,curr_max_sz);

		if (curr_chunk.type == chunk_type::track) {
			auto curr_mtrk = make_mtrk(curr_p,curr_max_sz);
			if (!curr_mtrk) {
				result.error = "!curr_mtrk";
				return result;
			}
			result.smf.append_mtrk(curr_mtrk.mtrk);
		} else if (curr_chunk.type == chunk_type::unknown) {
			std::vector<unsigned char> curr_uchk {};
			curr_uchk.reserve(curr_chunk.size);
			std::copy(curr_p,curr_p+curr_chunk.size,
				std::back_inserter(curr_uchk));
			result.smf.append_uchk(curr_uchk);
		} else {
			result.error = "curr_chunk.type != track || unknown";
			return result;
		}

		o += curr_chunk.size;
	}
	// If o<fdata.size(), might indicate the file is zero-padded after 
	// the final mtrk chunk.  o>fdata.size() is clearly an error.  
	if (o != fdata.size()) {
		result.error = "offset != fdata.size().";
		return result;
	}

	return result;
}


std::vector<all_smf_events_dt_ordered_t> get_events_dt_ordered(const smf2_t& smf) {
	std::vector<all_smf_events_dt_ordered_t> result;
	//result.reserve(smf.nchunks...
	
	for (int i=0; i<smf.ntrks(); ++i) {
		const auto& curr_trk = smf.get_track(i);
		uint32_t cumtk = 0;
		for (const auto& e : curr_trk) {
			cumtk += e.delta_time();
			result.push_back({e,cumtk,i});
		}
	}

	auto lt_ev = [](const all_smf_events_dt_ordered_t& lhs, 
					const all_smf_events_dt_ordered_t& rhs)->bool {
		if (lhs.cumtk == rhs.cumtk) {
			return lhs.trackn < rhs.trackn;
		} else {
			return lhs.cumtk < rhs.cumtk;
		}
	};
	std::sort(result.begin(),result.end(),lt_ev);

	return result;
}

std::string print(const std::vector<all_smf_events_dt_ordered_t>& evs) {
	struct width_t {
		int def {12};  // "default"
		int sep {3};
		int tick {10};
		int type {10};
		int trk {8};
		int dat_sz {12};
	};
	width_t w {};

	std::stringstream ss {};
	ss << std::left;
	ss << std::setw(w.tick) << "Tick";
	ss << std::setw(w.type) << "Type";
	ss << std::setw(w.dat_sz) << "Data_size";
	ss << std::setw(w.trk) << "Track";
	ss << std::setw(w.trk) << "Bytes";
	ss << "\n";
	
	for (const auto& e : evs) {
		ss << std::setw(w.tick) << std::to_string(e.cumtk);
		ss << std::setw(w.type) << print(e.ev.type());
		ss << std::setw(w.dat_sz) << std::to_string(e.ev.data_size());
		ss << std::setw(w.trk) << std::to_string(e.trackn);
		ss << dbk::print_hexascii(e.ev.data(), e.ev.size(), ' ');
		ss << "\n";
	}

	return ss.str();
}


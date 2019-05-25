#include "smf_t.h"
#include "midi_raw.h"
#include "mthd_container_t.h"
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


/*
smf2_t::smf2_t(const validate_smf_result_t& maybe_smf) {
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
}*/
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

mthd_view_t smf2_t::get_header_view() const {
	return this->mthd_.get_view();
}
const mtrk_t& smf2_t::get_track(int trackn) const {
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

	s += smf.fname();
	s += "\n";

	s += "Header (MThd) \t(data_size = ";
	//s += " smf_t has no get_header()-like method yet :(\n";
	s += std::to_string(smf.mthd_data_size());
	s += ", size = ";
	s += std::to_string(smf.mthd_size());
	s += "):\n";
	auto mthd = smf.get_header_view();
	s += print(mthd);
	s += "\n";

	// where smf.get_track_view(i) => mtrk_view_t
	for (int i=0; i<smf.ntrks(); ++i) {
		const mtrk_t& curr_trk = smf.get_track(i);
		s += ("Track (MTrk) " + std::to_string(i) 
				+ "\t(data_size = " + std::to_string(curr_trk.size())
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


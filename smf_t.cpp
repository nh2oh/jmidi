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
const std::vector<mtrk_event_t>& smf2_t::get_track(int trackn) const {
	return this->mtrks_[trackn];
}

void smf2_t::set_fname(const std::string& fname) {
	this->fname_ = fname;
}

void smf2_t::set_mthd(const validate_mthd_chunk_result_t& val_mthd) {
	this->mthd_.set(val_mthd);
}
void smf2_t::append_mtrk(const std::vector<mtrk_event_t>& mtrk) {
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
		const std::vector<mtrk_event_t>& curr_trk = smf.get_track(i);
		s += ("Track (MTrk) " + std::to_string(i) 
				+ "\t(data_size = " + std::to_string(curr_trk.size())
				+ ", size = " + std::to_string(curr_trk.size()) + "):\n");
		for (const auto& e : curr_trk) {
			auto curr_trk = smf.get_track_view(i);
			
			s += print(curr_trk);
			s += "\n";
		}
	}
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

	// Run validate_chunk_header() on the fdata to find the chunk boundaries
	// and verify sizes.  
	//mthd_t mthd {};
	//std::vector<std::vector<unsigned char>> mtrks_raw {};
	//std::vector<std::vector<unsigned char>> uchks_raw {};

	int32_t o {0};
	while (o<fdata.size()) {
		auto curr_chunk = validate_chunk_header(fdata.data()+o,fdata.size()-o);
		if (curr_chunk.type == chunk_type::invalid) {
			result.error += "curr_chunk.type == chunk_type::invalid\ncurr_chunk_type.msg== ";
			result.error += print_error(curr_chunk);
			return result;
		} else if (curr_chunk.type == chunk_type::header) {
			if (o > 0) {
				result.error += "curr_chunk.type == chunk_type::header but offset > 0.  ";
				result.error += "A valid midi file must contain only one MThd @ the very start.  \n";
				return result;
			}
			auto val_mthd = validate_mthd_chunk(fdata.data()+o,curr_chunk.size);
			if (val_mthd.error!=mthd_validation_error::no_error) {
				result.error += "val_mthd.error!=mthd_validation_error::no_error\n";
				return result;
			}
			result.smf.set_mthd(val_mthd);
		} else if (curr_chunk.type == chunk_type::track) {
			if (o == 0) {
				result.error += "curr_chunk.type == chunk_type::track but offset == 0.  ";
				result.error += "A valid midi file must begin w/an MThd chunk.  \n";
				return result;
			}
			auto curr_mtrk = make_mtrk_event_vector(fdata.data()+o,curr_chunk.size);
			if (!curr_mtrk) {
				result.error = "!currmaybemtrk";
				return result;
			}
			result.smf.append_mtrk(curr_mtrk.mtrk);
		} else if (curr_chunk.type == chunk_type::unknown) {
			if (o == 0) {
				result.error += "curr_chunk.type == chunk_type::unknown but offset == 0.  ";
				result.error += "A valid midi file must begin w/an MThd chunk.  \n";
				return result;
			}
			std::vector<unsigned char> curr_uchk {};
			std::copy(fdata.data()+o,fdata.data()+o+curr_chunk.size,
				std::back_inserter(curr_uchk));
			result.smf.append_uchk(curr_uchk);
		}
		o += curr_chunk.size;
	}
	// Would indicate the file is zero-padded after the final mtrk
	if (o != fdata.size()) {
		result.error = "offset != fdata.size().";
		return result;
	}

	return result;
}

maybe_mtrk_vector_t::operator bool() const {
	return this->error=="No error";
}

maybe_mtrk_vector_t make_mtrk_event_vector(const unsigned char *p, uint32_t max_sz) {
	maybe_mtrk_vector_t result {};
	auto chunk_detect = validate_chunk_header(p,max_sz);
	if (chunk_detect.type != chunk_type::track) {
		result.error = "chunk_detect.type != chunk_type::track";
		return result;
	}
	
	// Process the data section of the mtrk chunk until the number of bytes
	// processed == chunk_detect.data_size, or an end-of-track meta event is
	// encountered.  Note that an end-of-track could be hit before processing
	// chunk_detect.data_size bytes if the chunk is 0-padded past the end of 
	// the end-of-track msg.  
	bool found_eot = false;
	uint32_t mtrk_data_size = chunk_detect.data_size;
	uint32_t o = 8;  // offset into the chunk data section; skip "MTrk" & the 4-byte length
	unsigned char rs {0};  // value of the running-status
	validate_mtrk_event_result_t curr_event;
	while ((o<chunk_detect.data_size) && !found_eot) {
		const unsigned char *curr_p = p+o;  // ptr to start of present event
		uint32_t curr_max_sz = chunk_detect.data_size-o;  // max excursion for present event beyond p
		
		curr_event = validate_mtrk_event_dtstart(curr_p,curr_max_sz,rs);
		if (curr_event.error!=mtrk_event_validation_error::no_error) {
			result.error = "mtrk_validation_error::event_error";
			return result;
		}

		rs = curr_event.running_status;
		result.mtrk.push_back(mtrk_event_t(curr_p,rs,curr_max_sz));

		if (curr_event.type==smf_event_type::meta) {  // Test for end-of-track msg
			if (curr_event.size>=4) {
				uint32_t last_four = dbk::be_2_native<uint32_t>(curr_p+curr_event.size-4);
				if ((last_four&0x00FFFFFFu)==0x00FF2F00u) {
					found_eot = true;
				}
			}
		}
		o += curr_event.size;
	}

	// The final event must be a meta-end-of-track msg.  
	// Note that it is possible that found_eot==true even though 
	// o<mtrk_data_size.  I am allowing for this (which could perhaps => that 
	// the track is 0-padded after the end-of-track msg).  
	if (!found_eot) {
		result.error = "mtrk_validation_error::no_end_of_track";
		return result;
	}

	return result;
}


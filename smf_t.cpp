#include "smf_t.h"
#include "midi_raw.h"
#include "mthd_t.h"
#include "mtrk_t.h"
#include <string>
#include <cstdint>
#include <vector>
#include <algorithm>  // std::copy() in smf_t::smf_t(const validate_smf_result_t& maybe_smf)
#include <iostream>
#include <iomanip>  // std::setw()
#include <ios>  // std::left
#include <sstream>
#include <iostream>
#include <filesystem>
#include <fstream>



smf_t::size_type smf_t::size() const {
	return this->ntrks();
}
smf_t::size_type smf_t::nchunks() const {
	return this->mtrks_.size()+this->uchks_.size()+1;
	// + 1 to account for the MThd 
}
smf_t::size_type smf_t::ntrks() const {
	return this->mtrks_.size();
}
smf_t::size_type smf_t::nuchks() const {
	return this->uchks_.size();
}
smf_t::size_type smf_t::nbytes() const {
	smf_t::size_type n = 0;
	n = this->mthd_.nbytes();  // TODO:  Implement + rename to nbytes
	for (const auto& e : this->mtrks_) {
		n += e.nbytes();
	}
	for (const auto& e : this->uchks_) {
		n += e.size();
	}
	return n;
}
smf_t::iterator smf_t::begin() {
	if (this->mtrks_.size()==0) {
		return smf_t::iterator(nullptr);
	}
	return smf_t::iterator(&(this->mtrks_[0]));
}
smf_t::iterator smf_t::end() {
	if (this->mtrks_.size()==0) {
		return smf_t::iterator(nullptr);
	}
	return smf_t::iterator(&(this->mtrks_[0]) + this->mtrks_.size());
}
smf_t::const_iterator smf_t::cbegin() const {
	if (this->mtrks_.size()==0) {
		return smf_t::const_iterator(nullptr);
	}
	return smf_t::const_iterator(&(this->mtrks_[0]));
}
smf_t::const_iterator smf_t::cend() const {
	if (this->mtrks_.size()==0) {
		return smf_t::const_iterator(nullptr);
	}
	return smf_t::const_iterator(&(this->mtrks_[0]) + this->mtrks_.size());
}
smf_t::const_iterator smf_t::begin() const {
	return this->cbegin();
}
smf_t::const_iterator smf_t::end() const {
	return this->cend();
}
smf_t::reference smf_t::push_back(smf_t::const_reference mtrk) {
	this->mtrks_.push_back(mtrk);
	this->chunkorder_.push_back(0);
	return this->mtrks_.back();
}
smf_t::iterator smf_t::insert(smf_t::iterator it, smf_t::const_reference mtrk) {
	auto n = it-this->begin();
	this->mtrks_.insert((this->mtrks_.begin()+n),mtrk);
	this->chunkorder_.insert((this->chunkorder_.begin()+n),0);
	return it;
}
smf_t::const_iterator smf_t::insert(smf_t::const_iterator it, smf_t::const_reference mtrk) {
	auto n = it-this->begin();
	this->mtrks_.insert((this->mtrks_.begin()+n),mtrk);
	this->chunkorder_.insert((this->chunkorder_.begin()+n),0);
	return it;
}
smf_t::iterator smf_t::erase(smf_t::iterator it) {
	auto n = it-this->begin();
	this->mtrks_.erase((this->mtrks_.begin()+n));
	this->chunkorder_.erase((this->chunkorder_.begin()+n));
	return this->begin()+n;
}
smf_t::const_iterator smf_t::erase(smf_t::const_iterator it) {
	auto n = it-this->begin();
	this->mtrks_.erase((this->mtrks_.begin()+n));
	this->chunkorder_.erase((this->chunkorder_.begin()+n));
	return this->begin()+n;
}
const smf_t::uchk_value_type& smf_t::push_back(const smf_t::uchk_value_type& uchk) {
	this->uchks_.push_back(uchk);
	this->chunkorder_.push_back(1);
	return this->uchks_.back();
}
smf_t::uchk_iterator smf_t::insert(smf_t::uchk_iterator it,
				const smf_t::uchk_value_type& uchk) {
	auto n = it-this->uchks_.begin();
	this->uchks_.insert(it,uchk);
	this->chunkorder_.insert((this->chunkorder_.begin()+n),1);
	return it;
}
smf_t::uchk_const_iterator smf_t::insert(smf_t::uchk_const_iterator it,
				const smf_t::uchk_value_type& uchk) {
	auto n = it-this->uchks_.begin();
	this->uchks_.insert(it,uchk);
	this->chunkorder_.insert((this->chunkorder_.begin()+n),1);
	return it;
}
smf_t::uchk_iterator smf_t::erase(smf_t::uchk_iterator it) {
	auto n = it-this->uchks_.begin();
	this->uchks_.erase((this->uchks_.begin()+n));
	this->chunkorder_.erase((this->chunkorder_.begin()+n));
	return this->uchks_.begin()+n;
}
smf_t::uchk_const_iterator smf_t::erase(smf_t::uchk_const_iterator it) {
	auto n = it-this->uchks_.begin();
	this->uchks_.erase((this->uchks_.begin()+n));
	this->chunkorder_.erase((this->chunkorder_.begin()+n));
	return this->uchks_.begin()+n;
}
smf_t::reference smf_t::operator[](smf_t::size_type n) {
	return this->mtrks_[n];
}
smf_t::const_reference smf_t::operator[](smf_t::size_type n) const {
	return this->mtrks_[n];
}
const smf_t::uchk_value_type& smf_t::get_uchk(smf_t::size_type n) const {
	return this->uchks_[n];
}
smf_t::uchk_value_type& smf_t::get_uchk(smf_t::size_type n) {
	return this->uchks_[n];
}
int32_t smf_t::format() const {
	return this->mthd_.format();
}
time_division_t smf_t::division() const {
	return this->mthd_.division();
}
int32_t smf_t::mthd_size() const {
	return this->mthd_.size();
}
const std::string& smf_t::fname() const {
	return (*this).fname_;
}

const mthd_t& smf_t::mthd() const {
	return this->mthd_;
}
mthd_t& smf_t::mthd() {
	return this->mthd_;
}

const std::string& smf_t::set_fname(const std::string& fname) {
	this->fname_ = fname;
	return (*this).fname_;
}
void smf_t::set_mthd(const validate_mthd_chunk_result_t& val_mthd) {
	this->mthd_ = mthd_t(val_mthd);
}

// TODO:  The call to nbytes() is v. expensive
std::string print(const smf_t& smf) {
	std::string s {};
	s.reserve(20*smf.size());  // TODO: Magic constant 20

	s += smf.fname();
	s += "\nnum bytes = " + std::to_string(smf.nbytes()) + ", "
		"num chunks = " + std::to_string(smf.nchunks()) + ", "
		"num tracks = " + std::to_string(smf.ntrks());
	s += "\n\n";

	s += print(smf.mthd());
	s += "\n\n";

	for (int i=0; i<smf.ntrks(); ++i) {
		const mtrk_t& curr_trk = smf[i];
		s += ("Track (MTrk) " + std::to_string(i) 
				+ ":\tsize = " + std::to_string(curr_trk.size()) + " events, "
				+ "nbytes = " + std::to_string(curr_trk.nbytes()) + " "
					+ "(including header)\n");
				//+ ", size = " + std::to_string(curr_trk.size()) + "):\n");
		s += print(curr_trk);
		s += "\n";
	}

	return s;
}







maybe_smf_t::operator bool() const {
	return this->error == "No error";
}
// TODO:  I can make this more effecient by moving the curr_uckh, curr_mtrk
// vectors out of the loop & clearing them after usage to avoid allocations.  
maybe_smf_t read_smf(const std::string& fn) {
	maybe_smf_t result {};
	
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

	int n_mtrks_read = 0;  int n_uchks_read = 0;
	// Note: Thid loop terminates based on fdata.size(), not on the number of 
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
			++n_mtrks_read;
			result.smf.push_back(curr_mtrk.mtrk);
		} else if (curr_chunk.type == chunk_type::unknown) {
			std::vector<unsigned char> curr_uchk {};
			curr_uchk.reserve(curr_chunk.size);
			std::copy(curr_p,curr_p+curr_chunk.size,
				std::back_inserter(curr_uchk));
			result.smf.push_back(curr_uchk);
			++n_uchks_read;
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

	if (n_mtrks_read != result.smf.mthd().ntrks()) {
		result.error = "The number-of-tracks reported by the header chunk is "
			"inconsistent with the number of MTrk chunks in the file.  ";
		return result;
	}

	return result;
}


std::vector<all_smf_events_dt_ordered_t> get_events_dt_ordered(const smf_t& smf) {
	std::vector<all_smf_events_dt_ordered_t> result;
	//result.reserve(smf.nchunks...
	
	for (int i=0; i<smf.ntrks(); ++i) {
		const auto& curr_trk = smf[i];
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
		//ss << dbk::print_hexascii(e.ev.data(), e.ev.size(), ' ');
		std::string temp_s;
		print_hexascii(e.ev.data(), e.ev.data()+e.ev.size(), std::back_inserter(temp_s), ' ');
		ss << temp_s;
		ss << "\n";
	}
	
	return ss.str();
}

/*
linked_and_orphans_with_trackn_t get_linked_onoff_pairs(const smf_t& smf) {
	linked_and_orphans_with_trackn_t result;
	for (int i=0; i<smf.ntrks(); ++i) {
		const auto& curr_trk = smf.get_track(i);
		auto curr_trk_linked = get_linked_onoff_pairs(curr_trk.begin(),curr_trk.end());
		uint32_t cumtk = 0;
		for (int j=0; j<curr_trk_linked.linked.size(); ++j) {
			result.linked.push_back({i,curr_trk_linked.linked[j]});
		}
		for (int j=0; j<curr_trk_linked.orphan_on.size(); ++j) {
			result.orphan_on.push_back({i,curr_trk_linked.orphan_on[j]});
		}
		for (int j=0; j<curr_trk_linked.orphan_off.size(); ++j) {
			result.orphan_off.push_back({i,curr_trk_linked.orphan_off[j]});
		}
	}

	auto lt_ev = [](const linked_pair_with_trackn_t& lhs, 
					const linked_pair_with_trackn_t& rhs)->bool {
		if (lhs.ev_pair.cumtk_on == rhs.ev_pair.cumtk_on) {
			return lhs.trackn < rhs.trackn;
		} else {
			return lhs.ev_pair.cumtk_on < rhs.ev_pair.cumtk_on;
		}
	};
	std::sort(result.linked.begin(),result.linked.end(),lt_ev);

	return result;
}

std::string print(const linked_and_orphans_with_trackn_t& evs) {
std::string s {};
	struct width_t {
		int def {12};  // "default"
		int tick {12};
		int trk {7};
		int p1p2 {10};
		int ch {10};
		int sep {3};
	};
	width_t w {};

	std::stringstream ss {};
	ss << std::left;
	ss << std::setw(w.trk) << "Track";
	ss << std::setw(w.ch) << "Ch (on)";
	ss << std::setw(w.p1p2) << "p1 (on)";
	ss << std::setw(w.p1p2) << "p2 (on)";
	ss << std::setw(w.tick) << "Tick (on)";
	//ss << std::setw(w.sep) << " ";
	ss << std::setw(w.ch) << "Ch (off)";
	ss << std::setw(w.p1p2) << "p1 (off)";
	ss << std::setw(w.p1p2) << "p2 (off)";
	ss << std::setw(w.tick) << "Tick off";
	//ss << std::setw(w.sep) << " ";
	ss << std::setw(w.def) << "Duration";
	ss << "\n";

	auto half = [&ss,&w](uint32_t cumtk, const mtrk_event_t& onoff)->void {
		auto md = onoff.midi_data();
		ss << std::setw(w.ch) << std::to_string(md.ch);
		ss << std::setw(w.p1p2) << std::to_string(md.p1);
		ss << std::setw(w.p1p2) << std::to_string(md.p2);
		ss << std::setw(w.tick) << std::to_string(cumtk);
	};
	
	for (const auto& e : evs.linked) {
		ss << std::setw(w.trk) << std::to_string(e.trackn);
		half(e.ev_pair.cumtk_on,e.ev_pair.on);
		//ss << std::setw(w.sep) << " ";
		half(e.ev_pair.cumtk_off,e.ev_pair.off);
		//ss << std::setw(w.sep) << " ";
		ss << std::to_string(e.ev_pair.cumtk_off-e.ev_pair.cumtk_on);
		ss << "\n";
	}
	
	if (evs.orphan_on.size()>0) {
		ss << "FILE CONTAINS ORPHAN NOTE-ON EVENTS:\n";
		for (const auto& e : evs.orphan_on) {
			ss << std::setw(w.trk) << std::to_string(e.trackn);
			half(e.orph_ev.cumtk,e.orph_ev.ev);
			ss << std::setw(w.sep) << "\n";
		}
	}
	if (evs.orphan_off.size()>0) {
		ss << "FILE CONTAINS ORPHAN NOTE-OFF EVENTS:\n";
		for (const auto& e : evs.orphan_off) {
			ss << std::setw(w.trk) << std::to_string(e.trackn);
			half(e.orph_ev.cumtk,e.orph_ev.ev);
			ss << std::setw(w.sep) << "\n";
		}
	}

	return ss.str();
}
*/


#include "smf_t.h"
#include "midi_raw.h"
#include "mthd_t.h"
#include "mtrk_t.h"
#include "midi_vlq.h"
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
/*void smf_t::set_mthd(const validate_mthd_chunk_result_t& val_mthd) {
	this->mthd_ = mthd_t(val_mthd);
}*/
void smf_t::set_mthd(const maybe_mthd_t& mthd) {
	if (mthd) {
		this->mthd_ = mthd.mthd;
	}
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
	return this->is_valid;
}
maybe_smf_t read_smf(const std::filesystem::path& fp) {
	return read_smf(fp,nullptr);
}
maybe_smf_t read_smf(const std::filesystem::path& fp, smf_error_t *err) {
	maybe_smf_t result {};

	// Read the file into fdata, close the file
	std::vector<unsigned char> fdata {};
	std::basic_ifstream<unsigned char> f(fp,std::ios_base::in|std::ios_base::binary);
	if (!f.is_open() || !f.good()) {
		if (err) {
			err->code = smf_error_t::errc::file_read_error;
			//*err += "Unable to open file:  (!f.is_open() || !f.good()).  "
			//	"std::basic_ifstream<unsigned char> f";
		}
		return result;
	}
	f.seekg(0,std::ios::end);
	auto fsize = f.tellg();
	f.seekg(0,std::ios::beg);
	fdata.resize(fsize);
	f.read(fdata.data(),fsize);
	f.close();

	const unsigned char *beg = fdata.data();
	const unsigned char *end = fdata.data()+fdata.size();

	result.smf.set_fname(fp.string());

	auto maybe_mthd = make_mthd(beg,end);
	if (!maybe_mthd) {
		if (err) {
			mthd_error_t mthd_err {};
			make_mthd(beg,end,&mthd_err);
			err->code = smf_error_t::errc::mthd_error;
			err->chunk_err.mthd_err_obj = mthd_err;
		}
		return result;
	}
	result.smf.set_mthd(maybe_mthd);

	const unsigned char *p = beg+result.smf.mthd().nbytes();
	auto expect_ntrks = result.smf.mthd().ntrks();
	int n_mtrks_read = 0;
	int n_uchks_read = 0;
	while ((p<end) && (n_mtrks_read<expect_ntrks)) {
		auto header = read_chunk_header(p,end);
		if (!header) {
			if (err) {
				chunk_header_error_t header_error {};
				read_chunk_header(p,end,&header_error);
				err->code = smf_error_t::errc::generic_chunk_header_error;
				err->chunk_err.generic_chunk_err_obj = header_error;
				err->expect_num_mtrks = expect_ntrks;
				err->num_mtrks_read = n_mtrks_read;
				err->num_uchks_read = n_uchks_read;
				err->termination_offset = p-beg;
			}
			return result;
		}

		auto nbytes_read = header.length+8;

		if (header.id == chunk_id::mtrk) {
			mtrk_error_t mtrk_error {};
			// It is significant that i am passing p+header.length rather 
			// than end.  This causes make_mtrk_permissive() to never read
			// more than header.length bytes.  This can be changed, but must
			// be synchronyzed with the way p is incremented at the bottom
			// of this loop.  See below.  
			auto max_nbytes = end-p;
			if (header.length > max_nbytes) {
				max_nbytes = header.length;
			}
			auto curr_mtrk = make_mtrk_permissive(p,p+max_nbytes,
				&mtrk_error);
			// push_back the mtrk even if invalid; make_mtrk will return a
			// partial mtrk terminating at the event right before the error,
			// and this partial mtrk may be useful to the user.  
			result.smf.push_back(curr_mtrk.mtrk);
			if (!curr_mtrk) {
				if (err) {
					err->code = smf_error_t::errc::mtrk_error;
					err->chunk_err.mtrk_err_obj = mtrk_error;
					err->expect_num_mtrks = expect_ntrks;
					err->num_mtrks_read = n_mtrks_read;
					err->num_uchks_read = n_uchks_read;
					err->termination_offset = p-beg;
				}
				return result;
			}
			++n_mtrks_read;
		} else if ((header.id==chunk_id::unknown) && ((header.length+p+8)>end)) {
			result.smf.push_back(
				std::vector<unsigned char>(p,p+header.length));
			++n_uchks_read;
		} else {
			// header.id could perhaps == chunk_id::mtrk, or == chunk_id::unknown
			// with (header.length<=(pend-p-8)) (=> overflow)
			if (err) {
				err->code = smf_error_t::errc::unknown_chunk_implies_overflow;
				err->expect_num_mtrks = expect_ntrks;
				err->num_mtrks_read = n_mtrks_read;
				err->num_uchks_read = n_uchks_read;
				err->termination_offset = p-beg;
			}
			return result;
		}

		// make_mtrk[_permissive]() may or may not read the number of bytes
		// indicated by header.length, for example, if an invalid event or a
		// premature EOT is hit (for chunk_id::unknown chunks, header.length
		// bytes are always read).  p += (header.length+8) assumes the file 
		// is not so fucked up that the chunk spacing can't be calculated 
		// from the length field in the headers.  
		p += nbytes_read;
	}

	// If p<pend, might indicate the file is zero-padded after 
	// the final mtrk chunk.  p>pend is clearly an error, but
	// should be impossible.  
	if (p<end) {
		if (err) {
			err->code = smf_error_t::errc::terminated_before_end_of_file;
			err->expect_num_mtrks = expect_ntrks;
			err->num_mtrks_read = n_mtrks_read;
			err->num_uchks_read = n_uchks_read;
			err->termination_offset = p-beg;
		}
		return result;
	}

	if (n_mtrks_read != expect_ntrks) {
		if (err) {
			err->code = smf_error_t::errc::unexpected_num_mtrks;
			err->expect_num_mtrks = expect_ntrks;
			err->num_mtrks_read = n_mtrks_read;
			err->num_uchks_read = n_uchks_read;
			err->termination_offset = p-beg;
		}
		return result;
	}

	result.is_valid = true;
	return result;
}
std::string explain(const smf_error_t& err) {
	std::string s {};
	if (err.code==smf_error_t::errc::no_error) {
		return s;
	}

	auto append_values = [&s,err]()->void {
		s += "Expected number of MTrks == ";
		s += std::to_string(err.expect_num_mtrks);
		s += ", number of MTrks read-in == ";
		s += std::to_string(err.num_mtrks_read);
		s += ", number of 'unknown' chunks read-in == ";
		s += std::to_string(err.num_uchks_read);
		s += ", Terminated on chunk with offset == ";
		s += std::to_string(err.termination_offset);
		s += " from the beginning of the file.  ";
	};

	s += "Error reading SMF:  ";
	if (err.code==smf_error_t::errc::file_read_error) {
		s += "Error opening or reading file.  ";
	} else if (err.code==smf_error_t::errc::mthd_error) {
		s += explain(err.chunk_err.mthd_err_obj);
	} else if (err.code==smf_error_t::errc::mtrk_error) {
		s += explain(err.chunk_err.mtrk_err_obj);
		append_values();
	} else if (err.code==smf_error_t::errc::generic_chunk_header_error) {
		s += explain(err.chunk_err.generic_chunk_err_obj);
		append_values();
	} else if (err.code==smf_error_t::errc::unknown_chunk_implies_overflow) {
		s += "The file is not large enough to accommodate the number of bytes "
			"specified by the 'length' field of the present 'unknown' chunk type "
			"header.  ";
		append_values();
	} else if (err.code==smf_error_t::errc::terminated_before_end_of_file) {
		s += "Terminated scanning before the reaching the end of the file.  "
			"The file may have padding or other garbage past the end of the "
			"last chunk.  ";
		append_values();
	} else if (err.code==smf_error_t::errc::unexpected_num_mtrks) {
		s += "Terminated scanning before the reading the expected number of "
			"MTrk chunks.  The ntrks field of the MThd chunk may be incorrect.  ";
		append_values();
	} else if (err.code==smf_error_t::errc::other) {
		s += "Error code smf_error_t::errc::other.  ";
	} else {
		s += "Unknown error.  ";
	}

	return s;
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


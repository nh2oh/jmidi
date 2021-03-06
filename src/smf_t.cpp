#include "smf_t.h"
#include "mthd_t.h"
#include "mtrk_t.h"
#include "midi_vlq.h"
#include "midi_status_byte.h"
#include "print_hexascii.h"
#include <string>
#include <cstdint>
#include <vector>
#include <algorithm>  // std::copy() in smf_t::smf_t(const validate_smf_result_t& maybe_smf)
#include <iomanip>  // std::setw()
#include <ios>  // std::left
#include <sstream>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <iterator>


jmid::smf_t::smf_t() noexcept {
	//...
}
jmid::smf_t::smf_t(const jmid::smf_t& rhs) {
	this->mthd_ = rhs.mthd_;
	this->mtrks_ = rhs.mtrks_;
	this->uchks_ = rhs.uchks_;
	this->chunkorder_ = rhs.chunkorder_;
}
jmid::smf_t::smf_t(jmid::smf_t&& rhs) noexcept {
	this->mthd_ = std::move(rhs.mthd_);
	this->mtrks_ = std::move(rhs.mtrks_);
	this->uchks_ = std::move(rhs.uchks_);
	this->chunkorder_ = std::move(rhs.chunkorder_);
}
jmid::smf_t& jmid::smf_t::operator=(const jmid::smf_t& rhs) {
	this->mthd_ = rhs.mthd_;
	this->mtrks_ = rhs.mtrks_;
	this->uchks_ = rhs.uchks_;
	this->chunkorder_ = rhs.chunkorder_;
	return *this;
}
jmid::smf_t& jmid::smf_t::operator=(jmid::smf_t&& rhs) noexcept {
	this->mthd_ = std::move(rhs.mthd_);
	this->mtrks_ = std::move(rhs.mtrks_);
	this->uchks_ = std::move(rhs.uchks_);
	this->chunkorder_ = std::move(rhs.chunkorder_);
	return *this;
}
jmid::smf_t::~smf_t() noexcept {
	//...
}

jmid::smf_t::size_type jmid::smf_t::size() const {
	return this->ntrks();
}
jmid::smf_t::size_type jmid::smf_t::nchunks() const {
	return this->mtrks_.size()+this->uchks_.size()+1;
	// + 1 to account for the MThd 
}
jmid::smf_t::size_type jmid::smf_t::ntrks() const {
	return this->mtrks_.size();
}
jmid::smf_t::size_type jmid::smf_t::nuchks() const {
	return this->uchks_.size();
}
jmid::smf_t::size_type jmid::smf_t::nbytes() const {
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
jmid::smf_t::iterator jmid::smf_t::begin() {
	if (this->mtrks_.size()==0) {
		return smf_t::iterator(nullptr);
	}
	return smf_t::iterator(&(this->mtrks_[0]));
}
jmid::smf_t::iterator jmid::smf_t::end() {
	if (this->mtrks_.size()==0) {
		return jmid::smf_t::iterator(nullptr);
	}
	return jmid::smf_t::iterator(&(this->mtrks_[0]) + this->mtrks_.size());
}
jmid::smf_t::const_iterator jmid::smf_t::cbegin() const {
	if (this->mtrks_.size()==0) {
		return jmid::smf_t::const_iterator(nullptr);
	}
	return jmid::smf_t::const_iterator(&(this->mtrks_[0]));
}
jmid::smf_t::const_iterator jmid::smf_t::cend() const {
	if (this->mtrks_.size()==0) {
		return jmid::smf_t::const_iterator(nullptr);
	}
	return jmid::smf_t::const_iterator(&(this->mtrks_[0]) + this->mtrks_.size());
}
jmid::smf_t::const_iterator jmid::smf_t::begin() const {
	return this->cbegin();
}
jmid::smf_t::const_iterator jmid::smf_t::end() const {
	return this->cend();
}
jmid::smf_t::reference jmid::smf_t::push_back(jmid::smf_t::const_reference mtrk) {
	this->mtrks_.push_back(mtrk);
	this->chunkorder_.push_back(0);
	this->mthd_.set_ntrks(this->ntrks());
	return this->mtrks_.back();
}
jmid::smf_t::reference jmid::smf_t::push_back(jmid::mtrk_t&& mtrk) {
	this->mtrks_.push_back(std::move(mtrk));
	this->chunkorder_.push_back(0);
	this->mthd_.set_ntrks(this->mtrks_.size());
	return this->mtrks_.back();
}
jmid::smf_t::iterator jmid::smf_t::insert(jmid::smf_t::iterator it, jmid::smf_t::const_reference mtrk) {
	auto n = it-this->begin();
	this->mtrks_.insert((this->mtrks_.begin()+n),mtrk);
	this->chunkorder_.insert((this->chunkorder_.begin()+n),0);
	this->mthd_.set_ntrks(this->mtrks_.size());
	return it;
}
jmid::smf_t::const_iterator jmid::smf_t::insert(jmid::smf_t::const_iterator it, jmid::smf_t::const_reference mtrk) {
	auto n = it-this->begin();
	this->mtrks_.insert((this->mtrks_.begin()+n),mtrk);
	this->chunkorder_.insert((this->chunkorder_.begin()+n),0);
	this->mthd_.set_ntrks(this->mtrks_.size());
	return it;
}
jmid::smf_t::iterator jmid::smf_t::erase(jmid::smf_t::iterator it) {
	auto n = it-this->begin();
	this->mtrks_.erase((this->mtrks_.begin()+n));
	this->chunkorder_.erase((this->chunkorder_.begin()+n));
	this->mthd_.set_ntrks(this->mtrks_.size());
	return this->begin()+n;
}
jmid::smf_t::const_iterator jmid::smf_t::erase(jmid::smf_t::const_iterator it) {
	auto n = it-this->begin();
	this->mtrks_.erase((this->mtrks_.begin()+n));
	this->chunkorder_.erase((this->chunkorder_.begin()+n));
	this->mthd_.set_ntrks(this->mtrks_.size());
	return this->begin()+n;
}
const jmid::smf_t::uchk_value_type& jmid::smf_t::push_back(const jmid::smf_t::uchk_value_type& uchk) {
	this->uchks_.push_back(uchk);
	this->chunkorder_.push_back(1);
	return this->uchks_.back();
}
jmid::smf_t::uchk_iterator jmid::smf_t::insert(jmid::smf_t::uchk_iterator it,
				const jmid::smf_t::uchk_value_type& uchk) {
	auto n = it-this->uchks_.begin();
	this->uchks_.insert(it,uchk);
	this->chunkorder_.insert((this->chunkorder_.begin()+n),1);
	return it;
}
jmid::smf_t::uchk_const_iterator jmid::smf_t::insert(jmid::smf_t::uchk_const_iterator it,
				const jmid::smf_t::uchk_value_type& uchk) {
	auto n = it-this->uchks_.begin();
	this->uchks_.insert(it,uchk);
	this->chunkorder_.insert((this->chunkorder_.begin()+n),1);
	return it;
}
jmid::smf_t::uchk_iterator jmid::smf_t::erase(jmid::smf_t::uchk_iterator it) {
	auto n = it-this->uchks_.begin();
	this->uchks_.erase((this->uchks_.begin()+n));
	this->chunkorder_.erase((this->chunkorder_.begin()+n));
	return this->uchks_.begin()+n;
}
jmid::smf_t::uchk_const_iterator jmid::smf_t::erase(jmid::smf_t::uchk_const_iterator it) {
	auto n = it-this->uchks_.begin();
	this->uchks_.erase((this->uchks_.begin()+n));
	this->chunkorder_.erase((this->chunkorder_.begin()+n));
	return this->uchks_.begin()+n;
}
jmid::smf_t::reference jmid::smf_t::operator[](jmid::smf_t::size_type n) {
	return this->mtrks_[n];
}
jmid::smf_t::const_reference jmid::smf_t::operator[](jmid::smf_t::size_type n) const {
	return this->mtrks_[n];
}
const jmid::smf_t::uchk_value_type& jmid::smf_t::get_uchk(jmid::smf_t::size_type n) const {
	return this->uchks_[n];
}
jmid::smf_t::uchk_value_type& jmid::smf_t::get_uchk(jmid::smf_t::size_type n) {
	return this->uchks_[n];
}
std::int32_t jmid::smf_t::format() const {
	return this->mthd_.format();
}
jmid::time_division_t jmid::smf_t::division() const {
	return this->mthd_.division();
}
std::int32_t jmid::smf_t::mthd_size() const {
	return this->mthd_.size();
}
const jmid::mthd_t& jmid::smf_t::mthd() const {
	return this->mthd_;
}
jmid::mthd_t& jmid::smf_t::mthd() {
	return this->mthd_;
}
void jmid::smf_t::set_mthd(const jmid::maybe_mthd_t& mthd) {
	if (mthd) {
		this->mthd_ = mthd.mthd;
	}
	this->mthd_.set_ntrks(this->mtrks_.size());
}
void jmid::smf_t::set_mthd(const jmid::mthd_t& mthd) {
	this->mthd_ = mthd;
	this->mthd_.set_ntrks(this->mtrks_.size());
}
void jmid::smf_t::set_mthd(jmid::mthd_t&& mthd) noexcept {
	this->mthd_ = std::move(mthd);
	this->mthd_.set_ntrks(this->mtrks_.size());
}

// TODO:  The call to nbytes() is v. expensive
std::string jmid::print(const jmid::smf_t& smf) {
	std::string s {};
	s.reserve(20*smf.size());  // TODO: Magic constant 20

	s += "num bytes = " + std::to_string(smf.nbytes()) + ", "
		"num chunks = " + std::to_string(smf.nchunks()) + ", "
		"num tracks = " + std::to_string(smf.ntrks());
	s += "\n\n";

	s += jmid::print(smf.mthd());
	s += "\n\n";

	for (int i=0; i<smf.ntrks(); ++i) {
		const jmid::mtrk_t& curr_trk = smf[i];
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







jmid::maybe_smf_t::operator bool() const {
	return this->error==smf_error_t::errc::no_error;
}
jmid::maybe_smf_t jmid::read_smf(const std::filesystem::path& fp, 
			jmid::smf_error_t *err, std::int32_t max_stream_bytes) {
	jmid::maybe_smf_t result;
	result.nbytes_read = 0;
	result.error = jmid::smf_error_t::errc::no_error;
	auto sz = std::filesystem::file_size(fp);
	if (sz > max_stream_bytes) {
		if (err) {
			// TODO:  Wrong error code
			err->code = jmid::smf_error_t::errc::other;
		}
		return result;
	}

	std::basic_ifstream<char> f(fp,std::ios_base::in|std::ios_base::binary);
	std::istreambuf_iterator<char> it(f);
	auto end = std::istreambuf_iterator<char>();
	if (!f.is_open() || !f.good()) {
		result.error = jmid::smf_error_t::errc::file_read_error;
		if (err) {
			err->code = jmid::smf_error_t::errc::file_read_error;
		}
		return result;
	}
	
	//jmid::make_smf(it,end,&result,err,max_stream_bytes);
	auto p_result = &(result.smf);
	smf_error_t local_err_obj;
	jmid::make_smf2(it,end,p_result,&local_err_obj);
	if (err!=nullptr) {
		*err = local_err_obj;
	}
	result.error = local_err_obj.code;
	return result;
};

jmid::maybe_smf_t jmid::read_smf_bulkfileread(const std::filesystem::path& fp, 
					jmid::smf_error_t *err, std::vector<char> *pfdata,
					std::int32_t max_stream_bytes) {
	jmid::maybe_smf_t result;
	result.nbytes_read = 0;
	result.error = jmid::smf_error_t::errc::no_error;
	auto sz = std::filesystem::file_size(fp);
	if (sz > max_stream_bytes) {
		if (err) {
			// TODO:  Wrong error code
			err->code = jmid::smf_error_t::errc::other;
		}
		return result;
	}
	std::basic_ifstream<char> f(fp,std::ios_base::in|std::ios_base::binary);
	if (!f.is_open() || !f.good()) {
		if (err) {
			err->code = jmid::smf_error_t::errc::file_read_error;
		}
		return result;
	}
	f.seekg(0,std::ios::end);
	auto fsize = f.tellg();
	f.seekg(0,std::ios::beg);
	
	std::vector<char> fdata;
	if (!pfdata) {
		fdata.resize(fsize);
		pfdata = &fdata;
	} else {
		pfdata->resize(fsize);
	}

	f.read(pfdata->data(),fsize);
	f.close();

	const char *it = pfdata->data();
	const char *end = pfdata->data()+pfdata->size();
	//jmid::make_smf(it,end,&result,err,max_stream_bytes);
	auto p_result = &(result.smf);
	smf_error_t local_err_obj;
	jmid::make_smf2(it,end,p_result,&local_err_obj);
	if (err!=nullptr) {
		*err = local_err_obj;
	}
	result.error = local_err_obj.code;
	return result;
}

std::string jmid::print(jmid::smf_error_t::errc ec) {
	std::string s;
	switch (ec) {
	case jmid::smf_error_t::errc::file_read_error:
		s = "smf_error_t::errc::file_read_error";
		break;
	case jmid::smf_error_t::errc::mthd_error:
		s = "smf_error_t::errc::mthd_error";
		break;
	case jmid::smf_error_t::errc::mtrk_error:
		s = "smf_error_t::errc::mtrk_error";
		break;
	case jmid::smf_error_t::errc::overflow_reading_uchk:
		s = "smf_error_t::errc::overflow_reading_uchk";
		break;
	case jmid::smf_error_t::errc::terminated_before_end_of_file:
		s = "smf_error_t::errc::terminated_before_end_of_file";
		break;
	case jmid::smf_error_t::errc::unexpected_num_mtrks:
		s = "smf_error_t::errc::unexpected_num_mtrks";
		break;
	case jmid::smf_error_t::errc::other:
		s = "smf_error_t::errc::other";
		break;
	default:
		s = "smf_error_t::errc::?";
		break;
	}
	return s;
}
std::string jmid::explain(const jmid::smf_error_t& err) {
	std::string s {};
	if (err.code==jmid::smf_error_t::errc::no_error) {
		return s;
	}

	s.reserve(500);
	auto append_values = [&s,err]()->void {
		s += "Expected number of MTrks == ";
		s += std::to_string(err.expect_num_mtrks);
		s += ", number of MTrks read-in == ";
		s += std::to_string(err.num_mtrks_read);
		s += ", number of 'unknown' chunks read-in == ";
		s += std::to_string(err.num_uchks_read);
		s += ".  ";
	};

	s += "Error reading SMF:  ";
	if (err.code==jmid::smf_error_t::errc::file_read_error) {
		s += "Error opening or reading file.  ";
	} else if (err.code==jmid::smf_error_t::errc::mthd_error) {
		s += jmid::explain(err.mthd_err_obj);
	} else if (err.code==jmid::smf_error_t::errc::mtrk_error) {
		s += jmid::explain(err.mtrk_err_obj);
		append_values();
	} else if (err.code==jmid::smf_error_t::errc::overflow_reading_uchk) {
		s += "Encountered end-of-input prior to reading in the full data "
			"section of the present uchk.  ";
		append_values();
	} else if (err.code==jmid::smf_error_t::errc::terminated_before_end_of_file) {
		s += "Read in the final MTrk chunk, but did not encounter end-of-input.  "
			"The file may have padding or other garbage past the end of the "
			"last chunk, or the number of MTrks may be misreported in the MThd.  ";
		append_values();
	} else if (err.code==jmid::smf_error_t::errc::unexpected_num_mtrks) {
		s += "Terminated scanning before the reading the expected number of "
			"MTrk chunks.  The ntrks field of the MThd chunk may be incorrect.  ";
		append_values();
	} else if (err.code==jmid::smf_error_t::errc::other) {
		s += "smf_error_t::errc::other.  ";
	} else {
		s += "Unknown error.  ";
	}

	return s;
}


std::filesystem::path jmid::write_smf(const jmid::smf_t& smf, const std::filesystem::path& p) {
	std::basic_ofstream<char> fsout(p,std::ios::out|std::ios::binary);
	std::ostreambuf_iterator<char> it(fsout);
	jmid::write_smf(smf,it);
	fsout.close();
	return p;
}

bool jmid::has_midifile_extension(const std::filesystem::path& fp) {
	if (!std::filesystem::is_regular_file(fp)) {
		return false;
	}
	const auto ext_mid_lc = std::filesystem::path(".mid");
	const auto ext_mid_uc = std::filesystem::path(".MID");
	const auto ext_midi_lc = std::filesystem::path(".midi");
	const auto ext_midi_uc = std::filesystem::path(".MIDI");
	const auto ext = fp.extension();  
	if ((ext!=ext_mid_lc) && (ext!=ext_mid_uc) 
			&& (ext!=ext_midi_lc) && (ext!=ext_midi_uc)) {
		return false;
	}

	return true;
}



std::vector<jmid::all_smf_events_dt_ordered_t> jmid::get_events_dt_ordered(const jmid::smf_t& smf) {
	std::vector<jmid::all_smf_events_dt_ordered_t> result;
	//result.reserve(smf.nchunks...
	
	for (int i=0; i<smf.ntrks(); ++i) {
		const auto& curr_trk = smf[i];
		std::uint32_t cumtk = 0;
		for (const auto& e : curr_trk) {
			cumtk += e.delta_time();
			result.push_back({e,cumtk,i});
		}
	}

	auto lt_ev = [](const jmid::all_smf_events_dt_ordered_t& lhs, 
					const jmid::all_smf_events_dt_ordered_t& rhs)->bool {
		if (lhs.cumtk == rhs.cumtk) {
			return lhs.trackn < rhs.trackn;
		} else {
			return lhs.cumtk < rhs.cumtk;
		}
	};
	std::sort(result.begin(),result.end(),lt_ev);

	return result;
}

std::string print(const std::vector<jmid::all_smf_events_dt_ordered_t>& evs) {
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
		ss << std::setw(w.type) << print(jmid::classify_status_byte(e.ev.status_byte()));
		ss << std::setw(w.dat_sz) << std::to_string(e.ev.data_size());
		ss << std::setw(w.trk) << std::to_string(e.trackn);
		//ss << dbk::print_hexascii(e.ev.data(), e.ev.size(), ' ');
		std::string temp_s;
		jmid::print_hexascii(e.ev.data(), e.ev.data()+e.ev.size(), std::back_inserter(temp_s), ' ');
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


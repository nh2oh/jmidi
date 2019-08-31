#include "midi_time.h"
#include "midi_vlq.h"
#include "midi_status_byte.h"
#include "print_hexascii.h"
#include "smf_t.h"
#include "util.h"
#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include <filesystem>
#include <utility>


class dfa {
public:
	enum class state : std::int8_t {
		event_start,
		ch_event_in_rs,
		ch_event_not_rs,
		meta_event,
		sysex_event,
		finished,
		invalid
	};
	state init(const unsigned char *, const unsigned char *);
	std::pair<dfa::state,const unsigned char*> get_next();
	bool finished();
private:
	const unsigned char *p_ {nullptr};
	const unsigned char *end_ {nullptr};
	state st_ {state::event_start};
	unsigned char rs_ {0};
};
std::string print(dfa::state st) {
	std::string s;
	switch (st) {
		case dfa::state::ch_event_in_rs:
			s = "ch_event_in_rs";  break;
		case dfa::state::ch_event_not_rs:
			s = "ch_event_not_rs";  break;
		case dfa::state::meta_event:
			s = "meta_event";  break;
		case dfa::state::sysex_event:
			s = "sysex_event";  break;
		case dfa::state::event_start:
			s = "event_start";  break;
		case dfa::state::finished:
			s = "finished";  break;
		case dfa::state::invalid:
			s = "invalid";  break;
		default:
			s = "???";  break;
	}
	return s;
}


dfa::state dfa::init(const unsigned char *beg, const unsigned char *end) {
	this->p_= beg;
	this->end_= end;
	return this->st_;
}
std::pair<dfa::state,const unsigned char*> dfa::get_next() {
	if (jmid::is_mtrk_header_id(this->p_,this->end_)) {
		this->p_ += 8;
	}
	if (this->st_!=dfa::state::event_start) {
		// Can happen if a user calls get_next on an ::invalid object
		this->st_ = dfa::state::invalid;
		return {this->st_,this->p_};
	}
	this->st_ = dfa::state::event_start;
	// this->st_ == event_start, this->p_ points @ start of delta time 
	// field.  
	jmid::dt_field_interpreted dt;
	this->p_ = jmid::read_delta_time(this->p_,this->end_,dt);
	if (!dt.is_valid) {
		this->st_ = dfa::state::invalid;
		return {this->st_,this->p_};
	}

	//
	// set this->st_ for the present event
	//
	auto s_or_p1 = *this->p_++;
	auto s = jmid::get_status_byte(s_or_p1,this->rs_);
	this->rs_ = jmid::get_running_status_byte(s,this->rs_);
	if (jmid::is_channel_status_byte(s)) {
		if (jmid::is_data_byte(s_or_p1)) {
			this->st_ = dfa::state::ch_event_in_rs;
		} else {
			this->st_ = dfa::state::ch_event_not_rs;
		}
	} else if (jmid::is_meta_status_byte(s)) {
		this->st_ = dfa::state::meta_event;
	} else if (jmid::is_sysex_status_byte(s)) {
		this->st_ = dfa::state::sysex_event;
	} else {
		this->st_ = dfa::state::invalid;
		return {this->st_,this->p_};
	}

	//
	// Process the event following the delta-time
	//
	// this->st_ != invalid; this->st_, this->rs_, s, are set
	if (this->st_==state::ch_event_not_rs) {
		// s, this->rs_ are set, this->p_ is the first data byte
		if (!jmid::is_data_byte(*this->p_++)) {  // p1 invalid
			this->st_ = dfa::state::invalid;
		}
		if ((jmid::channel_status_byte_n_data_bytes(s)==2) 
			&& (!jmid::is_data_byte(*this->p_++))) {  // p2 invalid
			this->st_ = dfa::state::invalid;
		}
	} else if (this->st_==dfa::state::ch_event_in_rs) {
		// s, this->rs_ are set, p1 is valid, this->p_ points at the
		// potential p2
		if ((jmid::channel_status_byte_n_data_bytes(s)==2)
			&& (!jmid::is_data_byte(*this->p_++))) {  // p2 invalid
			this->st_ = dfa::state::invalid;
		}
	} else if (this->st_==dfa::state::meta_event) {
		// s == 0xFF, this->rs_==0, this->p_ points at the meta-type byte
		++(this->p_);
		jmid::vlq_field_interpreted len;
		this->p_ = jmid::read_vlq(this->p_,this->end_,len);
		if (!len.is_valid) {
			this->st_ = dfa::state::invalid;
		}
		while (this->p_!=this->end_ && len.val>0) {
			++this->p_;
			--len.val;
		}
		if (len.val>0) {
			this->st_ = dfa::state::invalid;
		}
	} else if (this->st_==dfa::state::sysex_event) {
		// s == 0xF0 || 0xF7, this->rs_==0, this->p_ points at the
		// start of the length field.  
		jmid::vlq_field_interpreted len;
		this->p_ = jmid::read_vlq(this->p_,this->end_,len);
		if (!len.is_valid) {
			this->st_ = dfa::state::invalid;
		}
		while (this->p_!=this->end_ && len.val>0) {
			++this->p_;
			--len.val;
		}
		if (len.val>0) {
			this->st_ = dfa::state::invalid;
		}
	} else {
		std::abort();  // Should never happen
	}

	std::pair<dfa::state,const unsigned char*> r(this->st_,this->p_);

	if (this->st_ != dfa::state::invalid) {
		if (this->p_!=this->end_) {
			this->st_ = dfa::state::event_start;
		} else {
			this->st_ = dfa::state::finished;
		}
	}

	return r;
}

bool dfa::finished() {
	return ((this->p_==this->end_) || (this->st_==dfa::state::invalid)
		|| (this->st_==dfa::state::finished));
}


int main(int argc, char *argv[]) {
	//std::filesystem::path pth("C:\\Users\\ben\\Desktop\\midi_archive\\desktop\\hallelujah_joy_to_the_world.mid");
	std::filesystem::path pth("C:\\Users\\ben\\Desktop\\midi_archive\\desktop\\midi_random_collection\\");
	
	int filenum = 0;
	auto rdi = std::filesystem::recursive_directory_iterator(pth);
	for (const auto& dir_ent : rdi) {
		auto curr_path = dir_ent.path();
		if (!jmid::has_midifile_extension(curr_path)) {
			continue;
		}

		std::vector<char> mf(std::filesystem::file_size(curr_path));
		jmid::read_binary_csio(curr_path,mf);
		jmid::maybe_smf_t smf = jmid::read_smf(curr_path,nullptr,
			std::filesystem::file_size(curr_path));
		if (!smf) {
			continue;
		}
		++filenum;
		if (filenum < 310) { continue; }
		if ((filenum==122)||(filenum==279)||(filenum==285)||(filenum==318)) {
			continue;
		}

		auto pbeg = reinterpret_cast<const unsigned char*>(mf.data());
		auto pend = pbeg + mf.size();

		dfa sm;
		sm.init(pbeg+22,pend);//sm.init(p+22,p+590);
		std::cout << "File " << filenum << ")  " << curr_path << "\n";
		std::string out_str;
		const unsigned char *ev_beg = pbeg;
		while (!sm.finished()) {
			auto ev = sm.get_next();
			out_str += print(ev.first) + "\t";
			jmid::print_hexascii(ev_beg, ev.second,std::back_inserter(out_str),'\0',' ');
			if (ev.first==dfa::state::invalid) {
				std::abort();
			}
			std::cout << out_str << std::endl;

			out_str.clear();
			ev_beg = ev.second;

			/*if (sm.finished()) {
				ev_beg += 8;
				if (ev_beg < pend) {
					sm = dfa();
					sm.init(ev_beg,pend);
				}
			}*/
		}  // to next track

	}

	return 0;
}



int midi_files_from_fuzz_corpus(std::filesystem::path pth) {
	//std::filesystem::path pth = "C:\\Users\\ben\\Desktop\\midi_archive\\make_smf_corp\\";

	std::vector<std::filesystem::path> files;
	std::filesystem::recursive_directory_iterator rdi(pth);
	for (auto& di : rdi) {
		if (std::filesystem::is_directory(di)) {
			continue;
		}
		files.push_back(di.path());
	}

	for (int i=0; i<files.size(); ++i) {
		std::string fname = files[i].filename().string();
		auto smf = jmid::read_smf(files[i],nullptr,
			std::filesystem::file_size(files[i]));
		if (smf) {
			fname = "valid_" + fname + ".midi";
		} else {
			fname = "invalid_" + fname + ".midi";
		}
		std::filesystem::rename(files[i],files[i].parent_path()/fname);
	}

	return 0;
}



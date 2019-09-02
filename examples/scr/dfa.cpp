#include "dfa.h"
#include "generic_chunk_low_level.h"
#include "midi_delta_time.h"
#include "midi_vlq.h"
#include "midi_status_byte.h"
#include "smf_t.h"
#include "util.h"
#include "print_hexascii.h"
#include <cstdint>
#include <utility>
#include <string>
#include <iostream>
#include <charconv>


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


int dfa_parse_midis(const std::filesystem::path& pth) {
	//std::filesystem::path pth("C:\\Users\\ben\\Desktop\\midi_archive\\desktop\\hallelujah_joy_to_the_world.mid");
	//std::filesystem::path pth("C:\\Users\\ben\\Desktop\\midi_archive\\desktop\\midi_random_collection\\");
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
		}  // to next track
	}

	return 0;
}

std::string print(dfa2::state st) {
	std::string s;
	switch (st) {
	case dfa2::state::delta_time:
		s = "delta_time";
		break;
	case dfa2::state::status_byte:
		s = "status_byte";
		break;
	case dfa2::state::meta_type:
		s = "meta_type";
		break;
	case dfa2::state::vlq_payl_length:
		s = "vlq_payl_length";
		break;
	case dfa2::state::payload:
		s = "payload";
		break;
	case dfa2::state::invalid:
		s = "invalid";
		break;
	case dfa2::state::at_end_success:
		s = "at_end_success";
		break;
	default:
		s = "???";
		break;
	}
	return s;
}
std::string print(const dfa2& o) {
	std::string s = '{' + print(o.st_) + ", "
		+ std::to_string(o.val_) + ", "
		+ std::to_string(o.max_iter_remain_) + ", "
		+ std::to_string(o.rs_) + '}';
	return s;
}

bool dfa2::operator()(unsigned char c) {
	std::string sb = print(*this) + ";  "
		+ "c==" + std::to_string(c);
	unsigned char s = 0;
	switch (this->st_) {
	case dfa2::state::delta_time:
		if (this->max_iter_remain_ == 0) {
			this->st_ = dfa2::state::invalid;
			sb += "  ->  " + print(*this) + "  =>  false";
			std::cout << sb << std::endl;
			return false;  // Expect should never occur?
		}
		this->val_ += c&0x7Fu;
		this->max_iter_remain_ -= 1;
		if ((c&0x80u) && (this->max_iter_remain_ > 0)) {
			this->val_ <<= 7;
		} else if ((c&0x80u) && (this->max_iter_remain_ == 0)) {
			this->st_ = dfa2::state::invalid;
			sb += "  ->  " + print(*this) + "  =>  false";
			std::cout << sb << std::endl;
			return false;
		} else {  // this->max_iter_remain_==0 || !(c&0x80u)
			this->st_ = dfa2::state::status_byte;
		}
		sb += "  ->  " + print(*this) + "  =>  true";
		std::cout << sb << std::endl;
		return true;
	case dfa2::state::status_byte:
		s = jmid::get_status_byte(c,this->rs_);
		this->rs_= jmid::get_running_status_byte(c,this->rs_);
		if (jmid::is_channel_status_byte(s)) {
			auto n = jmid::channel_status_byte_n_data_bytes(s);
			if (jmid::is_data_byte(c)) { --n; };
			if (n <= 0) {  // in rs, single data-byte event
				this->st_ = dfa2::state::at_end_success;
			} else {
				this->max_iter_remain_ = n;
				this->st_ = dfa2::state::payload;
			}
		} else if (jmid::is_meta_status_byte(s)) {
			this->st_ = dfa2::state::meta_type;
		} else if (jmid::is_sysex_status_byte(s)) {
			this->st_ = dfa2::state::vlq_payl_length;
			this->max_iter_remain_ = 4;
			this->val_ = 0;
		} else {
			this->st_ = dfa2::state::invalid;
			sb += "  ->  " + print(*this) + "  =>  false";
			std::cout << sb << std::endl;
			return false;
		}
		sb += "  ->  " + print(*this) + "  =>  true";
		std::cout << sb << std::endl;
		return true;
	case dfa2::state::meta_type:
		this->st_ = dfa2::state::vlq_payl_length;
		this->max_iter_remain_ = 4;
		this->val_ = 0;
		sb += "  ->  " + print(*this) + "  =>  true";
		std::cout << sb << std::endl;
		return true;
	case dfa2::state::vlq_payl_length:
		if (this->max_iter_remain_ == 0) {
			this->st_ = dfa2::state::invalid;
			sb += "  ->  " + print(*this) + "  =>  false";
			std::cout << sb << std::endl;
			return false;  // Expect should never occur?
		}
		this->val_ += c&0x7Fu;
		this->max_iter_remain_ -= 1;
		if (c&0x80u) {
			if (this->max_iter_remain_ > 0) {
				this->val_ <<= 7;
			} else if (this->max_iter_remain_ == 0) {
				this->st_ = dfa2::state::invalid;
				sb += "  ->  " + print(*this) + "  =>  false";
				std::cout << sb << std::endl;
				return false;
			}
		} else {  //!(c&0x80u)
			if (this->val_ > 0) {
				this->st_ = dfa2::state::payload;
				this->max_iter_remain_ = this->val_;
			} else {
				this->st_ = dfa2::state::at_end_success;
				sb += "  ->  " + print(*this) + "  =>  true";
				std::cout << sb << std::endl;
				return true;
			}
		}
		sb += "  ->  " + print(*this) + "  =>  true";
		std::cout << sb << std::endl;
		return true;
	case dfa2::state::payload:
		if (this->max_iter_remain_ > 1) {
			this->max_iter_remain_ -= 1;
			sb += "  ->  " + print(*this) + "  =>  true";
			std::cout << sb << std::endl;
			return true;
		}
		if (this->max_iter_remain_ == 1) {
			this->max_iter_remain_ -= 1;
			this->st_ = dfa2::state::at_end_success;
			sb += "  ->  " + print(*this) + "  =>  true";
			std::cout << sb << std::endl;
			return true;
		}
		if (this->max_iter_remain_ == 0) {
			this->st_ = dfa2::state::at_end_success;
			sb += "  ->  " + print(*this) + "  =>  false";
			std::cout << sb << std::endl;
			return false;
		}
	case dfa2::state::at_end_success:
		sb += "  ->  " + print(*this) + "  =>  false";
		std::cout << sb << std::endl;
		return false;
	case dfa2::state::invalid:
		sb += "  ->  " + print(*this) + "  =>  false";
		std::cout << sb << std::endl;
		return false;
	default:
		std::cout << sb << " ...wat" << std::endl;
		this->st_ = dfa2::state::invalid;
		return false;
	}

	std::cout << sb << " ...wat" << std::endl;
	this->st_ = dfa2::state::invalid;
	return false;
}
unsigned char dfa2::get_rs() const {
	return this->rs_;
}
dfa2::state dfa2::get_state() const {
	return this->st_;
}

int dfa2_parse_midis(const std::filesystem::path& pth) {
	//pth = std::filesystem::path(R"(C:\Users\ben\Desktop\midi_archive\desktop\midi_simple_valid\)");
	//std::filesystem::path pth("C:\\Users\\ben\\Desktop\\midi_archive\\desktop\\midi_random_collection\\");
	
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
		/*if (filenum < 310) { continue; }
		if ((filenum==122)||(filenum==279)||(filenum==285)||(filenum==318)) {
			continue;
		}*/

		auto pbeg = reinterpret_cast<const unsigned char*>(mf.data());
		auto pend = pbeg + mf.size();
		//pbeg += 58;
		std::vector<unsigned char> curr_event;
		std::string out_str;
		auto sm = dfa2();
		while (pbeg<pend) {
			if (jmid::is_mthd_header_id(pbeg,pend)) {
				pbeg += 14;
				continue;
			} else if (jmid::is_mtrk_header_id(pbeg,pend)) {
				pbeg += 8;
				continue;
			}
			
			sm = dfa2(sm.get_rs());
			auto p = std::find_if_not(pbeg,pend,sm);
			std::copy(pbeg,p,std::back_inserter(curr_event));
			pbeg = p;
			/*auto it = std::back_inserter(curr_event);
			while (pbeg!=pend) {
				if (sm(*pbeg)) {
					*it++ = *pbeg++;
				} else {
					break;
				}
				if (sm.get_state()==dfa2::state::invalid) {
					break;
				}
			}
			if (sm.get_state()==dfa2::state::invalid) {
				std::abort();
			}*/
			jmid::print_hexascii(curr_event.begin(),curr_event.end(),
				std::back_inserter(out_str),'\0',' ');
			std::cout << out_str << '\n';

			//pbeg += curr_event.size();
			curr_event.clear();
			out_str.clear();
		}  // To next event
		std::cout << "---------------------------\n";
	}  // To next file
	return 0;
}





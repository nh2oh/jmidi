#include "mtrk_container_t.h"
#include "midi_raw.h"
#include "dbklib\byte_manipulation.h"
#include <string>


mtrk_iterator_t::mtrk_iterator_t() {
	this->s_ = 0x00u;
}
// Private ctor used by friend class mtrk_view_t.begin(),.end()
mtrk_iterator_t::mtrk_iterator_t(const unsigned char *p, unsigned char s) {  
	this->p_ = p;
	this->s_ = s;
}
mtrk_event_container_sbo_t mtrk_iterator_t::operator*() const {
	return mtrk_event_container_sbo_t(p,event.size,this->s_);
}

// TODO:  Should write an "..._unsafe_...()" func to just get the event size to replace
// the call to parse_mtrk_event_type()
mtrk_iterator_t& mtrk_iterator_t::operator++() {
	uint32_t maxinc = this->container_->size() - this->container_offset_;
	const unsigned char *p = this->container_->data() + this->container_offset_;

	auto event = parse_mtrk_event_type(p,this->midi_status_,maxinc);
	if (maxinc<=event.size) {  // this is the last event; return the end iterator
		this->container_offset_ = 0;
		this->midi_status_ = 0x00u;
	} else {
		p += event.size;
		auto dt = midi_interpret_vl_field(p);
		this->container_offset_ += event.size;
		this->midi_status_ = mtrk_event_get_midi_status_byte_unsafe(p+dt.N,this->midi_status_);
	}

	return *this;
}
bool mtrk_iterator_t::operator<(const mtrk_iterator_t& rhs) const {
	if (this->container_ == rhs.container_) {
		return this->container_offset_ < rhs.container_offset_;
	} else {
		return false;
	}
}
bool mtrk_iterator_t::operator==(const mtrk_iterator_t& rhs) const {
	// I *could* compare midi_status_ as well, however, it is an error condition if it's
	// different for the same offset.  
	if (this->container_ == rhs.container_) {
		return this->container_offset_ == rhs.container_offset_;
	} else {
		return false;
	}
}
bool mtrk_iterator_t::operator!=(const mtrk_iterator_t& rhs) const {
	return !(*this==rhs);
}










unsigned char mtrk_container_iterator_t::midi_status() const {
	return this->midi_status_;
}
mtrk_event_container_t mtrk_container_iterator_t::operator*() const {
	const unsigned char *p = this->container_->p_+this->container_offset_;
	//
	// _why_ am i calling parse_midi_event() ???  Should i not detect the type then call
	// parse_midi_..., parse_meta_,...  ???
	//
	//auto sz = parse_channel_event(p, this->midi_status_).data_length;

	auto type = detect_mtrk_event_type_dtstart_unsafe(p,this->midi_status_);
	int32_t maxinc = this->container_->size_-this->container_offset_;
	int32_t sz {0};
	if (type == smf_event_type::channel_mode || type == smf_event_type::channel_voice) {
		sz = parse_channel_event(p,this->midi_status_,maxinc).size;
	} else if (type == smf_event_type::meta) {
		sz = parse_meta_event(p,maxinc).size;
	} else if (type == smf_event_type::sysex_f0 || type == smf_event_type::sysex_f7) {
		sz = parse_sysex_event(p,maxinc).size;
	} else {
		std::abort();
	}

	return mtrk_event_container_t {p,sz};
}

mtrk_event_container_sbo_t mtrk_container_iterator_t::operator!() const {
	const unsigned char *p = this->container_->p_+this->container_offset_;

	auto type = detect_mtrk_event_type_dtstart_unsafe(p,this->midi_status_);
	int32_t maxinc = this->container_->size_-this->container_offset_;
	int32_t sz {0};  unsigned char s = 0;
	if (type == smf_event_type::channel_mode || type == smf_event_type::channel_voice) {
		sz = parse_channel_event(p,this->midi_status_,maxinc).size;
		s = parse_channel_event(p,this->midi_status_,maxinc).status_byte;
	} else if (type == smf_event_type::meta) {
		sz = parse_meta_event(p,maxinc).size;
	} else if (type == smf_event_type::sysex_f0 || type == smf_event_type::sysex_f7) {
		sz = parse_sysex_event(p,maxinc).size;
	} else {
		std::abort();
	}

	return mtrk_event_container_sbo_t(p,sz,s);
}

mtrk_container_iterator_t& mtrk_container_iterator_t::operator++() {
	// Get the size of the presently indicated event and increment 
	// this->container_offset_ by that ammount.  
	const unsigned char *curr_p = this->container_->p_ + this->container_offset_;
	int32_t max_inc = this->container_->size_-this->container_offset_;
	parse_mtrk_event_result_t curr_event = parse_mtrk_event_type(curr_p, this->midi_status_, max_inc);
	int32_t curr_size {0};
	if (curr_event.type==smf_event_type::sysex_f0 || curr_event.type==smf_event_type::sysex_f7) {
		auto sx = parse_sysex_event(curr_p,max_inc);
		curr_size = sx.size;
	} else if (curr_event.type==smf_event_type::meta) {
		auto mt = parse_meta_event(curr_p,max_inc);
		curr_size = mt.size;
	} else if (curr_event.type==smf_event_type::channel_voice || curr_event.type==smf_event_type::channel_mode) {
		auto md = parse_channel_event(curr_p,this->midi_status_,max_inc);
		curr_size = md.size;
	}

	this->container_offset_ += curr_size;

	// If this was the last event set the midi_status_ to 0 and return.  The value
	// of 0 is also set for the value of midi_status_ when an iterator is returned
	// from the end() method of the parent container.  
	if (this->container_offset_ == this->container_->size_) {
		this->midi_status_ = 0;
		return *this;
	}

	// If this was not the last event, set midi_status_ for the event pointed to by 
	// this->container_offset_.  Below i refer to this as the "new" event.  
	const unsigned char *new_p = this->container_->p_ + this->container_offset_;
	max_inc = this->container_->size_-this->container_offset_;
	parse_mtrk_event_result_t new_event = parse_mtrk_event_type(new_p,this->midi_status_,max_inc);
	if (new_event.type==smf_event_type::channel_voice || new_event.type==smf_event_type::channel_mode) {
		// this->midi_status_ currently holds the midi status byte applicable to the
		// previous event
		auto md = parse_channel_event(new_p,this->midi_status_,max_inc);
		if (md.is_valid) {
			this->midi_status_ = md.status_byte;
		} else {
			this->midi_status_ = 0;  // Not a valid status byte
		}
	} else {
		// sysex && meta events reset the midi status
		this->midi_status_ = 0;  // Not a valid status byte
	}

	return *this;
}

bool mtrk_container_iterator_t::operator<(const mtrk_container_iterator_t& rhs) const {
	if (this->container_ == rhs.container_) {
		return this->container_offset_ < rhs.container_offset_;
	} else {
		return false;
	}
}
bool mtrk_container_iterator_t::operator==(const mtrk_container_iterator_t& rhs) const {
	// I *could* compare midi_status_ as well, however, it is an error condition if it's
	// different for the same offset.  
	if (this->container_ == rhs.container_) {
		return this->container_offset_ == rhs.container_offset_;
	} else {
		return false;
	}
}
bool mtrk_container_iterator_t::operator!=(const mtrk_container_iterator_t& rhs) const {
	return !(*this==rhs);
}




mtrk_view_t::mtrk_view_t(const validate_mtrk_chunk_result_t& mtrk) {
	if (!mtrk.is_valid) {
		std::abort();
	}

	this->p_ = mtrk.p;
	this->size_ = mtrk.size;
}
mtrk_view_t::mtrk_view_t(const unsigned char *p, uint32_t sz) {
	this->p_ = p;
	this->size_ = sz;
}
uint32_t mtrk_view_t::data_length() const {
	return this->size_-8;
	// Alternatively, could do:  return dbk::be_2_native<uint32_t>(this->p_+4);
	// However, *(p_+4) might not be in cache, but size_ will always be hot.  
}
uint32_t mtrk_view_t::size() const {
	return this->size_;
}
const unsigned char *mtrk_view_t::data() const {
	return this->p_;
}
mtrk_iterator_t mtrk_view_t::begin() const {
	// From the std p.135:  "The first event in each MTrk chunk must specify status"
	// thus we know midi_event_get_status_byte(this->p_+8) will succeed
	return mtrk_iterator_t::mtrk_iterator_t(this, uint32_t{8}, midi_event_get_status_byte(this->p_+8));
}
mtrk_iterator_t mtrk_view_t::end() const {
	// Note that i am supplying an invalid midi status byte for this one-past-the-end 
	// iterator.  
	return mtrk_iterator_t::mtrk_iterator_t(this, this->size(), unsigned char {0});
}
bool mtrk_view_t::validate() const {
	bool tf = true;
	tf = tf && (this->size_-8 == dbk::be_2_native<uint32_t>(this->p_+4));

	auto mtrk = validate_mtrk_chunk(this->p_,this->size_);
	tf = tf && mtrk.is_valid;

	return tf;
}

std::string print(const mtrk_view_t& mtrk) {
	std::string s {};

	for (mtrk_iterator_t it = mtrk.begin(); it != mtrk.end(); ++it) {
		//auto ev = *it;
		auto ev = *it;
		s += print(ev,mtrk_sbo_print_opts::debug);
		s += "\n";
	}
	//for (auto const& e : mtrk) {
	//	s += print(e);
	//	s += "\n";
	//}

	return s;
}





mtrk_container_t::mtrk_container_t(const validate_mtrk_chunk_result_t& mtrk) {
	if (!mtrk.is_valid) {
		std::abort();
	}

	this->p_ = mtrk.p;
	this->size_ = mtrk.size;
}
mtrk_container_t::mtrk_container_t(const unsigned char *p, uint32_t sz) {
	this->p_ = p;
	this->size_ = sz;
}

// TODO:  int<->uint conversion
int32_t mtrk_container_t::data_size() const {
	return dbk::be_2_native<uint32_t>(this->p_+4);
}
uint32_t mtrk_container_t::size() const {
	//return this->data_size()+4;
	return this->size_;
}
mtrk_container_iterator_t mtrk_container_t::begin() const {
	// From the std p.135:  "The first event in each MTrk chunk must specify status"
	// thus we know midi_event_get_status_byte(this->p_+8) will succeed
	return mtrk_container_iterator_t {this, int32_t{8}, midi_event_get_status_byte(this->p_+8)};
}
mtrk_container_iterator_t mtrk_container_t::end() const {
	// Note that i am supplying an invalid midi status byte for this one-past-the-end 
	// iterator.  
	return mtrk_container_iterator_t {this, this->size(), unsigned char {0}};
}

std::string print(const mtrk_container_t& mtrk) {
	std::string s {};

	for (mtrk_container_iterator_t it = mtrk.begin(); it != mtrk.end(); ++it) {
		//auto ev = *it;
		auto ev = !it;
		s += print(ev,mtrk_sbo_print_opts::debug);
		s += "\n";
	}
	//for (auto const& e : mtrk) {
	//	s += print(e);
	//	s += "\n";
	//}

	return s;
}




int32_t mtrk_event_container_t::data_size() const {
	return (this->size() - midi_interpret_vl_field(this->p_).N);
}
int32_t mtrk_event_container_t::delta_time() const {
	return midi_interpret_vl_field(this->p_).val;
}
int32_t mtrk_event_container_t::size() const {
	return this->size_;
}
// Assumes that this indicates a valid event
smf_event_type mtrk_event_container_t::type() const {
	return detect_mtrk_event_type_unsafe(this->data_begin());
}
// Starts at the delta-time
const unsigned char *mtrk_event_container_t::begin() const {
	return this->p_;
}
// Starts at the delta-time
const unsigned char *mtrk_event_container_t::raw_begin() const {
	return this->p_;
}
// Starts just after the delta-time
const unsigned char *mtrk_event_container_t::data_begin() const {
	return (this->p_ + midi_interpret_vl_field(this->p_).N);
}
const unsigned char *mtrk_event_container_t::end() const {
	return this->p_ + this->size_;
}

std::string print(const mtrk_event_container_t& evnt) {
	std::string s {};
	s += ("delta_time == " + std::to_string(evnt.delta_time()) + ", ");
	s += ("type == " + print(evnt.type()) + ", ");
	s += ("data_size == " + std::to_string(evnt.data_size()) + ", ");
	s += ("size == " + std::to_string(evnt.size()) + "\n\t");

	auto ss = evnt.size();
	auto ds = evnt.data_size();
	std::string tests = dbk::print_hexascii(evnt.begin(), 10, ' ');

	s += ("[" + dbk::print_hexascii(evnt.begin(), evnt.size()-evnt.data_size(), ' ') + "] ");
	s += dbk::print_hexascii(evnt.data_begin(), evnt.data_size(), ' ');

	return s;
}

std::string print(const mtrk_event_container_sbo_t& evnt, mtrk_sbo_print_opts opts) {
	std::string s {};
	s += ("delta_time == " + std::to_string(evnt.delta_time()) + ", ");
	s += ("type == " + print(evnt.type()) + ", ");
	s += ("data_size == " + std::to_string(evnt.data_size()) + ", ");
	s += ("size == " + std::to_string(evnt.size()) + "\n\t");

	auto ss = evnt.size();
	auto ds = evnt.data_size();
	auto n = ss-ds;

	s += ("[" + dbk::print_hexascii(evnt.data(), evnt.size()-evnt.data_size(), ' ') + "] ");
	s += dbk::print_hexascii(evnt.data()+n, evnt.data_size(), ' ');

	if (opts == mtrk_sbo_print_opts::debug) {
		s += "\n\t";
		if (evnt.is_small()) {
			s += "sbo=>small == ";
		} else {
			s += "sbo=>big   == ";
		}
		s += "{";
		s += dbk::print_hexascii(evnt.raw_data(), sizeof(mtrk_event_container_sbo_t), ' ');
		s += "}; \n";
		s += "\tbigsmall_flag==";
		s += dbk::print_hexascii(evnt.raw_flag(), 1, ' ');
	}
	return s;
};


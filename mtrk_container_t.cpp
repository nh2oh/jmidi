#include "mtrk_container_t.h"
#include "midi_raw.h"
#include "dbklib\byte_manipulation.h"
#include <string>


// Private ctor used by friend class mtrk_view_t.begin(),.end()
mtrk_iterator_t::mtrk_iterator_t(const unsigned char *p, unsigned char s) {
	this->p_ = p;
	this->s_ = s;
}
mtrk_event_container_sbo_t mtrk_iterator_t::operator*() const {
	// Note that this->s_ indicates the status of the event prior to this->p_.  
	auto sz = mtrk_event_get_size_dtstart_unsafe(this->p_,this->s_);
	return mtrk_event_container_sbo_t(this->p_,sz,this->s_);
}
mtrk_iterator_t& mtrk_iterator_t::operator++() {
	auto dt = midi_interpret_vl_field(this->p_);
	this->s_ = mtrk_event_get_midi_status_byte_unsafe(this->p_+dt.N,this->s_);
	auto sz = mtrk_event_get_size_dtstart_unsafe(this->p_,this->s_);
	this->p_+=sz;
	// Note that this->s_ now indicates the status of the *prior* event.  
	// If i were to attempt to update it:
	// this->s_ = mtrk_event_get_midi_status_byte_unsafe(this->p_,this->s_);
	// i will dereference an invalid this->p_ in the case that the prior event
	// was the last in the mtrk sequence.  
	// One possible alternative design is to check for the end-of-sequence
	// mtrk event {0x00u,0xFFu,0x2Fu}
	return *this;
}
bool mtrk_iterator_t::operator==(const mtrk_iterator_t& rhs) const {
	// Note that this->s_ is not compared; it should be impossible for two
	// iterators w/ == p_ to have != s_, since for each, p_ must have been
	// set by an identical sequence of operations (operator++()...).  
	// However, the .end() method of mtrk_view_t constructs an iterator w/
	// p_ pointing one past the end of the seq and midi status byte 0x00u,
	// hence for the end iterator, s_ is not nec. valid.  
	return this->p_ == rhs.p_;
}
bool mtrk_iterator_t::operator!=(const mtrk_iterator_t& rhs) const {
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
	// Note that in an mtrk_iterator_t, the member s_ is the status from the 
	// _prior_ event.  
	// See the std p.135:  "The first event in each MTrk chunk must specify status"
	// ...yet i have many midi files which begin w/ 0x00u,0xFFu,...
	return mtrk_iterator_t::mtrk_iterator_t(this->p_+8,0x00u);
}
mtrk_iterator_t mtrk_view_t::end() const {
	// Note that i am supplying an invalid midi status byte for this 
	// one-past-the-end iterator.  
	return mtrk_iterator_t::mtrk_iterator_t(this->p_+this->size(),0x00u);
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


//
// For callers who have pre-computed the exact size of the event and who
// can also supply a midi status byte if applicible, ex, an mtrk_container_iterator_t.  
// For values of sz => bigsmall_t => big_t, d_.b.tag.midi_status == 0, since
// the indicated event can not be channel_voice or channel_mode.  
// For values of sz => bigsmall_t => small_t, and where the indicated event is 
// channel_voice or channel_mode, the last byte of the local data array,
// d_.s.arry[sizeof(small_t)-1] == the status byte.  
//
// TODO:  parse_mtrk_event_type can return smf_event_type::invalid in field .type;
// could replace w/ detect_mtrk_event_type_unsafe()...
//
mtrk_event_container_sbo_t::mtrk_event_container_sbo_t(const unsigned char *p, uint32_t sz, unsigned char s) {
	if (sz > sizeof(small_t)) {
		this->d_.b.p = new unsigned char[sz];
		this->d_.b.size = sz;
		this->d_.b.capacity = sz;
		std::copy(p,p+sz,this->d_.b.p);

		auto ev = parse_mtrk_event_type(p,s,sz);
		this->d_.b.dt_fixed = ev.delta_t.val;
		this->d_.b.sevt = ev.type;
		this->d_.b.midi_status = 0x00;
		// Since all smf_event_type::channel_voice || channel_mode messages have
		// a max size of 4 + 1 + 2 == 7, ev.type will always be sysex_f*, meta, or
		// invalid.  The former two cancel any running status so the midi_status
		// byte does not apply.  

		this->set_flag_big();
	} else {
		auto end = std::copy(p,p+sz,this->d_.s.arry.begin());
		while (end!=this->d_.s.arry.end()) {
			*end++ = 0x00;
		}
			
		// If this is a meta or sysex_f0/f7 event, chev.is_valid == false;
		// but == true if this is a channel_voice/mode event.  Write the 
		// status byte to the field d_.s.arry[sizeof(small_t)-3], which is
		// the same offset as d_.b.midi_status.  
		// TODO:  This is some **really**nasty** hardcoding here
		auto chev = parse_channel_event(&(this->d_.s.arry[0]),s,sizeof(small_t));
		if (chev.is_valid) {
			this->d_.s.arry[sizeof(small_t)-3] = chev.status_byte;
		}
		this->set_flag_small();
	}
};
void mtrk_event_container_sbo_t::set_flag_big() {
	this->d_.b.bigsmall_flag = 0x00u;
	//if (this->is_small()) {
	//	this->d_.b.bigsmall_flag = 0x00u;
	//}
};
void mtrk_event_container_sbo_t::set_flag_small() {
	this->d_.b.bigsmall_flag = 0x01u;
	//if (this->is_big()) {
	//	this->d_.b.bigsmall_flag = 0x01u;
	//}
};
//
// is_small() && is_big() are only public so that the function
// print(mtrk_event_container_sbo_t,mtrk_sbo_print_opts::debug) can report 
// on the state of the object.  
//
// Note that b/c my flag is not external to the big/small_t structs, i have
// to make a possibly invalid read here.  I arbitrarily choose the convention
// of reading from d_.b (see also this->raw_flag()).  
//
bool mtrk_event_container_sbo_t::is_small() const {
	return (this->d_.b.bigsmall_flag == 0x01u);
};
bool mtrk_event_container_sbo_t::is_big() const {
	return !is_small();
};
//
// Copy ctor
//
mtrk_event_container_sbo_t::mtrk_event_container_sbo_t(const mtrk_event_container_sbo_t& rhs) {
	this->d_ = rhs.d_;
	if (this->is_big()) {
		// Deep copy the pointed-at range
		unsigned char *new_p = new unsigned char[this->d_.b.capacity];
		std::copy(this->d_.b.p,this->d_.b.p+this->d_.b.capacity,new_p);
		this->d_.b.p = new_p;
	}
};
//
// Copy assignment; overwrites a pre-existing lhs 'this' w/ rhs
//
mtrk_event_container_sbo_t& mtrk_event_container_sbo_t::operator=(const mtrk_event_container_sbo_t& rhs) {
	if (this->is_big()) {
		delete this->d_.b.p;
	}
	this->d_ = rhs.d_;
	
	if (this->is_big()) {
		// Deep copy the pointed-at range
		unsigned char *new_p = new unsigned char[this->d_.b.capacity];
		std::copy(this->d_.b.p,this->d_.b.p+this->d_.b.capacity,new_p);
		this->d_.b.p = new_p;
	}
	return *this;
};
//
// Move ctor
//
mtrk_event_container_sbo_t::mtrk_event_container_sbo_t(mtrk_event_container_sbo_t&& rhs) {
	this->d_ = rhs.d_;
	if (this->is_big()) {
		rhs.set_flag_small();  // prevents ~rhs() from freeing its memory
		// Note that this state of rhs is invalid as it almost certinally does
		// not contain a valid "small" mtrk event
	}
};
//
// Move assignment
//
mtrk_event_container_sbo_t& mtrk_event_container_sbo_t::operator=(mtrk_event_container_sbo_t&& rhs) {
	if (this->is_big()) {
		delete this->d_.b.p;
	}
	this->d_ = rhs.d_;
	if (rhs.is_big()) {
		rhs.set_flag_small();  // prevents ~rhs() from freeing its memory
		// Note that this state of rhs is invalid as it almost certinally does
		// not contain a valid "small" mtrk event
	}
	return *this;
};
mtrk_event_container_sbo_t::~mtrk_event_container_sbo_t() {
	if (this->is_big()) {
		delete this->d_.b.p;
	}
};
unsigned char mtrk_event_container_sbo_t::operator[](int32_t i) const {
	if (this->is_small()) {
		return this->d_.s.arry[i];
	} else {
		return this->d_.b.p[i];
	}
};
// TODO:  If small, this returns a ptr to a stack-allocated object...  Bad?
const unsigned char *mtrk_event_container_sbo_t::data() const {
	if (this->is_small()) {
		return &(this->d_.s.arry[0]);
	} else {
		return this->d_.b.p;
	}
};
// Pointer directly to start of this->data_, without regard to 
// this->is_small()
const unsigned char *mtrk_event_container_sbo_t::raw_data() const {
	return &(this->d_.s.arry[0]);
};
const unsigned char *mtrk_event_container_sbo_t::raw_flag() const {
	//return &(this->bigsmall_flag);
	return &(this->d_.b.bigsmall_flag);
};
int32_t mtrk_event_container_sbo_t::delta_time() const {
	if (this->is_small()) {
		return midi_interpret_vl_field(&(this->d_.s.arry[0])).val;
	} else {
		return midi_interpret_vl_field(this->d_.b.p).val;
	}
};
smf_event_type mtrk_event_container_sbo_t::type() const {  // channel_{voice,mode},sysex_{f0,f7},meta,invalid
	if (this->is_small()) {
		return detect_mtrk_event_type_dtstart_unsafe(&(this->d_.s.arry[0]),
			this->d_.s.arry[sizeof(small_t)-3]);
	} else {
		return this->d_.b.sevt;
	}
};
// size of the event not including the delta-t field (but including the length field 
// and type-byte in the case of sysex & meta events)
int32_t mtrk_event_container_sbo_t::data_size() const {  // Does not include the delta-time
	if (this->is_small()) {
		// 041219
		//detect_mtrk_event_type_dtstart_unsafe

		// TODO:  this->d_.s.arry[sizeof(small_t)-1] is not the running_status...
		auto evt = parse_mtrk_event_type(&(this->d_.s.arry[0]),
			this->d_.s.arry[sizeof(small_t)-3],sizeof(small_t));
		if (evt.type == smf_event_type::invalid) {
			std::abort();
		}
		return evt.data_length;
	} else {
		return this->d_.b.size - midi_interpret_vl_field(this->d_.b.p).N;
		//return this->d_.b.size - midi_vl_field_size(this->d_.b.dt_fixed);
		// TODO:  This assumes that the dt field does not contain a leading sequence
		// of 0x80's
	}
};
int32_t mtrk_event_container_sbo_t::size() const {  // Includes the delta-time
	if (this->is_small()) {
		auto evt = parse_mtrk_event_type(&(this->d_.s.arry[0]),
			this->d_.s.arry[sizeof(small_t)-3],sizeof(small_t));
		if (evt.type == smf_event_type::invalid) {
			std::abort();
		}
		if (evt.size <= 0) {
			std::abort();
		}
		return evt.size;
	} else {
		return this->d_.b.size;
	}
};



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


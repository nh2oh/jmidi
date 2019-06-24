#include "mtrk_event_t2.h"
#include "mtrk_event_iterator_t2.h"
#include "midi_raw.h"
#include "midi_vlq.h"
#include "dbklib\byte_manipulation.h"
#include <string>
#include <cstdint>
#include <cstring>  // std::memcpy()
#include <exception>  // std::abort()
#include <algorithm>  
#include <utility>  // std::move()



std::string print(const mtrk_event_t2& evnt, mtrk_sbo_print_opts opts) {
	std::string s {};
	s += ("delta_time = " + std::to_string(evnt.delta_time()) + ", ");
	s += ("type = " + print(evnt.type()) + ", ");
	s += ("size = " + std::to_string(evnt.size()) + ", ");
	s += ("data_size = " + std::to_string(evnt.data_size()) + "\n");
	
	s += "\t[";
	dbk::print_hexascii(evnt.dt_begin(),evnt.dt_end(),std::back_inserter(s),'\0',' ');
	s += "] ";
	dbk::print_hexascii(evnt.event_begin(),evnt.end(),std::back_inserter(s),'\0',' ');
	
	if (opts == mtrk_sbo_print_opts::detail || opts == mtrk_sbo_print_opts::debug) {
		if (evnt.type()==smf_event_type::meta) {
			s += "\n";
			s += ("\tmeta type: " + print(classify_meta_event(evnt)) + "; ");
			if (meta_has_text(evnt)) {
				s += ("text payload: \"" + meta_generic_gettext(evnt) + "\"; ");
			} else if (is_tempo(evnt)) {
				s += ("value = " + std::to_string(get_tempo(evnt)) + " us/q; ");
			} else if (is_timesig(evnt)) {
				auto data = get_timesig(evnt);
				s += ("value = " + std::to_string(data.num) + "/" 
					+ std::to_string(static_cast<int>(std::exp2(data.log2denom)))
					+ ", " + std::to_string(data.clckspclk) + " MIDI clocks/click, "
					+ std::to_string(data.ntd32pq) + " notated 32'nd nts / MIDI q nt; ");
			}
		}
	} 
	if (opts == mtrk_sbo_print_opts::debug) {
		s += "\n\t";
		if (evnt.is_small()) {
			s += "sbo=>small = ";
		} else {
			s += "sbo=>big   = ";
		}
		s += "{";
		dbk::print_hexascii(evnt.raw_begin(),evnt.raw_end(),
			std::back_inserter(s),'\0',' ');
		s += "}; \n";
		s += "\tbigsmall_flag = ";
		auto f = evnt.flags();
		s += dbk::print_hexascii(&f, 1, ' ');
	}
	return s;
}



unsigned char *mtrk_event_t2::small_t::begin() {
	return &(this->d_[0]);
}
const unsigned char *mtrk_event_t2::small_t::begin() const {
	return &(this->d_[0]);
}
unsigned char *mtrk_event_t2::small_t::end() {
	return &(this->d_[0]) + mtrk_event_get_size_dtstart_unsafe(&(this->d_[0]),0x00u);
}
const unsigned char *mtrk_event_t2::small_t::end() const {
	return &(this->d_[0]) + mtrk_event_get_size_dtstart_unsafe(&(this->d_[0]),0x00u);
}
uint64_t mtrk_event_t2::small_t::size() const {
	return mtrk_event_get_size_dtstart_unsafe(&(this->d_[0]),0x00u);
}
constexpr uint64_t mtrk_event_t2::small_t::capacity() const {
	static_assert(sizeof(this->d_) > 1);
	return sizeof(this->d_)-1;
}
uint64_t mtrk_event_t2::big_t::size() const {
	return this->sz_;
}
uint64_t mtrk_event_t2::big_t::capacity() const {
	return this->cap_;
}
unsigned char *mtrk_event_t2::big_t::begin() {
	return this->p_;
}
const unsigned char *mtrk_event_t2::big_t::begin() const {
	return this->p_;
}
unsigned char *mtrk_event_t2::big_t::end() {
	return this->p_ + this->sz_;
}
const unsigned char *mtrk_event_t2::big_t::end() const {
	return this->p_ + this->sz_;
}
bool mtrk_event_t2::sbo_t::is_small() const {
	return ((this->u_.s_.flags_)&0x80u)==0x80u;
}
bool mtrk_event_t2::sbo_t::is_big() const {
	return !(this->is_small());
}
void mtrk_event_t2::sbo_t::set_flag_small() {
	this->u_.s_.flags_ |= 0x80u;  // Note i always go through s_
}
void mtrk_event_t2::sbo_t::set_flag_big() {
	this->u_.s_.flags_ &= 0x7Fu;  // Note i always go through s_
}
void mtrk_event_t2::sbo_t::free_if_big() {
	if (this->is_big()) {
		delete this->u_.b_.p_;
		this->big_adopt(nullptr,0,0);  // sets flag big
	}
}
void mtrk_event_t2::sbo_t::big_adopt(unsigned char *p, uint32_t sz, uint32_t c) {
	this->set_flag_big();
	this->u_.b_.p_=p;
	this->u_.b_.sz_=sz;
	this->u_.b_.cap_=c;
}
void mtrk_event_t2::sbo_t::zero_object() {
	this->set_flag_small();
	for (auto it=this->u_.s_.begin(); it!=this->u_.s_.end(); ++it) {
		*it=0x00u;
	}
}
uint64_t mtrk_event_t2::sbo_t::size() const {
	if (this->is_small()) {
		return this->u_.s_.size();
	} else {
		return this->u_.b_.size();
	}
}
uint64_t mtrk_event_t2::sbo_t::capacity() const {
	if (this->is_small()) {
		return this->u_.s_.capacity();
	} else {
		return this->u_.b_.capacity();
	}
}
uint32_t mtrk_event_t2::sbo_t::resize(uint32_t new_cap) {
	new_cap = std::max(static_cast<uint64_t>(new_cap),this->small_capacity());
	if (new_cap == this->small_capacity()) {
		// Resize the object to fit in a small_t.  If it's presently small,
		// do nothing.  
		if (this->is_big()) {
			auto p = this->u_.b_.p_;
			auto sz = this->u_.b_.sz_;
			auto new_sz = std::min(sz,new_cap);
			this->zero_object();
			this->set_flag_small();
			std::copy(p,p+new_sz,this->u_.s_.begin());
			// No need to std::fill(...,0x00u); already called zero_object()
			delete p;
		}
	} else if (new_cap > this->small_capacity()) {
		// After resizing, the object will be held in a big_t
		if (this->is_big()) {  // presently big
			if (new_cap != this->u_.b_.cap_) {  // ... and are changing the cap
				auto p = this->u_.b_.p_;
				auto sz = this->u_.b_.sz_;
				auto new_sz = std::min(sz,new_cap);
				// Note that if new_cap is < sz, the data will be truncated
				// and the object will almost certainly be invalid.  
				unsigned char *new_p = new unsigned char[new_cap];
				auto new_end = std::copy(p,p+new_sz,new_p);
				std::fill(new_end,new_p+new_cap,0x00u);
				delete p;
				this->big_adopt(new_p,new_sz,new_cap);
				this->set_flag_big();
			}
			// if already big and new_cap==present cap;  do nothing
		} else {  // presently small 
			// Copy the present small object into a new big object.  The present
			// capacity of small_capacity() is < new_cap
			auto sz = this->u_.s_.size();
			unsigned char *new_p = new unsigned char[new_cap];
			auto new_end = std::copy(this->u_.s_.begin(),this->u_.s_.end(),new_p);
			std::fill(new_end,new_p+new_cap,0x00u);
			this->zero_object();
			this->big_adopt(new_p,sz,new_cap);
			this->set_flag_big();
		}
	}
	return new_cap;
}
constexpr uint64_t mtrk_event_t2::sbo_t::small_capacity() {
	return this->u_.s_.capacity();
}
unsigned char *mtrk_event_t2::sbo_t::begin() {
	if (this->is_small()) {
		return this->u_.s_.begin();
	} else {
		return this->u_.b_.begin();
	}
}
const unsigned char *mtrk_event_t2::sbo_t::begin() const {
	if (this->is_small()) {
		return this->u_.s_.begin();
	} else {
		return this->u_.b_.begin();
	}
}
unsigned char *mtrk_event_t2::sbo_t::end() {
	if (this->is_small()) {
		return this->u_.s_.end();
	} else {
		return this->u_.b_.end();
	}
}
const unsigned char *mtrk_event_t2::sbo_t::end() const {
	if (this->is_small()) {
		return this->u_.s_.end();
	} else {
		return this->u_.b_.end();
	}
}
unsigned char *mtrk_event_t2::sbo_t::raw_begin() {
	return &(this->u_.raw_[0]);
}
const unsigned char *mtrk_event_t2::sbo_t::raw_begin() const {
	return &(this->u_.raw_[0]);
}
unsigned char *mtrk_event_t2::sbo_t::raw_end() {
	return &(this->u_.raw_[0]) + this->u_.raw_.size();
}
const unsigned char *mtrk_event_t2::sbo_t::raw_end() const {
	return &(this->u_.raw_[0]) + this->u_.raw_.size();
}

// Default ctor creates a meta text-event of length 0 
mtrk_event_t2::mtrk_event_t2() {  
	default_init();
}
//
// For callers who have pre-computed the exact size of the event and who
// can also supply a midi status byte if applicible, ex, an 
// mtrk_container_iterator_t.  
//
mtrk_event_t2::mtrk_event_t2(const unsigned char *p, uint32_t sz, unsigned char rs) {
	auto cap = sz;
	auto dt = midi_interpret_vl_field(p,sz);
	auto s = get_status_byte(*(p+dt.N),rs);
	bool has_local_status = (s==*(p+dt.N));
	if (!has_local_status) {
		cap += 1;
	}

	if (cap<=this->d_.small_capacity()) {  // small
		this->d_.set_flag_small();
	} else {  // big
		this->d_.set_flag_big();
		unsigned char* new_p = new unsigned char[cap];  
		this->d_.big_adopt(new_p,sz,cap);
	}
	unsigned char *dest = this->d_.begin();
	auto dest_end = std::copy(p,p+dt.N,dest);
	p+=dt.N;
	
	*dest_end++ = s;
	if (has_local_status) {
		++p;
	}

	dest_end = std::copy(p,p+(sz-dt.N),dest_end);
	std::fill(dest_end,this->d_.end(),0x00u);
}
// TODO:  Clamp dt to the max allowable value
mtrk_event_t2::mtrk_event_t2(const uint32_t& dt, const unsigned char *p, 
										uint32_t sz, unsigned char rs) {
	auto cap = sz;
	auto dtN = midi_vl_field_size(dt);
	cap += dtN;
	auto s = get_status_byte(*p,rs);
	bool has_local_status = (s==*p);
	if (!has_local_status) {
		cap += 1;
	}

	if (cap<=this->d_.small_capacity()) {  // small
		this->d_.set_flag_small();
	} else {  // big
		this->d_.set_flag_big();
		unsigned char* new_p = new unsigned char[cap];  
		this->d_.big_adopt(new_p,sz,cap);
	}
	unsigned char *dest = this->d_.begin();
	auto dest_end = std::copy(p,p+dtN,dest);
	p+=dtN;
	
	*dest_end++ = s;
	if (has_local_status) {
		++p;
	}

	dest_end = std::copy(p,p+(sz-dtN),dest_end);
	std::fill(dest_end,this->d_.end(),0x00u);
}
mtrk_event_t2::mtrk_event_t2(uint32_t dt, midi_ch_event_t md) {
	md = normalize(md);
	this->d_.set_flag_small();
	auto dest_end = midi_write_vl_field(this->d_.begin(),dt);
	unsigned char s = 0x80u|(md.status_nybble);
	s += 0x0Fu&(md.ch);
	*dest_end++ = s;
	*dest_end++ = 0x7Fu&(md.p1);
	if (channel_status_byte_n_data_bytes(s)==2) {
		*dest_end++ = 0x7Fu&(md.p2);
	}
	std::fill(dest_end,this->d_.end(),0x00u);
}
//
// Copy ctor
//
mtrk_event_t2::mtrk_event_t2(const mtrk_event_t2& rhs) {
	if (rhs.size()>this->d_.small_capacity()) {
		this->d_.set_flag_big();
		unsigned char *new_p = new unsigned char[rhs.size()];
		auto data_end = std::copy(rhs.begin(),rhs.end(),new_p);
		std::fill(data_end,new_p+rhs.size(),0x00u);
		this->d_.big_adopt(new_p,rhs.size(),rhs.size());
	} else {  // rhs is small
		this->d_.set_flag_small();
		auto data_end = std::copy(rhs.begin(),rhs.end(),this->begin());
		std::fill(data_end,this->end(),0x00u);
	}
}
//
// Copy assignment; overwrites a pre-existing lhs 'this' w/ rhs
//
mtrk_event_t2& mtrk_event_t2::operator=(const mtrk_event_t2& rhs) {
	auto src_beg = rhs.data();  // Saves abt 6 calls to rhs.data()
	auto src_sz = rhs.size();  // Saves abt 10 calls to rhs.size()
	if (src_sz>this->d_.small_capacity()) {  // rhs requires a 'big' container
		if (this->capacity() < src_sz) {
			// ... and the present container (big or small) has insufficient capacity
			this->d_.free_if_big();
			unsigned char *new_p = new unsigned char[src_sz];
			auto data_end = std::copy(src_beg,src_beg+src_sz,new_p);
			std::fill(data_end,new_p+src_sz,0x00u);
			this->d_.set_flag_big();
			this->d_.big_adopt(new_p,src_sz,src_sz);
		} else {
			// ... and the present container (must be big) has sufficient capacity
			this->d_.set_flag_big();
			auto data_end = std::copy(src_beg,src_beg+src_sz,this->begin());
			std::fill(data_end,this->end(),0x00u);
			this->d_.big_adopt(this->data(),src_sz,this->capacity());
		}
	} else {  // rhs requires a small container
		this->d_.free_if_big();
		this->d_.set_flag_small();
		auto data_end = std::copy(src_beg,src_beg+src_sz,this->begin());
		std::fill(data_end,this->end(),0x00u);
	}
	return *this;
}
//
// Move ctor
//
mtrk_event_t2::mtrk_event_t2(mtrk_event_t2&& rhs) noexcept {
	this->d_ = rhs.d_;
	rhs.default_init();
	// Prevents ~rhs() from freeing its memory.  Sets the 'small' flag and
	// zeros all elements of the data array.  
}
//
// Move assign
//
mtrk_event_t2& mtrk_event_t2::operator=(mtrk_event_t2&& rhs) noexcept {
	this->d_.free_if_big();
	this->d_ = rhs.d_;
	rhs.default_init();
	// Prevents ~rhs() from freeing its memory.  Sets the 'small' flag and
	// zeros all elements of the data array.  
	return *this;
}
//
// dtor
//
mtrk_event_t2::~mtrk_event_t2() {
	this->d_.free_if_big();
}

uint64_t mtrk_event_t2::size() const {
	return this->d_.size();
}
uint64_t mtrk_event_t2::capacity() const {
	return this->d_.capacity();
}

unsigned char *mtrk_event_t2::data() {
	return this->d_.begin();
}
const unsigned char *mtrk_event_t2::data() const {
	return this->d_.begin();
}
mtrk_event_iterator_t2 mtrk_event_t2::begin() {
	return mtrk_event_iterator_t2(*this);
}
mtrk_event_const_iterator_t2 mtrk_event_t2::begin() const {
	return mtrk_event_const_iterator_t2(*this);
}
mtrk_event_iterator_t2 mtrk_event_t2::end() {
	return mtrk_event_iterator_t2(*this);
}
mtrk_event_const_iterator_t2 mtrk_event_t2::end() const {
	return mtrk_event_const_iterator_t2(*this);
}
mtrk_event_const_iterator_t2 mtrk_event_t2::dt_begin() const {
	return this->begin();
}
mtrk_event_iterator_t2 mtrk_event_t2::dt_begin() {
	return this->begin();
}
mtrk_event_const_iterator_t2 mtrk_event_t2::dt_end() const {
	return advance_to_vlq_end(this->begin());
}
mtrk_event_iterator_t2 mtrk_event_t2::dt_end() {
	return advance_to_vlq_end(this->begin());
}
mtrk_event_const_iterator_t2 mtrk_event_t2::event_begin() const {
	return advance_to_vlq_end(this->begin());
}
mtrk_event_iterator_t2 mtrk_event_t2::event_begin() {
	return advance_to_vlq_end(this->begin());
}
mtrk_event_const_iterator_t2 mtrk_event_t2::payload_begin() const {
	auto it = this->event_begin();
	if (this->type()==smf_event_type::meta) {
		it += 2;  // 0xFFu, type-byte
		it = advance_to_vlq_end(it);
	} else if (this->type()==smf_event_type::sysex_f0
					|| this->type()==smf_event_type::sysex_f7) {
		it += 1;  // 0xF0u or 0xF7u
		it = advance_to_vlq_end(it);
	} // else { smf_event_type::channel_voice, _mode, unknown, invalid...
	return it;
}
mtrk_event_iterator_t2 mtrk_event_t2::payload_begin() {
	auto it = this->event_begin();
	if (this->type()==smf_event_type::meta) {
		it += 2;  // 0xFFu, type-byte
		it = advance_to_vlq_end(it);
	} else if (this->type()==smf_event_type::sysex_f0
					|| this->type()==smf_event_type::sysex_f7) {
		it += 1;  // 0xF0u or 0xF7u
		it = advance_to_vlq_end(it);
	} // else { smf_event_type::channel_voice, _mode, unknown, invalid...
	return it;
}
const unsigned char& mtrk_event_t2::operator[](uint32_t i) const {
	return *(this->data()+i);
};
unsigned char& mtrk_event_t2::operator[](uint32_t i) {
	return *(this->data()+i);
};


smf_event_type mtrk_event_t2::type() const {
	auto p = advance_to_vlq_end(this->data());
	return classify_status_byte(*p);
	// Faster than classify_mtrk_event_dtstart_unsafe(s,rs), b/c the latter
	// calls classify_status_byte(get_status_byte(s,rs)); here, i do not 
	// need to worry about the possibility of the rs byte.  
}
uint32_t mtrk_event_t2::delta_time() const {
	return midi_interpret_vl_field(this->data()).val;
}
unsigned char mtrk_event_t2::status_byte() const {
	auto p = advance_to_vlq_end(this->data());
	return *p;
}
unsigned char mtrk_event_t2::running_status() const {
	auto p = advance_to_vlq_end(this->data());
	return get_running_status_byte(*p,0x00u);
}
uint32_t mtrk_event_t2::data_size() const {  // Not indluding delta-t
	return mtrk_event_get_data_size_dtstart_unsafe(this->data(),0x00u);
}


uint32_t mtrk_event_t2::set_delta_time(uint32_t dt) {
	auto new_dt_size = midi_vl_field_size(dt);
	auto curr_dt_size = this->dt_end()-this->begin();
	if (curr_dt_size == new_dt_size) {
		midi_write_vl_field(this->begin(),dt);
	} else if (curr_dt_size > new_dt_size) {
		midi_rewrite_dt_field_unsafe(dt,this->data(),0x00u);
	} else if (curr_dt_size < new_dt_size) {
		// The new dt is bigger than the current dt, and w/ the new
		// dt, the event...
		auto new_size = this->size()+(new_dt_size-curr_dt_size);
		if (this->capacity() >= new_size) {  // ... still fits
			midi_rewrite_dt_field_unsafe(dt,this->data(),0x00u);
		} else {  // ... won't fit
			this->d_.resize(new_size);
			midi_rewrite_dt_field_unsafe(dt,this->data(),0x00u);
			//unsigned char *new_p = new unsigned char[new_size];
			//auto curr_end = midi_write_vl_field(new_p,dt);
			//curr_end = std::copy(this->begin(),this->end(),curr_end);
			//std::fill(curr_end,new_p+new_size,0x00u);
			//this->d_.free_if_big();
			//this->d_.set_flag_big();
			//this->d_.big_adopt(new_p,new_size,new_size);
		}
	}
	return this->delta_time();
}

bool mtrk_event_t2::operator==(const mtrk_event_t2& rhs) const {
	auto it_lhs = this->begin();  auto lhs_end = this->end();
	auto it_rhs = rhs.begin();  auto rhs_end = rhs.end();
	if ((lhs_end-it_lhs) != (rhs_end-it_rhs)) {
		return false;
	}
	while (it_lhs!=lhs_end && it_rhs!=rhs_end) {
		if (*it_lhs != *it_rhs) {
			return false;
		}
		++it_lhs;
		++it_rhs;
	}
	return true;
}
bool mtrk_event_t2::operator!=(const mtrk_event_t2& rhs) const {
	return !(*this==rhs);
}


void mtrk_event_t2::default_init() {
	this->d_.set_flag_small();
	std::array<unsigned char,4> evdata = {0x00,0xFF,0x01,0x00};
	auto end = std::copy(evdata.begin(),evdata.end(),this->d_.begin());
	std::fill(end,this->d_.end(),0x00u);
}
const unsigned char *mtrk_event_t2::raw_begin() const {
	return this->d_.raw_begin();
}
const unsigned char *mtrk_event_t2::raw_end() const {
	return this->d_.raw_end();
}
unsigned char mtrk_event_t2::flags() const {
	return *(this->d_.raw_begin());
}
bool mtrk_event_t2::is_big() const {
	return !(this->flags()&0x80u);
}
bool mtrk_event_t2::is_small() const {
	return !(this->is_big());
}






/*





std::string mtrk_event_t2::text_payload() const {
	std::string s {};
	if (this->type()==smf_event_type::meta 
				|| this->type()==smf_event_type::sysex_f0
				|| this->type()==smf_event_type::sysex_f7) {
		std::copy(this->payload_begin(),this->end(),std::back_inserter(s));
	}
	return s;
}
uint32_t mtrk_event_t2::uint32_payload() const {
	return be_2_native<uint32_t>(this->payload_begin(),this->end());
}
mtrk_event_t2::channel_event_data_t mtrk_event_t2::midi_data() const {
	mtrk_event_t2::channel_event_data_t result {0x00u,0x80u,0x80u,0x80u};
	result.is_valid = false;
	// Note that 0x00u is invalid as a status nybble, and 0x80u is invalid
	// as a channel number and value for p1, p2.  
	if (this->type()==smf_event_type::channel) {
		result.is_valid = true;
		
		auto dt = midi_interpret_vl_field(this->data());
		auto p = this->data()+dt.N;
		result.is_running_status = *p!=this->midi_status_;

		result.status_nybble = this->midi_status_&0xF0u;
		result.ch = this->midi_status_&0x0Fu;
		if (!result.is_running_status) {
			++p;
		}
		result.p1 = *p++;
		if (channel_status_byte_n_data_bytes(this->midi_status_)==2) {
			result.p2 = *p;
		}
	}

	return result;
}






bool mtrk_event_t2::validate() const {
	bool tf = true;

	if (this->is_small()) {
		tf &= !this->is_big();
		tf &= (this->small_ptr()==&(this->d_[0]));
		tf &= (this->small_ptr()==this->data());
		tf &= (this->small_ptr()==this->raw_data());

		auto dt_loc = midi_interpret_vl_field(this->small_ptr());
		tf &= (dt_loc.val==this->delta_time());
		tf &= (dt_loc.N==midi_vl_field_size(this->delta_time()));

		auto size_loc = mtrk_event_get_size_dtstart_unsafe(this->small_ptr(),this->midi_status_);
		tf &= (size_loc==this->size());

		// TODO:  arbitrary max_size==6
		auto type_loc = classify_mtrk_event_dtstart(this->small_ptr(),this->midi_status_,6);
		tf &= (type_loc==this->type());

		// data_size() must be consistent w/ manual examination of the remote array
		auto data_size_loc = mtrk_event_get_data_size_dtstart_unsafe(this->small_ptr(),this->midi_status_);
		tf &= (data_size_loc==this->data_size());

		// Capacity must == the size of the sbo
		tf &= (this->capacity()==static_cast<uint32_t>(offs::max_size_sbo));
	} else {  // big
		tf &= this->is_big();
		tf &= !this->is_small();
		tf &= this->big_ptr()==this->data();
		tf &= this->big_ptr()!=this->raw_data();

		// Compare the local "cached" dt, size, type w/ that obtained from
		// reading the remote array.  
		auto dtval_loc = this->big_delta_t();
		auto dt_remote = midi_interpret_vl_field(this->big_ptr());
		tf &= (dt_remote.val==dtval_loc);
		tf &= (dt_remote.N==midi_vl_field_size(dtval_loc));
		tf &= (dt_remote.val==this->delta_time());

		auto size_loc = this->big_size();
		auto size_remote = mtrk_event_get_size_dtstart_unsafe(this->big_ptr(),this->midi_status_);
		tf &= (size_loc==size_remote);
		tf &= (size_loc==this->size());

		auto type_loc = this->big_smf_event_type();
		// TODO:  Arbitrary max_size==6
		auto type_remote = classify_mtrk_event_dtstart(this->big_ptr(),this->midi_status_,6);
		tf &= (type_loc==type_remote);
		tf &= (this->type()==type_loc);

		// data_size() must be consistent w/ manual examination of the remote array
		auto data_size_remote = mtrk_event_get_data_size_dtstart_unsafe(this->big_ptr(),this->midi_status_);
		tf &= (this->data_size()==data_size_remote);

		// Sanity checks for size, capacity values
		tf &= (this->big_size() >= 0);
		tf &= (this->big_cap() >= this->big_size());
	}

	// Local midi_status_ field:  Tests are independent of is_big()/small()
	// For midi events, this->midi_status_ must be == the status byte 
	// applic to the event
	if (this->type()==smf_event_type::channel) {
		// Present event is a midi event
		auto p = this->data()+midi_vl_field_size(this->delta_time());
		if (is_channel_status_byte(*p)) {  // not running status
			// The event-local status byte must match this->midi_status_
			tf &= (*p==this->midi_status_);
		} else {  // running status
			// TODO:  This is confusing as hell...
			// this->midi_status_ must accurately describe the # of midi
			// data bytes.  
			tf &= (is_channel_status_byte(this->midi_status_));
			//auto nbytesmsg = midi_channel_event_n_bytes(this->midi_status_,0x00u)-1;
			auto nbytesmsg = channel_event_get_data_size(p,this->midi_status_);
			tf &= (is_data_byte(*p));
			if (nbytesmsg==2) {
				tf &= (is_data_byte(*++p));
			}
		}
	}
	// For non-midi-events, this->midi_status must ==0x00u
	if (this->type()==smf_event_type::sysex_f0 
			|| this->type()==smf_event_type::sysex_f7
			|| this->type()==smf_event_type::meta
			|| this->type()==smf_event_type::invalid) {
		// Present event is _not_ a midi event
		tf &= (this->midi_status_==0x00u);
	}

	return tf;
}


*/

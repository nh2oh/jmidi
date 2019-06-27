#include "mtrk_event_t.h"
#include "mtrk_event_iterator_t.h"
#include "mtrk_event_methods.h"
#include "midi_raw.h"
#include "midi_vlq.h"
#include "dbklib\byte_manipulation.h"
#include <string>
#include <cstdint>
#include <algorithm>  




std::string print(const mtrk_event_t& evnt, mtrk_sbo_print_opts opts) {
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



// Default ctor creates a meta text-event of length 0 
mtrk_event_t::mtrk_event_t() {  
	default_init();
}
//
// For callers who have pre-computed the exact size of the event and who
// can also supply a midi status byte if applicible, ex, an 
// mtrk_container_iterator_t.  
//
mtrk_event_t::mtrk_event_t(const unsigned char *p, uint32_t sz, unsigned char rs) {
	auto p_end = p+sz;
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

	dest_end = std::copy(p,p_end,dest_end);
	std::fill(dest_end,this->d_.end(),0x00u);
}
// TODO:  Clamp dt to the max allowable value
mtrk_event_t::mtrk_event_t(const uint32_t& dt, const unsigned char *p, 
										uint32_t sz, unsigned char rs) {
	auto p_end = p+sz;
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

	dest_end = std::copy(p,p_end,dest_end);
	std::fill(dest_end,this->d_.end(),0x00u);
}
mtrk_event_t::mtrk_event_t(uint32_t dt, midi_ch_event_t md) {
	//md = normalize(md);
	this->d_.set_flag_small();
	auto dest_end = midi_write_vl_field(this->d_.begin(),dt);
	unsigned char s = (md.status_nybble)|(md.ch);
	*dest_end++ = s;
	*dest_end++ = (md.p1);
	*dest_end++ = (md.p2);
	//unsigned char s = 0x80u|(md.status_nybble);
	//s += 0x0Fu&(md.ch);
	//*dest_end++ = s;
	//*dest_end++ = 0x7Fu&(md.p1);
	//if (channel_status_byte_n_data_bytes(s)==2) {
	//	*dest_end++ = 0x7Fu&(md.p2);
	//}
	std::fill(dest_end,this->d_.end(),0x00u);
}
//
// Copy ctor
//
mtrk_event_t::mtrk_event_t(const mtrk_event_t& rhs) {
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
mtrk_event_t& mtrk_event_t::operator=(const mtrk_event_t& rhs) {
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
mtrk_event_t::mtrk_event_t(mtrk_event_t&& rhs) noexcept {
	this->d_ = rhs.d_;
	rhs.default_init();
	// Prevents ~rhs() from freeing its memory.  Sets the 'small' flag and
	// zeros all elements of the data array.  
}
//
// Move assign
//
mtrk_event_t& mtrk_event_t::operator=(mtrk_event_t&& rhs) noexcept {
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
mtrk_event_t::~mtrk_event_t() {
	this->d_.free_if_big();
}

uint64_t mtrk_event_t::size() const {
	auto sz = mtrk_event_get_size_dtstart_unsafe(this->d_.begin(),0x00u);
	auto cap = this->d_.capacity();
	return sz > cap ? cap : sz;
	//&(this->d_[0]) + mtrk_event_get_size_dtstart_unsafe(&(this->d_[0]),0x00u);
	// return this->d_.size();
}
uint64_t mtrk_event_t::capacity() const {
	return this->d_.capacity();
}

unsigned char *mtrk_event_t::data() {
	return this->d_.begin();
}
const unsigned char *mtrk_event_t::data() const {
	return this->d_.begin();
}
mtrk_event_iterator_t mtrk_event_t::begin() {
	return mtrk_event_iterator_t(*this);
}
mtrk_event_const_iterator_t mtrk_event_t::begin() const {
	return mtrk_event_const_iterator_t(*this);
}
mtrk_event_iterator_t mtrk_event_t::end() {
	return this->begin()+this->size();
}
mtrk_event_const_iterator_t mtrk_event_t::end() const {
	return this->begin()+this->size();
}
mtrk_event_const_iterator_t mtrk_event_t::dt_begin() const {
	return this->begin();
}
mtrk_event_iterator_t mtrk_event_t::dt_begin() {
	return this->begin();
}
mtrk_event_const_iterator_t mtrk_event_t::dt_end() const {
	return advance_to_vlq_end(this->begin());
}
mtrk_event_iterator_t mtrk_event_t::dt_end() {
	return advance_to_vlq_end(this->begin());
}
mtrk_event_const_iterator_t mtrk_event_t::event_begin() const {
	return advance_to_vlq_end(this->begin());
}
mtrk_event_iterator_t mtrk_event_t::event_begin() {
	return advance_to_vlq_end(this->begin());
}
mtrk_event_const_iterator_t mtrk_event_t::payload_begin() const {
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
mtrk_event_iterator_t mtrk_event_t::payload_begin() {
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
iterator_range_t<mtrk_event_const_iterator_t> mtrk_event_t::payload_range() const {
	auto sz = this->size();
	auto it = this->event_begin();
	if (this->type()==smf_event_type::meta) {
		it += 2;  // 0xFFu, type-byte
		it = advance_to_vlq_end(it);
	} else if (this->type()==smf_event_type::sysex_f0
					|| this->type()==smf_event_type::sysex_f7) {
		it += 1;  // 0xF0u or 0xF7u
		it = advance_to_vlq_end(it);
	} // else { smf_event_type::channel_voice, _mode, unknown, invalid...
	return {it,this->end()};
}
iterator_range_t<mtrk_event_iterator_t> mtrk_event_t::payload_range() {
	auto sz = this->size();
	auto it = this->event_begin();
	if (this->type()==smf_event_type::meta) {
		it += 2;  // 0xFFu, type-byte
		it = advance_to_vlq_end(it);
	} else if (this->type()==smf_event_type::sysex_f0
					|| this->type()==smf_event_type::sysex_f7) {
		it += 1;  // 0xF0u or 0xF7u
		it = advance_to_vlq_end(it);
	} // else { smf_event_type::channel_voice, _mode, unknown, invalid...
	return {it,this->end()};
}
const unsigned char& mtrk_event_t::operator[](uint32_t i) const {
	return *(this->data()+i);
};
unsigned char& mtrk_event_t::operator[](uint32_t i) {
	return *(this->data()+i);
};


smf_event_type mtrk_event_t::type() const {
	auto p = advance_to_vlq_end(this->data());
	return classify_status_byte(*p);
	// Faster than classify_mtrk_event_dtstart_unsafe(s,rs), b/c the latter
	// calls classify_status_byte(get_status_byte(s,rs)); here, i do not 
	// need to worry about the possibility of the rs byte.  
}
uint32_t mtrk_event_t::delta_time() const {
	return midi_interpret_vl_field(this->data()).val;
}
unsigned char mtrk_event_t::status_byte() const {
	auto p = advance_to_vlq_end(this->data());
	return *p;
}
unsigned char mtrk_event_t::running_status() const {
	auto p = advance_to_vlq_end(this->data());
	return get_running_status_byte(*p,0x00u);
}
uint32_t mtrk_event_t::data_size() const {  // Not indluding delta-t
	return mtrk_event_get_data_size_dtstart_unsafe(this->data(),0x00u);
}


uint32_t mtrk_event_t::set_delta_time(uint32_t dt) {
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

bool mtrk_event_t::operator==(const mtrk_event_t& rhs) const {
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
bool mtrk_event_t::operator!=(const mtrk_event_t& rhs) const {
	return !(*this==rhs);
}


void mtrk_event_t::default_init() {
	this->d_.set_flag_small();
	std::array<unsigned char,4> evdata = {0x00,0xFF,0x01,0x00};
	auto end = std::copy(evdata.begin(),evdata.end(),this->d_.begin());
	std::fill(end,this->d_.end(),0x00u);
}
const unsigned char *mtrk_event_t::raw_begin() const {
	return this->d_.raw_begin();
}
const unsigned char *mtrk_event_t::raw_end() const {
	return this->d_.raw_end();
}
unsigned char mtrk_event_t::flags() const {
	return *(this->d_.raw_begin());
}
bool mtrk_event_t::is_big() const {
	return !(this->flags()&0x80u);
}
bool mtrk_event_t::is_small() const {
	return !(this->is_big());
}

bool mtrk_event_unit_test_helper_t::is_big() {
	return this->p_->is_big();
}
bool mtrk_event_unit_test_helper_t::is_small() {
	return this->p_->is_small();
}
const unsigned char *mtrk_event_unit_test_helper_t::raw_begin() {
	return this->p_->raw_begin();
}
const unsigned char *mtrk_event_unit_test_helper_t::raw_end() {
	return this->p_->raw_begin();
}
unsigned char mtrk_event_unit_test_helper_t::flags() {
	return this->p_->flags();
}

/*





std::string mtrk_event_t::text_payload() const {
	std::string s {};
	if (this->type()==smf_event_type::meta 
				|| this->type()==smf_event_type::sysex_f0
				|| this->type()==smf_event_type::sysex_f7) {
		std::copy(this->payload_begin(),this->end(),std::back_inserter(s));
	}
	return s;
}
uint32_t mtrk_event_t::uint32_payload() const {
	return be_2_native<uint32_t>(this->payload_begin(),this->end());
}
mtrk_event_t::channel_event_data_t mtrk_event_t::midi_data() const {
	mtrk_event_t::channel_event_data_t result {0x00u,0x80u,0x80u,0x80u};
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

*/


/*

bool mtrk_event_t::validate() const {
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

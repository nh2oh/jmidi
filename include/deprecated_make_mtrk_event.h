#pragma once
/*
#include "mtrk_event_t.h"
#include "midi_delta_time.h"
#include "midi_vlq.h"
#include <cstdint>

namespace jmid {

//
// The friend dcln's copied from class mtrk_event_t:
//
//	template <typename InIt>
//	friend InIt make_mtrk_event(InIt, InIt, std::int32_t, 
//							unsigned char, maybe_mtrk_event_t*, 
//							mtrk_event_error_t*,std::int32_t);
//
//	template <typename InIt>
//	friend InIt make_mtrk_event2(InIt, InIt, unsigned char, mtrk_event_t*, 
//							mtrk_event_error_t*);
//




struct maybe_mtrk_event_t {
	mtrk_event_t event;
	std::ptrdiff_t nbytes_read;
	mtrk_event_error_t::errc error;
	operator bool() const;
};


//
// make_mtrk_event() overloads
//
//
// template <typename InIt>
// InIt make_mtrk_event(InIt it, InIt end, unsigned char rs, 
//			maybe_mtrk_event_t *result, mtrk_event_error_t *err);
// template <typename InIt>
// maybe_mtrk_event_t make_mtrk_event(InIt it, InIt end, 
//							unsigned char rs, mtrk_event_error_t *err);
//
// 'it' points at the first byte of the delta-time field of an MTrk
// event.  *err may be null, *result must not be null.  
//
//
//template <typename InIt>
//InIt make_mtrk_event(InIt it, InIt end, int32_t dt, 
//						unsigned char rs, maybe_mtrk_event_t *result, 
//						mtrk_event_error_t *err, std::int32_t max_event_nbytes);
//template <typename InIt>
//maybe_mtrk_event_t make_mtrk_event(InIt it, InIt end, int32_t dt, 
//							unsigned char rs, mtrk_event_error_t *err,
//							std::int32_t max_event_nbytes);
// 
// 'it' points one past the final byte of the delta-time field of an
// MTrk event.  *err may be null, *result may not be null.  
//
// max_event_nbytes is the maximum number of bytes that result.event is 
// allowed to occupy.  This should usually be set to some large value, say
// ~ the size of the file being read in, to allow for the possibility of
// large sysex or meta text events.  If it is made too large, however, a 
// corrupted vlq length field in a meta or sysex event could cause a massive
// and unnecessary allocation for the event's payload.  
//
//
template <typename InIt>
InIt make_mtrk_event(InIt it, InIt end, unsigned char rs, 
			maybe_mtrk_event_t *result, mtrk_event_error_t *err,
			std::int32_t max_event_nbytes) {

	int i = 0;  // Counts the number of times 'it' has been incremented
	unsigned char uc = 0;  // The last byte extracted from 'it'

	// TODO:  Need to resize the event on error

	auto set_error = [&err,&result,&rs,&i](mtrk_event_error_t::errc ec) -> void {
		result->error = ec;
		result->nbytes_read = i;
		if (err) {
			err->s = 0;
			err->rs = rs;
			err->code = ec;
		}
	};

	jmid::dt_field_interpreted dtf;
	it = jmid::read_delta_time(it,end,dtf);
	i += dtf.N;
	if (!dtf.is_valid) {
		set_error(mtrk_event_error_t::errc::invalid_delta_time);
		return it;
	}

	// result->nbytes_read,event,size, etc is untouched; if the caller 
	// passed in a ptr to an uninitialized result, these values are 
	// garbage; this is ok!
	it = make_mtrk_event(it, end, dtf.val, rs, result, err, max_event_nbytes);// max_stream_bytes-i);
	result->nbytes_read += i;
	return it;
};


// 
// maybe_mtrk_event_t *result is empty; it points one past the end of a 
// dt field.  
//
// TODO:  If exiting due to error, result->event.size() is in general >
// the number of bytes written in to it; count resize() to 
// dest-event.begin() in set_error().  
//
template <typename InIt>
InIt make_mtrk_event(InIt it, InIt end, std::int32_t dt, 
						unsigned char rs, maybe_mtrk_event_t *result, 
						mtrk_event_error_t *err, std::int32_t max_event_nbytes) {
	// Initialize 'result'
	// The largest possible event "header" is 10 bytes, corresponding to
	// a meta event w/a 4-byte delta time and a 4 byte vlq length field:
	// 10 == 4-byte dt + 0xFF + type-byte + 4-byte len vlq
	// TODO:
	//if (result->event.size() < 10) {
		// If the caller has already allocated something bigger, don't
		// force a big->small transition
		result->event.d_.resize(10);
	//}
	auto dest = result->event.d_.begin();
	//result->nbytes_read = 0;
	result->error = mtrk_event_error_t::errc::other;
	int i = 0;  // Counts the number of times 'it' has been incremented
	unsigned char uc = 0;  // The last byte extracted from 'it'
	unsigned char s = 0; 

	auto set_error = [&err,&result,&rs,&s,&i](mtrk_event_error_t::errc ec) -> void {
		result->error = ec;
		result->nbytes_read = i;
		if (err) {
			err->s = s;
			err->rs = rs;
			err->code = ec;
		}
	};

	// The delta-time field
	if (!jmid::is_valid_delta_time(dt)) {
		set_error(mtrk_event_error_t::errc::invalid_delta_time);
		return it;
	}
	dest = jmid::write_delta_time(dt,dest);
	max_event_nbytes -= jmid::delta_time_field_size(dt);

	// The status byte
	if ((it==end) || (max_event_nbytes<=0)) {
		set_error(mtrk_event_error_t::errc::no_data_following_delta_time);
		return it;
	}
	uc = static_cast<unsigned char>(*it++);  ++i;
	s = jmid::get_status_byte(uc,rs);
	*dest++ = s;
	--max_event_nbytes;

	if (jmid::is_channel_status_byte(s)) {
		auto n = jmid::channel_status_byte_n_data_bytes(s);
		if (jmid::is_data_byte(uc)) {
			*dest++ = uc;
			--max_event_nbytes;
			n-=1;
		}
		for (int j=0; j<n; ++j) {
			if ((it==end) || (max_event_nbytes<=0)) {
				set_error(mtrk_event_error_t::errc::channel_calcd_length_exceeds_input);
				return it;
			}
			uc = static_cast<unsigned char>(*it++);  ++i;
			if (!jmid::is_data_byte(uc)) {
				set_error(mtrk_event_error_t::errc::channel_invalid_data_byte);
				return it;
			}
			*dest++ = uc;
			--max_event_nbytes;
		}
		result->event.d_.resize(dest-result->event.d_.begin());
	} else if (jmid::is_sysex_or_meta_status_byte(s)) {
		// s == 0xFF || 0xF7 || 0xF0
		if (jmid::is_meta_status_byte(s)) {
			if (it==end || (max_event_nbytes<=0)) {
				set_error(mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header);
				return it;
			}
			uc = static_cast<unsigned char>(*it++);  ++i;  // The Meta-type byte
			*dest++ = uc;
			--max_event_nbytes;
		}
		if ((it==end) || (max_event_nbytes<=0)) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header);
			return it;
		}
		jmid::vlq_field_interpreted lenf;
		it = jmid::read_vlq(it,end,lenf);
		i += lenf.N;
		if ((!lenf.is_valid) || (max_event_nbytes<0)) {
			// TODO:  The (max_event_nbytes<0) test is not needed
			set_error(mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length);
			return it;
		}
		//auto len = inl_read_vlq();
		//if (uc & 0x80u) {
		//	set_error(mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length);
		//	return it;
		//}
		dest = jmid::write_vlq(static_cast<uint32_t>(lenf.val),dest);
		max_event_nbytes -= lenf.N;
		if (lenf.val > max_event_nbytes) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input);
			return it;
		}
		auto n_written = (dest-result->event.d_.begin());
		result->event.d_.resize(n_written+lenf.val);
		//std::int32_t expect_size = n_written+lenf.val;
		//result->event.d_.resize(std::min(expect_size,static_cast<std::int32_t>(max_stream_bytes+jmid::delta_time_field_size(dt))));
		dest = result->event.d_.begin()+n_written;
		for (int j=0; j<lenf.val; ++j) {
			if ((it==end) || (max_event_nbytes<=0)) {
				// TODO:  I should resize result->event to (dest-result->event.d_.begin())
				set_error(mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input);
				return it;
			}
			uc = static_cast<unsigned char>(*it++);  ++i;
			*dest++ = uc;
			--max_event_nbytes;
		}
	} else if (jmid::is_unrecognized_status_byte(s) || !jmid::is_status_byte(s)) {
		set_error(mtrk_event_error_t::errc::invalid_status_byte);
		return it;
	}

	result->event.d_.resize(dest - result->event.d_.begin());
	result->nbytes_read = i;
	result->error = mtrk_event_error_t::errc::no_error;
	return it;
};


// 
// it points at the first byte of a dt field.  
//
template <typename InIt>
maybe_mtrk_event_t make_mtrk_event(InIt it, InIt end, unsigned char rs, 
					mtrk_event_error_t *err, std::int32_t max_stream_bytes) {
	maybe_mtrk_event_t result;
	it = make_mtrk_event(it,end,rs,&result,err,max_stream_bytes);
	return result;
};
// 
// it points one past the end of a dt field.  
//
template <typename InIt>
maybe_mtrk_event_t make_mtrk_event(InIt it, InIt end, std::int32_t dt, 
							unsigned char rs, mtrk_event_error_t *err,
							std::int32_t max_stream_bytes) {
	maybe_mtrk_event_t result;
	it = make_mtrk_event(it,end,dt,rs,&result,err,max_stream_bytes);
	return result;
};












// 
// Overwrites the mtrk_event_t at result w/ the event read in from 
// [it,end).  
// If exiting due to error result.size() == 0.  
//
template <typename InIt>
InIt make_mtrk_event2(InIt it, InIt end, unsigned char rs, 
					mtrk_event_t *result, mtrk_event_error_t *err) {
	auto set_error = [&err](mtrk_event_error_t::errc ec, unsigned char s, 
							unsigned char rs) -> void {
		if (err!=nullptr) {
			err->code = ec;
			err->s = s;
			err->rs = rs;
		}
	};
	// Initialize 'result' and the error state
	// The largest possible event "header" is 10 bytes, corresponding to
	// a meta event w/a 4-byte delta time and a 4 byte vlq length field:
	// 10 == 4-byte dt + 0xFF + type-byte + 4-byte len vlq.  
	// TODO:  resize() should not cause a big->small transition; the caller
	// may have preallocated a 'big' event.  
	set_error(mtrk_event_error_t::errc::no_error,0,rs);
	// NB:  set_error() calls result->d_.resize(0), hence, calling set_error 
	// before resizing to 10 bytes.  
	result->d_.resize(10);
	auto dest = result->d_.begin();

	// The delta-time field
	jmid::dt_field_interpreted dtf;
	it = jmid::read_delta_time(it,end,dtf);
	if (!dtf.is_valid) {
		result->d_.resize(0);
		set_error(mtrk_event_error_t::errc::invalid_delta_time,0,rs);
		return it;
	}
	dest = jmid::write_delta_time(dtf.val,dest);

	// The status byte
	if (it==end) {
		result->d_.resize(0);
		set_error(mtrk_event_error_t::errc::no_data_following_delta_time,0,rs);
		return it;
	}
	unsigned char last = static_cast<unsigned char>(*it++);
	auto s = jmid::get_status_byte(last,rs);
	*dest++ = s;

	if (jmid::is_channel_status_byte(s)) {
		auto n = jmid::channel_status_byte_n_data_bytes(s);
		if (jmid::is_data_byte(last)) {
			*dest++ = last;
			--n;
		}
		for (int j=0; j<n; ++j) {
			if (it==end) {
				result->d_.resize(0);
				set_error(mtrk_event_error_t::errc::channel_calcd_length_exceeds_input,s,rs);
				return it;
			}
			last = static_cast<unsigned char>(*it++);
			if (!jmid::is_data_byte(last)) {
				result->d_.resize(0);
				set_error(mtrk_event_error_t::errc::channel_invalid_data_byte,s,rs);
				return it;
			}
			*dest++ = last;
		}
	} else if (jmid::is_sysex_or_meta_status_byte(s)) {
		// s == 0xFF || 0xF7 || 0xF0
		if (jmid::is_meta_status_byte(s)) {
			if (it==end) {
				result->d_.resize(0);
				set_error(mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header,s,rs);
				return it;
			}
			last = static_cast<unsigned char>(*it++);  // The Meta-type byte
			*dest++ = last;
		}

		// The vlq length field
		if (it==end) {
			result->d_.resize(0);
			set_error(mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header,s,rs);
			return it;
		}
		jmid::vlq_field_interpreted lenf;
		it = jmid::read_vlq(it,end,lenf);
		if (!lenf.is_valid) {
			result->d_.resize(0);
			set_error(mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length,s,rs);
			return it;
		}
		dest = jmid::write_vlq(static_cast<uint32_t>(lenf.val),dest);
		for (int j=0; j<lenf.val; ++j) {
			if (it==end) {
				result->d_.resize(0);
				set_error(mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input,s,rs);
				return it;
			}
			if (dest == result->d_.end()) {
				auto n_bytes_written = dest - result->d_.begin();
				// TODO:  Magic number 512
				result->d_.resize(n_bytes_written + std::min(lenf.val,512));
				dest = result->d_.begin() + n_bytes_written;
			}
			last = static_cast<unsigned char>(*it++);
			*dest++ = last;
		}
	} else if (jmid::is_unrecognized_status_byte(s) 
				|| !jmid::is_status_byte(s)) {
		result->d_.resize(0);
		set_error(mtrk_event_error_t::errc::invalid_status_byte,s,rs);
		return it;
	}

	result->d_.resize(dest - result->d_.begin());
	return it;
};

// 
// If exiting due to error result.size() == 0.  
//
template <typename InIt>
mtrk_event_t make_mtrk_event2(InIt it, InIt end, unsigned char rs, 
					mtrk_event_error_t *err) {
	mtrk_event_t result;
	it = make_mtrk_event2(it, end, rs, &result, err);
	return result;
};



}  // namespace jmid

*/

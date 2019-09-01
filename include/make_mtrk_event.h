#pragma once
#include "mtrk_event_t.h"
#include "midi_delta_time.h"
#include "midi_vlq.h"
#include <cstdint>

namespace jmid {

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


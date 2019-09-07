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
InIt make_mtrk_event3(InIt it, InIt end, unsigned char rs, 
					mtrk_event_t *result, mtrk_event_error_t *err) {
	auto set_error = [&result,&err](mtrk_event_error_t::errc ec, 
							unsigned char s, unsigned char rs) -> void {
		result->clear();  // Sets the size to 0 bytes
		if (err!=nullptr) {
			err->code = ec;
			err->s = s;
			err->rs = rs;
		}
	};
	set_error(mtrk_event_error_t::errc::no_error,0,rs);

	// The delta-time field
	jmid::dt_field_interpreted dtf;
	it = jmid::read_delta_time(it,end,dtf);
	if (!dtf.is_valid) {
		set_error(mtrk_event_error_t::errc::invalid_delta_time,0,rs);
		return it;
	}

	// The status byte
	if (it==end) {
		set_error(mtrk_event_error_t::errc::no_data_following_delta_time,0,rs);
		return it;
	}
	unsigned char last = static_cast<unsigned char>(*it++);
	auto s = jmid::get_status_byte(last,rs);

	if (jmid::is_channel_status_byte(s)) {
		jmid::ch_event_data_t md;
		md.status_nybble = s&0xF0u;
		md.ch = s&0x0Fu;
		auto n = jmid::channel_status_byte_n_data_bytes(s);
		if (jmid::is_data_byte(last)) {  // In rs; last is data byte p1
			md.p1 = last;
			if (n==2) {
				if (it==end) {
					set_error(mtrk_event_error_t::errc::channel_calcd_length_exceeds_input,s,rs);
					return it;
				}
				md.p2 = *it++;
				if (!jmid::is_data_byte(md.p2)) {
					set_error(mtrk_event_error_t::errc::channel_invalid_data_byte,s,rs);
					return it;
				}
			}  // In rs, n==2
		} else {  // Not in rs; last was the status byte
			if (it==end) {
				set_error(mtrk_event_error_t::errc::channel_calcd_length_exceeds_input,s,rs);
				return it;
			}
			md.p1 = *it++;
			if (!jmid::is_data_byte(md.p1)) {
				set_error(mtrk_event_error_t::errc::channel_invalid_data_byte,s,rs);
				return it;
			}
			if (n==2) {
				if (it==end) {
					set_error(mtrk_event_error_t::errc::channel_calcd_length_exceeds_input,s,rs);
					return it;
				}
				md.p2 = *it++;
				if (!jmid::is_data_byte(md.p2)) {
					set_error(mtrk_event_error_t::errc::channel_invalid_data_byte,s,rs);
					return it;
				}
			}  // Not in rs, n==2
		}  // In rs? 
		result->replace_unsafe(dtf.val,md);
	} else if (jmid::is_meta_status_byte(s)) {
		// s == 0xFF || 0xF7 || 0xF0
		jmid::meta_header_data mt;
		if (it==end) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header,s,rs);
			return it;
		}
		mt.type = *it++;
		if (!jmid::is_meta_type_byte(mt.type)) {
			set_error(mtrk_event_error_t::errc::other,s,rs);
			return it;
		}
		// The vlq length field
		if (it==end) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header,s,rs);
			return it;
		}
		jmid::vlq_field_interpreted lenf;
		it = jmid::read_vlq(it,end,lenf);
		if (!lenf.is_valid) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length,s,rs);
			return it;
		}
		mt.length = std::min(lenf.val,1000000);  // 1 Mb max
		auto mt_ct = result->replace_unsafe(dtf.val,mt,it,end);
		it = mt_ct.src_last;
		if (mt_ct.n_src_bytes_written != lenf.val) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input,s,rs);
			return it;
		}
	} else if (jmid::is_sysex_status_byte(s)) {
		// s == 0xF7 || 0xF0
		jmid::sysex_header_data sx;
		sx.type = s;
		if (!jmid::is_sysex_status_byte(sx.type)) {
			set_error(mtrk_event_error_t::errc::other,s,rs);
			return it;
		}
		// The vlq length field
		if (it==end) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header,s,rs);
			return it;
		}
		jmid::vlq_field_interpreted lenf;
		it = jmid::read_vlq(it,end,lenf);
		if (!lenf.is_valid) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length,s,rs);
			return it;
		}
		sx.length = std::min(lenf.val,1000000);  // 1 Mb max
		auto sx_ct = result->replace_unsafe(dtf.val,sx,it,end);
		it = sx_ct.src_last;
		if (sx_ct.n_src_bytes_written != lenf.val) {
			set_error(mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input,s,rs);
			return it;
		}
	} else if (jmid::is_unrecognized_status_byte(s) 
				|| !jmid::is_status_byte(s)) {
		set_error(mtrk_event_error_t::errc::invalid_status_byte,s,rs);
		return it;
	}

	return it;
};


template <typename InIt>
mtrk_event_t make_mtrk_event3(InIt it, InIt end, unsigned char rs, 
					mtrk_event_error_t *err) {
	mtrk_event_t result;
	it = make_mtrk_event3(it, end, rs, &result, err);
	return result;
};


}  // namespace jmid


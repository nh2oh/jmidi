#pragma once
#include "midi_raw.h"
#include "mthd_container_t.h"
#include "mtrk_container_t.h"
#include <cstdint>
#include <array>
#include <algorithm>
#include <string>
#include <vector>
#include <filesystem>


//
// In an alternative design, i just have functions like is_midi_msg(), is_channel_voice(),
// is_note_on() functions that are either members of a midi_event_t class (as below), take
// arguments of some sort of midi_event_t class, or take an mtrk_event_container_t.  Since 
// they return bools, no assurances about the underlying data are required.  This might help
// reduce the enum class explosion.  It might be simpler than having member functions like type()
// and channel_msg_type().  
//
// function template mtrk_event_container_t.is_a(T)
// T is one of a family of structs holding static field sizes, byte patterns for status bytes,
// etc.  
//
//


class midi_event_container_t {
public:
	//midi_event_container_t(mtrk_event_container_t mtrkev, unsigned char status) 
	//	: p_(mtrkev.raw_begin()), size_(mtrkev.size()), midi_status_(status) {};
	midi_event_container_t(const mtrk_event_container_t&, unsigned char);

	unsigned char raw_status() const;
	bool status_is_running() const;

	//midi_msg_t type() const;  // channel_voice, channel_mode, sysex_common, ...
	channel_msg_type channel_msg_type() const;  // channel_msg_type::...

	int8_t channel_number() const;  // for channel_voice || channel_mode types

	int8_t note_number() const;  // for channel_msg_type::note_on || note_off || key_pressure types
	int8_t velocity() const;  // for note_on || note_off
	int8_t key_pressure() const;   // for channel_msg_type::key_pressure
	int8_t control_number() const;  // for channel_msg_type::control_change
	int8_t control_value() const;  // for channel_msg_type::control_change
	int8_t program_number();  // for channel_msg_type::program_change
	int8_t channel_pressure();  // for channel_msg_type::channel_pressure
	int16_t pitch_bend_value() const;  // for channel_msg_type::pitch_bend
private:
	//const unsigned char *p_ {};  // points at the delta_t
	//int32_t size_ {0};  // delta_t + payload
	unsigned char midi_status_ {0};
	unsigned char p1_ {0};
	unsigned char p2_ {0};
};




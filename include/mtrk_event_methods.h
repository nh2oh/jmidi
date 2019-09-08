#pragma once
#include "mtrk_event_t.h"
#include "aux_types.h"
#include "midi_time.h"
#include <string>
#include <cstdint>
#include <vector>


namespace jmid {

// Needed for the friend dcln of 
// print(const mtrk_event_t&, mtrk_sbo_print_opts).  
// See mtrk_event_methods.h,.cpp
enum class mtrk_sbo_print_opts {
	normal,
	detail,
};

std::string print_type(const jmid::mtrk_event_t&);
std::string print(const jmid::mtrk_event_t&,
			mtrk_sbo_print_opts=mtrk_sbo_print_opts::normal);

// Returns true if both events have the same byte-pattern on
// [event_begin(),end()); false otherwise.  
bool is_eq_ignore_dt(const jmid::mtrk_event_t&, const jmid::mtrk_event_t&);

//
// Meta event classification & data access.  Functions to classify and extract 
// data from mtrk_event_t's containing meta events.  
//
// These classification is_*() functions examine payload_begin() of the
// event passed in for the 0xFF and subsequent meta type byte and return
// true if the correct signature is present.  They do _not_ verify other
// important aspects of the the event, ex, if size() is large enough to
// contain the event, or, for events with fixed size specified in the MIDI
// std (ex, a timesig always has a payload of 4 bytes), whether the length
// field is correct.  This means that the get_{timesig,keysig,...}() family
// of functions have to make these checks before extracting the payload.  
// These functions are designed this way because A) it should be impossible 
// to "safely" (ie, using only library-provided factory functions and w/o 
// calling unsafe accessors to directly modify the byte array for the event)
// create or obtain an mtrk_event_t invalid in this way, and B) because in 
// practice i expect calls to is_*() functions to be far more frequent than 
// calls to get_*() functions and i want to optimize for the common case.  
//
// Meta event categories.  I include the leading 0xFF byte so that it is 
// possible to have "unknown" and "invalid" message types.  
// 
enum class meta_event_t : std::uint16_t {
	seqn = 0xFF00u,  // Sequence _number_
	text = 0xFF01u,
	copyright = 0xFF02u,
	trackname = 0xFF03u,  // AKA "sequence name"
	instname = 0xFF04u,
	lyric = 0xFF05u,
	marker = 0xFF06u,
	cuepoint = 0xFF07u,
	chprefix = 0xFF20u,
	eot = 0xFF2Fu,
	tempo = 0xFF51u,
	smpteoffset = 0xFF54u,
	timesig = 0xFF58u,
	keysig = 0xFF59u,
	seqspecific = 0xFF7Fu,

	invalid = 0x0000u,
	unknown = 0xFFFFu
};
meta_event_t classify_meta_event_impl(const std::uint16_t&);
bool meta_hastext_impl(const std::uint16_t&);
meta_event_t classify_meta_event(const jmid::mtrk_event_t&);
std::string print(const meta_event_t&);
bool is_meta(const jmid::mtrk_event_t&, const meta_event_t&);
bool is_meta(const jmid::mtrk_event_t&);
bool is_seqn(const jmid::mtrk_event_t&);
bool is_text(const jmid::mtrk_event_t&);
bool is_copyright(const jmid::mtrk_event_t&);
bool is_trackname(const jmid::mtrk_event_t&);
bool is_instname(const jmid::mtrk_event_t&);
bool is_lyric(const jmid::mtrk_event_t&);
bool is_marker(const jmid::mtrk_event_t&);
bool is_cuepoint(const jmid::mtrk_event_t&);
bool is_chprefix(const jmid::mtrk_event_t&);
bool is_eot(const jmid::mtrk_event_t&);
bool is_tempo(const jmid::mtrk_event_t&);
bool is_smpteoffset(const jmid::mtrk_event_t&);
bool is_timesig(const jmid::mtrk_event_t&);
bool is_keysig(const jmid::mtrk_event_t&);
bool is_seqspecific(const jmid::mtrk_event_t&);
// Returns true if the event is a meta_event_t w/a text payload,
// ex:  meta_event_t::text, meta_event_t::lyric, etc. 
bool meta_has_text(const jmid::mtrk_event_t&);
// Get the text of any meta event w/a text payload, ex:
// meta_event_t::text, ::copyright, ::trackname, instname::, ...
std::string meta_generic_gettext(const jmid::mtrk_event_t&);
// TODO:  std::vector<unsigned char> meta_generic_getpayload(...

// Value returned represents "usec/midi-q-nt"
// 500000 => 120 q nts/min
std::int32_t get_tempo(const jmid::mtrk_event_t&, std::int32_t=500000);
// A default {}-constructed midi_timesig_t contains the defaults as
// stipulated by the MIDI std.  
jmid::midi_timesig_t get_timesig(const jmid::mtrk_event_t&, jmid::midi_timesig_t={});
// A default {}-constructed midi_keysig_t contains defaults corresponding
// to C-major.  
jmid::midi_keysig_t get_keysig(const jmid::mtrk_event_t&, jmid::midi_keysig_t={});
// Gets the payload for a sequencer-specific meta-event into a vector of
// unsigned char.  For the first (sing e argument) overload, if the event is
// not a meta_event_t::seqspecific, an empty vector is returned.  Callers can 
// use the second overload to pass in a ref to a preallocated vector.  In 
// this latter case, if ev is a meta_event_t::seqspecific, clear() is called
// on the passed in vector and the event payload is push_back()'ed into it.  
// If the event is not a meta_event_t::seqspecific, the passed in vector is 
// returned unmodified.  
std::vector<unsigned char> get_seqspecific(const jmid::mtrk_event_t&);
std::vector<unsigned char> get_seqspecific(const jmid::mtrk_event_t&, std::vector<unsigned char>&);

//
// For all these make_* functions, parameter 1 is a delta-time.  
//
jmid::mtrk_event_t make_seqn(const std::int32_t&, const std::uint16_t&);
jmid::mtrk_event_t make_chprefix(const std::int32_t&, const std::uint8_t&);
// mtrk_event_t make_tempo(const int32_t&, const uint32_t&)
// Parameter 2 is us/quarter-note
// A meta tempo event stores the tempo as a 24-bit int, hence the max
// value is 0xFFFFFFu (==16777215); values larger are truncated to this
// max value.  
jmid::mtrk_event_t make_tempo(const std::int32_t&, const std::uint32_t&);
jmid::mtrk_event_t make_eot(const std::int32_t&);
// TODO:  Are there bounds on the values of the ts params?
jmid::mtrk_event_t make_timesig(const std::int32_t&, 
								const jmid::midi_timesig_t&);
jmid::mtrk_event_t make_instname(const std::int32_t&, const std::string&);
jmid::mtrk_event_t make_trackname(const std::int32_t&, const std::string&);
jmid::mtrk_event_t make_lyric(const std::int32_t&, const std::string&);
jmid::mtrk_event_t make_marker(const std::int32_t&, const std::string&);
jmid::mtrk_event_t make_cuepoint(const std::int32_t&, const std::string&);
jmid::mtrk_event_t make_text(const std::int32_t&, const std::string&);
jmid::mtrk_event_t make_copyright(const std::int32_t&, const std::string&);



//
// Channel event data access
//
// For non-channel events, returns the ch_event_data_t specified by arg2.  
// If arg2 is empty, returns a ch_event_data_t containing invalid values
// in the event the mtrk_event_t is a non-channel event.  
jmid::ch_event_data_t get_channel_event(const jmid::mtrk_event_t&, jmid::ch_event_data_t={});
// Copies at most the first 3 bytes of the range [payload_begin(),
// payoad_end()) into the appropriate fields of a midi_ch_data_t.  
jmid::ch_event_data_t get_channel_event_impl(const jmid::mtrk_event_t&);
//
// Channel event classification
//
// The is_*(const mtrk_event_t&) duplicate functionality in the 
// corresponding is_*(const ch_event_data_t&) functions in midi_raw.h.  
// Since mtrk_event_t's are always valid, the is_*(const mtrk_event_t&)
// is often much more effecient than its is_*(const ch_event_data_t&)
// partner.  
// is_*(const ch_event_data_t&) functions are needed for input validation
// in the make_*(const ch_event_data_t) family.  
//
// In the future, this set of functions may be expanded to things like 
// is_bank_select(), is_pan(), is_foot_controller(), etc.  These require 
// reading p1.  
//
bool is_channel(const jmid::mtrk_event_t&);
bool is_channel_voice(const jmid::mtrk_event_t&);
bool is_channel_mode(const jmid::mtrk_event_t&);
// is_note_on(), is_note_off() evaluate both the status nybble as well as
// p2.  For a status nybble of 0x9nu, p2 must be > 0 for the  event to 
// qualify as a note-on event.  Events w/a status nybble of 0x9nu and p2==0 
// are considered note-off events.  
bool is_note_on(const jmid::mtrk_event_t&);
bool is_note_off(const jmid::mtrk_event_t&);
bool is_key_pressure(const jmid::mtrk_event_t&);  // 0xAnu
bool is_control_change(const jmid::mtrk_event_t&);
bool is_program_change(const jmid::mtrk_event_t&);
bool is_channel_pressure(const jmid::mtrk_event_t&);  // 0xDnu
bool is_pitch_bend(const jmid::mtrk_event_t&);
// TODO:  Not sure if the int,int... overloads should be here?
// Should i put these in some sort of low-level implementation
// file?  Could be moved to midi_raw.h and implemented in terms of
// midi_ch_data_t.  
// is_onoff_pair(const mtrk_event_t& on, const mtrk_event_t& off)
bool is_onoff_pair(const jmid::mtrk_event_t&, const jmid::mtrk_event_t&);
// is_onoff_pair(int on_ch, int on_note, const mtrk_event_t& off)
bool is_onoff_pair(int, int, const jmid::mtrk_event_t&);
// is_onoff_pair(int on_ch, int on_note, int off_ch, int off_note)
bool is_onoff_pair(int, int, int, int);
// TODO:  More generic is_onoff_pair(), ex for pedal up/down, other
// control msgs.  

//
// Channel-event factories 
//
// "Safe" factory functions, which create mtrk_event_t objects encoding midi
// channel events with payloads corresponding to the given delta-time and 
// the event data in the ch_event_data_t argument.  These functions overwrite
// the value of the status nybble and other fields in the ch_event_data_t   
// argument as necessary so that the event that is returned always has the
// intended type (where "intent" is inferred by the particular factory 
// function and not necessarily the fields of the ch_event_data_t input).  
// For example, make_note_on() will return an mtrk_event_t w/ status nybble
// == 0x90u, even if the status_nybble of the ch_event_data_t passed in is
// == 0xA0u.  
//
jmid::mtrk_event_t make_ch_event(std::int32_t, const jmid::ch_event_data_t&);
// status nybble, channel, p1, p2
jmid::mtrk_event_t make_ch_event(std::int32_t, int, int, int, int);
// Sets the status nybble to 0x90u and p2 to be the greater of the value
// passed in or 1 (a note-on event can not have a velocity of 0).  
jmid::mtrk_event_t make_note_on(std::int32_t, jmid::ch_event_data_t);
jmid::mtrk_event_t make_note_on(std::int32_t, int, int, int);
// Makes a channel event w/ status nybble == 0x80u
jmid::mtrk_event_t make_note_off(std::int32_t, jmid::ch_event_data_t);
jmid::mtrk_event_t make_note_off(std::int32_t, int, int, int);
// Makes a channel event w/ status nybble == 0x90u (normally => note on),
// but w/a p2 of 0.  
jmid::mtrk_event_t make_note_off90(std::int32_t, jmid::ch_event_data_t);
jmid::mtrk_event_t make_key_pressure(std::int32_t, jmid::ch_event_data_t);  // 0xA0u
jmid::mtrk_event_t make_key_pressure(std::int32_t, int, int, int);
jmid::mtrk_event_t make_control_change(std::int32_t, jmid::ch_event_data_t);  // 0xB0u
jmid::mtrk_event_t make_control_change(std::int32_t, int, int, int);
jmid::mtrk_event_t make_program_change(std::int32_t, jmid::ch_event_data_t);  // 0xC0u
jmid::mtrk_event_t make_program_change(std::int32_t, int, int);
jmid::mtrk_event_t make_channel_pressure(std::int32_t, jmid::ch_event_data_t);  // 0xD0u
jmid::mtrk_event_t make_channel_pressure(std::int32_t, int, int);
jmid::mtrk_event_t make_pitch_bend(std::int32_t, jmid::ch_event_data_t);  // 0xE0u
jmid::mtrk_event_t make_pitch_bend(std::int32_t, int, int, int);
jmid::mtrk_event_t make_channel_mode(std::int32_t, jmid::ch_event_data_t);  // 0xB0u
jmid::mtrk_event_t make_channel_mode(std::int32_t, int, int, int);

// On/off event pairs
// onoff_pair_t make_onoff_pair(int32_t duration int channel, int note, 
//								int velocity_on, int velocity_off);
// Returns a pair of mtrk_event_t such that the "on" event has a delta 
// time == 0 ticks and the off event has a delta-time == duration.  
// mtrk_event_t make_matching_off(int32_t delta-time, 
//									const mtrk_event_t& note_on);
struct onoff_pair_t {
	jmid::mtrk_event_t on;
	jmid::mtrk_event_t off;
};
onoff_pair_t make_onoff_pair(std::int32_t, int, int, int, int);
jmid::mtrk_event_t make_matching_off(std::int32_t, const jmid::mtrk_event_t&);
jmid::mtrk_event_t make_matching_off90(std::int32_t, const jmid::mtrk_event_t&);

//
// Sysex event classification
//
bool is_sysex(const jmid::mtrk_event_t&);
bool is_sysex_f0(const jmid::mtrk_event_t&);
bool is_sysex_f7(const jmid::mtrk_event_t&);
// Sysex factories
jmid::mtrk_event_t make_sysex_f0(const std::int32_t&,
								const std::vector<unsigned char>&);
jmid::mtrk_event_t make_sysex_f7(const std::int32_t&,
								const std::vector<unsigned char>&);

}  // namespace jmid



#pragma once
#include "midi_raw.h"  // Declares smf_event_type
#include "mtrk_event_t.h"
#include <string>
#include <cstdint>
#include <vector>

std::string print(const mtrk_event_t&,
			mtrk_sbo_print_opts=mtrk_sbo_print_opts::normal);

// Returns true if both events have the same byte-pattern on
// [event_begin(),end()); false otherwise.  
bool is_eq_ignore_dt(const mtrk_event_t&, const mtrk_event_t&);

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
enum class meta_event_t : uint16_t {
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
meta_event_t classify_meta_event_impl(const uint16_t&);
bool meta_hastext_impl(const uint16_t&);
meta_event_t classify_meta_event(const mtrk_event_t&);
std::string print(const meta_event_t&);
bool is_meta(const mtrk_event_t&, const meta_event_t&);
bool is_meta(const mtrk_event_t&);
bool is_seqn(const mtrk_event_t&);
bool is_text(const mtrk_event_t&);
bool is_copyright(const mtrk_event_t&);
bool is_trackname(const mtrk_event_t&);
bool is_instname(const mtrk_event_t&);
bool is_lyric(const mtrk_event_t&);
bool is_marker(const mtrk_event_t&);
bool is_cuepoint(const mtrk_event_t&);
bool is_chprefix(const mtrk_event_t&);
bool is_eot(const mtrk_event_t&);
bool is_tempo(const mtrk_event_t&);
bool is_smpteoffset(const mtrk_event_t&);
bool is_timesig(const mtrk_event_t&);
bool is_keysig(const mtrk_event_t&);
bool is_seqspecific(const mtrk_event_t&);
// Returns true if the event is a meta_event_t w/a text payload,
// ex:  meta_event_t::text, meta_event_t::lyric, etc. 
bool meta_has_text(const mtrk_event_t&);
// Get the text of any meta event w/a text payload, ex:
// meta_event_t::text, ::copyright, ::trackname, instname::, ...
std::string meta_generic_gettext(const mtrk_event_t&);
// TODO:  std::vector<unsigned char> meta_generic_getpayload(...

// Value returned represents "usec/midi-q-nt"
// 500000 => 120 q nts/min
int32_t get_tempo(const mtrk_event_t&, int32_t=500000);
// A default {}-constructed midi_timesig_t contains the defaults as
// stipulated by the MIDI std.  
midi_timesig_t get_timesig(const mtrk_event_t&, midi_timesig_t={});
// A default {}-constructed midi_keysig_t contains defaults corresponding
// to C-major.  
midi_keysig_t get_keysig(const mtrk_event_t&, midi_keysig_t={});
// Gets the payload for a sequencer-specific meta-event into a vector of
// unsigned char.  For the first (sing e argument) overload, if the event is
// not a meta_event_t::seqspecific, an empty vector is returned.  Callers can 
// use the second overload to pass in a ref to a preallocated vector.  In 
// this latter case, if ev is a meta_event_t::seqspecific, clear() is called
// on the passed in vector and the event payload is push_back()'ed into it.  
// If the event is not a meta_event_t::seqspecific, the passed in vector is 
// returned unmodified.  
std::vector<unsigned char> get_seqspecific(const mtrk_event_t&);
std::vector<unsigned char> get_seqspecific(const mtrk_event_t&, std::vector<unsigned char>&);

//
// For all these make_* functions, parameter 1 is a delta-time.  
//
mtrk_event_t make_seqn(const int32_t&, const uint16_t&);
mtrk_event_t make_chprefix(const int32_t&, const uint8_t&);
// mtrk_event_t make_tempo(const int32_t&, const uint32_t&)
// Parameter 2 is us/quarter-note
// A meta tempo event stores the tempo as a 24-bit int, hence the max
// value is 0xFFFFFFu (==16777215); values larger are truncated to this
// max value.  
mtrk_event_t make_tempo(const int32_t&, const uint32_t&);
mtrk_event_t make_eot(const int32_t&);
// TODO:  Are there bounds on the values of the ts params?
mtrk_event_t make_timesig(const int32_t&, const midi_timesig_t&);
mtrk_event_t make_instname(const int32_t&, const std::string&);
mtrk_event_t make_trackname(const int32_t&, const std::string&);
mtrk_event_t make_lyric(const int32_t&, const std::string&);
mtrk_event_t make_marker(const int32_t&, const std::string&);
mtrk_event_t make_cuepoint(const int32_t&, const std::string&);
mtrk_event_t make_text(const int32_t&, const std::string&);
mtrk_event_t make_copyright(const int32_t&, const std::string&);
// Writes the delta time, 0xFF, type, a vl-length, then the string into
// the event.  If the uint8_t type byte does not correspond to a
// text-containing meta event, returns a default-constructed mtrk_event_t
// (which is a meta text event w/ payload size 0).  
mtrk_event_t make_meta_generic_text(const int32_t&, const meta_event_t&,
									const std::string&);

//
// Channel event data access
//
// For non-channel events, returns the ch_event_data_t specified by arg2.  
// If arg2 is empty, returns a ch_event_data_t containing invalid values
// in the event the mtrk_event_t is a non-channel event.  
ch_event_data_t get_channel_event(const mtrk_event_t&, ch_event_data_t={});
// Copies at most the first 3 bytes of the range [payload_begin(),
// payoad_end()) into the appropriate fields of a midi_ch_data_t.  
ch_event_data_t get_channel_event_impl(const mtrk_event_t&);
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
bool is_channel(const mtrk_event_t&);
bool is_channel_voice(const mtrk_event_t&);
bool is_channel_mode(const mtrk_event_t&);
// is_note_on(), is_note_off() evaluate both the status nybble as well as
// p2.  For a status nybble of 0x9nu, p2 must be > 0 for the  event to 
// qualify as a note-on event.  Events w/a status nybble of 0x9nu and p2==0 
// are considered note-off events.  
bool is_note_on(const mtrk_event_t&);
bool is_note_off(const mtrk_event_t&);
bool is_key_pressure(const mtrk_event_t&);  // 0xAnu
bool is_control_change(const mtrk_event_t&);
bool is_program_change(const mtrk_event_t&);
bool is_channel_pressure(const mtrk_event_t&);  // 0xDnu
bool is_pitch_bend(const mtrk_event_t&);
// TODO:  Not sure if the int,int... overloads should be here?
// Should i put these in some sort of low-level implementation
// file?  Could be moved to midi_raw.h and implemented in terms of
// midi_ch_data_t.  
// is_onoff_pair(const mtrk_event_t& on, const mtrk_event_t& off)
bool is_onoff_pair(const mtrk_event_t&, const mtrk_event_t&);
// is_onoff_pair(int on_ch, int on_note, const mtrk_event_t& off)
bool is_onoff_pair(int, int, const mtrk_event_t&);
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
mtrk_event_t make_ch_event_generic_unsafe(int32_t, const ch_event_data_t&) noexcept;
mtrk_event_t make_ch_event(int32_t, const ch_event_data_t&);
// status nybble, channel, p1, p2
mtrk_event_t make_ch_event(int32_t, int, int, int, int);
// Sets the status nybble to 0x90u and p2 to be the greater of the value
// passed in or 1 (a note-on event can not have a velocity of 0).  
mtrk_event_t make_note_on(int32_t, ch_event_data_t);
mtrk_event_t make_note_on(int32_t, int, int, int);
// Makes a channel event w/ status nybble == 0x80u
mtrk_event_t make_note_off(int32_t, ch_event_data_t);
mtrk_event_t make_note_off(int32_t, int, int, int);
// Makes a channel event w/ status nybble == 0x90u (normally => note on),
// but w/a p2 of 0.  
mtrk_event_t make_note_off90(int32_t, ch_event_data_t);
mtrk_event_t make_key_pressure(int32_t, ch_event_data_t);  // 0xA0u
mtrk_event_t make_key_pressure(int32_t, int, int, int);
mtrk_event_t make_control_change(int32_t, ch_event_data_t);  // 0xB0u
mtrk_event_t make_control_change(int32_t, int, int, int);
mtrk_event_t make_program_change(int32_t, ch_event_data_t);  // 0xC0u
mtrk_event_t make_program_change(int32_t, int, int);
mtrk_event_t make_channel_pressure(int32_t, ch_event_data_t);  // 0xD0u
mtrk_event_t make_channel_pressure(int32_t, int, int);
mtrk_event_t make_pitch_bend(int32_t, ch_event_data_t);  // 0xE0u
mtrk_event_t make_pitch_bend(int32_t, int, int, int);
mtrk_event_t make_channel_mode(int32_t, ch_event_data_t);  // 0xB0u
mtrk_event_t make_channel_mode(int32_t, int, int, int);



//
// Sysex event classification
//
bool is_sysex(const mtrk_event_t&);
bool is_sysex_f0(const mtrk_event_t&);
bool is_sysex_f7(const mtrk_event_t&);
// Sysex factories
// mtrk_event_t make_meta_sysex_generic_impl(int32_t, unsigned char, 
//					bool, const unsigned char *,
//					const unsigned char *);
mtrk_event_t make_meta_sysex_generic_impl(int32_t, unsigned char, 
					unsigned char, bool, const unsigned char *,
					const unsigned char *);
// Create a sysex event w/ type byte == 0xF0u and payload == to the contents 
// of the provided vector.  In the event returned, the final byte of the 
// payload is always 0xF7u, even if this is not the final byte of the input
// vector.  If the final byte of the vector /is/ an 0xF7u, this is 
// interpreted as the terminating F7 and an additional 0xF7u is _not_ written.  
// Ex:
// make_sysex_f0(0, {0x01u,0x02u,0x03u,0x04u})  // no terminal F7
// =>  {0x00u,  0xF0u,  0x05u,  0x01u,0x02u,0x03u,0x04u,0xF7u}
//
// make_sysex_f0(0, {0x01u,0x02u,0x03u,0x04u,0xF7u})
// =>  {0x00u,  0xF0u,  0x05u,  0x01u,0x02u,0x03u,0x04u,0xF7u}
//
// make_sysex_f0(0, {0x03u,0x04u,0xF7u,0xF7u})  // two terminal F7's
// =>  {0x00u,  0xF0u,  0x04u,  0x03u,0x04u,0xF7u,0xF7u}
//
// make_sysex_f0(0, {0x04u,0xF7u,0xF7u,0xF7u})  // three terminal F7's
// =>  {0x00u,  0xF0u,  0x04u,  0x04u,0xF7u,0xF7u,0xF7u}
mtrk_event_t make_sysex_f0(const uint32_t&, const std::vector<unsigned char>&);
mtrk_event_t make_sysex_f7(const uint32_t&, const std::vector<unsigned char>&);


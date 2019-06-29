#pragma once
#include "midi_raw.h"  // Declares smf_event_type
#include "mtrk_event_t.h"
#include <string>
#include <cstdint>
#include <vector>

std::string print(const mtrk_event_t&,
			mtrk_sbo_print_opts=mtrk_sbo_print_opts::normal);


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
uint32_t get_tempo(const mtrk_event_t&, uint32_t=500000);
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
mtrk_event_t make_seqn(const uint32_t&, const uint16_t&);
mtrk_event_t make_chprefix(const uint32_t&, const uint8_t&);
// mtrk_event_t make_tempo(const uint32_t&, const uint32_t&)
// Parameter 2 is us/quarter-note
// A meta tempo event stores the tempo as a 24-bit int, hence the max
// value is 0xFFFFFFu (==16777215); values larger are truncated to this
// max value.  
mtrk_event_t make_tempo(const uint32_t&, const uint32_t&);
mtrk_event_t make_eot(const uint32_t&);
// TODO:  Are there bounds on the values of the ts params?
mtrk_event_t make_timesig(const uint32_t&, const midi_timesig_t&);
mtrk_event_t make_instname(const uint32_t&, const std::string&);
mtrk_event_t make_trackname(const uint32_t&, const std::string&);
mtrk_event_t make_lyric(const uint32_t&, const std::string&);
mtrk_event_t make_marker(const uint32_t&, const std::string&);
mtrk_event_t make_cuepoint(const uint32_t&, const std::string&);
mtrk_event_t make_text(const uint32_t&, const std::string&);
mtrk_event_t make_copyright(const uint32_t&, const std::string&);
// Writes the delta time, 0xFF, type, a vl-length, then the string into
// the event.  If the uint8_t type byte does not correspond to a
// text-containing meta event, returns a default-constructed mtrk_event_t
// (which is a meta text event w/ payload size 0).  
mtrk_event_t make_meta_generic_text(const uint32_t&, const meta_event_t&,
									const std::string&);

//
// Channel event classification
//
// The is_*(const mtrk_event_t&) wrap the corresponding 
// is_*(const midi_ch_event_t&) functions in midi_raw.h.  This is so because
// the validation methods are ultimately needed to for the 
// make_*(const midi_ch_event_t) functions.  
//
// Note that for many events, the status byte, as well as p1 and p2 are
// needed to classify the event in a useful way.  For example, where 
// s&0xF0u==0x90u, (nominally a note-on status byte), p2 has to be > 0 
// for the event to qualify as a note-on (otherwise it is a note-off).  
// Where s&0xF0u==0xB0u, p1 distinguishes a channel_voice from a channel_mode
// event.  
// For the purpose of creating an enum class to hold the channel_event types,
// an underlying type of uint8_t, or even uint16_t may be insufficient.  
// See also the enum class for meta event types where i included the leading 
// 0xFFu to allow for ::invalid and ::unrecognized events.  One important
// difference between here and the case of the meta events, however, is that
// the meta event enum class can be static_cast to a uint16_t to obtain the
// first two bytes of the meta event; this is not possible for channel events
// because of the nature of the equality conditions, which involve shifting
// and masking s, p1, p2, in different ways for each type.  
//
// In the future, this set of functions may be expanded to things like 
// is_bank_select(), is_pan(), is_foot_controller(), etc.  These require 
// reading p1.  
//
midi_ch_event_t get_channel_event(const mtrk_event_t&, midi_ch_event_t={});
// Copies at most the first 3 bytes of the range [payload_begin(),
// payoad_end()) into the appropriate fields of a midi_ch_data_t.  Performs
// no checks on the validity or type() of the input event beforehand, and
// performs no validity checks on the result.  
midi_ch_event_t get_channel_event_impl(const mtrk_event_t&);
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
// the event data in the midi_ch_event_t argument.  These functions overwrite
// the value of the status nybble and other fields in the midi_ch_event_t   
// argument as necessary so that the event that is returned always has the
// intended type (where "intent" is inferred by the particular factory 
// function and not necessarily the fields of the midi_ch_event_t input).  
// For example, make_note_on() will return an mtrk_event_t w/ status nybble
// == 0x90u, even if the status_nybble of the midi_ch_event_t passed in is
// == 0xA0u.  
//
// Events w/ only one data byte (program_change and channel_pressure) have
// md.p2 set to 0x00u.  
//

// TODO:  This is not really needed given the "unsafe" ctor
// midi_event_t(const uint32_t& dt, const midi_ch_event_t& md)
mtrk_event_t make_ch_event_generic_unsafe(const uint32_t&, const midi_ch_event_t&);
// The make_*() functions below call normalize(midi_ch_event_t) on the input
// and thus silently convert invalid values for the data & status bytes into
// valid values.  These values may be unexpected; users should ensure they
// are passing valid data.  This is in keeping w/ the practice that the 
// factory functions can not be used to create an invalid mtrk_event_t.  
//
// Sets the status nybble to 0x90u and p2 to be the greater of the value
// passed in or 1 (a note-on event can not have a velocity of 0).  
mtrk_event_t make_note_on(const uint32_t&, midi_ch_event_t);
// Makes a channel event w/ status nybble == 0x80u
mtrk_event_t make_note_off(const uint32_t&, midi_ch_event_t);
// Makes a channel event w/ status nybble == 0x90u (normally => note on),
// but w/a p2 of 0.  
mtrk_event_t make_note_off90(const uint32_t&, midi_ch_event_t);
mtrk_event_t make_key_pressure(const uint32_t&, midi_ch_event_t);  // 0xA0u
mtrk_event_t make_control_change(const uint32_t&, midi_ch_event_t);  // 0xB0u
mtrk_event_t make_program_change(const uint32_t&, midi_ch_event_t);  // 0xC0u
mtrk_event_t make_channel_pressure(const uint32_t&, midi_ch_event_t);  // 0xD0u
mtrk_event_t make_pitch_bend(const uint32_t&, midi_ch_event_t);  // 0xE0u
mtrk_event_t make_channel_mode(const uint32_t&, midi_ch_event_t);  // 0xB0u




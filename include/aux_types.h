#pragma once
#include <string>
#include <cstdint>


//
// aux types
//

// P.134:  
// All MIDI Files should specify tempo and time signature.  If they don't,
// the time signature is assumed to be 4/4, and the tempo 120 beats per 
// minute.
struct midi_timesig_t {
	uint8_t num {4};  // "nn"
	uint8_t log2denom {2};  // denom == std::exp2(log2denom); "dd"
	uint8_t clckspclk {24};  // "Number of MIDI clocks in a metronome tick"; "cc"
	uint8_t ntd32pq {8};  // "Number of notated 32'nd notes per MIDI q nt"; "bb"
};
bool operator==(const midi_timesig_t&, const midi_timesig_t&);
bool operator!=(const midi_timesig_t&, const midi_timesig_t&);

// P.139
// The default-constructed values of si==0, mi==0 => C-major
struct midi_keysig_t {
	int8_t sf {0};  // 0=> key of C;  >0 => n sharps;  <0 => n flats
	int8_t mi {0};  // 0=>major, 1=>minor
};
uint8_t nsharps(const midi_keysig_t&);
uint8_t nflats(const midi_keysig_t&);
bool is_major(const midi_keysig_t&);
bool is_minor(const midi_keysig_t&);
// TODO:  Missing 'select channel mode'
// TODO:  These are unused
/*enum : uint8_t {
	note_off = 0x80u,
	note_on = 0x90u,
	key_pressure = 0xA0u,
	ctrl_change = 0xB0u,
	prog_change = 0xC0u,  // 1 data byte
	ch_pressure = 0xD0u,  // 1 data byte
	pitch_bend = 0xE0u
};*/
struct ch_event_data_t {
	uint8_t status_nybble {0x00u};  // most-significant nybble of the status byte
	uint8_t ch {0x00u};  // least-significant nybble of the status byte
	uint8_t p1 {0x00u};
	uint8_t p2 {0x00u};
	operator bool() const;
};
bool verify(const ch_event_data_t&);
ch_event_data_t make_midi_ch_event_data(int, int, int, int);
// "Forcefully" sets bits in the fields of the input ch_event_data_t such
// that they are valid values.  
// "to_nearest_valid_value()" ?
ch_event_data_t normalize(ch_event_data_t);
bool is_note_on(const ch_event_data_t&);
bool is_note_off(const ch_event_data_t&);
bool is_key_pressure(const ch_event_data_t&);  // 0xAnu
bool is_control_change(const ch_event_data_t&);
bool is_program_change(const ch_event_data_t&);
bool is_channel_pressure(const ch_event_data_t&);  // 0xDnu
bool is_pitch_bend(const ch_event_data_t&); 
bool is_channel_mode(const ch_event_data_t&);


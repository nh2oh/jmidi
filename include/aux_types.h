#pragma once
#include <string>
#include <cstdint>

namespace jmid {

//
// aux types
//

class delta_time {
public:
	explicit delta_time();  // == 0
	explicit delta_time(std::int32_t);
	const std::int32_t& get() const;
private:
	std::int32_t d_;
};


// 
// class meta_header & struct meta_header_data
// Holds the meta type byte directly following the 0xFFu and the vlq length
// field of a meta event.  
// Struct meta_header_data can be populated with invalid values (ex, a
// type_byte > 128, a length < 0, etc.  Invalid data can be detected via 
// meta_header_data::operator bool().  
// Class meta_header can never hold invalid data (type_byte() is always 
// < 128; length() is always >= 0 && <= 0x0FFFFFFF).  
// If a ctor or a setter is called with an out-of-range input:
// - For an invalid type byte, the value 127 is set.
// - For an invalid length, a valid value closest in value to the input
//   is set.  
//   Ex, for any < 0, the value 0 is written; for any input > 0x0FFFFFFF,
//   the value 0x0FFFFFFF is written.  
struct meta_header_data {
	std::int32_t length;  // vlq length
	std::uint8_t type;  // meta-type byte following the 0xFFu
	operator bool() const;
};
bool operator==(const meta_header_data&, const meta_header_data&);
bool operator!=(const meta_header_data&, const meta_header_data&);
class meta_header {
public:
	explicit meta_header();  // Txt event w/ len 0 ({0xFFu,0x01u,0x00u})
	explicit meta_header(std::uint8_t,std::int32_t);
	explicit meta_header(std::uint8_t,std::uint64_t);
	explicit meta_header(std::uint8_t,std::int64_t);
	explicit meta_header(meta_header_data);
	meta_header_data get() const;
	std::int32_t length() const;
	std::uint8_t type_byte() const;
	std::uint8_t set_type(std::uint8_t);
	std::int32_t set_length(std::int32_t);
private:
	meta_header_data d_;
	friend bool operator==(const meta_header&, const meta_header&);
	friend bool operator!=(const meta_header&, const meta_header&);
};
bool operator==(const meta_header&, const meta_header&);
bool operator!=(const meta_header&, const meta_header&);


// 
// class sysex_header & struct sysex_header_data
// Holds the sysex type byte (0xF7u or 0xF0u) and the vlq length field of
// a sysex event.  
// Struct sysex_header_data can be populated with invalid values (ex, a
// type_byte != 0xF7u or 0xF0u, a length < 0, etc.  Invalid data can be
// detected via sysex_header_data::operator bool().  
// Class sysex_header can never hold invalid data (type_byte() is always 
// == 0xF0u or 0xF7u; length() is always >= 0 && <= 0x0FFFFFFF).  
// If a ctor or a setter is called with an out-of-range input:
// - For an invalid type byte, the value 0xF7u is set.
// - For an invalid length, a valid value closest in value to the input
//   is set.  
//   Ex, for any < 0, the value 0 is written; for any input > 0x0FFFFFFF,
//   the value 0x0FFFFFFF is written.  
struct sysex_header_data {
	std::int32_t length;  // vlq length
	std::uint8_t type;  // 0xF0u or 0xF7u
	operator bool() const;
};
bool operator==(const sysex_header_data&, const sysex_header_data&);
bool operator!=(const sysex_header_data&, const sysex_header_data&);
class sysex_header {
public:
	explicit sysex_header();  // 0xF7u, length == 0
	explicit sysex_header(std::uint8_t,std::int32_t);
	explicit sysex_header(std::uint8_t,std::uint64_t);
	explicit sysex_header(std::uint8_t,std::int64_t);
	explicit sysex_header(sysex_header_data);
	sysex_header_data get() const;
	std::int32_t length() const;
	std::uint8_t type_byte() const;
	std::uint8_t set_type(std::uint8_t);
	std::int32_t set_length(std::int32_t);
private:
	sysex_header_data d_;
	friend bool operator==(const sysex_header&, const sysex_header&);
	friend bool operator!=(const sysex_header&, const sysex_header&);
};
bool operator==(const sysex_header&, const sysex_header&);
bool operator!=(const sysex_header&, const sysex_header&);

// P.134:  
// All MIDI Files should specify tempo and time signature.  If they don't,
// the time signature is assumed to be 4/4, and the tempo 120 beats per 
// minute.
struct midi_timesig_t {
	std::int8_t num {4};  // "nn"
	std::int8_t log2denom {2};  // denom == std::exp2(log2denom); "dd"
	std::int8_t clckspclk {24};  // "Number of MIDI clocks in a metronome tick"; "cc"
	std::int8_t ntd32pq {8};  // "Number of notated 32'nd notes per MIDI q nt"; "bb"
};
bool operator==(const midi_timesig_t&, const midi_timesig_t&);
bool operator!=(const midi_timesig_t&, const midi_timesig_t&);

// P.139
// The default-constructed values of si==0, mi==0 => C-major
struct midi_keysig_t {
	std::int8_t sf {0};  // 0=> key of C;  >0 => n sharps;  <0 => n flats
	std::int8_t mi {0};  // 0=>major, 1=>minor
};
std::int8_t nsharps(const midi_keysig_t&);
std::int8_t nflats(const midi_keysig_t&);
bool is_major(const midi_keysig_t&);
bool is_minor(const midi_keysig_t&);
struct ch_event_data_t {
	std::uint8_t status_nybble {0x00u};  // most-significant nybble of the status byte
	std::uint8_t ch {0x00u};  // least-significant nybble of the status byte
	std::uint8_t p1 {0x00u};
	std::uint8_t p2 {0x00u};
	operator bool() const;
};
class ch_event {  // Always valid
public:
	explicit ch_event(const ch_event_data_t&);
	explicit ch_event(int, int, int, int);
	ch_event_data_t get() const;
	std::uint8_t status_nybble() const;
	std::uint8_t ch() const;
	std::uint8_t p1() const;
	std::uint8_t p2() const;
private:
	ch_event_data_t d_;
};
std::string print(const ch_event_data_t&);
bool verify(const ch_event_data_t&);
// ch_event_data_t make_midi_ch_event_data(int status_nybble, 
//								int channel, int p1, int p2);
ch_event_data_t make_midi_ch_event_data(int, int, int, int);
// Clamps fields of the input ch_event_data_t to the allowed range.  
// TODO:  "to_nearest_valid_value()" ?
ch_event_data_t normalize(ch_event_data_t);
bool is_note_on(const ch_event_data_t&);
bool is_note_off(const ch_event_data_t&);
bool is_key_pressure(const ch_event_data_t&);  // 0xAnu
bool is_control_change(const ch_event_data_t&);
bool is_program_change(const ch_event_data_t&);
bool is_channel_pressure(const ch_event_data_t&);  // 0xDnu
bool is_pitch_bend(const ch_event_data_t&); 
bool is_channel_mode(const ch_event_data_t&);


}  // namespace jmid

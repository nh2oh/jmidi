#pragma once
#include <string>
#include <cstdint>
#include <limits>
#include <array>
#include <type_traits>

// midi_time_t 
// Provides the information needed to convert midi ticks to seconds.  
// The tpq field is contained in the MThd chunk of an smf; there is no
// standardized default value for this quantity.  The value for usec/qnt
// is obtained from a meta set-tempo event; the default is 120 "bpm" 
// (see below).  
// TODO:  Replace w/ time_division_t
//
struct midi_time_t {
	// From MThd; no default specified in the std, arbitrarily choosing 48.  
	uint16_t tpq {48};
	// From a set-tempo meta msg; default => 120 usec/qnt ("bpm"):
	// 500,000 us => 500 ms => 0.5 s / qnt
	// => 2 qnt/s => 120 qnt/min => "120 bpm"
	uint32_t uspq {500000};
};
// Do not check for division by 0 for the case where midi_time_t.tpq==0
double ticks2sec(const uint32_t&, const midi_time_t&);
uint32_t sec2ticks(const double&, const midi_time_t&);

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
enum : uint8_t {
	note_off = 0x80u,
	note_on = 0x90u,
	key_pressure = 0xA0u,
	ctrl_change = 0xB0u,
	prog_change = 0xC0u,  // 1 data byte
	ch_pressure = 0xD0u,  // 1 data byte
	pitch_bend = 0xE0u
};
struct midi_ch_event_t {
	uint8_t status_nybble {0x00u};  // most-significant nybble of the status byte
	uint8_t ch {0x00u};  // least-significant nybble of the status byte
	uint8_t p1 {0x00u};
	uint8_t p2 {0x00u};
};

bool verify(const midi_ch_event_t&);
// "Forcefully" sets bits in the fields of the input midi_ch_event_t such
// that they are valid values.  
// TODO:  Change behavior to act more like "clamp" ?
midi_ch_event_t normalize(midi_ch_event_t);
bool is_note_on(const midi_ch_event_t&);
bool is_note_off(const midi_ch_event_t&);
bool is_key_pressure(const midi_ch_event_t&);  // 0xAnu
bool is_control_change(const midi_ch_event_t&);
bool is_program_change(const midi_ch_event_t&);
bool is_channel_pressure(const midi_ch_event_t&);  // 0xDnu
bool is_pitch_bend(const midi_ch_event_t&); 
bool is_channel_mode(const midi_ch_event_t&);



//
// Class time_division_t
//
// Represents a 2-byte time-division field in an MThd chunk.  It is
// impossible for a time_division_t to hold a value that is invalid for a
// time division field.  Ctors silently convert invalid inputs into valid
// values.  
//
// The "support" class smpte_t represents the pair of SMPTE fields; it
// does not enforce any invariants and is only used for as a convenient
// return type for the time_division_t::get_smpte() getter and for the
// SMPTE ctor time_division_t::time_division_t(smpte_t).  
// 
struct smpte_t {
	int32_t time_code;
	int32_t subframes;
};
class time_division_t {
public:
	enum class type : uint8_t {
		ticks_per_quarter,
		smpte
	};

	// 120 tiks-per-quarter; type() == ticks_per_quarter
	time_division_t()=default;
	// Construct a ticks-per-quarter-valued time-division object
	// The input is clamped to [1,32767] (32767==0x7FFF).  
	explicit time_division_t(int32_t);
	// This is declared deleted b/c is is ambiguous; does the user mean for
	// the uint16_t argument to be interpreted as a number of tpq, or as a
	// "raw value," as by make_time_division_from_raw(uint16_t)?
	explicit time_division_t(uint16_t)=delete;
	// Construct a SMPTE-valued time-division object
	// Allowed values for arg 1 (SMPTE-time-code) are -24, -25, -29, -30.  
	// If an invalid value is passed in, -24 is substituted.  The value for
	// arg 2 (subdivisions-per-frame) is clamped to [1,255].  
	explicit time_division_t(int32_t, int32_t);
	explicit time_division_t(smpte_t);
	
	// Getters
	// If called on an object of the wrong type, the value returned 
	// will be valid as the requested type of quantity, but will not 
	// correspond to the value held in the object.  A time_division_t 
	// /never/ holds or returns invalid data.  
	// See also the non-member T get_*(...) family of functions
	time_division_t::type get_type() const;
	uint16_t get_raw_value() const;
	smpte_t get_smpte() const;
	int32_t get_tpq() const;

	bool operator==(const time_division_t&) const;
	bool operator!=(const time_division_t&) const;
private:
	// SMPTE => Society of Motion Picture and Television Engineers 
	// 16-bit:  [[1] frames-per-second] [resolution-within-frame]
	// Number of subframes per frame ("units per frame," "ticks per frame")
	// Legal values (std p.133): "4 (MIDI time code resolution),
	// 8, 10, 80 (bit resolution), or 100"
	std::array<unsigned char,2> d_ {0x00u,0x78u};  // 120 ticks-per-quarter
};

//
// maybe_time_division_t make_*() "Factory" functions
//
// For invalid input, .is_valid == false, however, the time_division_t 
// object in .value will always contain a valid value (a consequence of the
// hard invariants enfoeced by the time_divison_t class).  Where is_valid
// == false, the object in .value obviously does not correspond to the 
// values passed to the make_*() function.  
//
struct maybe_time_division_t {
	time_division_t value {};
	bool is_valid {false};
};
// maybe_time_division_t
// make_time_division_smpte(int32_t SMPTE-time-code, int32_t subdivs);
// maybe_time_division_t
// make_time_division_tpq(int32_t ticks-per-quarter-note);
maybe_time_division_t make_time_division_smpte(smpte_t);
maybe_time_division_t make_time_division_smpte(int32_t,int32_t);
maybe_time_division_t make_time_division_tpq(int32_t);
// maybe_time_division_t
// make_time_division_from_raw(int16_t raw_val);
// raw_val is the value of the d_ array "deserialized" as a BE-encoded 
// quantity; same as returned by the time_division_t::raw_value() getter.  
maybe_time_division_t make_time_division_from_raw(uint16_t);
bool is_smpte(time_division_t);
bool is_tpq(time_division_t);
bool is_valid_time_division_raw_value(uint16_t);
//
// T get_*(time_division_t tdiv) functions
//
// These are provided alternatives to the time_division_t-member getter  
// methods T time_division_t::get_*().  The member getters always return
// a valid value, even where a getter is called on an object of incorrect 
// type (ex, if time_division_t::get_smpte() is called on a time_division_t
// with type()==time_division_t::type::ticpq_per_quarter).  These non-member 
// "getters" return a caller-provided default value if the time_division_t
// has type() not corresponding to the getter.  
// 
smpte_t get_smpte(time_division_t, smpte_t={0,0});
int32_t get_tpq(time_division_t, int32_t=0);


// double ticks2sec(const uint32_t& ticks, const time_division_t& tdiv, 
//					int32_t tempo=500000);
// For a smpte tdiv, the tempo argument is ignored, and 
// seconds = ticks/(smpte.time_code*smpte_subframes)
// For a tpq tdiv, 
// seconds = ticks*(tempo/tpq)
double ticks2sec(const uint32_t&, const time_division_t&, int32_t=500000);
// int32_t sec2ticks(const double& sec, const time_division_t& tdiv,
//						int32_t tempo=500000);
// For a smpte tdiv, the tempo argument is ignored, and 
// ticks = sec/(smpte.time_code*smpte_subframes)
// For a tpq tdiv, 
// ticks = sec*(tpq/tempo)
int32_t sec2ticks(const double&, const time_division_t&, int32_t=500000);



// 
// SMF event-types, status bytes, data bytes, and running-status
//
enum class smf_event_type : uint8_t {  // MTrk events
	channel,
	sysex_f0,
	sysex_f7,
	meta,
	// !is_status_byte() 
	invalid, 
	// is_status_byte() 
	//     && (!is_channel_status_byte() && !is_meta_or_sysex_status_byte())
	// Also, is_unrecognized_status_byte()
	// Ex, s==0xF1u
	unrecognized
};
// About "invalid":
// "invalid" is clearly not a member of the class of things that are "SMF
// events."  
// Supporting:  Because users switch behavior on functions that return a 
// value of this type, for example, while parsing raw a MIDI bytestream and
// it's convienient to include this error case.  
// Opposing:  Users doing that should use a maybe-type.  Also, the 
// mtrk_event_t container can not hold 'invalid' values, so there is an 
// inconsistency here.  
std::string print(const smf_event_type&);

// The most lightweight status-byte classifiers in the lib
smf_event_type classify_status_byte(unsigned char);
smf_event_type classify_status_byte(unsigned char,unsigned char);
// _any_ "status" byte, including sysex, meta, or channel_{voice,mode}.  
// Returns true even for things like 0xF1u that are invalid in an smf.  
// Same as !is_data_byte()
bool is_status_byte(const unsigned char);
// True for status bytes invalid in an smf, ex, 0xF1u
bool is_unrecognized_status_byte(const unsigned char);
bool is_channel_status_byte(const unsigned char);
bool is_sysex_status_byte(const unsigned char);
bool is_meta_status_byte(const unsigned char);
bool is_sysex_or_meta_status_byte(const unsigned char);
bool is_data_byte(const unsigned char);
// unsigned char get_status_byte(unsigned char s, unsigned char rs);
// The status byte applicible to an event w/ "maybe-a-status-byte" s
// and an "inherited" running status byte rs.  Where is_status_byte(s)
// => true, returns s.  Otherwise, if is_channel_status_byte(rs) =>
// true, returns rs.  Otherwise, returns 0x00u.  
unsigned char get_status_byte(unsigned char, unsigned char);
// unsigned char get_running_status_byte(unsigned char s, unsigned char rs);
// The value for running-status that an event with status byte s 
// imparts to the stream.  
// If is_channel_status_byte(s) => true, returns s.  Otherwise, if 
// (is_data_byte(s) && is_channel_status_byte(rs)) => true, returns rs.  
// Otherwise, returns 0x00u.  
unsigned char get_running_status_byte(unsigned char, unsigned char);










//
// validate_mtrk_event_dtstart()
//
// arg 1:  Pointer to the first byte of a delta-t field
// arg 2:  Running-status byte set by the prior event if part of an event 
//         stream.  
// arg 3:  The maximum number of times p can be incremented.  
//
// validate_mtrk_event_dtstart() will parse & validate the delta-t field and
// then examine the fewest number of bytes immediately following needed to
// classify the event (in the context of the running-status) and calculate 
// its size.  For channel events, the size calculation is based on  the value
// of the status byte returned by get_status_byte(s,rs) and the MIDI std 
// Table I "Summary of Status Bytes".  For meta or sysex_{f0,f7} events, the 
// <length> field is validated and parsed.  
// 
// In no case are other data bytes of the message evaluated; detailed
// validation of the event payload is left to the 
// validate_{channel,sysex,meta}_event() family of functions.  
//
// TODO:  This should really be named something like:
//    validate_mtrk_event_header(...)
//
// For any error condition whatsoever, returns smf_event_type::invalid.  
// smf_event_type::unrecognized is _never_ returned.  Events that classify
// as smf_event_type::unrecognized are reported as smf_event_type::invalid.  
enum class mtrk_event_validation_error : uint8_t {
	invalid_dt_field,
	invalid_or_unrecognized_status_byte,
	unable_to_determine_size,
	channel_event_missing_data_byte,
	sysex_or_meta_overflow_in_header,
	sysex_or_meta_invalid_length_field,
	sysex_or_meta_length_implies_overflow,
	event_size_exceeds_max,
	unknown_error,
	no_error
};
struct validate_mtrk_event_result_t {
	uint32_t delta_t {0};
	uint32_t size {0};
	unsigned char running_status {0x00u};
	smf_event_type type {smf_event_type::invalid};
	mtrk_event_validation_error error {mtrk_event_validation_error::unknown_error};
};
// ptr, running-status, max_size
validate_mtrk_event_result_t validate_mtrk_event_dtstart(const unsigned char *,
													unsigned char, uint32_t=0);
std::string print(const mtrk_event_validation_error&);


// Returns smf_event_type::invalid if the dt field is invalid or if there is
// a size-overrun error (ex, if the dt field is valid but dt.N==max_size).  
// Only the status byte is examined; in no case are the data bytes of the
// message evaluated.  A channel event w/a valid midi-status byte followed 
// by two invalid data bytes will evaluate to smf_event_type::channel_voice.  
smf_event_type classify_mtrk_event_dtstart(const unsigned char *, 
										unsigned char=0x00u, uint32_t=0);
smf_event_type classify_mtrk_event_dtstart_unsafe(const unsigned char *, 
										unsigned char=0x00u);

// The most lightweight data_size calculators in the lib.  No error 
// checking (other than will not read past max_size).  Behavior is 
// undefined if the input is otherwise invalid.  
// ptr to the first byte past the delta-time field
uint32_t channel_event_get_data_size(const unsigned char *, unsigned char);
uint32_t meta_event_get_data_size(const unsigned char *, uint32_t);
uint32_t sysex_event_get_data_size(const unsigned char *, uint32_t);
//
// ...get_size() returns a size that includes the delta-time field;
// ...get_data_size() returns a size that does _not_ include the 
// delta-time.  
//
// The non-_dtstart functions return 0 if p points at smf_event_type::invalid
// || smf_event_type::unrecognized, or
// if the size of the event would exceed max_size.  Note that 0 is always
// an invalid size for an smf event.  
//
// The _unsafe() versions perform no overflow checks, nor do they examine
// the .is_valid field returned from midi_interpret_vl_field().  Behavior
// is undefined if the input points at invalid data.  
//
uint32_t mtrk_event_get_data_size(const unsigned char*, unsigned char, uint32_t);
uint32_t mtrk_event_get_size_dtstart(const unsigned char*, unsigned char, uint32_t);
uint32_t mtrk_event_get_data_size_unsafe(const unsigned char*, unsigned char);
uint32_t mtrk_event_get_size_dtstart_unsafe(const unsigned char*, unsigned char);
uint32_t mtrk_event_get_data_size_dtstart_unsafe(const unsigned char*, unsigned char);
// Implements table I of the midi std
uint8_t channel_status_byte_n_data_bytes(unsigned char);
// If p is not a valid midi event, returns 0x80u, which is an invalid data
// byte.  For ..._p2_...(), also returns 0x80u if the event has status byte
// ~ 0xC0u || 0xD0u (single-byte midi messages).  
unsigned char mtrk_event_get_midi_p1_dtstart_unsafe(const unsigned char*, unsigned char=0x00u);
unsigned char mtrk_event_get_midi_p2_dtstart_unsafe(const unsigned char*, unsigned char=0x00u);



unsigned char mtrk_event_get_meta_type_byte_dtstart_unsafe(const unsigned char*);
//midi_vl_field_interpreted mtrk_event_get_meta_length_field_dtstart_unsafe(const unsigned char*);
uint32_t mtrk_event_get_meta_payload_offset_dtstart_undafe(const unsigned char*);


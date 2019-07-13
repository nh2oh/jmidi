#pragma once
#include "midi_vlq.h"
#include <string>
#include <cstdint>
#include <limits>


class constants {
public:
	static constexpr int32_t max_chunk_length_i32 
		= std::numeric_limits<int32_t>::max();
	static constexpr uint32_t max_chunk_length_ui32 
		= std::numeric_limits<uint32_t>::max();
private:
};


template<typename It>
struct iterator_range_t {
	It begin;
	It end;
};


// midi_time_t 
// Provides the information needed to convert midi ticks to seconds.  
// The tpq field is contained in the MThd chunk of an smf; there is no
// standardized default value for this quantity.  The value for usec/qnt
// is obtained from a meta set-tempo event; the default is 120 "bpm" 
// (see below).  
// TODO:  Rename midi_tempo_t ?
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
// Validation & processing of generic SMF chunk headers
//
// There are two types of chunks: the Header chunk, containing data
// pertaining to the entire file (only one per file), and the Track chunk
// (possibly >1 per file).  Both have a length field that is is always 4
// bytes (is not a vl-type quantity).  From p. 132: "Your programs should 
// expect alien chunks and treat them as if they weren't there."  
//
enum class chunk_type : uint8_t {
	header,  // MThd
	track,  // MTrk
	unknown,  // The std requires that unrecognized chunk types be permitted
	invalid
};
// Reads only the first 4 bytes, matches for 'MThd' or 'MTrk', and returns
// chunk_type::header, ::track, ::unknown or ::invalid as appropriate.  
// The only way to get ::invalid is for one or more of the first 4 bytes
// to be non-ASCII (<32 || >127).  
chunk_type chunk_type_from_id(const unsigned char*, const unsigned char*);


bool is_mthd_header_id(const unsigned char*, const unsigned char*);
bool is_mtrk_header_id(const unsigned char*, const unsigned char*);
bool is_valid_header_id(const unsigned char*, const unsigned char*);
bool is_valid_chunk_length(uint32_t);
// Why a default of 0?  
int32_t get_chunk_length(const unsigned char*, const unsigned char*,int32_t=0);
enum class chunk_id : uint8_t {
	mthd,  // MThd
	mtrk,  // MTrk
	unknown  // The std requires that unrecognized chunk types be permitted
};
struct chunk_header_error_t {
	enum errc : uint8_t {
		overflow,
		invalid_id,
		length_exceeds_max,
		no_error,
		other
	};
	errc code {no_error};
	std::array<unsigned char,4> id {0x00u,0x00u,0x00u,0x00u};
	uint32_t len {0u};
	int32_t offset {0};
};
struct maybe_header_t {
	int32_t length {0};
	chunk_id id {chunk_id::unknown};
	bool is_valid {false};
};
maybe_header_t read_chunk_header(const unsigned char*, const unsigned char*,
							chunk_header_error_t*);

//
// Checks for the 4-char ASCII id and the 4-byte size.  Verifies that 
// the id + size field + the reported size does not exceed the max_size 
// supplied as the second argument.  Does _not_ inspect anything past the
// end of the length field.  
//
enum class chunk_validation_error : uint8_t {
	chunk_header_size_exceeds_underlying,
	chunk_data_size_exceeds_underlying,
	// The first 4 bytes of the chunk must be ASCII chars
	invalid_type_field,  
	unknown_error,
	no_error
};
struct validate_chunk_header_result_t {
	const unsigned char *p {}; 
	// 4-byte ASCII header + 4-byte length field + reported length
	uint32_t size {0};
	// The length field following the 4-ASCII char chunk "name;" the 
	// reported size of the data section.  
	uint32_t data_size {0};
	chunk_type type {chunk_type::invalid};
	chunk_validation_error error {chunk_validation_error::unknown_error};
};
validate_chunk_header_result_t validate_chunk_header(const unsigned char*, uint32_t=0);
std::string print_error(const validate_chunk_header_result_t&);
int32_t mthd_get_ntrks(const unsigned char*, uint32_t, int32_t=-1);


//
// Validation & processing of MThd chunks
//
// <Header Chunk> = <chunk type> <length> <format> <ntrks> <division>  
//
// Checks that:
// -> p points at the start of a valid MThd chunk (ie, the 'M' of MThd...),
// -> that the size of the chunk is >=6 (p 133:  "<length> is a 32-bit
//    representation of the number 6 (high byte first)."  
//    However,
//    p 134:  "Also, more parameters may be added to the MThd chunk in the 
//    future: it is important to read and honor the length, even if it is 
//    longer than 6.")
// -> ntrks==1 if format==0
//    ntrks==0 is not allowed.  
// -> division specifies a timecode of -24 || -25 || -29 ||-30 if it is a
//    SMPTE-type field.  
// 
// Although the std only defines format's 0,1,2, a format field w/ any other
// value is not considerded an error.  From p.143:  "We may decide to define
// other format IDs to support other structures. A program encountering an 
// unknown format ID may still read other MTrk chunks it finds from the file,
// as format 1 or 2, if its user can make sense..."
//
enum class mthd_validation_error : uint8_t {
	invalid_chunk,
	non_header_chunk,
	data_length_invalid,
	zero_tracks,
	inconsistent_ntrks_format_zero,
	invalid_time_division_field,
	unknown_error,
	no_error
};
struct validate_mthd_chunk_result_t {
	const unsigned char *p {};  // points at the 'M' of "MThd"...
	uint32_t size {0};  // Always == reported size (data_length) + 8
	mthd_validation_error error {mthd_validation_error::unknown_error};
};
validate_mthd_chunk_result_t validate_mthd_chunk(const unsigned char*, uint32_t=0);
std::string print_error(const validate_mthd_chunk_result_t&);



//
// Why do i include "invalid," which is clearly not a member of the "class
// of things-that-are-smf-events"?  Because users switch behavior on 
// functions that return a value of this type (ex, while iterating through
// an mtrk chunk, the type of event at the present position in the chunk
// is detected by validate_mtrk_event_dtstart()).  I want to force users to 
// deal with the error case rather than relying on the convention that some
// kind of validate_mtrk_event_result.is_valid field be checked before 
// moving forward.  
//
enum class smf_event_type : uint8_t {  // MTrk events
	channel,
	sysex_f0,
	sysex_f7,
	meta,
	invalid,  // !is_status_byte() 
	// is_status_byte() 
	//     && (!is_channel_status_byte() && !is_meta_or_sysex_status_byte())
	// Also, is_unrecognized_status_byte()
	// Ex, s==0xF1u
	unrecognized
};
std::string print(const smf_event_type&);
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
//
// get_status_byte(s,rs)
// The status byte applicible to the present event.  For meta && sysex
// events, will return 0xFFu, 0xF0u, 0xF7u as appropriate.  Returns
// 0x00u where no status byte can be determined for the event, which 
// occurs if !is_status_byte(s) && !is_channel_status_byte(rs).  
unsigned char get_status_byte(unsigned char, unsigned char);
// get_running_status_byte(s,rs)
// The status byte that the present event imparts to the stream.  In
// general, this is _not_ the status byte applicable to the event w/
// status byte == s: for meta or sysex events, will return 0x00u instead
// of 0xFFu, 0xF0u, 0xF7u, since these events reset the running-status.  
// Also returns 0x00u where the event-local status byte s is valid but 
// "unrecognized," ex, s==0xF1u.  Only for channel events is the byte
// applicable to the event the same as the running-status imparted to the
// stream.  
// Hence, always returns either a valid channel status byte, or returns 
// 0x00u.  If 0x00u, either:
// 1) s indicates a sysex_f0/f7, meta or valid but unrecognized status byte.  
// or,
// 2) s is a midi _data_ byte (!(s&0x80u)), and !is_channel_status_byte(rs).  
unsigned char get_running_status_byte(unsigned char, unsigned char);
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
// The non-_dtstart functions returns 0 if p points at smf_event_type::invalid
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
midi_vl_field_interpreted mtrk_event_get_meta_length_field_dtstart_unsafe(const unsigned char*);
uint32_t mtrk_event_get_meta_payload_offset_dtstart_undafe(const unsigned char*);
struct parse_meta_event_unsafe_result_t {
	uint32_t dt;
	uint32_t length;  // reported size of the payload field
	unsigned char type;
	// if *p is the first byte of the delta-time, p+=payload_offset is the
	// first byte past the length field
	uint8_t payload_offset; 
};
parse_meta_event_unsafe_result_t mtrk_event_parse_meta_dtstart_unsafe(const unsigned char*);


struct parse_meta_event_result_t {
	bool is_valid {false};
	midi_vl_field_interpreted delta_t {};
	midi_vl_field_interpreted length {};
	unsigned char type {0};
	int32_t size {0};
	int32_t data_length {};  // Everything not delta_time
};
parse_meta_event_result_t parse_meta_event(const unsigned char*,int32_t=0);
struct parse_sysex_event_result_t {
	bool is_valid {false};
	midi_vl_field_interpreted delta_t {};
	midi_vl_field_interpreted length {};
	uint8_t type {0};  // F0 or F7
	int32_t size {0};
	int32_t data_length {};  // Everything not delta_time
	bool has_terminating_f7 {false};
};
parse_sysex_event_result_t parse_sysex_event(const unsigned char*,int32_t=0);


// It is assumed that the buffer is large enough to accomodate the new dt
// value.  Returns a ptr to one past the end of the new dt value (ie, the
// first msg data byte).  
unsigned char *midi_rewrite_dt_field_unsafe(uint32_t, unsigned char*, unsigned char=0);


#pragma once
#include "midi_vlq.h"
#include <string>
#include <vector>
#include <array>
#include <limits> // CHAR_BIT
#include <type_traits> // std::enable_if<>, is_integral<>, is_unsigned<>
#include <cstdint>


//
// TODO:  I have to include midi_vlq.h to get a dfn of 
// struct midi_vl_field_interpreted, which is a data member of structs
// declared below.  Do i really need this as a data member?
//
// TODO:  validate_, parse_, detect_ naming inconsistency
// TODO:  The error handling here is complete ass
//

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
chunk_type chunk_type_from_id(const unsigned char*);
//
// Checks for the 4-char ASCII id and the 4-byte size.  Verifies that 
// the id + size field + the reported size does not exceed the max_size 
// supplied as the second argument.  Does _not_ inspect anything past the
// end of the length field.  
//
enum class chunk_validation_error : uint8_t {
	size_exceeds_underlying,
	// The first 4 bytes of the chunk must be ASCII chars
	invalid_type_field,  
	unknown_error,
	no_error
};
struct validate_chunk_header_result_t {
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
//
enum class mthd_validation_error : uint8_t {
	invalid_chunk,
	non_header_chunk,
	data_length_invalid,
	zero_tracks,
	inconsistent_ntrks_format_zero,
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
// Validation & processing of MTrk chunks
//
// validate_mtrk_chunk(const unsigned char*, uint32_t=0) checks that p
// points to the start of a valid MTrk chunk (ie, the 'M' of MTrk...),
// then iterates through the all events in the track and verifies that
// each begins with a valid delta-time vlq and can be classified as one
// of channel_voice, channel_mode, meta, or sysex f0/f7 (any event 
// classifying as smf_event_type::invalid returns w/ 
// error==mtrk_validation_error::event_error.  A return value with
// error==mtrk_validation_error::no_error ensures that an mtrk_event_t 
// can be constructed from all the events in the track.  
//
// If the input is a valid MTrk chunk, the validate_mtrk_chunk_result_t
// returned can be passed to the ctor of mtrk_container_t to instantiate
// a valid object.  
//
// All the rules of the midi standard as regards sequences of MTrk events
// are validated here for the case of the single track indicated by the 
// input.  Ex, that each midi event has a status byte (either as part of 
// the event or implictly through running status), etc.  
//
enum class mtrk_validation_error : uint8_t {
	invalid_chunk,  // No 4-char ASCII header, reported size exceeds max-size, ...
	non_track_chunk,  // *p != "MTrk"
	data_length_not_match,  // Reported length does not match length of track
	event_error,  // some sort of error w/an internal event
	delta_time_error,
	no_end_of_track,
	unknown_error,
	no_error
};
struct validate_mtrk_chunk_result_t {
	// points at the 'M' of "MTrk"...
	const unsigned char *p {nullptr};  
	// Always == reported size (data_length) + 8
	uint32_t size {0};  
	// The reported length; !including the "MTrk" and length fields
	// Thus, always == size-8.  
	uint32_t data_size {0};  
	mtrk_validation_error error {mtrk_validation_error::unknown_error};
};
// ptr, max_size
validate_mtrk_chunk_result_t validate_mtrk_chunk(const unsigned char*, uint32_t=0);
std::string print_error(const validate_mtrk_chunk_result_t&);


//
// There are no sys_realtime messages in an mtrk event stream.  The set of 
// valid sys_realtime status bytes includes 0xFF, the identifier byte for a 
// "meta" event.  
//
// Why do i include "invalid," which is clearly not a member of the "class
// " of things-that-are-smf-events"?  Because users switch behavior on 
// functions that return a value of this type (ex, while iterating through
// an mtrk chunk, the type of event at the present position in the chunk
// is detected by parse_mtrk_event()).  I want to force users to deal with 
// the error case rather than relying on the convention that some kind of
// validate_mtrk_event_result.is_valid field be checked before moving forward 
// with validate_mtrk_event_result.detected_type.  
//
// Sysex events and meta-events cancel any running status which was in 
// effect.  Running status does not apply to and may not be used for these
// messages (p.136).  
//
enum class smf_event_type : uint8_t {  // MTrk events
	channel_voice,
	channel_mode,
	sysex_f0,
	sysex_f7,
	meta,
	invalid
};
struct validate_mtrk_event_result_t {
	uint32_t delta_t {0};
	uint32_t size {0};
	smf_event_type type {smf_event_type::invalid};
};
// ptr, running-status, max_size
validate_mtrk_event_result_t validate_mtrk_event_dtstart(const unsigned char *,
													unsigned char, uint32_t=0);
std::string print(const smf_event_type&);
//
// arg 1:  Pointer to the first byte of a delta-t field
// arg 2:  MIDI status byte to be applied to the present event if, for the first byte
//         following the delta-t field, !(*p&0x80) (ie, if the first byte following the
//         delta-t field is not an 0xFF,0xF0,0xF7 or a midi status byte).  
// arg 3:  The maximum number of times p can be incremented.  
//
// parse_mtrk_event_type() will parse & validate the delta-t field and evaluate the status
// byte immediately following.  If the event is a MIDI event, either by a status byte
// local to the event or by virtue of the running-status byte (arg 2), the event size 
// calculation is based on the status of this byte; data bytes beyond the status byte are
// not evaluated.  If the event is a meta or sysex-f0/f7 type, the <length> field following
// the byte following the delta-t field is parsed and validated; bytes subsequent to the 
// <length> field are _not_ evaluated.  
//
// Will return smf_event_type::invalid under the following circumstances:
// -> the delta_t field is invalid (ex: > 4 bytes) or > max_size
// -> the size of the delta_t field + the calculated num-data/status-bytes > max_size
// -> the byte immediately following the delta_t field is not a status byte AND (s is not
//    a status byte, or is a disallowed running_status status byte such as FF, F0, F7).  
// -> an 0xB0 status byte is not followed by a valid data byte
// Note that except in the one limited case above, the data bytes are not validated as plausible
// data bytes.  This is up to the relevant parse_() function.  
//
//
// TODO:  Rename to "detect_...()" ?
//





//
// Pointer to the first data byte following the delta-time, _not_ to the start of the delta-time.  
// This is the most lightweight status-byte classifier that i have in this lib.  The pointer may
// have to be incremented by 1 byte to classify an 0xB0 status byte as channel_voice or 
// channel_mode.  In this case, will return smf_event_type::invalid if said data byte is an  
// invalid data byte (has its high bit set).  
//
// For meta and sysex_f0 messages (status byte == 0xFF, 0xF0 or 0xF7) the length field subsequent to
// the status byte is not validated.  It *only* classifies the status byte (and if neessary to make
// the classification, reads the first data byte).  
//
smf_event_type detect_mtrk_event_type_unsafe(const unsigned char*, unsigned char=0);
smf_event_type detect_mtrk_event_type_dtstart_unsafe(const unsigned char*, unsigned char=0);
//
// args:  ptr to first byte _following_ the dt, running status
// If *arg1 is a midi status byte, 
//   returns *arg1; it does not matter what arg2 is.  
// If *arg1 is _not_ a midi status byte and == 0xF0u||0xF7u||0xFFu,
//   returns 0x00u; it does not matter what arg2 is.  
// if *arg1 is _not_ a midi status byte and not 0xF0u||0xF7u||0xFFu,
//   returns arg2 iff arg2 is a midi status byte.  
//   returns 0x00u otherwise.  
// Hence, always returns either a valid midi status byte applicable to the
// event indicated by p, or returns 0x00u.  If 0x00u, either
//   1) p indicates a sysex_f0/f7 or meta event (=> *p==0xF0u||0xF7u||0xFFu)
//   or,
//   2) p indicates something that looks like a midi _data_ byte, but s is
//     invalid.  
//
unsigned char mtrk_event_get_midi_status_byte_dtstart_unsafe(const unsigned char*, unsigned char=0x00u);
unsigned char mtrk_event_get_midi_status_byte_unsafe(const unsigned char*, unsigned char=0x00u);
uint32_t mtrk_event_get_size_dtstart_unsafe(const unsigned char*, unsigned char=0x00u);
// Does not include the dlta-t field
uint32_t mtrk_event_get_data_size_dtstart_unsafe(const unsigned char*, unsigned char=0x00u);
// Does not include the delta-t field, ptr points at the first
// byte past the end of the dt field
uint32_t mtrk_event_get_data_size_unsafe(const unsigned char*, unsigned char=0x00u);
// If p is not a valid midi event, returns 0x80u, which is an invalid data
// byte.  For ..._p2_...(), also returns 0x80u if the event has status byte
// ~ 0xC0u || 0xD0u (single-byte midi messages).  
unsigned char mtrk_event_get_midi_p1_dtstart_unsafe(const unsigned char*, unsigned char=0x00u);
unsigned char mtrk_event_get_midi_p2_dtstart_unsafe(const unsigned char*, unsigned char=0x00u);
// Does not consider 0xFnu to be valid
bool is_midi_status_byte(const unsigned char);
bool is_midi_data_byte(const unsigned char);

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
enum class channel_msg_type : uint8_t {
	note_on,
	note_off,
	key_pressure,
	control_change,
	program_change,
	channel_pressure,
	pitch_bend,
	channel_mode,
	invalid
};
channel_msg_type mtrk_event_get_ch_msg_type_dtstart_unsafe(const unsigned char*, unsigned char=0x00u);
// NB:  validate_mtrk_chunk() keeps track of the most recent status byte by assigning from the 
// status_byte field of this struct.  Change this before adopting any sort of sign convention for
// implied running_status.  
struct parse_channel_event_result_t {
	bool is_valid {false};
	channel_msg_type type {channel_msg_type::invalid};
	midi_vl_field_interpreted delta_t {};
	bool has_status_byte {false};
	unsigned char status_byte {0};
	uint8_t n_data_bytes {0};  // 0, 1, 2
	int32_t size {0};
	int32_t data_length {0};  // Everything not delta_time
};
parse_channel_event_result_t parse_channel_event(const unsigned char*, unsigned char=0, int32_t=0);
bool midi_event_has_status_byte(const unsigned char*);
// TODO:  Deprecate
unsigned char midi_event_get_status_byte(const unsigned char*);  // dtstart
// For the midi _channel_ event implied by the status byte arg 1 (or, if arg 1
// is not a status byte, the status byte arg 2), returns the number of expected
// data bytes + 1 if the first arg is a valid status byte; + 0 if arg 1 is not
// a valid status byte but arg 2 is a valid status byte.  Returns 0 if neither
// arg 1, arg 2 are valid status bytes.
int midi_channel_event_n_bytes(unsigned char, unsigned char); 
// Result is only valid for channel_voice or channel_mode status bytes:  Does not 
// verify that the input is a legit channel_voice or _mode status byte.  
int8_t channel_number_from_status_byte_unsafe(unsigned char);
// arg 1 => status byte, arg 2 => first data byte, needed iff arg1 & 0xF0 == 0xB0.  
// Not really "_unsafe"-worthy since will return channel_msg_type::invalid if the
// status byte is not legit.  
channel_msg_type channel_msg_type_from_status_byte(unsigned char, unsigned char=0);

// It is assumed that the buffer is large enough to accomodate the new dt
// value.  Returns a ptr to one past the end of the new dt value (ie, the
// first msg data byte).  
unsigned char *midi_rewrite_dt_field_unsafe(uint32_t, unsigned char*, unsigned char=0);


//
// My smf_t, smf_container_t types do not index into each mtrk event, hence 
// the mtrk event list's do not need to be parsed to construct these objects.  
// The only important point is that the data be valid.  
//
struct chunk_idx_t {
	chunk_type type {};
	int32_t offset {0};
	uint32_t size {0};
};
struct validate_smf_result_t {
	bool is_valid {};
	std::string msg {};

	// Needed by the smf_container_t ctor
	std::string fname {};
	const unsigned char *p {};
	uint32_t size {0};
	int32_t n_mtrk {0};  // Number of MTrk chunks
	int32_t n_unknown {0};
	std::vector<chunk_idx_t> chunk_idxs {};
};
validate_smf_result_t validate_smf(const unsigned char*, int32_t, const std::string&);


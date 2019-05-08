#pragma once
#include <string>
#include <vector>
#include <array>
#include <limits> // CHAR_BIT
#include <type_traits> // std::enable_if<>, is_integral<>, is_unsigned<>
#include <cstdint>





//
// TODO:  validate_, parse_, detect_ naming inconsistency
//

// 
// The max size of a vl field is 4 bytes, and the largest value it may encode is
// 0x0FFFFFFF (BE-encoded as: FF FF FF 7F) => 268,435,455, which fits safely in
// an int32_t:  std::numeric_limits<int32_t>::max() == 2,147,483,647;
//
//
struct midi_vl_field_interpreted {
	int32_t val {0};
	int8_t N {0};
	bool is_valid {false};
};
midi_vl_field_interpreted midi_interpret_vl_field(const unsigned char*);


//
// Returns an integer with the bit representation of the input encoded as a midi
// vl quantity.  Ex for:
// input 0b1111'1111 == 0xFF == 255 => 0b1000'0001''0111'1111 == 0x817F == 33151
// The largest value a midi vl field can accommodate is 0x0FFFFFFF == 268,435,455
// (note that the MSB of the max value is 0, not 7, corresponding to 4 leading 
// 0's in the MSB of the binary representation).  
//
// The midi-vl representation of a given int is not necessarily unique; above,
// 0xFF == 255 => 0x81'7F == 33,151
//             => 0x80'81'7F == 8,421,759
//             => 0x80'80'81'7F == 2,155,905,407
//
template<typename T>
constexpr uint32_t midi_vl_field_equiv_value(T val) {
	static_assert(CHAR_BIT == 8);
	static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value);
	if (val > 0x0FFFFFFF) {
		return 0;
	}
	uint32_t res {0};

	for (int i=0; val>0; ++i) {
		if (i > 0) {
			res |= (0x80<<(i*8));
		}
		res += (val & 0x7Fu)<<(i*8);
		// The u is needed on the 0x7F; otherwise (val & 0x7F) may be cast to a signed
		// type (ex, if T == uint16_t), for which left-shift is implementation-defined.  
		val>>=7;
	}

	return res;
}


//
// Computes the size (in bytes) of the field required to encode a given number as
// a vl-quantity.  
// TODO:  Is shifting a signed value UB/ID ???
//
template<typename T>
constexpr int midi_vl_field_size(T val) {
	static_assert(std::is_integral<T>::value,"MIDI VL fields only encode integral values");
	int n {0};
	do {
		val >>= 7;
		++n;
	} while (val != 0);

	return n;
}


//
// TODO: untested
// TODO:  Need an overload so it's possible to use std::back_inseter(some_vector)
//
template<typename It, typename T>
It midi_write_vl_field(It beg, It end, T val) {
	static_assert(CHAR_BIT == 8);
	
	auto vlval = midi_vl_field_equiv_value(val);
	
	uint32_t mask=0xFF000000;
	while (vlval&mask==0 && mask>0) {
		mask>>=8;
	}

	while (mask>0 && beg!=end) {
		*beg = vlval&mask;
		mask>>=8;
		++beg;
	}

	return beg;
}


//
// Encodes T in the form of a VL quantity, the maximum size of which, according
// to the MIDI std is 4 bytes.  For values requiring less than 4 bytes in 
// encoded form, the rightmost bytes of the array will be 0.  
//
template<typename T, typename = typename std::enable_if<std::is_integral<T>::value,T>::type>
std::array<unsigned char,4> midi_encode_vl_field(T val) {
	static_assert(sizeof(T)<=4);  // The max size of a vl field is 4 bytes
	std::array<unsigned char,4> result {0x00,0x00,0x00,0x00};

	int i = 3;
	result[i] = val & 0x7F;
	while (i>0 && (val >>= 7) > 0) {
		--i;
		result[i] |= 0x80;
		result[i] += (val & 0x7F);
	}

	// Shift the elements of result to the left so that result[0] contains 
	// the first nonzero byte and any zero bytes are at the end of result (beyond
	// the first byte w/ bit 7 == 0).  
	for (int j=0; j<4; ++j) { 
		if (i<=3) {
			result[j] = result[i];
			++i;
		} else {
			result[j] = 0x00;
		}
	}

	return result;
};


//
// TODO:  The error handling here is complete ass
//

//
// There are two types of chunks: the Header chunk, containing data pertaining to the entire file 
// (only one per file), and the Track chunk (possibly >1 per file).  Both have a length field 
// that is is always 4 bytes (is not a vl-type quantity).  From p. 132: "Your programs should 
// expect alien chunks and treat them as if they weren't there."  
//
enum class chunk_type : uint8_t {
	header,  // MThd
	track,  // MTrk
	unknown,  // The std requires that unrecognized chunk types be permitted
	invalid
};

//
// Checks for the 4-char ASCII id and the 4-byte size.  Verifies that 
// the id + size field + the reported size does not exceed the max_size 
// supplied as the second argument.  Does _not_ inspect anything past the
// end of the length field.  
//
enum class chunk_validation_error : uint8_t {
	size_exceeds_underlying,
	invalid_type_field,  // refers to the 4-char ASCII field
	unknown_error,
	no_error
};
struct detect_chunk_type_result_t {
	chunk_type type {chunk_type::invalid};
	uint32_t size {0};  // 4-byte ASCII header + 4-byte length field + reported length
	uint32_t data_length {0};  // reported length (not including the 8 byte header)
	chunk_validation_error error {chunk_validation_error::unknown_error};
};
detect_chunk_type_result_t detect_chunk_type(const unsigned char*, uint32_t=0);
// Reads only the first 4 bytes, matches for 'MThd' or 'MTrk', and returns
// chunk_type::header, ::track, or ::unknown as appropriate.  Never returns
// ::invalid.  
chunk_type detect_chunk_type_unsafe(const unsigned char*);
std::string print_error(const detect_chunk_type_result_t&);

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
// -> ntrks<=1 if format==0
//    ntrks==0 is allowed though it is debatable if a valid smf may have 0
//    tracks.  
//
enum class mthd_validation_error : uint8_t {
	invalid_chunk,
	non_header_chunk,
	data_length_invalid,
	inconsistent_ntrks_format_zero,
	unknown_error,
	no_error
};
struct validate_mthd_chunk_result_t {
	const unsigned char *p {};  // points at the 'M' of "MThd"...
	uint32_t size {0};  //  Always == reported size (data_length) + 8
	mthd_validation_error error {mthd_validation_error::unknown_error};
	bool is_valid {false};  // TODO:  Get rid of this; callers should check error
};
validate_mthd_chunk_result_t validate_mthd_chunk(const unsigned char*, uint32_t=0);
std::string print_error(const validate_mthd_chunk_result_t&);

//
// Checks that p points to the start of a valid MTrk chunk (ie, the 'M' of MTrk...),
// then moves through the all events in the track and validates that each begins with a
// valid vl delta-time, is one of midi_channel, meta, or sysex f0/f7, and that for each such event a
// valid mtrk_event_container_t object can be created if necessary (it must be possible to
// determine the size in bytes of each event).  
//
// If the input is a valid MTrk chunk, the validate_mtrk_chunk_result_t returned can be
// passed to the ctor of mtrk_container_t to instantiate a valid object.  
//
// All the rules of the midi standard as regards sequences of MTrk events are validated
// here for the case of the single track indicated by the input.  Ex, that each midi 
// event has a status byte (either as part of the event or implictly through running 
// status), etc.  
//
enum class mtrk_validation_error : uint8_t {
	invalid_chunk,  // No 4-char ASCII header, reported size exceeds max-size, ...
	non_track_chunk,  // *p != "MTrk"
	data_length_not_match,  // Reported length does not match length of track
	event_error,  // some sort of error w/an internal event
	unknown_error,
	no_error
};
struct validate_mtrk_chunk_result_t {
	mtrk_validation_error error {mtrk_validation_error::unknown_error};
	bool is_valid {false};
	std::string msg {};
	int32_t data_length {0};  // the reported length; does not include the "MTrk" and length fields
	int32_t size {0};  //  Always == reported size (data_length) + 8
	const unsigned char *p {};  // points at the 'M' of "MTrk"...
};
validate_mtrk_chunk_result_t validate_mtrk_chunk(const unsigned char*, int32_t=0);
std::string print_error(const validate_mtrk_chunk_result_t&);

//
// There are no sys_realtime messages in an mtrk event stream.  The set of valid sys_realtime 
// status bytes includes 0xFF, which is the leading byte for a "meta" event.  
//
// Why do i include "invalid," which is clearly not a member of the "class" of things-that-are-
// smf-events?  Because users switch behavior on functions that return a value of this type (ex,
// while iterating through an mtrk chunk, the type of event at the present position in the chunk
// is detected by parse_mtrk_event()).  I want to force users to deal with the error case rather
// than relying on the convention that some kind of parse_mtrk_event_result.is_valid field be 
// checked before moving forward with parse_mtrk_event_result.detected_type.  
//
// Sysex events and meta-events cancel any running status which was in effect.  Running status
// does not apply to and may not be used for these messages (p.136).  
//
enum class smf_event_type : uint8_t {  // MTrk events
	channel_voice,
	channel_mode,
	sysex_f0,
	sysex_f7,
	meta,
	invalid
};

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
struct parse_mtrk_event_result_t {
	midi_vl_field_interpreted delta_t {};
	smf_event_type type {smf_event_type::invalid};

	// Added 030919
	int32_t size {0};  // Includes the size of the delta_t field
	int32_t data_length {0};
};
parse_mtrk_event_result_t parse_mtrk_event_type(const unsigned char*, unsigned char, int32_t=0);
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


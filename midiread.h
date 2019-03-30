#pragma once
/*
#include "dbklib\binfile.h"
#include <filesystem>
#include <array>
#include <algorithm>
#include <vector>
#include <string> //midi_file::print()
#include <type_traits>


std::string print_hexascii(const unsigned char*, int, const char = ' ');

//
// There are two types of chunks: the Header chunk, containing data pertaining to the entire file 
// (only one per file), and the Track chunk (possibly >1 per file).  Both chunk types have the 
// layout implied by struct midi_chunk.  Note that the length field is always 4 bytes and is not a
// vl-type quantity.  From p. 132: "Your programs should expect alien chunks and treat them as if
// they weren't there."  
//
enum class midi_chunk_t {
	header,  // MThd
	track,  // MTrk
	unknown,
	invalid  // Does not begin w/a 4 char ASCII id & a 4-byte length
};
//midi_chunk_t detect_midi_chunk_type(const unsigned char*);


// Feb 2:  midi_chunk read_midi_chunk(const dbk::binfile&, size_t) seems to be unused,
// but the struct is the input arg to:
// midi_file_header_data read_midi_fheaderchunk(const midi_chunk&);
// used in the mthd print routine
struct midi_chunk {
	std::array<char,4> id {};
	int32_t length {0};  // redundant w/ data.size()
	std::vector<unsigned char> data {};
};
midi_chunk read_midi_chunk(const dbk::binfile&, size_t);  


//
// Checks to see if the input is pointing at the start of a valid midi chunk (either a file-header
// chunk or a Track chunk).  Both have the layout implied by struct midi_chunk (above).  
// If is_valid, the user can subsequently call the correct parser:
//    read_midi_fheaderchunk(const midi_chunk&);
//    read_midi_trackchunk(const midi_chunk&);
// Note that is_valid only indicates that the input is the start of a valid chunk; it does _not_
// testify to the validity of the data contained within the chunk.  
//
struct detect_chunk_result {
	midi_chunk_t type {midi_chunk_t::unknown};
	int32_t data_length {};
	bool is_valid {};
};
detect_chunk_result detect_chunk_type(const unsigned char*);


// Header chunk data section
// fmt:  0, 1, 2
//  -> 0 => File contains a single multi-channel track
//  -> 1 => File contains one or more simultaneous tracks
//  -> 3 =>  File contains one or more sequentially independent single-track patterns.  
// ntrks:  Always 1 for a fmt-type 0 file.  
// time_div:  
//  -> bit 15==0 => bits [14-0] == ticks-per-quarter-note
//  -> bit 15==1 => bits [14-8] == "negative SMPTE format," bits [7-0] == ticks-per-frame
struct midi_file_header_data {
	int16_t fmt_type {0};
	int16_t num_trks {0};
	int16_t time_div {0};
};
midi_file_header_data read_midi_fheaderchunk(const midi_chunk&);
enum class midi_time_division_field_type_t {
	ticks_per_quarter,
	SMPTE
};
midi_time_division_field_type_t detect_midi_time_division_type(const unsigned char*);
uint16_t interpret_tpq_field(const unsigned char*);  // assumes midi_time_division_field_type_t::ticks_per_quarter
struct midi_smpte_field {
	int8_t time_code_fmt {0};
	uint8_t units_per_frame {0};
};
midi_smpte_field interpret_smpte_field(const unsigned char*);  // assumes midi_time_division_field_type_t::SMPTE

//
// MTrk events
// There are 3 types:  sysex_event, midi_event, meta_event
//
// Each MTrk event consists of a vl delta_time quantity followed by one of the three data structures
// below.  All MTrk events begin with a delta-time field, considered in the standard to be part of 
// the definition of an MTrk event "container," but not part of the {sysex,midi,meta}-event proper; 
// From the std:
// <MTrk event> = <delta-time> <event>
// <event> = <MIDI event> | <sysex event> | <meta-event> 
//
// Nevertheless, here i store the delta-time in each event struct.  The length of each MTrk event can 
// not be determined until the event is classified as sysex, meta, or midi, and parsed.  sysex and
// meta events explictly encode their length as a vl field; midi events are 2, 3, or 4, bytes (not
// including the vl delta-time field) depending on the presence or absense of a status byte, and, if
// present, its value.
//


//
// Checks to see if the input is pointing at the start of a valid MTrk event, which is a vl length
// followed by the data corresponding to the event.  If is_valid, the user can subsequently call the 
// correct parser:
//    sysex_event parse_sysex_event(const unsigned char*);
//    meta_event parse_meta_event(const unsigned char*);
//    midi_event parse_midi_event(const unsigned char*);
// Note that is_valid only indicates that the input is the start of a valid MTrk event; it does _not_
// testify to the validity of the event data following the length field.  
//
enum class mtrk_event_t {
	midi,
	sysex,
	meta,
	unknown
};
struct detect_mtrk_event_result {
	mtrk_event_t type {mtrk_event_t::unknown};
	midi_vl_field_interpreted delta_t {};
	bool is_valid {};
};
detect_mtrk_event_result detect_mtrk_event_type(const unsigned char*);


struct meta_event {
	midi_vl_field_interpreted delta_t {};
	// FF <type> <length> <bytes>
	uint8_t id {0};  // FF
	uint8_t type {0};
	midi_vl_field_interpreted length {};
	std::vector<unsigned char> data {};
};
meta_event parse_meta_event(const unsigned char*);
std::string print_meta_event(const meta_event&);

struct sysex_event {
	midi_vl_field_interpreted delta_t {};
	// F0 (sometimes F7 ...see std) <length> <bytes to be transmitted after F0||F7>
	uint8_t id {0};  // F0 or F7
	midi_vl_field_interpreted length {};
	std::vector<unsigned char> data {};
};
sysex_event parse_sysex_event(const unsigned char*);
std::string print_sysex_event(const sysex_event&);

//
// All midi events have an associated status byte, either as part of the event itself, immediately
// following the vl delta-time field, or implied by the most recently encountered status byte
// (ie, in a midi_event preceeding the present), a state referred to as "running status."  The 
// midi_event struct below encodes the appropriate status byte explictly for each event, even for 
// midi events not containing a status byte (ie, for midi events occuring in a sequence where 
// running-status is in use and which follow some prior midi message where the status byte is set).  
// This removes the implicit state from a sequence of midi messages.  If running_status == true,
// the message got its status byte from a prior message in the stream and the present message did
// not contain an explicit status byte.  If running_status == false, the contents of the status 
// field were encoded in the present message in the actual stream.  
// Note that the default value for the status field == 0b00000000 which is invalid as a status byte
// since the high bit is 0.  
//
struct midi_event {
	// Total length is delta_t.N+1+(1||2) if !running_status; delta_t.N+0+(1||2) if running_status
	midi_vl_field_interpreted delta_t {};
	bool running_status {false};
	unsigned char status {0};
	uint8_t p1 {0};
	uint8_t p2 {0};
};
uint8_t midi_event_num_data_bytes(const midi_event&);  // Returns 1 || 2
midi_event parse_midi_event(const unsigned char*, const unsigned char);  // p-to-event, prev status byte
std::string print_midi_event(const midi_event&);

// Classifies the indicated byte as either a midi-status or midi-data byte.  For this to make
// any sense at all the input must be pointing into a MIDI event.  
// Ptr to byte; does not examine beyond ptr
enum class midi_byte : uint8_t {
	status,
	data
};
midi_byte classify_byte(const unsigned char*);
// Iff the input is pointing at MIDI status byte (as verified by classify_byte()), 
// classify_status_byte() classifies the type of midi message to which the indicated status 
// byte belongs.  Classification is made by examining the status byte only.  
enum class status_byte_type : uint8_t {
	channel,
	system,
	unknown,
	invalid  // Not a status bye (MSBit != 1)
};
status_byte_type classify_status_byte(const unsigned char*);

//
// Does not verify that the input is a status byte; assumes this.
// May have to increment ptr by 1
// 
enum class ch_msg_type : uint8_t {
	voice,
	mode,
};
ch_msg_type classify_channel_status_byte(const unsigned char*);
// Only returns valid info if the input points at a valid channel status byte; the
// function does not verify this
int channel_number(const unsigned char*);

//
// Does not verify that the input is a status byte; assumes this.
// May have to increment ptr by 1
enum class sys_msg_type : uint8_t {
	exclusive,
	common,
	realtime,
	unknown
};
sys_msg_type classify_system_status_byte(const unsigned char*);



//
// One possibility here is that this class could just hold fdata_, validated
// upon construction, and possibly the chunk_idx_ vector; the mtrk_event_idx_ vector
// is probably not needed (?).  Only function is to certify a valid midi file.  
//
class midi_file {
public:
	midi_file() = default;
	midi_file(const std::filesystem::path&);

	std::string print() const;
	std::string print_mtrk_seq() const;
	std::string print_mthd() const;
private:
	// Offsets, lengths of header and all track chunks
	struct chunk_idx {
		uint64_t offset {0};  // global file offset
		uint64_t length {0};
		midi_chunk_t type {};  // header, track, unknown
	};
	// <vl-delta_t> <event data>
	struct mtrk_event_idx {
		uint64_t offset {0};  // global file offset of start of delta_t field
		uint64_t length {0};  // Includes the vl delta_t field
		mtrk_event_t type {mtrk_event_t::unknown};  // midi, sysex, meta, unknown
		
		// If type == mtrk_event_t::midi, the applicable status byte.  
		// Alternatives:
		// 1)  The global file offset of the applicable midi status byte
		// 2)  The global file offset of the event idx containing the status byte
		unsigned char midi_status {0};
	};

	std::vector<unsigned char> fdata_ {};
	std::vector<midi_file::chunk_idx> chunk_idx_ {};  // header chunk and all track chunks
	std::vector<std::vector<midi_file::mtrk_event_idx>> mtrk_event_idx_ {};  // inner idx => track num
};

*/


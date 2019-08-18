#pragma once
#include <string>
#include <cstdint>


// 
// Status byte/data byte classification, running-status calculation
//
enum class status_byte_type : uint8_t {
	channel,
	sysex_f0,
	sysex_f7,
	meta,
	invalid,  // !is_status_byte() 
	unrecognized  // is_unrecognized_status_byte()
};
std::string print(const status_byte_type&);
status_byte_type classify_status_byte(unsigned char);
status_byte_type classify_status_byte(unsigned char,unsigned char);
// _any_ "status" byte, including sysex, meta, or channel_{voice,mode}.  
// Returns true even for things like 0xF1u that are invalid in an smf.  
// Same as !is_data_byte()
bool is_status_byte(const unsigned char);
// True for status bytes invalid in an smf, ex, 0xF1u
// == (is_status_byte() 
//     && (!is_channel_status_byte() && !is_meta_or_sysex_status_byte()))
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
// Implements table I of the midi std
int32_t channel_status_byte_n_data_bytes(unsigned char);

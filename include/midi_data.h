#pragma once
#include <array>
#include <cstdint>

namespace mdata {
struct byte_desc_t {
	const uint8_t type {};
	const std::array<char,30> d {};
};
inline const std::array<const byte_desc_t,17> meta {
	byte_desc_t {0x00,"Sequence-Number"},
	byte_desc_t {0x01,"Text-Event"},
	byte_desc_t {0x02,"Copyright Notice"},
	byte_desc_t {0x03,"Sequence/Track Name"},
	byte_desc_t {0x04,"Instrument Name"},
	byte_desc_t {0x05,"Lyric"},
	byte_desc_t {0x06,"Marker"},
	byte_desc_t {0x07,"Cue Point"},
	byte_desc_t {0x20,"MIDI Channel Prefix"},
	byte_desc_t {0x2F,"End-of-Track"},
	byte_desc_t {0x51,"Set-Tempo"},
	byte_desc_t {0x54,"SMPTE Offset"},
	byte_desc_t {0x58,"Time-Signature"},
	byte_desc_t {0x59,"Key-Signature"},
	byte_desc_t {0x7F,"Sequencer-Specific"}
};

// For midi-status bytes of type "channel-voice," mask w/ 0xF0.  See Std p. 100.  
// Note that They byte must be channel-voice, not channel-mode, since for channel-mode, 
// 0xB0 => "Select channel mode"  Ex:
// (status_type & 0xF0) ==
inline const std::array<const byte_desc_t,7> midi_status_ch_voice {
	byte_desc_t {0x80,"Note-off"},
	byte_desc_t {0x90,"Note-on"},
	byte_desc_t {0xA0,"Aftertouch/Key-pressure"},
	byte_desc_t {0xB0,"Control-change"},
	byte_desc_t {0xC0,"Program-change"},
	byte_desc_t {0xD0,"Aftertouch/Channel-pressure"},
	byte_desc_t {0xE0,"Pitch-bend-change"}
};
inline const std::array<const byte_desc_t,3> midi_status_ch_mode {
	byte_desc_t {0xB0,"Select-channel-mode"}
};

};  // namespace mdata


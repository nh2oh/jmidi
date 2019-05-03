#include "midi_maybe.h"

maybe_result_t<midi_status_byte>
stream_get_midi_status_byte_dtstart(const unsigned char *p,	unsigned char s) {
	auto dt = midi_interpret_vl_field(p);
	p += dt.N;

	if ((*p)>>7) {
		if (*p!=0xF0u && *p!=0xF7u && *p!= 0xFFu) {
			// *p is a valid midi status byte
			return *p;
		} else {
			// *p indicates a sysex_f0/f7 or meta event, which resets the running status
			return 0x00u;
		}
	}

	// p does not indicate a midi status byte, nor is it the first byte of a sysex_f0/f7
	// or meta event.  p *may* be the first data byte of a midi msg in running status.  
	if ((s>>7) && s!=0xF0u && s!=0xF7u && s!= 0xFFu) {
		return s;  // s is a valid midi status byte
	}

	return 0x00u;
}


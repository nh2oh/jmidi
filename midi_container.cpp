
#include "midi_container.h"
#include "midi_raw.h"
#include "mtrk_container_t_old.h"
#include "dbklib\binfile.h"
#include <iostream>
#include <string>
#include <vector>


midi_event_container_t::midi_event_container_t(const mtrk_event_container_t& mtrkev,
				unsigned char status) {
	auto chev_parsed = parse_channel_event(mtrkev.raw_begin(),status,mtrkev.size());
	this->midi_status_ = chev_parsed.status_byte;
	
	int8_t offset = chev_parsed.has_status_byte ? 1 : 0;

	if (chev_parsed.n_data_bytes == 0) {
		this->p1_ = 0;
		this->p2_ = 0;
	} else if (chev_parsed.n_data_bytes == 1) {
		this->p1_ = *(mtrkev.data_begin()+offset);
		this->p2_ = 0;
	} else if (chev_parsed.n_data_bytes == 2) {
		this->p1_ = *(mtrkev.data_begin()+offset);
		this->p2_ = *(mtrkev.data_begin()+offset+1);
	}
}

channel_msg_type midi_event_container_t::channel_msg_type() const {
	return channel_msg_type_from_status_byte(this->midi_status_, this->p1_);
}

// for midi_msg_t::channel_voice || channel_mode
int8_t midi_event_container_t::channel_number() const {
	return channel_number_from_status_byte_unsafe(this->midi_status_);
}

// for channel_msg_t::note_on || note_off || key_pressure
int8_t midi_event_container_t::note_number() const {
	return this->p1_;
}




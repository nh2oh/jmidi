#include "midi_utils.h"
#include "midi_raw.h"
#include "midi_container.h"
#include <string>
#include <vector>
#include <iostream>
#include <exception>


//
// Maybe a better way to do this is to let some sort of note_event iterator be ctor-ible
// from an mtrk_container_t...  End-users don't want to nec. deal with all the details of
// which types of midi events turn notes on or off...
//
//
std::string print_notelist(const mtrk_container_t& mtrk) {
	struct note_onoff_table_t {
		int8_t voice {0};
		int8_t note {0};
		int32_t t_on {0};
		int32_t t_off {0};
	};
	std::vector<note_onoff_table_t> note_table {};

	int cumtime {0};
	unsigned char curr_midi_status {0};

	for (mtrk_container_iterator_t it=mtrk.begin(); it!=mtrk.end(); ++it) {
		mtrk_event_container_t curr_event = *it;
		cumtime += curr_event.delta_time();
		auto ct = curr_event.type();
		if (curr_event.type() != smf_event_type::channel_voice 
			&& curr_event.type() != smf_event_type::channel_mode) { continue; }

		midi_event_container_t curr_midi {curr_event, it.midi_status()};
		if (curr_midi.channel_msg_type() == channel_msg_type::note_on) {
			note_onoff_table_t curr_entry {};
			curr_entry.note = curr_midi.note_number();
			curr_entry.voice = curr_midi.channel_number();
			curr_entry.t_on = cumtime;
			note_table.push_back(curr_entry);
		} else if (curr_midi.channel_msg_type() == channel_msg_type::note_off) {
			if (note_table.size() == 0) { continue; }
			for (int i=note_table.size()-1; i>=0; --i) {
				if (note_table[i].voice != curr_midi.channel_number()
					|| note_table[i].note != curr_midi.note_number()
					|| note_table[i].t_off != 0) {
					continue;  // continue searching backwards to previous table entry...
				}

				note_table[i].t_off = cumtime;
			}
		}
	}  // To next mtrk_event in the mtrk_container_t

	std::string s {};
	for (const auto& e : note_table) {
		s.append(std::to_string(e.voice)).append("\t");
		s.append(std::to_string(e.note)).append("\t");
		s.append(std::to_string(e.t_on)).append("\t");
		s.append(std::to_string(e.t_off)).append("\t");
		s.append("\n");
	}

	return s;
}










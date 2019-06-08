#include "midi_examples.h"
#include "midi_raw.h"
#include "midi_raw_test_parts.h"
#include "mthd_t.h"
#include "mtrk_event_t.h"
#include "smf_t.h";
#include "dbklib\binfile.h"
#include "dbklib\byte_manipulation.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <type_traits>
#include <algorithm>
#include <exception>
#include <fstream>
#include <optional>
#include <vector>


int midi_example() {
	//auto mt_tests = testdata::make_random_meta_tests(100);
	//testdata::print_meta_tests(mt_tests);
	//testdata::print_midi_test_cases();

	//std::string fn = "D:\\cpp\\nh2oh\\au\\gt_aulib\\test_data\\clementi_no_rs.mid";
	//std::string fn = "D:\\cpp\\nh2oh\\au\\gt_aulib\\test_data\\tc_a_rs.mid";
	//std::string fn = "C:\\Users\\ben\\Desktop\\scr\\CLEMENTI.MID";
	//std::string fn = "C:\\Users\\ben\\Desktop\\A7.mid";
	//std::string fn = "C:\\Users\\ben\\Desktop\\scr\\test.mid";
	std::string fn = "C:\\Users\\ben\\Desktop\\scr\\hallelujah_joy_to_the_world.mid";
	//std::string fn = "C:\\Users\\ben\\Desktop\\twinkle.mid";
	//std::string fn = "C:\\Users\\ben\\Desktop\\scr\\test.mid";

	auto maybesmf = read_smf(fn);
	if (!maybesmf) {
		std::cout << "nope :(" << std::endl;
		std::abort();
	}
	std::cout << "print(maybesmf.smf):" << std::endl;
	std::cout << print(maybesmf.smf) << std::endl;

	//std::cout << "get_linked_onoff_pairs(maybesmf.smf):" << std::endl;
	//auto linked_pairs = get_linked_onoff_pairs(maybesmf.smf);
	//std::cout << print(linked_pairs) << std::endl << std::endl;

	//auto ordered_evs_all = get_events_dt_ordered(maybesmf.smf);
	//std::cout << print(ordered_evs_all) << std::endl;

	for (int trkn=0; trkn<maybesmf.smf.ntrks(); ++trkn) {
		midi_time_t curr_time {};
		curr_time.tpq = interpret_tpq_field(maybesmf.smf.division());
		auto trk = maybesmf.smf.get_track(trkn);
		std::cout << "duration(maybesmf.smf.get_track("
			<< std::to_string(trkn) << ")) == "
			<< std::to_string(duration(maybesmf.smf.get_track(trkn),curr_time))
			<< std::endl;
		std::cout << "print_linked_onoff_pairs(maybesmf.smf.get_track("
			<< std::to_string(trkn) << ")):" << std::endl;
		std::cout << print_linked_onoff_pairs(trk) << std::endl;
	}
	//std::cout << "print_linked_onoff_pairs(maybesmf.smf.get_track(2)):" << std::endl;
	//std::cout << print_linked_onoff_pairs(maybesmf.smf.get_track(2)) << std::endl;

	/*tk_integrator_t tki;
	lyric_integrator_t lyri;
	time_integrator_t timei;
	std::vector<integrator_t*> vi;
	vi.push_back(&tki);
	vi.push_back(&lyri);
	vi.push_back(&timei);
	auto trk = maybesmf.smf.get_track(4);
	int i=0;
	for (const auto& e : trk) {
		for (const auto& ie : vi) {
			*ie += e;
		}
	
		if (i%5==0) {
			std::cout << print(e) << std::endl;
			for (const auto& ie : vi) {
				std::cout << "\t" << ie->print() << "\n";
			}
		}
		std::cout << std::endl;
	}*/

	return 0;
}


int raw_bytes_as_midi_file() {
	std::vector<unsigned char> tc_a_rs {
		0x4D, 0x54, 0x68, 0x64,  // MThd
		0x00, 0x00, 0x00, 0x06,
		0x00, 0x01,  // Format 1 => simultanious tracks
		0x00, 0x02,  // 2 tracks (0,1)
		0x00, 0xF0,
	
		// Track 0 ---------------------------------------------------
		0x4D, 0x54, 0x72, 0x6B,  // MTrk
		0x00, 0x00, 0x00, 0x3D,  // 0x3D==61 bytes
	
		0x00,0xFF,0x01,0x0E,  // "some inst name"
			0x53,0x6F,0x6D,0x65,0x20,0x69,0x6E,0x73,0x74,0x20,0x6E,0x61,0x6D,0x65,
	
		0x00, 0xC0, 0x06,  // Program change to program 6 => Harpsichord
		0x00, 0xB0, 0x07, 0x64,  // Ctrl change 0x07 => set channel volume
	
		// note-on events; duration of each note is == 20
		0x00, 0x90, 0x48, 0x59,  // onset dt==0; off @ 20
		0x05, 0x49, 0x59,  // onset dt==5
		0x05, 0x4A, 0x59,  // onset dt==0x0A==10
		0x05, 0x4B, 0x59,  // onset dt==15; off @ 35
		0x05, 0x4C, 0x59,  // cum dt onset == 20; off @ 35
		
		0x00, 0x80, 0x48, 0x59,  // cum dt onset == 20
		0x05, 0x49, 0x59,  // cum dt onset == 25
		0x05, 0x4A, 0x59,  // cum dt onset == 30
		0x05, 0x4B, 0x59,  // cum dt onset == 35
		0x05, 0x4C, 0x59,   // cum dt onset == 40
	
		0x00, 0xFF, 0x2F, 0x00,   // End of track
	
		// Track 1 ---------------------------------------------------
		0x4D, 0x54, 0x72, 0x6B,  // MTrk
		0x00, 0x00, 0x00, 0x41,  // 0x41 == 65 bytes
		
		0x00,0xFF,0x01,0x14,  // text event 0x14==20 chars long ("some other inst name")
			0x53,0x6F,0x6D,0x65,0x20,0x6F,0x74,0x68,0x65,0x72,0x20,0x69,0x6E,0x73,0x74,0x20,0x6E,0x61,0x6D,0x65,
	
		0x00, 0xC0, 0x00,  // Program change to program 0 => Acoustic Grand Piano
		0x00, 0xB0, 0x07, 0x6F,  // Ctrl change 0x07 => set channel volume
		
		// Initial note-on is 1-tick after the first note of track 1
		// note-on events; duration of each note is == 10
		0x01, 0x90, 0x38, 0x59,  // cum dt onset == 1 (note on)
		0x0A, 0x3A, 0x59,  // cum dt onset == 0x01+0x0A==0x0B==11 (note on)
		0x00, 0x80, 0x38, 0x59,  // cum dt event onset => 0x01+0x0A==0x0B== 11 (note off)
		0x0A, 0x3A, 0x59,  // cum dt event onset => 0x01+0x0A+0x0A==0x15==21 (note off)
		
		0x00, 0x90, 0x3B, 0x59,  // cum dt event onset => 21 (note on)
		0x0A, 0x80, 0x3B, 0x59,  // cum dt event onset => ==31 (note off)
		
		0x00, 0x90, 0x3C, 0x59,  // cum dt event onset => 31 (note on)
		0x0A, 0x80, 0x3C, 0x59,  // cum dt event onset => ==41 (note off)

		0x00, 0xFF, 0x2F, 0x00   // End of track
	};

	std::abort();

	std::string fn = "D:\\cpp\\nh2oh\\au\\gt_aulib\\test_data\\tc_a_rs.mid";
	std::filesystem::path fp(fn);
	std::ofstream fptr(fp,std::ios_base::out|std::ios_base::binary);
	bool is_good = fptr.good();
	std::copy(tc_a_rs.begin(),tc_a_rs.end(),std::ostreambuf_iterator(fptr));
	fptr.close();

	return 0;
}


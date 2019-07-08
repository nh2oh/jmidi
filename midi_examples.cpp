#include "midi_examples.h"
#include "midi_raw.h"
#include "midi_raw_test_parts.h"
#include "mthd_t.h"
#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include "smf_t.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <type_traits>
#include <algorithm>
#include <exception>
#include <fstream>
#include <optional>
#include <vector>
#include <iterator>


int midi_example() {
	//midi_clamped_value_testing();
	//midi_setdt_testing();
	//midi_mtrk_split_testing();
	//std::cout << std::endl;
	//std::cout << std::endl;

	/*auto e1 = midi_ch_event_t {0x90u,1,57,25};
	mtrk_event_t me1(157,e1);
	auto e1md = get_channel_event(me1);
	auto e1dt = me1.delta_time();

	auto e2 = midi_ch_event_t {note_off,1,57,25};
	mtrk_event_t me2(1548,e2);
	auto e2md = get_channel_event(me2);
	auto e2dt = me2.delta_time();*/

	//auto mt_tests = testdata::make_random_meta_tests(100);
	//testdata::print_meta_tests(mt_tests);
	//testdata::print_midi_test_cases();

	//std::string fn = "D:\\cpp\\nh2oh\\au\\gt_aulib\\test_data\\clementi_no_rs.mid";
	//std::string fn = "D:\\cpp\\nh2oh\\au\\gt_aulib\\test_data\\tc_a_rs.mid";
	std::string fn = "C:\\Users\\ben\\Desktop\\scr\\CLEMENTI.MID";
	//std::string fn = "C:\\Users\\ben\\Desktop\\A7.mid";
	//std::string fn = "C:\\Users\\ben\\Desktop\\scr\\test.mid";
	//std::string fn = "C:\\Users\\ben\\Desktop\\scr\\hallelujah_joy_to_the_world.mid";
	//std::string fn = "C:\\Users\\ben\\Desktop\\twinkle.mid";
	//std::string fn = "C:\\Users\\ben\\Desktop\\scr\\test.mid";

	auto maybesmf = read_smf(fn);
	if (!maybesmf) {
		std::cout << "nope :(" << std::endl;
		std::abort();
	}
	std::cout << "print(maybesmf.smf):" << std::endl;
	std::cout << print(maybesmf.smf) << std::endl;

	auto trk1 = maybesmf.smf[1];
	//std::cout << print_event_arrays(trk1) << std::endl;
	//std::cout << print(trk1) << std::endl;
	std::cout << "print_linked_onoff_pairs(trk1):" << std::endl; 
	std::cout << print_linked_onoff_pairs(trk1) << std::endl;

	//std::cout << "get_linked_onoff_pairs(maybesmf.smf):" << std::endl;
	//auto linked_pairs = get_linked_onoff_pairs(maybesmf.smf);
	//std::cout << print(linked_pairs) << std::endl << std::endl;

	//auto ordered_evs_all = get_events_dt_ordered(maybesmf.smf);
	//std::cout << print(ordered_evs_all) << std::endl;

	for (int trkn=0; trkn<maybesmf.smf.ntrks(); ++trkn) {
		midi_time_t curr_time {};
		curr_time.tpq = get_tpq(maybesmf.smf.division());
		auto trk = maybesmf.smf[trkn];
		std::cout << "duration(maybesmf.smf.get_track("
			<< std::to_string(trkn) << ")) == "
			<< std::to_string(duration(maybesmf.smf[trkn],curr_time))
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

int midi_clamped_value_testing() {
	int64_t toobig = 0x2FFFFFFF;
	delta_time_t dt_from_toobig(toobig);
	std::cout << dt_from_toobig << std::endl;

	int64_t a = 0x0FFFFFFFu;
	int64_t i = 4;
	int64_t result = 0;
	int64_t exactresult = a + std::numeric_limits<uint32_t>::max(); 
	return 0;
}

int midi_setdt_testing() {
	std::vector<uint32_t> dts {
		0x00u,0x40u,0x7Fu,  // field size == 1
		0x80u,0x2000u,0x3FFFu,  // field size == 2
		0x4000u,0x100000u,0x1FFFFFu,  // field size == 3
		0x00200000u,0x08000000u,0x0FFFFFFFu,  // field size == 4
		// Attempt to write values exceeding the allowed max; field size
		// should be 4, and all values written should be == 0x0FFFFFFFu
		0x1FFFFFFFu,0x2FFFFFFFu,0x7FFFFFFFu,0x8FFFFFFFu,
		0x9FFFFFFFu,0xBFFFFFFFu,0xEFFFFFFFu,0xFFFFFFFFu
	};
	for (const auto& dt_init : dts) {
		auto curr_ev = mtrk_event_t();
		curr_ev.set_delta_time(dt_init);
		mtrk_event_unit_test_helper_t h(curr_ev);
		for (const auto& new_dt : dts) {
			std::cout << "dt_init==" <<  dt_init << "; " 
				<< "new_dt==" << new_dt << std::endl;
			std::cout << print(curr_ev,mtrk_sbo_print_opts::debug) 
				<< std::endl;

			curr_ev.set_delta_time(new_dt);
			std::cout << print(curr_ev,mtrk_sbo_print_opts::debug) 
				<< std::endl;
			std::cout << "-------------------------------------------" << std::endl << std::endl;

			auto curr_dt_ans = (0x0FFFFFFFu&new_dt);
			auto curr_dt_sz = 1;
			if ((new_dt>=0x00u) && (new_dt < 0x80u)) {
				curr_dt_sz = 1;
			} else if ((new_dt>= 0x80u) && (new_dt<0x4000u)) {
				curr_dt_sz = 2;
			} else if ((new_dt>= 0x4000u) && (new_dt<0x00200000u)) {
				curr_dt_sz = 3;
			} else {
				curr_dt_sz = 4;
			}
		}
	}

	return 0;
}




int raw_bytes_as_midi_file() {
	std::vector<unsigned char> tc_a_rs {
		0x4D, 0x54, 0x68, 0x64,  // MThd
		0x00, 0x00, 0x00, 0x06,
		0x00, 0x01,  // Format 1 => simultaneous tracks
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




int midi_mtrk_split_testing() {
	struct tsb_t {
		std::vector<unsigned char> d {};
		uint64_t cumtk {};
		uint64_t tkonset {};
	};
	std::vector<tsb_t> tsb {
		// meta instname: "acoustic grand";
		{{0x00u,0xFFu,0x04u,0x0Eu,0x61u,0x63u,0x6Fu,0x75u,0x73u,0x74u,0x69u,
			0x63u,0x20u,0x67u,0x72u,0x61u,0x6Eu,0x64u}, 0, 0},  
		{{0x00u,0xB0u,0x07u,0x64u}, 0, 0},
		{{0x00u,0xFFu,0x59u,0x02u,0x00u,0x00u}, 0, 0},  // Key sig

		{{0x05u,0x90u,0x48u,0x59u}, 0, 5},
		{{0x81u,0x05u,0x80u,0x48u,0x17u}, 5, 138},

		{{0x00u,0x90u,0x49u,0x0Cu}, 138, 138},  // 73 on
		{{0x00u,0x90u,0x50u,0x0Cu}, 138, 138},
		{{0x00u,0x80u,0x49u,0x0Cu}, 138, 138},  // 73 off
		{{0x00u,0x80u,0x50u,0x0Cu}, 138, 138},

		{{0x07u,0x90u,0x4Cu,0x5Fu}, 138, 145},
		{{0x43u,0x90u,0x48u,0x58u}, 145, 212},
		{{0x10u,0x80u,0x4Cu,0x0Cu}, 212, 228},
		{{0x32u,0x80u,0x48u,0x12u}, 228, 278},

		{{0x00u,0x90u,0x43u,0x58u}, 278, 278},  // dt==0
		{{0x1Bu,0x80u,0x43u,0x1Cu}, 278, 305},  // dt==27

		{{0x69u,0x90u,0x43u,0x5Fu}, 305, 410},  // dt==105
		{{0x21u,0x80u,0x43u,0x23u}, 410, 443},  // dt==33

		{{0x6Eu,0x90u,0x48u,0x5Eu}, 443, 553},
		{{0x71u,0x80u,0x48u,0x1Bu}, 553, 666},

		{{0x07u,0x90u,0x4Cu,0x65u}, 666, 673},
		{{0x43u,0x90u,0x48u,0x64u}, 673, 740},

		{{0x06u,0x80u,0x4Cu,0x16u}, 740, 746},
		{{0x32u,0x80u,0x48u,0x20u}, 746, 796},

		{{0x0Cu,0x90u,0x43u,0x65u}, 796, 808},  // dt==12
		{{0x23u,0x80u,0x43u,0x25u}, 808, 843},  // dt==35

		{{0x67u,0x90u,0x4Fu,0x66u}, 843, 946},
		{{0x22u,0x80u,0x4Fu,0x1Au}, 946, 980},

		{{0x63u,0x90u,0x4Du,0x63u}, 980, 1079},
		{{0x3Du,0x90u,0x4Cu,0x60u}, 1079, 1140},
		{{0x15u,0x80u,0x4Du,0x18u}, 1140, 1161},
		{{0x26u,0x90u,0x4Au,0x63u}, 1161, 1199},
		{{0x18u,0x80u,0x4Cu,0x0Fu}, 1199, 1223},
		{{0x1Cu,0x80u,0x4Au,0x23u}, 1223, 1251},

		{{0x00u,0xFFu,0x2Fu,0x00u}, 1251, 1251}  // EOT; idx==
	};  // std::vector<> tsb
	auto mtrkb = mtrk_t();  // auto to avoid MVP
	for (const auto& e : tsb) {
		auto curr_ev = mtrk_event_t(e.d.data(),e.d.size());
		mtrkb.push_back(curr_ev);
	}
	auto mtrkb_init = mtrkb;
	//----------------------------------------------------------
	auto isntnum73 = [](const mtrk_event_t& ev)->bool {
		auto md = get_channel_event(ev);
		return (is_channel_voice(ev) && (md.p1==73));  // 73 == 0x50u
	};
	auto isntnum43 = [](const mtrk_event_t& ev)->bool {
		auto md = get_channel_event(ev);
		return (is_channel_voice(ev) && (md.p1==67));  // 67 == 0x43u
	};
	auto ismeta = [](const mtrk_event_t& ev)->bool {
		return is_meta(ev);
	};

	std::cout << "mtrkb:\n" << print_event_arrays(mtrkb) << std::endl << std::endl;
	std::cout << "auto it = split_if(mtrkb.begin(),mtrkb.end(),isntnum73);\n"
		<< std::endl;
	auto it = split_if(mtrkb.begin(),mtrkb.end(),isntnum73);
	//std::cout << "auto it = split_if(mtrkb.begin(),mtrkb.end(),ismeta);\n"
	//	<< std::endl;
	//auto it = split_if(mtrkb.begin(),mtrkb.end(),ismeta);
	auto a = mtrk_t(mtrkb.begin(),it);
	auto b = mtrk_t(it,mtrkb.end());
	std::cout << "a:\n" << print_event_arrays(a) << std::endl << std::endl;
	std::cout << "b:\n" << print_event_arrays(b) << std::endl << std::endl;

	//auto c = mtrk_t();
	//merge(b.begin(),b.end(),a.begin(),a.end(),std::back_inserter(c));
	//merge(a.begin(),a.end(),b.begin(),b.end(),std::back_inserter(c));
	//std::cout << "c==merge(a.begin(),a.end(),b.begin(),b.end(),...):\n" 
	//	<< print_event_arrays(c) << std::endl << std::endl;

	auto c = mtrk_t();
	merge(b.begin(),b.end(),a.begin(),a.end(),std::back_inserter(c));
	std::cout << "c==merge(b.begin(),b.end(),a.begin(),a.end(),...):\n"
		<< print_event_arrays(c) << std::endl << std::endl;
	
	for (int i=0; i<mtrkb_init.size(); ++i) {
		if (mtrkb_init[i]!=c[i]) {
			std::cout << "oops:" << std::endl;
			std::cout << print(mtrkb_init[i]) << std::endl;
			std::cout << print(c[i]) << std::endl;
			std::cout << std::endl;
		}
	}

	return 0;
}



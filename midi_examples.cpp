#include "midi_examples.h"
#include "midi_raw.h"
#include "midi_raw_test_parts.h"
#include "mthd_t.h"
#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include "smf_t.h"
#include "midi_vlq.h"
#include "midi_delta_time.h"
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
#include <chrono>




int midi_example() {

	/*{
	std::filesystem::path oneth_inp = "C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\";
	std::filesystem::path oneth_op = "C:\\Users\\ben\\Desktop\\midi_archive\\one_thread_out.txt";
	auto start1 = std::chrono::high_resolution_clock::now();
	std::cout << "Starting 1-thread version:  " << std::endl;
	std::thread t_oneth(avg_and_max_event_sizes,oneth_inp,oneth_op);
	t_oneth.join();
	auto end1 = std::chrono::high_resolution_clock::now();
	auto d1 = std::chrono::duration_cast<std::chrono::seconds>(end1-start1).count();
	std::cout << "1-thread version finished in d == " 
		<< d1 << " seconds." << std::endl << std::endl;
	}

	{
	std::filesystem::path twoth_inp1 = "C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\0_to_I\\";
	std::filesystem::path twoth_inp2 = "C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\J_to_Z\\";
	std::filesystem::path twoth_op1 = "C:\\Users\\ben\\Desktop\\midi_archive\\two_thread_out1.txt";
	std::filesystem::path twoth_op2 = "C:\\Users\\ben\\Desktop\\midi_archive\\two_thread_out2.txt";
	auto start2 = std::chrono::high_resolution_clock::now();
	std::cout << "Starting 2-thread version:  " << std::endl;
	std::thread t_twoth1(avg_and_max_event_sizes,twoth_inp1,twoth_op1);
	std::thread t_twoth2(avg_and_max_event_sizes,twoth_inp2,twoth_op2);
	t_twoth1.join();
	t_twoth2.join();
	auto end2 = std::chrono::high_resolution_clock::now();
	auto d2 = std::chrono::duration_cast<std::chrono::seconds>(end2-start2).count();
	std::cout << "2-thread version finished in d == " 
		<< d2 << " seconds." << std::endl << std::endl;
	}*/

	/*{
	std::filesystem::path fourth_inp1 = "C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\0_to_I\\0_to_ch\\";
	std::filesystem::path fourth_inp2 = "C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\0_to_I\\cl_to_I\\";
	std::filesystem::path fourth_inp3 = "C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\J_to_Z\\J_to_P\\";
	std::filesystem::path fourth_inp4 = "C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\J_to_Z\\Q_to_Z\\";
	std::filesystem::path fourth_op1 = "C:\\Users\\ben\\Desktop\\midi_archive\\four_thread_out1.txt";
	std::filesystem::path fourth_op2 = "C:\\Users\\ben\\Desktop\\midi_archive\\four_thread_out2.txt";
	std::filesystem::path fourth_op3 = "C:\\Users\\ben\\Desktop\\midi_archive\\four_thread_out3.txt";
	std::filesystem::path fourth_op4 = "C:\\Users\\ben\\Desktop\\midi_archive\\four_thread_out4.txt";
	auto start4 = std::chrono::high_resolution_clock::now();
	std::cout << "Starting 4-thread version:  " << std::endl;
	std::thread t_fourth1(avg_and_max_event_sizes,fourth_inp1,fourth_op1);
	std::thread t_fourth2(avg_and_max_event_sizes,fourth_inp2,fourth_op2);
	std::thread t_fourth3(avg_and_max_event_sizes,fourth_inp3,fourth_op3);
	std::thread t_fourth4(avg_and_max_event_sizes,fourth_inp4,fourth_op4);
	t_fourth1.join();
	t_fourth2.join();
	t_fourth3.join();
	t_fourth4.join();
	auto end4 = std::chrono::high_resolution_clock::now();
	auto d4 = std::chrono::duration_cast<std::chrono::seconds>(end4-start4).count();
	std::cout << "4-thread version finished in d == " 
		<< d4 << " seconds." << std::endl << std::endl;
	}*/



	auto start1 = std::chrono::high_resolution_clock::now();
	//std::filesystem::path inp = "C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\J_to_Z\\";
	std::filesystem::path inp = "C:\\Users\\ben\\Desktop\\midi_broken_mthd\\";
	std::filesystem::path op = "C:\\Users\\ben\\Desktop\\midi_archive\\out.txt";
	inspect_mthds(inp,"yaaaaaaay.txt");
	//inspect_mthds(inp,"");
	//avg_and_max_event_sizes(inp,op);
	auto end1 = std::chrono::high_resolution_clock::now();
	auto d1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1-start1).count();
	std::cout << "1-thread version finished in d == " 
		<< d1 << " milliseconds." << std::endl << std::endl;


	//read_midi_directory_mthd_inspection("C:\\Users\\ben\\Desktop\\midi_broken_mtrk\\");
	//read_midi_directory_mthd_inspection("C:\\Users\\ben\\Desktop\\midi_broken_mthd\\");
	//read_midi_directory_mthd_inspection("C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\");
	//read_midi_directory("C:\\Users\\ben\\Desktop\\midi_broken_mthd\\");
	//read_midi_directory("C:\\Users\\ben\\Desktop\\midi_broken_mtrk\\");
	//read_midi_directory("C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\0\\");
	//read_midi_directory("C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\Arabic and Tribal Rhythms\\");
	//read_midi_directory("C:\\Users\\ben\\Desktop\\midi_archive\\crash\\");
	//read_midi_directory("C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\");
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

	//std::string fn;
	//fn = "D:\\cpp\\nh2oh\\au\\au\\gt_aulib\\test_data\\clementi_no_rs.mid";
	//std::string fn = "D:\\cpp\\nh2oh\\au\\au\\gt_aulib\\test_data\\tc_a_rs.mid";
	//std::string fn = "C:\\Users\\ben\\Desktop\\scr\\CLEMENTI.MID";
	//std::string fn = "C:\\Users\\ben\\Desktop\\A7.mid";
	//std::string fn = "C:\\Users\\ben\\Desktop\\scr\\test.mid";
	//std::string fn = "C:\\Users\\ben\\Desktop\\scr\\hallelujah_joy_to_the_world.mid";
	//std::string fn = "C:\\Users\\ben\\Desktop\\twinkle.mid";
	//std::string fn = "C:\\Users\\ben\\Desktop\\scr\\test.mid";

	// Invalid:
	//fn = "C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\1\\10.mid";
	/*
	auto maybesmf = read_smf(fn);
	if (!maybesmf) {
		std::cout << "nope :(" << std::endl;
		std::cout << maybesmf.error << std::endl;
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
	*/
	return 0;
}



int read_midi_directory(const std::filesystem::path& bp) {
	auto rdi = std::filesystem::recursive_directory_iterator(bp.parent_path());
	std::string errmsg;  errmsg.reserve(500);
	int n_midi_files = 0;  // The total number of midi files
	int n_err_files = 0;  // The number of midi files w/ errors
	for (const auto& dir_ent : rdi) {
		auto curr_path = dir_ent.path();
		if (!has_midifile_extension(curr_path)) {
			continue;
		}
		++n_midi_files;

		smf_error_t smf_error;
		auto maybesmf = read_smf(curr_path,&smf_error);
		std::cout << "File number " << n_midi_files << '\n' 
			<< curr_path.string() << '\n';
		if (!maybesmf) {
			++n_err_files;
			errmsg = explain(smf_error);
			std::cout << "Error! (" << n_err_files << ")\n" 
				<< errmsg << '\n';
		} else {
			std::cout << "File Ok!\n" 
				<< "Format = " << maybesmf.smf.format() << ", "
				<< "N MTrks = " << maybesmf.smf.ntrks() << '\n';
		}
		std::cout << "==============================================="
				"=================================\n\n";
	}
	return 0;
}

int inspect_mthds(const std::filesystem::path& bp, 
					const std::filesystem::path& of) {
	auto outf = std::ofstream(of);
	std::ostream* out = &outf;
	if (of.empty()) {
		out = &std::cout;
	}
	auto rdi = std::filesystem::recursive_directory_iterator(bp.parent_path());
	int n_midi_files = 0;
	for (const auto& dir_ent : rdi) {
		auto curr_path = dir_ent.path();
		if (!has_midifile_extension(curr_path)) {
			continue;
		}
		++n_midi_files;
		
		std::basic_ifstream<unsigned char> f(curr_path,
			std::ios_base::in|std::ios_base::binary);
		if (!f.is_open() || !f.good()) {
			std::cout << "Could not open file.  Wat.  " << std::endl;
			std::cout << std::endl;
			continue;
		}
		std::string s; s.reserve(1000);
		s += curr_path.string() + '\n';

		mthd_error_t mthd_error {};
		auto it = std::istreambuf_iterator(f);
		auto mthd = make_mthd(it,std::istreambuf_iterator<unsigned char>(),&mthd_error);
		if (mthd) {
			print(mthd.mthd,s);
		} else {
			s += explain(mthd_error);
		}
		*out << s << '\n';
		*out << "==============================================="
			"=================================\n";

		f.close();
	}
	return 0;
}

int avg_and_max_event_sizes(const std::filesystem::path& bp,
				const std::filesystem::path& of) {
	std::ofstream outfile(of);
	auto rdi = std::filesystem::recursive_directory_iterator(bp.parent_path());
	int n_midi_files = 0;
	for (const auto& dir_ent : rdi) {
		auto curr_path = dir_ent.path();
		if (!has_midifile_extension(curr_path)) {
			continue;
		}
		++n_midi_files;
		/*if (n_midi_files < 90186) {
			continue;
		}*/
		// Read the file into fdata, close the file
		auto maybe_smf = read_smf(curr_path);
		outfile << "File " << std::to_string(n_midi_files) << ")  " 
			<< curr_path.string() << '\n';
		if (!maybe_smf) {
			outfile << "\t!maybe_smf;  skipping...\n"; 
			continue;
		}
		
		int32_t cum_nbytes = 0;
		int32_t n_events = 0;
		int32_t n_events_gt31bytes = 0;
		int32_t n_events_2431bytes = 0;
		int32_t n_events_leq23bytes = 0;
		int32_t max_sz = 0;
		auto biggest_event = mtrk_event_t();
		for (const auto& trk : maybe_smf.smf) {
			for (const auto& ev : trk) {
				auto sz = ev.size();
				if (sz > 31) {
					++n_events_gt31bytes;
				} else if ((sz>=24)&&(sz<=31)) {
					++n_events_2431bytes;
				} else if (sz<=23) {
					++n_events_leq23bytes;
				}
				if (sz > max_sz) {
					max_sz = ev.size();
					biggest_event = ev;
				}
				cum_nbytes += ev.size();
				++n_events;
			}  // To next event in track
		}  // To next track in smf
		outfile << "n_events == " << std::to_string(n_events) 
			<< "; avg event size == " 
			<< std::to_string((1.0*cum_nbytes)/(1.0*n_events))
			<< '\n'
			<< "n <= 23 bytes == " << std::to_string(n_events_leq23bytes)
			<< "; n >= 24 && <= 31 bytes == " << std::to_string(n_events_2431bytes)
			<< "; n > 31 bytes == " << std::to_string(n_events_gt31bytes)
			<< ";\n";
		outfile << print(biggest_event,mtrk_sbo_print_opts::detail) << '\n';
		outfile << "==============================================="
				"=================================\n\n";

	}
	outfile << n_midi_files << " Midi files\n";
	outfile.close();
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
		auto curr_ev = make_mtrk_event(e.d.data(),e.d.data()+e.d.size(),0,nullptr).event;
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



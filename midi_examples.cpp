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
#include <thread>
#include <cstddef>

const std::vector<midi_test_stuff_t> midi_test_dirs {
	{"all",R"(C:\Users\ben\Desktop\midi_archive\midi_archive\)",
	R"(C:\Users\ben\Desktop\midi_archive\all.txt)"},

	{"2th1",R"(C:\Users\ben\Desktop\midi_archive\midi_archive\0_to_I\)",
	R"(C:\Users\ben\Desktop\midi_archive\2th1.txt)"},
	{"2th2",R"(C:\Users\ben\Desktop\midi_archive\midi_archive\J_to_Z\)",
	R"(C:\Users\ben\Desktop\midi_archive\2th2.txt)"},

	{"4th1",R"(C:\Users\ben\Desktop\midi_archive\midi_archive\0_to_I\0_to_ch\)",
	R"(C:\Users\ben\Desktop\midi_archive\4th1.txt)"},
	{"4th2",R"(C:\Users\ben\Desktop\midi_archive\midi_archive\0_to_I\cl_to_I\)",
	R"(C:\Users\ben\Desktop\midi_archive\4th2.txt)"},
	{"4th3",R"(C:\Users\ben\Desktop\midi_archive\midi_archive\J_to_Z\J_to_P\)",
	R"(C:\Users\ben\Desktop\midi_archive\4th3.txt)"},
	{"4th4",R"(C:\Users\ben\Desktop\midi_archive\midi_archive\J_to_Z\Q_to_Z\)",
	R"(C:\Users\ben\Desktop\midi_archive\4th4.txt)"},

	{"mthd",R"(C:\Users\ben\Desktop\midi_broken_mthd\)",
	R"(C:\Users\ben\Desktop\midi_archive\mthd.txt)"},
	{"mtrk",R"(C:\Users\ben\Desktop\midi_broken_mtrk\)",
	R"(C:\Users\ben\Desktop\midi_archive\mtrk.txt)"},
	{"simple",R"(C:\Users\ben\Desktop\midi_simple_valid\)",
	R"(C:\Users\ben\Desktop\midi_archive\simple_valid.txt)"}
};
inout_dirs_t get_midi_test_dirs(const std::string& name) {
	auto p = [&name](const midi_test_stuff_t& e)->bool {
		return e.name==name;
	};
	inout_dirs_t result;
	auto it = std::find_if(midi_test_dirs.begin(),midi_test_dirs.end(),p);
	result.inp = it->inp;
	result.outp = it->outp;
	return result;
}
	

int midi_example() {
	event_sizes_benchmark();
	return 0;
}


int event_sizes_benchmark() {
	int mode = 0;  // 0 => batch, 1=>istreambuf_iterator
	int Nth = 2;
	if (Nth!=4) { Nth=2; };
	auto tstart = std::chrono::high_resolution_clock::now();
	std::cout << "avg_and_max_event_sizes() benchmark\n"
		<< "Starting " << Nth << "-thread version in mode " << mode 
		<< ":  " << std::endl;
	std::vector<std::thread> vth;
	for (int i=0; i<Nth; ++i) {
		std::string name = std::to_string(Nth) + "th" + std::to_string(i+1);
		auto dirs = get_midi_test_dirs(name);
		vth.emplace_back(std::thread(avg_and_max_event_sizes,
			dirs.inp,dirs.outp,mode));
	}
	for (auto& t : vth) { t.join(); }
	auto tend = std::chrono::high_resolution_clock::now();
	auto tdelta = std::chrono::duration_cast<std::chrono::milliseconds>(tend-tstart);;
	std::cout << Nth << "-thread version finished in d == " 
		<< tdelta.count() << " milliseconds." << std::endl << std::endl;
}

int avg_and_max_event_sizes(const std::filesystem::path& bp,
				const std::filesystem::path& of, const int mode) {
	std::ofstream outfile(of);
	std::vector<char> fdata;  // Used if mode == 0
	auto rdi = std::filesystem::recursive_directory_iterator(bp.parent_path());
	int n_midi_files = 0;
	for (const auto& dir_ent : rdi) {
		auto curr_path = dir_ent.path();
		if (!has_midifile_extension(curr_path)) {
			continue;
		}
		std::basic_ifstream<char> f(curr_path,
				std::ios_base::in|std::ios_base::binary);
		if (!f.is_open() || !f.good()) {
			continue;
		}
		++n_midi_files;

		maybe_smf_t maybe_smf;
		smf_error_t smf_error;
		if (mode == 0) {  // Batch
			// Read the file into fdata, close the file
			f.seekg(0,std::ios::end);
			auto fsize = f.tellg();
			f.seekg(0,std::ios::beg);
			fdata.resize(fsize);
			f.read(fdata.data(),fsize);
			make_smf(fdata.data(),fdata.data()+fdata.size(),
				&maybe_smf,&smf_error);
		} else if (mode == 1) {  // iostreams
			std::istreambuf_iterator<char> it(f);
			auto end = std::istreambuf_iterator<char>();	
			make_smf(it,end,&maybe_smf,&smf_error);
		}
		f.close();

		outfile << "File " << std::to_string(n_midi_files) << ")  " 
			<< curr_path.string() << '\n';
		if (!maybe_smf) {
			outfile << "\t!maybe_smf:  " << explain(smf_error) 
				<< "\nskipping...\n"; 
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
				<< "N MTrks = " << maybesmf.smf.ntrks() << ", "
				<< "N bytes = " << maybesmf.smf.nbytes() <<'\n';
			int trkn = 0;
			for (const auto& trk : maybesmf.smf) {
				std::cout << "Track " << trkn++ << ":  " 
					<< trk.size() << " events, " << trk.nbytes()
					<< " bytes.\n";
			}
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


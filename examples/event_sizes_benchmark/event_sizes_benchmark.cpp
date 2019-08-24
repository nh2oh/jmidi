#include "event_sizes_benchmark.h"
#include "smf_t.h"
#include "util.h"
#include <chrono>
#include <filesystem>
#include <thread>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <regex>


int main(int argc, char *argv[]) {
	auto opts = get_options(argc,argv);
	std::cout << "Running the event_sizes_benchmark benchmark with:\n"
		<< "\tNth == " << opts.Nth << " threads.\n"
		<< "\tmode == " << opts.mode << " \n"
		<< std::endl;

	std::filesystem::path basedir 
		//= "C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\0_to_I\\cl_to_I\\";
		//= "C:\\Users\\ben\\Desktop\\midi_random_collection\\";
		= "C:\\Users\\ben\\Desktop\\midi_archive\\midi_archive\\";
	event_sizes_benchmark(opts.mode,opts.Nth,basedir);

	return 0;
}


opts_t get_options(int argc, char **argv) {
	struct opts_type {
		std::regex rx;
		int val;
		int def_val;
	};
	std::array<opts_type,2> opts {{
		// The number of threads
		{std::regex("-Nth=(\\d+)"),-1,4},
		// The file-read mode:  0=>batch, 1=>istreambuf_iterator0
		{std::regex("-mode=(\\d+)"),-1,0}
	}};

	for (int i=0; i<argc; ++i) {
		for (int j=0; j<opts.size(); ++j) {
			std::cmatch curr_match;
			std::regex_match(argv[i],curr_match,opts[j].rx);
			if (curr_match.empty()) { continue; }
			opts[j].val = std::stoi(curr_match[1].str());
			// Each argv[i] should match only one of the options in opts:
			break;
		}
	}

	opts_t result;
	result.Nth = opts[0].val;
	if (opts[0].val <= 0) {
		result.Nth = opts[0].def_val;
	}
	result.mode = opts[1].val;
	if ((opts[1].val != 0) && (opts[1].val != 1) && (opts[1].val != 2)) {
		result.mode = opts[1].def_val;
	}
	return result;
}


int event_sizes_benchmark(int mode, int Nth,
							const std::filesystem::path& basedir) {
	if ((mode != 0) && (mode != 1) && (mode != 2)) {
		mode = 0;  // 0 => batch, 1=>istreambuf_iterator
	}
	if (Nth <= 0) {
		Nth = 1;
	}

	auto rdi = std::filesystem::recursive_directory_iterator(basedir);//.parent_path());
	std::vector<std::vector<std::filesystem::path>> thread_files(Nth);
	int i=0;
	for (const auto& dir_ent : rdi) {
		auto curr_path = dir_ent.path();
		if (!has_midifile_extension(curr_path)) {
			continue;
		}
		thread_files[i%Nth].push_back(curr_path);
		++i;
	}

	auto tstart = std::chrono::high_resolution_clock::now();
	std::cout << "avg_and_max_event_sizes() benchmark\n"
		<< "Starting " << Nth << "-thread version in mode " << mode 
		<< ":  " << std::endl;
	std::vector<std::thread> vth;
	for (int i=0; i<Nth; ++i) {
		std::string outf_name = std::to_string(Nth) + "th" 
			+ std::to_string(i+1) + ".txt";
		auto outp = basedir.parent_path()/outf_name;
		vth.emplace_back(std::thread(avg_and_max_event_sizes,
			thread_files[i],outp,mode));
	}
	for (auto& t : vth) { t.join(); }
	auto tend = std::chrono::high_resolution_clock::now();
	auto tdelta = std::chrono::duration_cast<std::chrono::milliseconds>(tend-tstart);
	std::cout << Nth << "-thread version finished in d == " 
		<< tdelta.count() << " milliseconds." << std::endl << std::endl;

	return 0;
}

int avg_and_max_event_sizes(const std::vector<std::filesystem::path>& files,
				const std::filesystem::path& of, const int mode) {
	std::ofstream outfile(of);
	std::vector<char> fdata;  // Used if mode == 0
	int n_midi_files = 0;
	for (const auto& curr_path : files) {
		if (!has_midifile_extension(curr_path)) {
			continue;
		}

		maybe_smf_t maybe_smf;
		smf_error_t smf_error;
		if (mode == 0) {  // Batch
			std::basic_ifstream<char> f(curr_path,
				std::ios_base::in|std::ios_base::binary);
			if (!f.is_open() || !f.good()) {
				continue;
			}
			++n_midi_files;
			// Read the file into fdata, close the file
			f.seekg(0,std::ios::end);
			auto fsize = f.tellg();
			f.seekg(0,std::ios::beg);
			fdata.resize(fsize);
			f.read(fdata.data(),fsize);
			//auto n = std::min(fdata.size(),std::size_t{25});
			make_smf(fdata.data(),fdata.data()+fdata.size(),
				&maybe_smf,&smf_error);
			f.close();
		} else if (mode == 1) {  // iostreams
			std::basic_ifstream<char> f(curr_path,
				std::ios_base::in|std::ios_base::binary);
			if (!f.is_open() || !f.good()) {
				continue;
			}
			++n_midi_files;
			std::istreambuf_iterator<char> it(f);
			auto end = std::istreambuf_iterator<char>();
			make_smf(it,end,&maybe_smf,&smf_error);
			f.close();
		} else if (mode == 2) {  // csio
			auto nbytes = std::filesystem::file_size(curr_path);
			fdata.resize(nbytes);
			auto b = read_binary_csio(curr_path,fdata);
			if (!b) { continue; }
			++n_midi_files;
			make_smf(fdata.data(),fdata.data()+fdata.size(),
				&maybe_smf,&smf_error);
		}
		

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
		outfile << jmid::print(biggest_event,jmid::mtrk_sbo_print_opts::detail) << '\n';
		outfile << "==============================================="
				"=================================\n\n";
	}
	outfile << n_midi_files << " Midi files\n";
	outfile.close();
	return 0;
}


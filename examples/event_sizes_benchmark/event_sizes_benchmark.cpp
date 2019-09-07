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
		<< "\tpath == " << opts.path << "\n"
		<< "\tNth == " << opts.Nth << " threads.\n"
		<< "\tmode == " << opts.mode << " \n"
		<< std::endl;

	event_sizes_benchmark(opts.mode,opts.Nth,opts.path);

	return 0;
}



int event_sizes_benchmark(int mode, int Nth,
							const std::filesystem::path& basedir) {
	auto rdi = std::filesystem::recursive_directory_iterator(basedir);
	std::vector<std::vector<std::filesystem::path>> thread_files(Nth);
	int i=0;
	for (const auto& dir_ent : rdi) {
		auto curr_path = dir_ent.path();
		if (!jmid::has_midifile_extension(curr_path)) {
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
		std::string outf_name = std::to_string(i+1) + "_"
			+ std::to_string(Nth) + "th_mode" 
			+ std::to_string(mode) +  + ".txt";
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
		if (!jmid::has_midifile_extension(curr_path)) {
			continue;
		}

		jmid::smf_t smf;
		jmid::smf_error_t smf_error;
		smf_error.code = jmid::smf_error_t::errc::no_error;
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
			jmid::make_smf2(fdata.data(),fdata.data()+fdata.size(),
				&smf,&smf_error);
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
			jmid::make_smf2(it,end,&smf,&smf_error);
			f.close();
		} else if (mode == 2) {  // csio
			auto nbytes = std::filesystem::file_size(curr_path);
			fdata.resize(nbytes);
			nbytes = jmid::read_binary_csio(curr_path,fdata);
			fdata.resize(nbytes);
			++n_midi_files;
			jmid::make_smf2(fdata.data(),fdata.data()+fdata.size(),
				&smf,&smf_error);
		}
		

		outfile << "File " << std::to_string(n_midi_files) << ")  " 
			<< curr_path.string() << '\n';
		if (smf_error.code!=jmid::smf_error_t::errc::no_error) {
			outfile << "\t!maybe_smf:  " << explain(smf_error) 
				//<< "\n\tnbytes_read == " << maybe_smf.nbytes_read
				<< ".  skipping...\n"; 
			continue;
		}
		
		int32_t cum_nbytes = 0;
		int32_t n_events = 0;
		int32_t n_events_gt31bytes = 0;
		int32_t n_events_2431bytes = 0;
		int32_t n_events_leq23bytes = 0;
		int32_t max_sz = 0;
		auto biggest_event = jmid::mtrk_event_t();
		for (const auto& trk : smf) {
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


opts_t get_options(int argc, char **argv) {
	opts_t result;
	bool found_nth = false;
	bool found_path = false;
	bool found_mode = false;
	for (int i=0; i<argc; ++i) {
		std::cmatch curr_match;
		if (!found_nth) {
			std::regex rx("-Nth=(\\d+)");
			std::regex_match(argv[i],curr_match,rx);
			if (!curr_match.empty()) {
				result.Nth = std::stoi(curr_match[1].str());
				found_nth = true;
				continue;
			}
		}
		if (!found_mode) {
			std::regex rx("-mode=(\\d+)");
			std::regex_match(argv[i],curr_match,rx);
			if (!curr_match.empty()) {
				result.mode = std::stoi(curr_match[1].str());
				found_mode = true;
				continue;
			}
		}
		if (!found_path) {
			std::regex rx("-path=(.+)");
			std::regex_match(argv[i],curr_match,rx);
			if (!curr_match.empty()) {
				result.path = curr_match[1].str();
				found_path = true;
				continue;
			}
		}
	}

	if (!found_nth) { result.Nth = 1; }
	if (!found_mode) { result.mode = 0; }
	if (!found_path) { result.path = "."; }

	if ((result.mode != 0) && (result.mode != 1) 
		&& (result.mode != 2)) {
		result.mode = 0;  // 0 => batch, 1=>istreambuf_iterator
	}
	if (result.Nth <= 0) {
		result.Nth = 1;
	}

	return result;
}



#include "midi_examples.h"
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
#include <sstream>
#include <random>
#include <cfenv>


const std::vector<midi_test_stuff_t> midi_test_dirs {
	{"write",R"(C:\Users\ben\Desktop\midi_archive\self_written\)",
	R"(C:\Users\ben\Desktop\midi_archive\self_written\out.midi)"},

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
	R"(C:\Users\ben\Desktop\midi_archive\simple_valid.txt)"},
	{"random_collection",R"(C:\Users\ben\Desktop\midi_random_collection\)",
	R"(C:\Users\ben\Desktop\midi_archive\midi_random_collection.txt)"}
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

	//function_counts();


	//auto d = get_midi_test_dirs("random_collection");
	//classify_smf_errors(d.inp,d.outp.parent_path()/"errors.txt");//  "random_collection"
	//event_sizes_benchmark();
	
	//auto d = get_midi_test_dirs("4th1");
	//avg_and_max_event_sizes(d.inp,d.outp,0);


	/*
	auto d = get_midi_test_dirs("write");
	auto smf_out_path = make_midifile(d.outp,false);

	smf_error_t smf_read_err;
	auto smf_in = read_smf(smf_out_path,&smf_read_err);
	if (!smf_in) {
		std::cout << "shit";
	}
	std::cout << print(smf_in.smf) << std::endl;
	*/

	/*auto smf_out2_path 
		= smf_out_path.replace_filename(smf_out_path.stem()+="_again.midi");
	write_smf(smf_in.smf,smf_out2_path);*/
	

	return 0;
}

int function_counts() {
	std::filesystem::path pth 
		= "D:\\cpp\\nh2oh\\au\\au\\gt_aulib\\test_data\\tempo_duration\\tdiv25_tempo250k.midi";
	//std::filesystem::path pth = R"(C:\Users\ben\Desktop\hallelujah_joy_to_the_world.mid)";
	jmid::maybe_smf_t maybe_smf = jmid::read_smf(pth,nullptr);

	return 0;
}




int classify_smf_errors(const std::filesystem::path& inp,
						const std::filesystem::path& outf) {
	std::ofstream outfile(outf);
	std::vector<char> fdata;
	auto rdi = std::filesystem::recursive_directory_iterator(inp.parent_path());
	int n_midi_files = 0;
	for (const auto& dir_ent : rdi) {
		auto curr_path = dir_ent.path();
		if (!jmid::has_midifile_extension(curr_path)) {
			continue;
		}
		std::basic_ifstream<char> f(curr_path,
				std::ios_base::in|std::ios_base::binary);
		if (!f.is_open() || !f.good()) {
			continue;
		}
		++n_midi_files;
		if (n_midi_files%10000 == 0) {
			std::cout << "File num " << n_midi_files << std::endl;
		}

		// Read the file into fdata, close the file
		f.seekg(0,std::ios::end);
		auto fsize = f.tellg();
		f.seekg(0,std::ios::beg);
		fdata.resize(fsize);
		f.read(fdata.data(),fsize);
		f.close();

		jmid::maybe_smf_t maybe_smf;
		jmid::smf_error_t smf_error;
		make_smf(fdata.data(),fdata.data()+fdata.size(),
			&maybe_smf,&smf_error);
		if (maybe_smf) {
			continue;  // The file is ok
		}

		outfile << curr_path.string() << "\n";
		switch (smf_error.code) {
		case jmid::smf_error_t::errc::mthd_error:
			outfile << "smf_error_t::errc::mthd_error";
			break;
		case jmid::smf_error_t::errc::mtrk_error:
			outfile << "smf_error_t::errc::mtrk_error";
			break;
		case jmid::smf_error_t::errc::overflow_reading_uchk:
			outfile << "smf_error_t::errc::overflow_reading_uchk";
			break;
		case jmid::smf_error_t::errc::unexpected_num_mtrks:
			outfile << "smf_error_t::errc::unexpected_num_mtrks";
			break;
		case jmid::smf_error_t::errc::other:
			outfile << "smf_error_t::errc::other";
			break;
		default:
			outfile << "smf_error_t::errc::???";
			break;
		}
		
		if (smf_error.code==jmid::smf_error_t::errc::mtrk_error) {
			outfile << '\t';
			switch (smf_error.mtrk_err_obj.code) {
			case jmid::mtrk_error_t::errc::header_overflow:
				outfile << "mtrk_error_t::errc::header_overflow";
				break;
			case jmid::mtrk_error_t::errc::valid_but_non_mtrk_id:
				outfile << "mtrk_error_t::errc::valid_but_non_mtrk_id";
				break;
			case jmid::mtrk_error_t::errc::invalid_id:
				outfile << "mtrk_error_t::errc::invalid_id";
				break;
			case jmid::mtrk_error_t::errc::length_gt_mtrk_max:
				outfile << "mtrk_error_t::errc::length_gt_mtrk_max";
				break;
			case jmid::mtrk_error_t::errc::invalid_event:
				outfile << "mtrk_error_t::errc::invalid_event";
				break;
			case jmid::mtrk_error_t::errc::no_eot_event:
				outfile << "mtrk_error_t::errc::no_eot_event";
				break;
			case jmid::mtrk_error_t::errc::other:
				outfile << "mtrk_error_t::errc::other";
				break;
			default:
				outfile << "mtrk_error_t::errc::???";
				break;
			}
		}

		if (smf_error.code==jmid::smf_error_t::errc::mtrk_error
				&& smf_error.mtrk_err_obj.code==jmid::mtrk_error_t::errc::invalid_event) {
			outfile << '\t';
			switch (smf_error.mtrk_err_obj.event_error.code) {
			case jmid::mtrk_event_error_t::errc::invalid_delta_time:
				outfile << "mtrk_event_error_t::errc::invalid_delta_time";
				break;
			case jmid::mtrk_event_error_t::errc::no_data_following_delta_time:
				outfile << "mtrk_event_error_t::errc::no_data_following_delta_time";
				break;
			case jmid::mtrk_event_error_t::errc::invalid_status_byte:
				outfile << "mtrk_event_error_t::errc::invalid_status_byte";
				break;
			case jmid::mtrk_event_error_t::errc::channel_calcd_length_exceeds_input:
				outfile << "mtrk_event_error_t::errc::channel_calcd_length_exceeds_input";
				break;
			case jmid::mtrk_event_error_t::errc::channel_invalid_data_byte:
				outfile << "mtrk_event_error_t::errc::channel_invalid_data_byte";
				break;
			case jmid::mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header:
				outfile << "mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header";
				break;
			case jmid::mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length:
				outfile << "mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length";
				break;
			case jmid::mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input:
				outfile << "mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input";
				break;
			case jmid::mtrk_event_error_t::errc::other:
				outfile << "mtrk_event_error_t::errc::other";
				break;
			default:
				outfile << "mtrk_event_error_t::errc::???";
				break;
			}
		}

		outfile << "\n\n";
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
		if (!jmid::has_midifile_extension(curr_path)) {
			continue;
		}
		++n_midi_files;

		jmid::smf_error_t smf_error;
		auto maybesmf = jmid::read_smf(curr_path,&smf_error);
		std::cout << "File number " << n_midi_files << '\n' 
			<< curr_path.string() << '\n';
		if (!maybesmf) {
			++n_err_files;
			errmsg = jmid::explain(smf_error);
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



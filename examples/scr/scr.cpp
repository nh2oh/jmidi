#include "dfa.h"
#include "smf_t.h"
#include "util.h"
#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include <filesystem>
#include <utility>



int main(int argc, char *argv[]) {
	//std::filesystem::path f(argv[1]);
	std::filesystem::path f1(R"(C:\Users\ben\Desktop\midi_archive\desktop\test.mid)");
	if (!std::filesystem::exists(f1) || !jmid::has_midifile_extension(f1)) {
		std::cout << "Specify a valid midi file" << std::endl;
		return 1;
	}
	std::vector<char> mf1(std::filesystem::file_size(f1));
	jmid::read_binary_csio(f1,mf1);

	jmid::smf_error_t smf_error;
	jmid::smf_t smf;
	auto it1 = jmid::make_smf2(mf1.begin(),mf1.end(),&smf,&smf_error);
	if (smf_error.code != jmid::smf_error_t::errc::no_error) {
		std::cout << jmid::explain(smf_error) << std::endl;
		return 2;
	}


	std::filesystem::path f2(R"(C:\Users\ben\Desktop\midi_archive\desktop\CLEMENTI.MID)");
	if (!std::filesystem::exists(f2) || !jmid::has_midifile_extension(f2)) {
		std::cout << "Specify a valid midi file" << std::endl;
		return 1;
	}
	std::vector<char> mf2(std::filesystem::file_size(f2));
	jmid::read_binary_csio(f2,mf2);
	
	auto it2 = jmid::make_smf2(mf2.begin(),mf2.end(),&smf,&smf_error);

	std::cout << f2.string() 
		<< " (" << std::filesystem::file_size(f2) << " bytes):\n";
	if (smf_error.code != jmid::smf_error_t::errc::no_error) {
		std::cout << jmid::explain(smf_error) << std::endl;
		return 2;
	}
	std::cout << jmid::print(smf.mthd()) << "\n";
	int trkn = 0;
	for (const auto& trk : smf) {
		std::cout << "Track " << trkn << '\n';
		for (const auto& ev : trk) {
			std::cout << jmid::print(ev,jmid::mtrk_sbo_print_opts::detail) << '\n';
		}
		trkn++;
	}

	return 0;
}



int midi_files_from_fuzz_corpus(std::filesystem::path pth) {
	//std::filesystem::path pth = "C:\\Users\\ben\\Desktop\\midi_archive\\make_smf_corp\\";

	std::vector<std::filesystem::path> files;
	std::filesystem::recursive_directory_iterator rdi(pth);
	for (auto& di : rdi) {
		if (std::filesystem::is_directory(di)) {
			continue;
		}
		files.push_back(di.path());
	}

	for (int i=0; i<files.size(); ++i) {
		std::string fname = files[i].filename().string();
		auto smf = jmid::read_smf(files[i],nullptr,
			std::filesystem::file_size(files[i]));
		if (smf) {
			fname = "valid_" + fname + ".midi";
		} else {
			fname = "invalid_" + fname + ".midi";
		}
		std::filesystem::rename(files[i],files[i].parent_path()/fname);
	}

	return 0;
}



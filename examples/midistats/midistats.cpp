#include "midi_time.h"
#include "smf_t.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <algorithm>
#include <vector>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cout << "Specify a path containing >= 1 midi file." << std::endl;
		return 1;
	}
	std::filesystem::path p(argv[1]);
	if (!std::filesystem::exists(p) || !std::filesystem::is_directory(p)) {
		std::cout << "Specify a path containing >= 1 midi file." << std::endl;
		return 1;
	}

	struct tdiv_counts_t {
		jmid::time_division_t tdiv;
		int count {0};
	};
	std::vector<tdiv_counts_t> tdiv_counts;
	auto rdi = std::filesystem::recursive_directory_iterator(p);
	for (const auto& dir_ent : rdi) {
		auto curr_path = dir_ent.path();
		if (!jmid::has_midifile_extension(curr_path)) {
			continue;
		}
		
		jmid::smf_error_t smf_error;
		jmid::maybe_smf_t smf = jmid::read_smf(curr_path,&smf_error,
			std::filesystem::file_size(curr_path));
		if (!smf) {
			continue;
		}

		auto curr_tdiv = smf.smf.division();
		auto tdiv_eq = [&curr_tdiv](const tdiv_counts_t& tdivc)->bool {
			return tdivc.tdiv==curr_tdiv;
		};
		auto it = std::find_if(tdiv_counts.begin(),tdiv_counts.end(),
			tdiv_eq);
		if (it!=tdiv_counts.end()) {
			it->count += 1;
		} else {
			tdiv_counts.push_back({curr_tdiv,1});
		}
	}

	for (const auto& tdivc : tdiv_counts) {
		if (jmid::is_tpq(tdivc.tdiv)) {
			std::cout << "tpq\t" 
				<< jmid::get_tpq(tdivc.tdiv)
				<< "\t" << tdivc.count;
		} else if (jmid::is_smpte(tdivc.tdiv)) {
			auto smpte = jmid::get_smpte(tdivc.tdiv);
			std::cout << "smpte\t" 
				<< "tcf==" << smpte.time_code 
				<< " subframes==" << smpte.subframes
				<< "\t" << tdivc.count;
		} else {
			std::cout << "unkn-tdiv\t" << tdivc.count;
		}
		std::cout << std::endl;
	}

	return 0;
}

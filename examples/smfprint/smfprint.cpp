#include "mthd_t.h"
#include "smf_t.h"
#include <iostream>
#include <filesystem>
#include <string>


int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cout << "Specify a midi file." << std::endl;
		return 1;
	}

	std::filesystem::path f(argv[1]);
	if (!std::filesystem::exists(f) || !jmid::has_midifile_extension(f)) {
		std::cout << "Specify a valid midi file" << std::endl;
		return 1;
	}
	
	std::cout << f.string() 
		<< " (" << std::filesystem::file_size(f) << " bytes):\n";

	jmid::smf_error_t smf_error;
	jmid::maybe_smf_t smf = jmid::read_smf(f,&smf_error,std::filesystem::file_size(f));
	if (smf_error.code!=jmid::smf_error_t::errc::no_error) {
		std::cout << jmid::explain(smf_error) << std::endl;
		return 2;
	}

	std::cout << jmid::print(smf.smf.mthd()) << "\n";

	int trkn = 0;
	for (const auto& trk : smf.smf) {
		std::cout << "Track " << trkn << '\n';
		for (const auto& ev : trk) {
			std::cout << jmid::print(ev,jmid::mtrk_sbo_print_opts::detail) << '\n';
		}
		trkn++;
	}

	// Alternatively...
	// jmid::print(jmid::smf_t)
	// std::cout << jmid::print(smf.smf) << "\n";

	return 0;
}

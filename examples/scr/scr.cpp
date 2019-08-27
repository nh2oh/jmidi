#include "midi_time.h"
#include "smf_t.h"
#include "util.h"
//#include <cfenv>
#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include <filesystem>

//int err_duration();

int main(int argc, char *argv[]) {
	std::filesystem::path pth = "C:\\Users\\ben\\Desktop\\midi_archive\\make_smf_corp\\";

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
		//di.replace_filename(std::filesystem::path(fname));
	}

	return 0;
}

/*
int err_duration() {
	std::vector<jmid::time_division_t> tdivs {
		// Small values
		//jmid::time_division_t(1),jmid::time_division_t(2),jmid::time_division_t(3),
		//jmid::time_division_t(20),jmid::time_division_t(24),jmid::time_division_t(25),
		// "Normal" values
		jmid::time_division_t(48),jmid::time_division_t(96),jmid::time_division_t(120),
		jmid::time_division_t(152),jmid::time_division_t(960),
		// Max allowed value for ticks-per-qnt
		jmid::time_division_t(0x7FFF)
	};

	int n_dotmod=0;
	int n_nq=0;
	int n_ntk=0;
	int n_ntk_nexact=0;
	for (const auto& tdiv : tdivs) {
		for (int p=11; p>-3; --p) {
			for (int n=0; n<3; ++n) {
				std::feclearexcept(FE_ALL_EXCEPT);
				double cdot_mod = 1.0+((std::exp2(n)-1.0)/std::exp2(n));
				if (std::fetestexcept(FE_INEXACT)) {
					++n_dotmod;
				}
				std::feclearexcept(FE_ALL_EXCEPT);
				double nq = 4.0*std::exp2(-p)*cdot_mod;  // number-of-whole-notes
				if (std::fetestexcept(FE_INEXACT)) {
					++n_nq;
				}
				std::feclearexcept(FE_ALL_EXCEPT);
				double ntk = nq*(tdiv.get_tpq());
				if (std::fetestexcept(FE_INEXACT)) {
					++n_ntk;
				}

				if (ntk!=static_cast<double>(static_cast<int32_t>(ntk))) {
					n_ntk_nexact++;
				}
			}
		}
	}
	*/
	/*int n_a=0; int n_b=0; int n_c=0; int n_ans=0; int n_d=0;
	for (const auto& tdiv : tdivs) {
		for (int ntks = 1; ntks<100000; ++ntks) {
			for (int p=11; p>-3; --p) {
				for (int n=0; n<3; ++n) {
					auto curr_tpq = tdiv.get_tpq();
					std::feclearexcept(FE_ALL_EXCEPT);
					double a = static_cast<double>(ntks);
					if (std::fetestexcept(FE_INEXACT)) {
						++n_a;
					}

					std::feclearexcept(FE_ALL_EXCEPT);
					double b = std::exp2(p) + std::exp2(p)*((std::exp2(n)-1.0)/std::exp2(n));
					if (std::fetestexcept(FE_INEXACT)) {
						++n_b;
					}

					std::feclearexcept(FE_ALL_EXCEPT);
					double c = b*curr_tpq;
					if (std::fetestexcept(FE_INEXACT)) {
						++n_c;
					}

					std::feclearexcept(FE_ALL_EXCEPT);
					double d = static_cast<double>(ntks)/c;
					if (std::fetestexcept(FE_INEXACT)) {
						++n_d;
					}

					if (std::round(c)!=curr_tpq) {
						n_ans++;
					}
				}
			}
		}
	}*/
/*
	return 0;
}
*/


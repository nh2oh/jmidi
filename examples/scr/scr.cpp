#include "midi_raw.h"
#include <cfenv>
#include <vector>
#include <string>
#include <cmath>

int err_duration();




int main(int argc, char *argv[]) {

	return 0;
}


int err_duration() {
	std::vector<time_division_t> tdivs {
		// Small values
		//time_division_t(1),time_division_t(2),time_division_t(3),
		//time_division_t(20),time_division_t(24),time_division_t(25),
		// "Normal" values
		time_division_t(48),time_division_t(96),time_division_t(120),
		time_division_t(152),time_division_t(960),
		// Max allowed value for ticks-per-qnt
		time_division_t(0x7FFF)
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

	return 0;
}



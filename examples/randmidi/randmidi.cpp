#include "mtrk_event_methods.h"
#include "util.h"
#include "smf_t.h"
#include "midi_time.h"
#include <string>
#include <iostream>
#include <random>
#include <array>
#include <filesystem>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cout << "Specify an output directory and "
			"optional filename." << std::endl;
		return 1;
	}
	std::filesystem::path outfile(argv[1]);
	if (outfile.empty()
		|| (std::filesystem::is_directory(outfile) 
			&& !std::filesystem::exists(outfile))) {
		// Caller either did not specify a valid path or specified 
		// a nonexistent directory
		std::cout << "Specify an output directory and "
			"optional filename." << std::endl;
		return 1;
	}
	std::cout << "randmidi:  Generate a random midi file.  " << std::endl;
	
	std::random_device randdev;
	std::default_random_engine re(randdev());
	//std::uniform_int_distribution<int> rd_nt(0,24);
	std::array<double,24> major_weights = {
		1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0,
		1.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0
	};
	std::discrete_distribution<int> rd_nt(major_weights.begin(),
											major_weights.end());

	auto mthd = jmid::mthd_t();
	mthd.set_format(0);
	mthd.set_ntrks(1);
	auto tdiv = jmid::time_division_t(96);
	mthd.set_division(tdiv);
	std::array<int,4> nt2tks {
		jmid::note2ticks(1,0,tdiv),  // half
		jmid::note2ticks(2,0,tdiv),  // quarter
		jmid::note2ticks(3,0,tdiv),  // eighth
		jmid::note2ticks(4,0,tdiv)  // sixteenth
	};
	std::uniform_int_distribution<size_t> rd_nv(0,nt2tks.size()-1);
	
	auto mtrk = jmid::mtrk_t();
	mtrk.push_back(jmid::make_seqn(0,0));
	mtrk.push_back(jmid::make_copyright(0,"Ben Knowles 2019"));
	mtrk.push_back(jmid::make_trackname(0,"Track 1 (0)"));
	mtrk.push_back(jmid::make_instname(0,"Acoustic Grand"));
	mtrk.push_back(jmid::make_timesig(0,{4,2,24,8}));
	// tempo:  500000 us/q => 0.5 s/q => 2 q/s => 120 q/min => "120 bpm"
	mtrk.push_back(jmid::make_tempo(0,250000));
	mtrk.push_back(jmid::make_program_change(0,0,0));
	// 0 => Acoustic Grand; 6 => Harpsichord
	
	int ch = 0;  int vel = 60;
	int target_nbars = 12;
	int target_ntks = target_nbars*4*tdiv.get_tpq();
	int ntks = 0;
	while (ntks < target_ntks) {
		auto curr_ntval = nt2tks[rd_nv(re)];
		auto curr_ntnum = 60+rd_nt(re);
		auto curr_pair 
			= jmid::make_onoff_pair(curr_ntval,ch,curr_ntnum,vel,vel);
		mtrk.push_back(curr_pair.on);
		mtrk.push_back(curr_pair.off);

		ntks += curr_ntval;
	}
	mtrk.push_back(jmid::make_eot(0));

	auto smf = jmid::smf_t();
	smf.set_mthd(mthd);
	smf.push_back(mtrk);
	
	if (std::filesystem::is_directory(outfile)) {
		outfile /= (randstr(4)+"_randmidi.midi");
	}
	std::cout << "Output == " << outfile << std::endl;
	auto smf_path_result = jmid::write_smf(smf,outfile);

	return 0;
}





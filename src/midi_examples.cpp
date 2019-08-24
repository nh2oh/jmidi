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


	//benchmark_vlqs();
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
			case mtrk_event_error_t::errc::invalid_delta_time:
				outfile << "mtrk_event_error_t::errc::invalid_delta_time";
				break;
			case mtrk_event_error_t::errc::no_data_following_delta_time:
				outfile << "mtrk_event_error_t::errc::no_data_following_delta_time";
				break;
			case mtrk_event_error_t::errc::invalid_status_byte:
				outfile << "mtrk_event_error_t::errc::invalid_status_byte";
				break;
			case mtrk_event_error_t::errc::channel_calcd_length_exceeds_input:
				outfile << "mtrk_event_error_t::errc::channel_calcd_length_exceeds_input";
				break;
			case mtrk_event_error_t::errc::channel_invalid_data_byte:
				outfile << "mtrk_event_error_t::errc::channel_invalid_data_byte";
				break;
			case mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header:
				outfile << "mtrk_event_error_t::errc::sysex_or_meta_overflow_in_header";
				break;
			case mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length:
				outfile << "mtrk_event_error_t::errc::sysex_or_meta_invalid_vlq_length";
				break;
			case mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input:
				outfile << "mtrk_event_error_t::errc::sysex_or_meta_calcd_length_exceeds_input";
				break;
			case mtrk_event_error_t::errc::other:
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
		if (!jmid::has_midifile_extension(curr_path)) {
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

		jmid::mthd_error_t mthd_error {};
		auto it = std::istreambuf_iterator(f);
		auto mthd = jmid::make_mthd(it,std::istreambuf_iterator<unsigned char>(),&mthd_error);
		if (mthd) {
			jmid::print(mthd.mthd,s);
		} else {
			s += jmid::explain(mthd_error);
		}
		*out << s << '\n';
		*out << "==============================================="
			"=================================\n";

		f.close();
	}
	return 0;
}


int conversions() {

	auto dbl_is_aprx_eq = [](double a, double b, int nulp)->bool {
		auto e = std::numeric_limits<double>::epsilon();
		auto m = std::numeric_limits<double>::min();
		auto d = std::abs(a-b);
		auto s = std::abs(a+b);
		return (d <= e*s*nulp || d < m);
	};

	std::random_device rdev;
	std::default_random_engine re(rdev());
	std::uniform_int_distribution<int32_t> rdtk(0x0,0x0FFFFFFF);
	std::uniform_int_distribution<int32_t> rdtpo(1,0xFFFFFF);
	std::uniform_int_distribution<int32_t> rdtdiv(1,0x7FFF);
	std::uniform_int_distribution<int32_t> rdtpotdiv(1,512);

	std::fesetround(FE_TONEAREST);

	int nulps_eq = 1;
	int n_trials = 100000;
	double usps = 1000000.0;
	std::array<double,4> errs {0.0,0.0,0.0,0.0};
	std::array<double,4> nfails {0,0,0,0};
	double m1_err = 0.0;  double m2_err = 0.0;  double m3_err = 0.0;  double m4_err = 0.0;
	int m1_nfails = 0;  int m2_nfails = 0;  int m3_nfails = 0;  int m4_nfails = 0;
	int m1_ninexact = 0;  int m2_ninexact = 0;  int m3_ninexact = 0;  int m4_ninexact = 0;
	for (int i=0; i<n_trials; ++i) {
		double tpo = static_cast<double>(rdtpo(re));
		double tdiv = static_cast<double>(rdtdiv(re));
		//double rtpotdiv = rdtpotdiv(re);
		//tpo = rtpotdiv*tdiv;
		double tk = static_cast<double>(rdtk(re));
		double z = (usps*tdiv)/tpo;

		// method 1
		std::feclearexcept(FE_ALL_EXCEPT);
		double m1_n = tpo*tk;  // (us/q)*tk
		if (std::fetestexcept(FE_INEXACT)) { ++m1_ninexact; }
		std::feclearexcept(FE_ALL_EXCEPT);
		double m1_d = tdiv*usps;  // (tk/q) * (us/s)
		if (std::fetestexcept(FE_INEXACT)) { ++m1_ninexact; }
		std::feclearexcept(FE_ALL_EXCEPT);
		double m1_s = m1_n/m1_d;  // (us/q)*tk*(q/tk)*(s/us)
		if (std::fetestexcept(FE_INEXACT)) { ++m1_ninexact; }
		std::feclearexcept(FE_ALL_EXCEPT);
		double m1_tk_back = m1_s*z;
		m1_err += std::abs(tk - m1_tk_back);
		m1_nfails += !dbl_is_aprx_eq(tk,m1_tk_back,nulps_eq);

		// method 2
		std::feclearexcept(FE_ALL_EXCEPT);
		double m2_spq = tpo/usps;  // us/q * s/us = s/q
		if (std::fetestexcept(FE_INEXACT)) { ++m2_ninexact; }
		std::feclearexcept(FE_ALL_EXCEPT);
		double m2_b = m2_spq*tk;  // s/q * tk = (s*tk)/q
		if (std::fetestexcept(FE_INEXACT)) { ++m2_ninexact; }
		std::feclearexcept(FE_ALL_EXCEPT);
		double m2_s = m2_b/tdiv;  // (s*tk)/q * q/tk = s
		if (std::fetestexcept(FE_INEXACT)) { ++m2_ninexact; }
		std::feclearexcept(FE_ALL_EXCEPT);
		double m2_tk_back = m2_s*z;
		m2_err += std::abs(tk - m2_tk_back);  // s*(tk/q) * (us/s)/(us/q) = s*(tk/q)*(us/s)*(q/us) = tk
		// m2_s*tdiv == m2_b == m2_a*tk == (tpo/usps)*tk
		m2_nfails += !dbl_is_aprx_eq(tk,m2_tk_back,nulps_eq);

		// method 3
		std::feclearexcept(FE_ALL_EXCEPT);
		double m3_tkspus = tk/usps;  // (tk*s)/us
		if (std::fetestexcept(FE_INEXACT)) { ++m3_ninexact; }
		std::feclearexcept(FE_ALL_EXCEPT);
		double m3_usptk = tpo/tdiv;  // 
		if (std::fetestexcept(FE_INEXACT)) { ++m3_ninexact; }
		std::feclearexcept(FE_ALL_EXCEPT);
		double m3_s = m3_tkspus*m3_usptk;  // s
		if (std::fetestexcept(FE_INEXACT)) { ++m3_ninexact; }
		std::feclearexcept(FE_ALL_EXCEPT);
		double m3_tk_back = m3_s*z;
		m3_err += std::abs(tk - m3_tk_back);
		m3_nfails += !dbl_is_aprx_eq(tk,m3_tk_back,nulps_eq);

		// method 4
		std::feclearexcept(FE_ALL_EXCEPT);
		double m4_a = tk*(1.0/usps);  // 
		if (std::fetestexcept(FE_INEXACT)) { ++m4_ninexact; }
		std::feclearexcept(FE_ALL_EXCEPT);
		double m4_b = tpo*(1.0/tdiv);  // 
		if (std::fetestexcept(FE_INEXACT)) { ++m4_ninexact; }
		std::feclearexcept(FE_ALL_EXCEPT);
		double m4_s = m4_a*m4_b;  // s
		if (std::fetestexcept(FE_INEXACT)) { ++m4_ninexact; }
		std::feclearexcept(FE_ALL_EXCEPT);
		double m4_tk_back = m4_s*z;
		m4_err += std::abs(tk - m4_tk_back);
		m4_nfails += !dbl_is_aprx_eq(tk,m4_tk_back,nulps_eq);
	}
	std::cout 
		<< "\nm1_nfails== " << m1_nfails << "\tm1_ninexact== " << m1_ninexact << "\tm1_err== " << m1_err
		<< "\nm2_nfails== " << m2_nfails << "\tm2_ninexact== " << m2_ninexact << "\tm2_err== " << m2_err
		<< "\nm3_nfails== " << m3_nfails << "\tm3_ninexact== " << m3_ninexact << "\tm3_err== " << m3_err
		<< "\nm4_nfails== " << m4_nfails << "\tm4_ninexact== " << m4_ninexact << "\tm4_err== " << m4_err
		<< std::endl;
	
	return 0;
}

int ratiosfp() {

	/*auto dbl_is_aprx_eq = [](double a, double b, int nulp)->bool {
		auto e = std::numeric_limits<double>::epsilon();
		auto m = std::numeric_limits<double>::min();
		auto d = std::abs(a-b);
		auto s = std::abs(a+b);
		return (d <= e*s*nulp || d < m);
	};*/

	std::random_device rdev;
	std::default_random_engine re(rdev());
	std::uniform_int_distribution<int32_t> rdn(0x0,0xFFFFFF);
	std::uniform_int_distribution<int32_t> rdd(1,0x7FFF);

	std::fesetround(FE_TONEAREST);

	int n_trials = 1'000'000;
	int n_inexact = 0;
	double err = 0.0;
	for (int i=0; i<n_trials; ++i) {
		double n = static_cast<double>(rdn(re));
		double d = static_cast<double>(rdd(re));

		std::feclearexcept(FE_ALL_EXCEPT);
		double r = n/d;
		if (std::fetestexcept(FE_INEXACT)) {
			++n_inexact;
			//std::cout << n_inexact;
		} else {
			std::cout << "n=="<<n<<";d=="<<d<<"\n";
		}
		double n_back = r*d;
		err += std::abs(n_back-n);
	}
	double p_inexact = static_cast<double>(n_inexact)/static_cast<double>(n_trials);
	std::cout << "100*p_inexact == " << 100*p_inexact 
		<< "\terr == " << err << std::endl;

	return n_inexact;
}



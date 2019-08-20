#include "vlq_benchmark.h"
#include "midi_vlq.h"
#include "midi_vlq_deprecated.h"
#include <iostream>
#include <random>
#include <chrono>
#include <vector>
#include <array>
#include <cstddef>
#include <cstdint>
#include <regex>
#include <array>
#include <string>

struct opts_t {
	int64_t N;
	int64_t N_rpts;
};
opts_t get_options(int, char**);

int main (int argc, char *argv[]) {
	auto opts = get_options(argc,argv);
	std::cout << "Running vlq-writer benchmark with:\n"
		<< "\tN == " << opts.N << " random numbers.\n"
		<< "\tN_rpts == " << opts.N_rpts << " repititions of each routine\n"
		<< "\tstd::uniform_int_distribution<uint32_t> rd(0u,0xFFFFFFFFu)\n"
		<< std::endl;

	std::cout << "Generating random numbers..." << std::endl;
	std::random_device rdev;
	std::default_random_engine re(rdev());
	std::uniform_int_distribution<uint32_t> rd(0u,0xFFFFFFFFu);
	std::vector<uint32_t> rints(opts.N);
	for (int64_t i=0; i<rints.size(); ++i) {
		rints[i] = rd(re);
	};

	// For each repeat opts.N_rpts, each of the 3 functions is run exactly
	// once.  The order each function is run in each set of repeats is 
	// randomized as dictated by test_order.  
	std::array<int,3> fest_func_idx {0,1,2};
	std::vector<int> test_order;
	for (int i=0; i<opts.N_rpts; ++i) {
		std::shuffle(fest_func_idx.begin(),fest_func_idx.end(),re);
		for (const auto& idx : fest_func_idx) {
			test_order.push_back(idx);
		}
	}

	std::cout << "Starting runs..." << std::endl;
	std::array<unsigned char,8> dest;
	uint64_t result_sum = 0;
	int64_t tot_ms_new = 0;  int64_t tot_ms_a = 0;  int64_t tot_ms_b = 0;
	for (const auto& idx : test_order) {
		if (idx==0) {
			std::cout << "\tStarting write_vlq():  ";
			auto tstart = std::chrono::high_resolution_clock::now();
			for (size_t i=0; i<opts.N; ++i) {
				write_vlq(rints[i],dest.data());
				result_sum += dest[2];
			}
			auto tend = std::chrono::high_resolution_clock::now();
			auto tdelta = std::chrono::duration_cast<std::chrono::milliseconds>(tend-tstart);
			std::cout << "Took " << tdelta.count() << " milliseconds." << std::endl;
			tot_ms_new += tdelta.count();
		} 
		if (idx==1) {
			std::cout << "\tStarting write_vlq_old_a():  ";
			auto tstart = std::chrono::high_resolution_clock::now();
			for (size_t i=0; i<opts.N; ++i) {
				write_vlq_old_a(rints[i],dest.data());
				result_sum += dest[2];
			}
			auto tend = std::chrono::high_resolution_clock::now();
			auto tdelta = std::chrono::duration_cast<std::chrono::milliseconds>(tend-tstart);
			std::cout << "Took  " << tdelta.count() << " milliseconds." << std::endl;
			tot_ms_a += tdelta.count();
		}
		if (idx==2) {
			std::cout << "\tStarting write_vlq_old_b():  ";
			auto tstart = std::chrono::high_resolution_clock::now();
			for (size_t i=0; i<opts.N; ++i) {
				write_vlq_old_b(rints[i],dest.data());
				result_sum += dest[2];
			}
			auto tend = std::chrono::high_resolution_clock::now();
			auto tdelta = std::chrono::duration_cast<std::chrono::milliseconds>(tend-tstart);
			std::cout << "Took  " << tdelta.count() << " milliseconds." << std::endl;
			tot_ms_b += tdelta.count();
		}
	}
	std::cout << "------------------------------------\n";

	std::cout << "write_vlq():  " << tot_ms_new << " ms total\n";
	std::cout << "write_vlq_old_a():  " << tot_ms_a << " ms total\n";
	std::cout << "write_vlq_old_b():  " << tot_ms_b << " ms total\n";

	std::cout << "result_sum (ignore this) == " << result_sum << std::endl;

	return 0;
}


opts_t get_options(int argc, char **argv) {
	struct opts_type {
		std::regex rx;
		int64_t val;
		int64_t def_val;
	};
	std::array<opts_type,2> opts {{
		// The number of random numbers to generate
		{std::regex("-N=(\\d+)"),-1,10'000'000},
		// How many times each write_vlq procedure should be run through
		// the set of random numbers.  
		{std::regex("-rpt=(\\d+)"),-1,10}
	}};

	for (int i=0; i<argc; ++i) {
		for (int j=0; j<opts.size(); ++j) {
			std::cmatch curr_match;
			std::regex_match(argv[i],curr_match,opts[j].rx);
			if (curr_match.empty()) { continue; }
			opts[j].val = std::stol(curr_match[1].str());
			// Each argv[i] should match only one of the options in opts:
			break;
		}
	}

	opts_t result;
	result.N = opts[0].val;
	if (opts[0].val < 0) {
		result.N = opts[0].def_val;
	}
	result.N_rpts = opts[1].val;
	if (opts[1].val < 0) {
		result.N_rpts = opts[1].def_val;
	}
	return result;
}


#include "vlq_benchmark.h"
#include "midi_vlq.h"
#include "midi_vlq_deprecated.h"
#include "midi_examples.h"
#include <iostream>
#include <random>
#include <chrono>
#include <vector>
#include <array>

int main () {
	std::random_device rdev;
	std::default_random_engine re(rdev());
	std::uniform_int_distribution<uint32_t> rd(0u,0xFFFFFFFFu);
	
	const size_t N = 10'000'000;
	std::vector<uint32_t> rints(N);
	for (size_t i=0; i<rints.size(); ++i) {
		rints[i] = rd(re);
	};

	std::array<unsigned char,8> dest;
	uint64_t result_sum=0;
	std::cout << "benchmark_vlqs() benchmark\n";
	int n_new=0;  int n_old1 = 0;  int n_old = 0;
	while ((n_new<10) || (n_old1<10) || (n_old<10)) {
		if ((rd(re)%2==0) && n_new<10) {
			std::cout << "Starting write_vlq():  ";
			auto tstart = std::chrono::high_resolution_clock::now();
			for (size_t i=0; i<N; ++i) {
				write_vlq(rints[i],dest.data());
				result_sum += dest[2];
			}
			auto tend = std::chrono::high_resolution_clock::now();
			auto tdelta = std::chrono::duration_cast<std::chrono::milliseconds>(tend-tstart);
			std::cout << "Took " << tdelta.count() << " milliseconds." << std::endl;
			++n_new;
		} 
		if ((rd(re)%2==0) && n_old1<10) {
			std::cout << "Starting write_vlq_old1():  ";
			auto tstart = std::chrono::high_resolution_clock::now();
			for (size_t i=0; i<N; ++i) {
				write_vlq_old1(rints[i],dest.data());
				result_sum += dest[2];
			}
			auto tend = std::chrono::high_resolution_clock::now();
			auto tdelta = std::chrono::duration_cast<std::chrono::milliseconds>(tend-tstart);
			std::cout << "Took  " << tdelta.count() << " milliseconds." << std::endl;
			++n_old1;
		}
		if ((rd(re)%2==0) && n_old<10) {
			std::cout << "Starting write_vlq_old():  ";
			auto tstart = std::chrono::high_resolution_clock::now();
			for (size_t i=0; i<N; ++i) {
				write_vlq_old(rints[i],dest.data());
				result_sum += dest[2];
			}
			auto tend = std::chrono::high_resolution_clock::now();
			auto tdelta = std::chrono::duration_cast<std::chrono::milliseconds>(tend-tstart);
			std::cout << "Took  " << tdelta.count() << " milliseconds." << std::endl;
			++n_old;
		}

		std::cout << "------------------------------------" << std::endl;
	}

	std::cout << result_sum << std::endl;

	return 0;
}

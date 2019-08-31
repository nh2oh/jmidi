#include "util.h"
#include <filesystem>
#include <vector>
#include <cstdio>
#include <random>
#include <string>


// TODO:  Test cases w/ inputs containing 0x1A, input w/ spaces preceeding an \n
// TODO:  Test cases w/ path's that do not convert to std::string<char> w/o 
// throwing.  
std::size_t jmid::read_binary_csio(std::filesystem::path pth, std::vector<char>& dest) {
	auto fp = std::fopen(pth.string().c_str(), "rb");
	if (!fp) {
		return 0;
	}
	auto nread = std::fread(dest.data(), sizeof(char), dest.size(), fp);
	std::fclose(fp);
	return nread;
}


std::string jmid::randstr(int n) {
	std::string s;
	if (n<=0) {
		return s;
	}
	std::random_device rdev;
	std::default_random_engine re(rdev());
	std::uniform_int_distribution<int> rd(0,24);

	for (int i=0; i<n; ++i) {
		s.push_back('A' + rd(re));
	}
	return s;
}

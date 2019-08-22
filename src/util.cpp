#include "util.h"
#include <filesystem>
#include <vector>
#include <cstdio>
#include <random>
#include <string>

bool read_binary_csio(std::filesystem::path pth, std::vector<char>& dest) {
	auto fp = std::fopen(pth.string().c_str(), "r");
	if (!fp) {
		return false;
	}
	auto nread = std::fread(dest.data(), sizeof(char), dest.size(), fp);

	std::fclose(fp);
	return nread==dest.size();
}


std::string randstr(int n) {
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

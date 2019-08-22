#include "util.h"
#include <filesystem>
#include <vector>
#include <cstdio>

bool read_binary_csio(std::filesystem::path pth, std::vector<char>& dest) {
	auto fp = std::fopen(pth.string().c_str(), "r");
	if (!fp) {
		return false;
	}
	auto nread = std::fread(dest.data(), sizeof(char), dest.size(), fp);

	std::fclose(fp);
	return nread==dest.size();
}

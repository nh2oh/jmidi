#pragma once
#include <filesystem>
#include <vector>
#include <string>


bool read_binary_csio(std::filesystem::path, std::vector<char>&);

// A random string w/ n capital letters
std::string randstr(int);


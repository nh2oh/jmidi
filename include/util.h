#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include <cstddef>  // std::size_t

namespace jmid {

// bool read_binary_csio(std::filesystem::path pth, 
//							std::vector<char>& dest)
// Uses C-style I/O (std::fopen, std::fread()) to read a file into dest.  
// Reads at most dest.size() bytes.  Returns true if exactly dest.size()
// bytes were read; false otherwise.  
std::size_t read_binary_csio(std::filesystem::path, std::vector<char>&);

// Generate a random string w/ n capital letters.  Instantiates its own 
// local std::random_device and random_engine, thus, is expensive to 
// call.  
std::string randstr(int);

}  // namespace jmid

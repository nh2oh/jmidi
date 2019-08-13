#pragma once
#include <filesystem>
#include <string>
#include <vector>

struct midi_test_stuff_t {
	std::string name;
	std::filesystem::path inp;
	std::filesystem::path outp;
};
struct inout_dirs_t {
	std::filesystem::path inp;
	std::filesystem::path outp;
};
inout_dirs_t get_midi_test_dirs(const std::string&);
extern const std::vector<midi_test_stuff_t> midi_test_dirs;


int midi_example();

int read_midi_directory(const std::filesystem::path&);
int inspect_mthds(const std::filesystem::path&, const std::filesystem::path&);

int event_sizes_benchmark();
int avg_and_max_event_sizes(const std::filesystem::path&,
	const std::filesystem::path&, const int);

int classify_smf_errors(const std::filesystem::path&,
	const std::filesystem::path&);

int conversions();
int ratiosfp();

std::filesystem::path make_midifile(std::filesystem::path, bool);

std::string randfn();



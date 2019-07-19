#pragma once
#include <filesystem>

int midi_example();
int raw_bytes_as_midi_file();

int midi_mtrk_split_testing();
int midi_setdt_testing();

int midi_clamped_value_testing();
int read_midi_directory(const std::filesystem::path&);
int read_midi_directory_mthd_inspection(const std::filesystem::path&);
int avg_and_max_event_sizes(const std::filesystem::path&);

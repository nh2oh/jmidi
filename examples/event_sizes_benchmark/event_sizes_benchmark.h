#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

struct opts_t {
	int Nth;
	int mode;
};
opts_t get_options(int, char**);

int event_sizes_benchmark(int, int, const std::filesystem::path&);
int avg_and_max_event_sizes(const std::vector<std::filesystem::path>&,
				const std::filesystem::path&, const int);


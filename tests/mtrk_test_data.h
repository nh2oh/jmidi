#pragma once
#include "mtrk_event_t.h"
#include <vector>
#include <cstdint>


namespace mtrk_tests {

struct mtrk_properties_t {
	int32_t size {0};  // nevents
	int32_t duration_tks {0};
};

struct eventdata_with_cumtk_t {
	std::vector<unsigned char> d {};
	int32_t cumtk {};
};
// Test set a
// Created June 09 2019
extern std::vector<eventdata_with_cumtk_t> tsa;
extern mtrk_properties_t tsa_props;

// Test set b
// Created June 15, 2019
struct tsb_t {
	std::vector<unsigned char> d {};
	int32_t cumtk {};
	int32_t tkonset {};
};
extern std::vector<tsb_t> tsb;
extern std::vector<tsb_t> tsb_note_67_events;
extern std::vector<tsb_t> tsb_non_note_67_events;
extern std::vector<tsb_t> tsb_meta_events;
extern std::vector<tsb_t> tsb_non_meta_events;

};  // namespace mtrk_tests


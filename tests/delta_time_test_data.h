#pragma once
#include <vector>
#include <array>
#include <cstdint>

// Delta-time test data
// Set A:  Positive values, valid and invalid [0, limits<int32_t>::max()]
// Set B:  Negative values [limits<int32_t>::min(), 0]
// Set C:  Values from sets A & B interspersed with eachother.  Used for
// testing repeated calls to set_delta_time()-like functions.  
// Set D:  The invalid values from Sets A & B.  
// Set D:  The valid values from Set A.  
struct dt_test_data_t {
	int32_t dt_input;
	int ans_n_bytes;  // number of bytes occupied by the vlq-encoded input
	int32_t ans_value;  // inputs > 0x0FFFFFFFu are clamped to 0x0FFFFFFFu
};
extern const std::vector<dt_test_data_t> dt_test_set_a;
extern const std::vector<dt_test_data_t> dt_test_set_b;
extern const std::vector<dt_test_data_t> dt_test_set_c;
extern const std::vector<dt_test_data_t> dt_test_set_d;
extern const std::vector<dt_test_data_t> dt_test_set_e;

// Manually encoded fields
// Set A:  Valid fields w/ the most-compact encoding
// Set B:  Values on [0,max-allowed-dt]; fields are _not_ encoded in the
// most compact way possible.  
// Set C:  Overlong sequences
struct dt_fields_t {
	std::array<unsigned char,8> field {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	int32_t val {0};
	int n_bytes {0};
};
extern const std::vector<dt_fields_t> dt_fields_test_set_a;
extern const std::vector<dt_fields_t> dt_fields_test_set_b;
extern const std::vector<dt_fields_t> dt_fields_test_set_c;



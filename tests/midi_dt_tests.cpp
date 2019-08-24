#include "gtest/gtest.h"
#include "delta_time_test_data.h"
#include "midi_delta_time.h"
#include <array>


//
// bool jmid::is_valid_delta_time(int32_t dt_val);
//
// A selection of valid and invalid dt values.  
TEST(delta_time_tests, IsValidDeltaTime) {
	// Valid
	for (const auto& tc : dt_fields_test_set_a) {
		EXPECT_TRUE(jmid::is_valid_delta_time(tc.val));
	}
	// Invalid
	for (const auto& tc : dt_test_set_d) {
		EXPECT_FALSE(jmid::is_valid_delta_time(tc.dt_input));
	}
	EXPECT_FALSE(jmid::is_valid_delta_time(0x0FFFFFFF+1));
	// Largest value of an int32_t; 0x7FFFFFFF+1 overflows
	EXPECT_FALSE(jmid::is_valid_delta_time(0x7FFFFFFF));  
}


//
// int32_t jmid::to_nearest_valid_delta_time(int32_t dt_val);
//
// A selection of valid and invalid dt values.  
TEST(delta_time_tests, ToNearestValidDeltaTime) {
	// Valid & invalid
	for (const auto& tc : dt_test_set_a) {
		EXPECT_EQ(jmid::to_nearest_valid_delta_time(tc.dt_input),tc.ans_value);
	}
	// All are invalid (<0)
	for (const auto& tc : dt_test_set_d) {
		EXPECT_EQ(jmid::to_nearest_valid_delta_time(tc.dt_input),tc.ans_value);
	}
}


//
// int32_t jmid::delta_time_field_size(int32_t dt_val);
//
// A selection of valid and invalid dt values.  
TEST(delta_time_tests, DeltaTimeFieldSize) {
	// Valid & invalid
	for (const auto& tc : dt_test_set_a) {
		EXPECT_EQ(jmid::delta_time_field_size(tc.dt_input),tc.ans_n_bytes);
	}
	// All are invalid (<0)
	for (const auto& tc : dt_test_set_d) {
		EXPECT_EQ(jmid::delta_time_field_size(tc.dt_input),tc.ans_n_bytes);
	}
}


//
// template<typename OIt>
// OIt jmid::write_delta_time(int32_t val, OIt it)
//
// All input values are >= 0, but some exceed the max value allowed for a
// delta time, 0x0FFFFFFF.  These inputs are clamped to 0x0FFFFFFF.  
TEST(delta_time_tests, WriteDtPositiveValues) {
	std::array<unsigned char,8> field;
	for (const auto& tc : dt_test_set_a) {
		// Write
		field.fill(0x00u);
		auto it = jmid::write_delta_time(tc.dt_input,field.begin());
		EXPECT_EQ(it-field.begin(),tc.ans_n_bytes);

		// Read
		auto dt = jmid::read_delta_time(field.begin(),field.end());
		EXPECT_EQ(dt.val,tc.ans_value);
		EXPECT_EQ(dt.N,tc.ans_n_bytes);
		EXPECT_TRUE(dt.is_valid);
	}
}


//
// template<typename OIt>
// OIt jmid::write_delta_time(int32_t val, OIt it)
//
// All input values are negative.  Because negative values are invalid for
// delta-times, the value written should be the closest valid value, 0.  
TEST(delta_time_tests, WriteDtNegativeValues) {
	std::array<unsigned char,8> field;
	for (const auto& tc : dt_test_set_b) {
		// Write
		field.fill(0x00u);
		auto it = jmid::write_delta_time(tc.dt_input,field.begin());
		EXPECT_EQ(it-field.begin(),tc.ans_n_bytes);

		// Read
		auto dt = jmid::read_delta_time(field.begin(),field.end());
		EXPECT_EQ(dt.val,tc.ans_value);
		EXPECT_EQ(dt.N,tc.ans_n_bytes);
		EXPECT_TRUE(dt.is_valid);
	}
}


//
// template<typename InIt>
// dt_field_interpreted jmid::read_delta_time(InIt beg, InIt end);
//
// template<typename OIt>
// OIt jmid::write_delta_time(int32_t val, OIt it)
//
TEST(delta_time_tests, ReadWriteDtAllValuesValid) {
	std::array<unsigned char,8> field;
	for (const auto& tc : dt_fields_test_set_a) {
		// Read
		auto dt = jmid::read_delta_time(tc.field.begin(),tc.field.end());
		EXPECT_EQ(dt.val,tc.val);
		EXPECT_EQ(dt.N,tc.n_bytes);
		EXPECT_TRUE(dt.is_valid);
		
		// Write
		field.fill(0x00u);
		auto it = jmid::write_delta_time(tc.val,field.begin());
		EXPECT_EQ(it-field.begin(),tc.n_bytes);
		ASSERT_EQ(tc.field.size(),field.size());
		for (int i=0; i<field.size(); ++i) {
			bool b = tc.field[i]==field[i];
			EXPECT_EQ(tc.field[i],field[i]);
		}
	}
}


//
// template<typename InIt>
// dt_field_interpreted jmid::read_delta_time(InIt beg, InIt end);
//
TEST(delta_time_tests, ReadDtFieldsContainingUnormalizedData) {
	for (const auto& tc : dt_fields_test_set_b) {
		// Read
		auto dt = jmid::read_delta_time(tc.field.begin(),tc.field.end());
		EXPECT_EQ(dt.val,tc.val);
		EXPECT_EQ(dt.N,tc.n_bytes);
		EXPECT_TRUE(dt.is_valid);
	}
}


//
// template<typename InIt>
// dt_field_interpreted jmid::read_delta_time(InIt beg, InIt end);
//
TEST(delta_time_tests, ReadDtFieldsContainingOverlongData) {
	for (const auto& tc : dt_fields_test_set_c) {
		// Read
		auto dt = jmid::read_delta_time(tc.field.begin(),tc.field.end());
		EXPECT_EQ(dt.val,tc.val);
		EXPECT_EQ(dt.N,4);
		EXPECT_FALSE(dt.is_valid);
	}
}


//
// template<typename InIt>
// dt_field_interpreted jmid::read_delta_time(InIt beg, InIt end);
//
// Truncate the final byte of the input field by passing a shortened range
// to jmid::read_delta_time().  Expect dt_field_interpreted.is_valid == false.  
TEST(delta_time_tests, ReadDtTruncateField) {
	for (const auto& tc : dt_fields_test_set_a) {
		if (tc.n_bytes<=1) {
			continue;
		}
		// Read, but truncate the final byte
		auto dt = jmid::read_delta_time(tc.field.begin(),
			tc.field.begin()+tc.n_bytes-1);
		EXPECT_EQ(dt.N,tc.n_bytes-1);
		EXPECT_FALSE(dt.is_valid);
	}
}


//
// template<typename InIt>
// InIt jmid::advance_to_dt_end(InIt beg, InIt end)
//
TEST(delta_time_tests, AdvanceToDtEnd) {
	for (const auto& tc : dt_fields_test_set_a) {
		auto it = jmid::advance_to_dt_end(tc.field.begin(),tc.field.end());
		EXPECT_EQ(it-tc.field.begin(),tc.n_bytes);
	}
	for (const auto& tc : dt_fields_test_set_b) {
		auto it = jmid::advance_to_dt_end(tc.field.begin(),tc.field.end());
		EXPECT_EQ(it-tc.field.begin(),tc.n_bytes);
	}
	std::array<unsigned char,6> overlong {0x80,0x80,0x80,0x80,0x80,0x80};
	auto it = jmid::advance_to_dt_end(overlong.begin(),overlong.end());
	EXPECT_EQ(it-overlong.begin(),4);
}





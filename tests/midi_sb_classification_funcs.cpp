#include "gtest/gtest.h"
#include "midi_status_byte.h"
#include "midi_testdata_status_bytes.h"



TEST(status_and_data_byte_classification, IsStatusByte) {
	for (const auto& e : sbs_invalid) {
		EXPECT_FALSE(is_status_byte(e));
	}
	for (const auto& e : sbs_unrecognized) {
		EXPECT_TRUE(is_status_byte(e));
	}
	for (const auto& e : sbs_meta_sysex) {
		EXPECT_TRUE(is_status_byte(e));
	}
	for (const auto& e : sbs_ch_mode_voice) {
		EXPECT_TRUE(is_status_byte(e));
	}
}


TEST(status_and_data_byte_classification, IsUnrecognizedStatusByte) {
	for (const auto& e : sbs_invalid) {
		EXPECT_FALSE(is_unrecognized_status_byte(e));
	}
	for (const auto& e : sbs_unrecognized) {
		EXPECT_TRUE(is_unrecognized_status_byte(e));
	}
	for (const auto& e : sbs_meta_sysex) {
		EXPECT_FALSE(is_unrecognized_status_byte(e));
	}
	for (const auto& e : sbs_ch_mode_voice) {
		EXPECT_FALSE(is_unrecognized_status_byte(e));
	}
}


TEST(status_and_data_byte_classification, IsChannelStatusByte) {
	for (const auto& e : sbs_invalid) {
		EXPECT_FALSE(is_channel_status_byte(e));
	}
	for (const auto& e : sbs_unrecognized) {
		EXPECT_FALSE(is_channel_status_byte(e));
	}
	for (const auto& e : sbs_meta_sysex) {
		EXPECT_FALSE(is_channel_status_byte(e));
	}
	for (const auto& e : sbs_ch_mode_voice) {
		EXPECT_TRUE(is_channel_status_byte(e));
	}
}


TEST(status_and_data_byte_classification, IsSysexStatusByte) {
	for (const auto& e : sbs_invalid) {
		EXPECT_FALSE(is_sysex_status_byte(e));
	}
	for (const auto& e : sbs_unrecognized) {
		EXPECT_FALSE(is_sysex_status_byte(e));
	}
	for (const auto& e : sbs_ch_mode_voice) {
		EXPECT_FALSE(is_sysex_status_byte(e));
	}
	EXPECT_TRUE(is_sysex_status_byte(0xF0u));
	EXPECT_TRUE(is_sysex_status_byte(0xF7u));
	EXPECT_FALSE(is_sysex_status_byte(0xFFu));
}


TEST(status_and_data_byte_classification, IsMetaStatusByte) {
	for (const auto& e : sbs_invalid) {
		EXPECT_FALSE(is_meta_status_byte(e));
	}
	for (const auto& e : sbs_unrecognized) {
		EXPECT_FALSE(is_meta_status_byte(e));
	}
	for (const auto& e : sbs_ch_mode_voice) {
		EXPECT_FALSE(is_meta_status_byte(e));
	}
	EXPECT_TRUE(is_meta_status_byte(0xFFu));
	EXPECT_FALSE(is_meta_status_byte(0xF0u));
	EXPECT_FALSE(is_meta_status_byte(0xF7u));
}


TEST(status_and_data_byte_classification, IsSysexOrMetaStatusByte) {
	for (const auto& e : sbs_invalid) {
		EXPECT_FALSE(is_sysex_or_meta_status_byte(e));
	}
	for (const auto& e : sbs_unrecognized) {
		EXPECT_FALSE(is_sysex_or_meta_status_byte(e));
	}
	for (const auto& e : sbs_ch_mode_voice) {
		EXPECT_FALSE(is_sysex_or_meta_status_byte(e));
	}
	for (const auto& e : sbs_meta_sysex) {
		EXPECT_TRUE(is_sysex_or_meta_status_byte(e));
	}
}


TEST(status_and_data_byte_classification, IsDataByte) {
	for (const auto& e : dbs_valid) {
		EXPECT_TRUE(is_data_byte(e));
	}
	for (const auto& e : dbs_invalid) {
		EXPECT_FALSE(is_data_byte(e));
	}
	for (const auto& e : sbs_unrecognized) {
		EXPECT_FALSE(is_data_byte(e));
	}
	for (const auto& e : sbs_meta_sysex) {
		EXPECT_FALSE(is_data_byte(e));
	}
	for (const auto& e : sbs_ch_mode_voice) {
		EXPECT_FALSE(is_data_byte(e));
	}
}


TEST(status_and_data_byte_classification, ClassifyStatusByteSingleArg) {
	for (const auto& e : sbs_invalid) {
		EXPECT_EQ(classify_status_byte(e),status_byte_type::invalid);
	}
	for (const auto& e : sbs_unrecognized) {
		EXPECT_EQ(classify_status_byte(e),status_byte_type::unrecognized);
	}
	for (const auto& e : sbs_ch_mode_voice) {
		EXPECT_EQ(classify_status_byte(e),status_byte_type::channel);
	}
	EXPECT_EQ(classify_status_byte(0xFFu),status_byte_type::meta);
	EXPECT_EQ(classify_status_byte(0xF7u),status_byte_type::sysex_f7);
	EXPECT_EQ(classify_status_byte(0xF0u),status_byte_type::sysex_f0);

	for (const auto& e : dbs_valid) {
		EXPECT_EQ(classify_status_byte(e),status_byte_type::invalid);
	}
}


TEST(status_and_data_byte_classification, ClassifyStatusByteTwoArgInvalidSBASLocal) {
	// sbs_invalid all qualify as data bytes; a byte such as 0xF1
	// is a "valid but unrecognized" status byte thus is not a member
	// of sbs_invalid.  Thus, none of the elements in sbs_invalid 
	// reset or take precedence over the running status.  
	for (const auto& loc : sbs_invalid) {
		// Valid rs
		for (const auto& rs : sbs_ch_mode_voice) {
			EXPECT_EQ(classify_status_byte(loc,rs),status_byte_type::channel);
		}
		// meta and sysex sbs are invalid as running-status bytes and never
		// take precedence over the local byte, even when the local byte
		// is invalid as a status byte.  Same deal w/ sbs_unrecognized.  
		for (const auto& rs : sbs_meta_sysex) {
			EXPECT_EQ(classify_status_byte(loc,rs),status_byte_type::invalid);
		}
		for (const auto& rs : sbs_unrecognized) {
			EXPECT_EQ(classify_status_byte(loc,rs),status_byte_type::invalid);
		}
		for (const auto& rs : dbs_valid) {
			EXPECT_EQ(classify_status_byte(loc,rs),status_byte_type::invalid);
		}
		for (const auto& rs : sbs_invalid) {
			EXPECT_EQ(classify_status_byte(loc,rs),status_byte_type::invalid);
		}
	}
}


TEST(status_and_data_byte_classification, ClassifyStatusByteTwoArgValidChSBAsLocal) {
	// the local sb always wins; rs is irrelevant
	for (const auto& loc : sbs_ch_mode_voice) {
		for (const auto& rs : sbs_meta_sysex) {
			EXPECT_EQ(classify_status_byte(loc,rs),status_byte_type::channel);
		}
		for (const auto& rs : sbs_unrecognized) {
			EXPECT_EQ(classify_status_byte(loc,rs),status_byte_type::channel);
		}
		for (const auto& rs : dbs_valid) {
			EXPECT_EQ(classify_status_byte(loc,rs),status_byte_type::channel);
		}
		for (const auto& rs : sbs_invalid) {
			EXPECT_EQ(classify_status_byte(loc,rs),status_byte_type::channel);
		}
	}
}


TEST(status_and_data_byte_classification, ClassifyStatusByteTwoArgValidSysexMetaSBAsLocal) {
	// All events are sysex or meta.  rs is irrelevant
	for (const auto& loc : sbs_meta_sysex) {
		// The single arg version was verified in a prev. test
		auto t = classify_status_byte(loc);  
		for (const auto& rs : sbs_ch_mode_voice) {
			EXPECT_EQ(classify_status_byte(loc,rs),t);
		}
		for (const auto& rs : sbs_meta_sysex) {
			EXPECT_EQ(classify_status_byte(loc,rs),t);
		}
		for (const auto& rs : sbs_unrecognized) {
			EXPECT_EQ(classify_status_byte(loc,rs),t);
		}
		for (const auto& rs : dbs_valid) {
			EXPECT_EQ(classify_status_byte(loc,rs),t);
		}
		for (const auto& rs : sbs_invalid) {
			EXPECT_EQ(classify_status_byte(loc,rs),t);
		}
	}
}

TEST(status_and_data_byte_classification, GetStatusLocalSbIsMetaSysex) {
	// With a valid event-local status byte 'loc', returns loc, no matter
	// what is passed in as the running-status.  
	for (const auto& loc : sbs_meta_sysex) {
		for (const auto& rs : sbs_ch_mode_voice) {
			EXPECT_EQ(get_status_byte(loc,rs),loc);
		}
		for (const auto& rs : sbs_meta_sysex) {
			EXPECT_EQ(get_status_byte(loc,rs),loc);
		}
		for (const auto& rs : sbs_unrecognized) {
			EXPECT_EQ(get_status_byte(loc,rs),loc);
		}
		for (const auto& rs : dbs_valid) {
			EXPECT_EQ(get_status_byte(loc,rs),loc);
		}
		for (const auto& rs : sbs_invalid) {
			EXPECT_EQ(get_status_byte(loc,rs),loc);
		}
	}
}


TEST(status_and_data_byte_classification, GetStatusLocalSbIsChannel) {
	// With a valid event-local status byte 'loc', returns loc, no matter
	// what is passed in as the running-status.  
	for (const auto& loc : sbs_ch_mode_voice) {
		for (const auto& rs : sbs_ch_mode_voice) {
			EXPECT_EQ(get_status_byte(loc,rs),loc);
		}
		for (const auto& rs : sbs_meta_sysex) {
			EXPECT_EQ(get_status_byte(loc,rs),loc);
		}
		for (const auto& rs : sbs_unrecognized) {
			EXPECT_EQ(get_status_byte(loc,rs),loc);
		}
		for (const auto& rs : dbs_valid) {
			EXPECT_EQ(get_status_byte(loc,rs),loc);
		}
		for (const auto& rs : sbs_invalid) {
			EXPECT_EQ(get_status_byte(loc,rs),loc);
		}
	}
}


TEST(status_and_data_byte_classification, GetStatusLocalSbIsData) {
	// With a data byte passed in as the first arg, will return 0x00u 
	// if anything other than a valid channel status byte is passed as
	// the running-status.  In this latter case, returns the rs byte.  
	for (const auto& loc : dbs_valid) {
		for (const auto& rs : sbs_ch_mode_voice) {
			EXPECT_EQ(get_status_byte(loc,rs),rs);
		}
		for (const auto& rs : sbs_meta_sysex) {
			EXPECT_EQ(get_status_byte(loc,rs),0x00u);
		}
		for (const auto& rs : sbs_unrecognized) {
			EXPECT_EQ(get_status_byte(loc,rs),0x00u);
		}
		for (const auto& rs : dbs_valid) {
			EXPECT_EQ(get_status_byte(loc,rs),0x00u);
		}
		for (const auto& rs : sbs_invalid) {
			EXPECT_EQ(get_status_byte(loc,rs),0x00u);
		}
	}
}


TEST(status_and_data_byte_classification, GetRSLocalSbIsMetaSysex) {
	// With an event-local status byte 'loc' => sysex or meta, the 
	// running status returned is always 0x00u, no matter what is passed in
	// for rs.  
	for (const auto& loc : sbs_meta_sysex) {
		for (const auto& rs : sbs_ch_mode_voice) {
			EXPECT_EQ(get_running_status_byte(loc,rs),0x00u);
		}
		for (const auto& rs : sbs_meta_sysex) {
			EXPECT_EQ(get_running_status_byte(loc,rs),0x00u);
		}
		for (const auto& rs : sbs_unrecognized) {
			EXPECT_EQ(get_running_status_byte(loc,rs),0x00u);
		}
		for (const auto& rs : dbs_valid) {
			EXPECT_EQ(get_running_status_byte(loc,rs),0x00u);
		}
		for (const auto& rs : sbs_invalid) {
			EXPECT_EQ(get_running_status_byte(loc,rs),0x00u);
		}
	}
}


TEST(status_and_data_byte_classification, GetRSLocalSbIsChannel) {
	// With an event-local status byte 'loc' => channel, the running
	// status returned is always == loc, no matter what is passed in
	// for rs.  
	for (const auto& loc : sbs_ch_mode_voice) {
		for (const auto& rs : sbs_ch_mode_voice) {
			EXPECT_EQ(get_running_status_byte(loc,rs),loc);
		}
		for (const auto& rs : sbs_meta_sysex) {
			EXPECT_EQ(get_running_status_byte(loc,rs),loc);
		}
		for (const auto& rs : sbs_unrecognized) {
			EXPECT_EQ(get_running_status_byte(loc,rs),loc);
		}
		for (const auto& rs : dbs_valid) {
			EXPECT_EQ(get_running_status_byte(loc,rs),loc);
		}
		for (const auto& rs : sbs_invalid) {
			EXPECT_EQ(get_running_status_byte(loc,rs),loc);
		}
	}
}


TEST(status_and_data_byte_classification, GetRSLocalSbIsDataByte) {
	// With a data byte passed in as the first arg, will return the 
	// value of rs only if rs is a channel status byte; otherwise, 
	// returns 0x00u
	for (const auto& loc : dbs_valid) {
		for (const auto& rs : sbs_ch_mode_voice) {
			EXPECT_EQ(get_running_status_byte(loc,rs),rs);
		}
		for (const auto& rs : sbs_meta_sysex) {
			EXPECT_EQ(get_running_status_byte(loc,rs),0x00u);
		}
		for (const auto& rs : sbs_unrecognized) {
			EXPECT_EQ(get_running_status_byte(loc,rs),0x00u);
		}
		for (const auto& rs : dbs_valid) {
			EXPECT_EQ(get_running_status_byte(loc,rs),0x00u);
		}
		for (const auto& rs : sbs_invalid) {
			EXPECT_EQ(get_running_status_byte(loc,rs),0x00u);
		}
	}
}




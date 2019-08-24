#include "gtest/gtest.h"
#include "mtrk_t.h"
#include "mtrk_event_t.h"
#include <string>
#include <utility>


TEST(mtrk_t_tests, DefaultCtor) {
	mtrk_t mtrk;
	EXPECT_EQ(mtrk.size(),0);
	EXPECT_EQ(mtrk.nticks(),0);
	EXPECT_EQ(mtrk.begin(),mtrk.end());
}


TEST(mtrk_t_tests, CopyCtor) {
	mtrk_t mtrk1;
	mtrk1.push_back(jmid::make_text(0,"This is a big text event that exceeds the "
		"local-object buffer of the mtrk_event_t."));
	mtrk1.push_back(jmid::make_ch_event(0,0x90,0,60,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x90,0,64,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x90,0,67,63));
	mtrk1.push_back(jmid::make_ch_event(100,0x80,0,60,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x80,0,64,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x80,0,67,63));
	mtrk1.push_back(jmid::make_eot(100));

	EXPECT_EQ(mtrk1.size(),8);
	EXPECT_EQ(mtrk1.nticks(),200);

	mtrk_t mtrk2 = mtrk1;

	EXPECT_EQ(mtrk1.size(),8);
	EXPECT_EQ(mtrk1.nticks(),200);
	EXPECT_EQ(mtrk2.size(),8);
	EXPECT_EQ(mtrk2.nticks(),200);

	for (int i=0; i<mtrk1.size(); ++i) {
		EXPECT_EQ(mtrk1[i],mtrk2[i]);
	}
}

TEST(mtrk_t_tests, CopyAssign) {
	mtrk_t mtrk1;
	mtrk1.push_back(jmid::make_text(0,"This is a big text event that exceeds the "
		"local-object buffer of the mtrk_event_t."));
	mtrk1.push_back(jmid::make_ch_event(0,0x90,0,60,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x90,0,64,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x90,0,67,63));
	mtrk1.push_back(jmid::make_ch_event(100,0x80,0,60,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x80,0,64,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x80,0,67,63));
	mtrk1.push_back(jmid::make_eot(100));

	EXPECT_EQ(mtrk1.size(),8);
	EXPECT_EQ(mtrk1.nticks(),200);

	mtrk_t mtrk2;
	mtrk2.push_back(jmid::make_text(0,"This is a text event for mtrk2"));

	EXPECT_EQ(mtrk1.size(),8);
	EXPECT_EQ(mtrk1.nticks(),200);
	EXPECT_EQ(mtrk2.size(),1);
	EXPECT_EQ(mtrk2.nticks(),0);

	mtrk2 = mtrk1;

	EXPECT_EQ(mtrk1.size(),8);
	EXPECT_EQ(mtrk1.nticks(),200);
	EXPECT_EQ(mtrk2.size(),8);
	EXPECT_EQ(mtrk2.nticks(),200);

	for (int i=0; i<mtrk1.size(); ++i) {
		EXPECT_EQ(mtrk1[i],mtrk2[i]);
	}
}


TEST(mtrk_t_tests, MoveCtor) {
	mtrk_t mtrk1;
	mtrk1.push_back(jmid::make_text(0,"This is a big text event that exceeds the "
		"local-object buffer of the mtrk_event_t."));
	mtrk1.push_back(jmid::make_ch_event(0,0x90,0,60,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x90,0,64,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x90,0,67,63));
	mtrk1.push_back(jmid::make_ch_event(100,0x80,0,60,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x80,0,64,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x80,0,67,63));
	mtrk1.push_back(jmid::make_eot(100));

	EXPECT_EQ(mtrk1.size(),8);
	EXPECT_EQ(mtrk1.nticks(),200);

	mtrk_t mtrk2 = std::move(mtrk1);

	EXPECT_EQ(mtrk1.size(),0);
	EXPECT_EQ(mtrk1.nticks(),0);
	EXPECT_EQ(mtrk2.size(),8);
	EXPECT_EQ(mtrk2.nticks(),200);
}


TEST(mtrk_t_tests, MoveAssign) {
	mtrk_t mtrk1;
	mtrk1.push_back(jmid::make_text(0,"This is a big text event that exceeds the "
		"local-object buffer of the mtrk_event_t."));
	mtrk1.push_back(jmid::make_ch_event(0,0x90,0,60,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x90,0,64,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x90,0,67,63));
	mtrk1.push_back(jmid::make_ch_event(100,0x80,0,60,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x80,0,64,63));
	mtrk1.push_back(jmid::make_ch_event(0,0x80,0,67,63));
	mtrk1.push_back(jmid::make_eot(100));

	EXPECT_EQ(mtrk1.size(),8);
	EXPECT_EQ(mtrk1.nticks(),200);

	mtrk_t mtrk2;
	mtrk2.push_back(jmid::make_text(0,"This is a text event for mtrk2"));

	EXPECT_EQ(mtrk1.size(),8);
	EXPECT_EQ(mtrk1.nticks(),200);
	EXPECT_EQ(mtrk2.size(),1);
	EXPECT_EQ(mtrk2.nticks(),0);

	mtrk2 = std::move(mtrk1);

	EXPECT_EQ(mtrk1.size(),0);
	EXPECT_EQ(mtrk1.nticks(),0);
	EXPECT_EQ(mtrk2.size(),8);
	EXPECT_EQ(mtrk2.nticks(),200);
}



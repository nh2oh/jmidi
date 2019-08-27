#include "gtest/gtest.h"
#include "mtrk_test_data.h"
#include "mtrk_t.h"
#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include <vector>
#include <cstdint>
#include <iterator>
#include <iostream>

using namespace mtrk_tests;

jmid::mtrk_t make_mtrk_tsb(const std::vector<tsb_t>& v) {
	auto mtrk_tsb = jmid::mtrk_t();  // auto to avoid MVP
	for (const auto& e : v) {
		auto curr_ev = jmid::make_mtrk_event(e.d.data(),e.d.data()+e.d.size(),
			0,nullptr,e.d.size()).event;
		mtrk_tsb.push_back(curr_ev);
	}
	return mtrk_tsb;
}


// 
// OIt split_copy_if(InIt beg, InIt end, OIt dest, UPred pred)
// Copy out all channel_voice events for note number 67
//
TEST(mtrk_t_tests, SplitCopyIfForNoteNum67WithTSB) {
	auto mtrk_b = make_mtrk_tsb(tsb);

	auto isntnum43 = [](const jmid::mtrk_event_t& ev)->bool {
		auto md = jmid::get_channel_event(ev);
		return (jmid::is_channel_voice(ev) && (md.p1==67));  // 67 == 0x43u
	};

	auto new_mtrk = jmid::mtrk_t();
	auto it = jmid::split_copy_if(mtrk_b.begin(),mtrk_b.end(),
		std::back_inserter(new_mtrk),isntnum43);

	int32_t tk_onset = 0;
	EXPECT_EQ(new_mtrk.size(),tsb_note_67_events.size());
	for (int i=0; i<new_mtrk.size(); ++i) {
		tk_onset += new_mtrk[i].delta_time();
		EXPECT_TRUE(jmid::is_channel_voice(new_mtrk[i]));
		auto md = jmid::get_channel_event(new_mtrk[i]);
		EXPECT_EQ(md.p1,67);
		EXPECT_EQ(tk_onset,tsb_note_67_events[i].tkonset);

		// The raw data in tsb_note_67_events have the delta time fields from
		// mtrk_b
		auto ev = jmid::make_mtrk_event(tsb_note_67_events[i].d.data(),
			tsb_note_67_events[i].d.data()+tsb_note_67_events[i].d.size(),
			0x00u,nullptr,tsb_note_67_events[i].d.size()).event;
		ev.set_delta_time(new_mtrk[i].delta_time());
		EXPECT_EQ(new_mtrk[i],ev);
	}
}


// 
// OIt split_copy_if(InIt beg, InIt end, OIt dest, UPred pred)
// Copy out all meta events
//
TEST(mtrk_t_tests, SplitCopyIfForMetaEventsWithTSB) {
	auto mtrk_b = make_mtrk_tsb(tsb);

	auto ismeta = [](const jmid::mtrk_event_t& ev)->bool {
		return jmid::is_meta(ev);
	};

	auto new_mtrk = jmid::mtrk_t();
	auto it = split_copy_if(mtrk_b.begin(),mtrk_b.end(),
		std::back_inserter(new_mtrk),ismeta);

	int32_t tk_onset = 0;
	EXPECT_EQ(new_mtrk.size(),tsb_meta_events.size());
	for (int i=0; i<new_mtrk.size(); ++i) { 
		tk_onset += new_mtrk[i].delta_time();
		EXPECT_TRUE(jmid::is_meta(new_mtrk[i]));
		EXPECT_EQ(tk_onset,tsb_meta_events[i].tkonset);

		// The raw data in tsb_meta_events have the delta time fields from
		// mtrk_b
		auto ev = jmid::make_mtrk_event(tsb_meta_events[i].d.data(),
			tsb_meta_events[i].d.data()+tsb_meta_events[i].d.size(),
			0x00u,nullptr,tsb_meta_events[i].d.size()).event;
		ev.set_delta_time(new_mtrk[i].delta_time());
		EXPECT_EQ(new_mtrk[i],ev);
	}
}


// 
// FwIt split_if(FwIt beg, FwIt end, UPred pred)
// Split out all channel_voice events for note number 67
//
TEST(mtrk_t_tests, SplitIfForNoteNum67WithTSB) {
	auto mtrk_b = make_mtrk_tsb(tsb);

	auto isntnum43 = [](const jmid::mtrk_event_t& ev)->bool {
		auto md = jmid::get_channel_event(ev);
		return (jmid::is_channel_voice(ev) && (md.p1==67));  // 67 == 0x43u
	};

	auto it = jmid::split_if(mtrk_b.begin(),mtrk_b.end(),isntnum43);
	auto mtrk_first = jmid::mtrk_t(mtrk_b.begin(),it);
	auto mtrk_second = jmid::mtrk_t(it,mtrk_b.end());
	
	int32_t tk_onset = 0;
	EXPECT_EQ(mtrk_first.size(),tsb_note_67_events.size());
	for (int i=0; i<mtrk_first.size(); ++i) {
		tk_onset += mtrk_first[i].delta_time();
		EXPECT_TRUE(jmid::is_channel_voice(mtrk_first[i]));
		auto md = jmid::get_channel_event(mtrk_first[i]);
		EXPECT_EQ(md.p1,67);
		EXPECT_EQ(tk_onset,tsb_note_67_events[i].tkonset);

		// The raw data in tsb_note_67_events have the delta time fields from
		// mtrk_b
		auto ev = jmid::make_mtrk_event(tsb_note_67_events[i].d.data(),
			tsb_note_67_events[i].d.data()+tsb_note_67_events[i].d.size(),
			0x00u,nullptr,tsb_note_67_events[i].d.size()).event;
		ev.set_delta_time(mtrk_first[i].delta_time());
		EXPECT_EQ(mtrk_first[i],ev);
	}

	EXPECT_EQ(mtrk_second.size(),tsb_non_note_67_events.size());
	tk_onset = 0;
	for (int i=0; i<mtrk_second.size(); ++i) {
		tk_onset += mtrk_second[i].delta_time();
		if (jmid::is_channel_voice(mtrk_second[i])) {
			auto md = jmid::get_channel_event(mtrk_second[i]);
			EXPECT_NE(md.p1,67);
		}
		EXPECT_EQ(tk_onset,tsb_non_note_67_events[i].tkonset);

		// The raw data in tsb_non_note_67_events have the delta time fields from
		// mtrk_b
		auto ev = jmid::make_mtrk_event(tsb_non_note_67_events[i].d.data(),
			tsb_non_note_67_events[i].d.data()+tsb_non_note_67_events[i].d.size(),
			0x00u,nullptr,tsb_non_note_67_events[i].d.size()).event;
		ev.set_delta_time(mtrk_second[i].delta_time());
		EXPECT_EQ(mtrk_second[i],ev);
	}
}

// 
// mtrk_t split_if(mtrk_t& mtrk, UPred pred)
// Split out all channel_voice events for note number 67
//
TEST(mtrk_t_tests, SplitIfMtrkOverloadForNoteNum67WithTSB) {
	auto mtrk_b = make_mtrk_tsb(tsb);

	auto isntnum43 = [](const jmid::mtrk_event_t& ev)->bool {
		auto md = jmid::get_channel_event(ev);
		return (jmid::is_channel_voice(ev) && (md.p1==67));  // 67 == 0x43u
	};

	auto mtrk_first = split_if(mtrk_b,isntnum43);
	EXPECT_EQ(mtrk_first.size(),tsb_note_67_events.size());
	EXPECT_EQ(mtrk_b.size(),tsb_non_note_67_events.size());
}


// 
// OIt merge(InIt beg1, InIt end1, InIt beg2, InIt end2, OIt dest)
//
// The event vectors (stored into an mtrk) tsb_note_67_events and
// tsb_non_note_67_events should merge into the same mtrk as
// tsb.  
//
TEST(mtrk_t_tests, MergeMtrkTSBNote67SplitProducts) {
	auto mtrk_b = make_mtrk_tsb(tsb);
	auto mtrk_non67 = make_mtrk_tsb(tsb_non_note_67_events);
	int32_t tkonset = 0; int32_t cumtk = 0;
	for (int i=0; i<mtrk_non67.size(); ++i) {
		mtrk_non67[i].set_delta_time(tsb_non_note_67_events[i].tkonset - cumtk);
		cumtk += mtrk_non67[i].delta_time();
	}
	auto mtrk_67 = make_mtrk_tsb(tsb_note_67_events);
	tkonset = 0; cumtk = 0;
	for (int i=0; i<mtrk_67.size(); ++i) {
		mtrk_67[i].set_delta_time(tsb_note_67_events[i].tkonset - cumtk);
		cumtk += mtrk_67[i].delta_time();
	}
	auto mtrk_merged = jmid::mtrk_t();
	auto it = merge(mtrk_non67.begin(),mtrk_non67.end(),
		mtrk_67.begin(),mtrk_67.end(),std::back_inserter(mtrk_merged));
	
	EXPECT_EQ(mtrk_b.size(),mtrk_merged.size());
	for (int i=0; i<mtrk_b.size(); ++i) {
		EXPECT_EQ(mtrk_b[i],mtrk_merged[i]);
	}
}

// 
// OIt merge(InIt beg1, InIt end1, InIt beg2, InIt end2, OIt dest)
//
// Merge the meta and non-meta events split out from tsb.  
//
TEST(mtrk_t_tests, MergeMtrkTSBMetaEventsSplitProducts) {
	auto mtrk_b = make_mtrk_tsb(tsb);
	auto mtrk_nonmeta = make_mtrk_tsb(tsb_non_meta_events);
	int32_t tkonset = 0; int32_t cumtk = 0;
	for (int i=0; i<mtrk_nonmeta.size(); ++i) {
		mtrk_nonmeta[i].set_delta_time(tsb_non_meta_events[i].tkonset - cumtk);
		cumtk += mtrk_nonmeta[i].delta_time();
	}
	auto mtrk_meta = make_mtrk_tsb(tsb_meta_events);
	tkonset = 0; cumtk = 0;
	for (int i=0; i<mtrk_meta.size(); ++i) {
		mtrk_meta[i].set_delta_time(tsb_meta_events[i].tkonset - cumtk);
		cumtk += mtrk_meta[i].delta_time();
	}
	auto mtrk_merged = jmid::mtrk_t();
	auto it = merge(mtrk_nonmeta.begin(),mtrk_nonmeta.end(),
		mtrk_meta.begin(),mtrk_meta.end(),std::back_inserter(mtrk_merged));
	
	// tsb begins w/ 2 meta events w/ dt==0, followed by a nonmeta w/ dt==0.  
	// In my call to merge() i use the _nonmeta vector as arg pair 1,2, which
	// means the nonmeta event will appear before the two meta events in the
	// merged sequence.  I could flip the order of the arg pairs in the call 
	// to merge(), but a similar problem would occur @ the last event in the 
	// track:  The meta EOT event would be placed before the final note-off
	// event.  
	auto temp = mtrk_merged[0];
	mtrk_merged[0] = mtrk_merged[1];
	mtrk_merged[1] = mtrk_merged[2];
	mtrk_merged[2] = temp;

	EXPECT_EQ(mtrk_b.size(),mtrk_merged.size());
	for (int i=0; i<mtrk_b.size(); ++i) {
		EXPECT_EQ(mtrk_b[i],mtrk_merged[i]);
	}
}



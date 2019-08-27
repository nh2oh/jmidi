#include "gtest/gtest.h"
#include "mtrk_test_data.h"
#include "mtrk_t.h"
#include "mtrk_event_t.h"
#include "aux_types.h"
#include <vector>
#include <cstdint>

using namespace mtrk_tests;

jmid::mtrk_t make_tsa() {
	auto mtrk_tsa = jmid::mtrk_t();  // auto to avoid MVP
	for (const auto& e : tsa) {
		auto curr_ev = jmid::make_mtrk_event(e.d.data(),e.d.data()+e.d.size(),
			0,nullptr,e.d.size()).event;
		mtrk_tsa.push_back(curr_ev);
	}
	return mtrk_tsa;
}


// 
// mtrk_t::mtrk_t(), mtrk_t::push_back(const mtrk_event_t&)
//
TEST(mtrk_t_tests, DefaultCtorMultiplePushBackTestSetA) {
	auto mtrk_tsa = make_tsa();  // "mtrk test set a"
	EXPECT_EQ(mtrk_tsa.size(),tsa.size());
	EXPECT_EQ(mtrk_tsa.nticks(),tsa_props.duration_tks);
	for (int i=0; i<tsa.size(); ++i) {
		auto curr_ev = jmid::make_mtrk_event(tsa[i].d.data(),
			tsa[i].d.data()+tsa[i].d.size(),0,nullptr,tsa[i].d.size()).event;
		EXPECT_EQ(mtrk_tsa[i],curr_ev);
	}
}


// 
// mtrk_t::insert(iterator, const mtrk_event_t&)
//
TEST(mtrk_t_tests, InsertEventWithZeroDtIntoTestSetAMethodInsert) {
	auto mtrk_tsa = make_tsa();  // "mtrk test set a"

	// On, off events for note num 57 on ch=0; both events have delta-time
	// == 0.  
	jmid::mtrk_event_t e_on(0,jmid::ch_event_data_t {0x90u,0,57,25});
	jmid::mtrk_event_t e_off(0,jmid::ch_event_data_t {0x90u,0,57,0});
	
	auto old_ev4 = *(mtrk_tsa.begin()+4);
	auto old_ev5 = *(mtrk_tsa.begin()+5);
	auto old_ev6 = *(mtrk_tsa.begin()+6);
	auto old_ev7 = *(mtrk_tsa.begin()+7);

	auto it_new5 = mtrk_tsa.insert(mtrk_tsa.begin()+5,e_on);
	EXPECT_EQ(mtrk_tsa.size(),tsa.size()+1);
	EXPECT_EQ(mtrk_tsa.nticks(),tsa_props.duration_tks);

	// The iterator returned by insert points at the new event
	EXPECT_EQ(*it_new5,e_on);
	EXPECT_EQ(mtrk_tsa[5],e_on);
	// The numerical idx of the new event == the numerical idx of the
	// event pointed to by the iterator passed to insert()
	EXPECT_EQ(*(mtrk_tsa.begin()+5),e_on);
	// Events @ idxs prior to that of the iterator passed to insert()
	// are unaffected
	EXPECT_EQ(*(mtrk_tsa.begin()+4),old_ev4);
	// Events w/ idx >= to that of the iterator passed to insert have
	// their old idxs-1.  
	EXPECT_EQ(*(mtrk_tsa.begin()+6),old_ev5);
	EXPECT_EQ(*(mtrk_tsa.begin()+7),old_ev6);
	EXPECT_EQ(*(mtrk_tsa.begin()+8),old_ev7);
}


// 
// at_cumtk_result_t<mtrk_const_iterator_t> at_cumtk(uint64_t) const;
//
TEST(mtrk_t_tests, AtCumtkTestSetA) {
	auto mtrk_tsa = make_tsa();  // "mtrk test set a"
	
	// at_cumtk() returns the first event w/a cumtk >= the cumtk provided
	struct test_t {
		int32_t cumtk_arg;
		jmid::mtrk_event_t ev_ans;
		int32_t cumtk_ans;
	};
	std::vector<test_t> tests {
		{0, mtrk_tsa[0], tsa[0].cumtk},

		// event 6 is the first event w/ cumtk >0 (==384)
		{1, mtrk_tsa[6], tsa[6].cumtk},  // cumtk==384
		{383, mtrk_tsa[6], tsa[6].cumtk},
		{384, mtrk_tsa[6], tsa[6].cumtk},  // cumtk==384

		// event  10 is the first event w/ cumtk >384 (==768)
		{385, mtrk_tsa[10], tsa[10].cumtk},  // cumtk==768

		{2687, mtrk_tsa[27], tsa[27].cumtk},  // cumtk==2688
		{2688, mtrk_tsa[27], tsa[27].cumtk}  // cumtk==2688
	};

	for (const auto& ctest : tests) {
		auto curr = mtrk_tsa.at_cumtk(ctest.cumtk_arg);
		auto curr_ev = *(curr.it);
		EXPECT_EQ(curr_ev,ctest.ev_ans);
		EXPECT_EQ(curr.tk,ctest.cumtk_ans);
	}

	auto expect_end = mtrk_tsa.at_cumtk(2689);
	EXPECT_EQ(expect_end.it,mtrk_tsa.end());
	EXPECT_EQ(expect_end.tk,2688);
}


// 
// at_cumtk_result_t<mtrk_const_iterator_t> at_tkonset(uint64_t) const;
//
TEST(mtrk_t_tests, AtTkonsetTestSetA) {
	auto mtrk_tsa = make_tsa();  // "mtrk test set a"
	
	// at_tkonset() returns the first event w/n onset tk >= the tk provided.  
	struct test_t {
		int32_t tkonset_arg;

		jmid::mtrk_event_t ev_ans;
		int32_t cumtk_ans;
		int32_t tkonset_ans;
	};
	std::vector<test_t> tests {
		{0, mtrk_tsa[0], tsa[0].cumtk, tsa[0].cumtk+mtrk_tsa[0].delta_time()},

		// event 5 is the first event w/ onset tk >0 (==384)
		{1, mtrk_tsa[5], tsa[5].cumtk, tsa[5].cumtk+mtrk_tsa[5].delta_time()},  // cumtk==384
		{383, mtrk_tsa[5], tsa[5].cumtk,tsa[5].cumtk+mtrk_tsa[5].delta_time()},
		{384, mtrk_tsa[5], tsa[5].cumtk,tsa[5].cumtk+mtrk_tsa[5].delta_time()},  // cumtk==384

		// event  9 is the first event w/ onset tk >384 (==768)
		{385, mtrk_tsa[9], tsa[9].cumtk, tsa[9].cumtk+mtrk_tsa[9].delta_time()},  // cumtk==384

		{2687, mtrk_tsa[26], tsa[26].cumtk, tsa[26].cumtk+mtrk_tsa[26].delta_time()},  // cumtk==2688
		{2688, mtrk_tsa[26], tsa[26].cumtk, tsa[26].cumtk+mtrk_tsa[26].delta_time()}  // cumtk==2688
	};

	for (const auto& ctest : tests) {
		auto curr = mtrk_tsa.at_tkonset(ctest.tkonset_arg);
		auto curr_ev = *(curr.it);
		auto curr_cumtk = curr.tk - curr_ev.delta_time();
		EXPECT_EQ(curr_ev,ctest.ev_ans);
		EXPECT_EQ(curr.tk,ctest.tkonset_ans);
		EXPECT_EQ(curr_cumtk,ctest.cumtk_ans);
	}

	auto expect_end = mtrk_tsa.at_tkonset(2689);
	EXPECT_EQ(expect_end.it,mtrk_tsa.end());
	EXPECT_EQ(expect_end.tk,2688);  // expect_end.cumtk is really a tk_onset
}



// 
// Insert at cumtk
// iterator insert(uint64_t, mtrk_event_t);
//
TEST(mtrk_t_tests, InsertWithZeroDtIntoTSAMethodInsertAtCumtk) {
	auto mtrk_tsa = make_tsa();  // "mtrk test set a"
	// Note-on event for note num 57 on ch=0, velocity==25, delta-time==0.  
	jmid::mtrk_event_t e_on(0,jmid::ch_event_data_t {0x90u,0,57,25});

	struct test_t {
		int32_t insert_at_cumtk {0};
		int idx_inserted {0};  // idx of the newly inserted event
		// in the new mtrk, the event that _used_ to have idx==idx_inserted
		int oldidx_inserted {0};  
		// in the new mtrk, the event that _used_ to have idx==idx_inserted+1
		int oldidx_inserted_p1 {0};
		// in the new mtrk, the cumtk, delta_time, tk_onset of the new event
		int cumtk_inserted {0};
		int dt_inserted {0};
		int tkonset_inserted {0};
	};
	std::vector<test_t> tests {
		{0,  0,1,2,  0,0,0},
		// Insertion directly prior to event 5 (cumtk==0,dt==384,tk_onset==384)
		{1,  5,6,7,  0,1,1},
		{2,  5,6,7,  0,2,2},
		{383,  5,6,7,  0,383,383},
		{384,  5,6,7,  0,384,384},
		// Insertion directly prior to event 9 (cumtk==384,dt==384,tk_onset==768)
		{385,  9,10,11,  384,1,385},
		{767,  9,10,11,  384,767-384,767},
		{768,  9,10,11,  384,768-384,768},
		// Insertion directly prior to event 12 (cumtk==768,dt==384,tk_onset==1152)
		{769,  12,13,14,  768,1,769},

		{2690,  31,-1,-1,  2688,2,2690}
	};

	for (const auto& currtest : tests) {
		auto new_mtrk_tsa = mtrk_tsa;
		auto expect_inserted_tkonset = currtest.insert_at_cumtk;
		auto expect_inserted_ev = e_on;
		expect_inserted_ev.set_delta_time(currtest.dt_inserted);

		auto it_new = new_mtrk_tsa.insert(currtest.insert_at_cumtk,e_on);
		EXPECT_EQ(*it_new,expect_inserted_ev);
		EXPECT_EQ(it_new->delta_time(),currtest.dt_inserted);
		auto seek_tkonset = new_mtrk_tsa.at_tkonset(expect_inserted_tkonset);
		EXPECT_EQ(seek_tkonset.tk,currtest.tkonset_inserted);
		EXPECT_EQ(seek_tkonset.it,it_new);
		EXPECT_EQ(seek_tkonset.tk-seek_tkonset.it->delta_time(),currtest.cumtk_inserted);

		EXPECT_EQ(*it_new,new_mtrk_tsa[currtest.idx_inserted]);
		if (currtest.oldidx_inserted >= 0) {
			EXPECT_EQ(mtrk_tsa[currtest.idx_inserted],
				new_mtrk_tsa[currtest.oldidx_inserted]);
		}
		if (currtest.oldidx_inserted_p1 >= 0) {
			EXPECT_EQ(mtrk_tsa[currtest.idx_inserted+1],
				new_mtrk_tsa[currtest.oldidx_inserted_p1]);
		}
	}
}


// 
// Insert no tk shift:  New event has a delta_time == 0
// iterator insert_no_tkshift(iterator, mtrk_event_t);
//
TEST(mtrk_t_tests, InsertWithZeroDtIntoTSAMethodInsertNoTkShift) {
	auto mtrk_tsa = make_tsa();  // "mtrk test set a"
	// Note-on event for note num 57 on ch=0, velocity==25, delta-time==0.  
	jmid::mtrk_event_t e_on(0,jmid::ch_event_data_t {0x90u,0,57,25});

	struct test_t {
		int insert_at_idx {0};
		int idx_inserted {0};  // idx of the newly inserted event
		// in the new mtrk, the cumtk, delta_time, tk_onset of the new event
		int32_t cumtk_inserted {0};
		int32_t dt_inserted {0};
		int32_t tkonset_inserted {0};
	};
	std::vector<test_t> tests {
		{0, 0, 0,0,0},
		// Insertion directly prior to event 5 (cumtk==0,dt==384,tk_onset==384)
		{1, 1, 0,0,0},
		{2, 2, 0,0,0},
		// Ev 5 is the first w/ nonzero dt.  The new event will go in _before_ 
		// 5, thus it's cumtk will be 0
		{5, 5, 0,0,0},
		{6, 6, 384,0,384},
		{8, 8, 384,0,384},
		{9, 9, 384,0,384},
		{10, 10, 768,0,768},
	};

	for (const auto& currtest : tests) {
		auto new_mtrk_tsa = mtrk_tsa;
		auto it_insert = jmid::mtrk_t::iterator(&(new_mtrk_tsa[currtest.insert_at_idx]));

		auto it_new = new_mtrk_tsa.insert_no_tkshift(it_insert,e_on);

		EXPECT_EQ(new_mtrk_tsa.nticks(),mtrk_tsa.nticks()+e_on.delta_time());

		EXPECT_EQ(*it_new,new_mtrk_tsa[currtest.idx_inserted]);
		EXPECT_EQ(it_new->delta_time(),currtest.dt_inserted);

		int32_t cumtk = 0;
		auto it=new_mtrk_tsa.begin();
		for (true; it!=new_mtrk_tsa.end(); ++it) {
			if (it == it_new) {
				break;
			}
			cumtk += it->delta_time();
		}
		EXPECT_EQ(cumtk,currtest.cumtk_inserted);
		EXPECT_EQ(cumtk+it->delta_time(),currtest.tkonset_inserted);
	}

	auto new_mtrk_tsa = mtrk_tsa;
	auto it_insert = new_mtrk_tsa.end();
	auto it_new = new_mtrk_tsa.insert(it_insert,e_on);
	EXPECT_EQ(new_mtrk_tsa.nticks(),mtrk_tsa.nticks()+e_on.delta_time());
	EXPECT_EQ(*it_new,e_on);
	EXPECT_EQ(it_new->delta_time(),0);
}


// 
// Insert no tk shift:  New event has a delta_time > 0
// iterator insert_no_tkshift(iterator, mtrk_event_t);
//
TEST(mtrk_t_tests, InsertWithNzeroDtIntoTSAMethodInsertNoTkShift) {
	struct test_t {
		int ev_dt {0};
		int insert_idx {0};
		int inserted_idx {0};
		int inserted_cumtk {0};
		int inserted_dt {0};
		int inserted_tkonset {0};  // Always == ev_dt+cumtk-at-insert_idx
		int next_ev_dt {0};
	};
	std::vector<test_t> tests {
		//
		// Set 1:
		// dt for the new event < the dt value of event at/near the
		// insertion point in the sequence.  
		// The cumtk at idx [0,6) == 0 => Desire a tk_onset for the new event
		// to be 383.  dt for event 5 == 384, so the new event must go 
		// directly before, and the dt of the new event 6 (old event 5) must
		// be set to 1.  
		{383, 0,  5, 0,383,383,1},
		{383, 1,  5, 0,383,383,1},
		{383, 5,  5, 0,383,383,1},  // 5 is the first event w/ dt>0 (==384)
		// The cumtk at idx [6,10) == 384 => Desire a tk_onset for the new event
		// to be 383+384==767.  dt for event 5,9==384, tk onset for event 9==768,
		// so the new event must go directly before event 9, and the dt of the 
		// new event 10 (old event 9) must be set to 1.  
		{383, 6,  9, 384,383,767,1},

		//
		// Set 2:
		// dt for the new event == the dt value of event at/near the
		// insertion point in the sequence
		{384, 0,  5,0,384,384,0},
		{384, 1,  5,0,384,384,0},
		{384, 5,  5,0,384,384,0},  // 5 is the first event w/ dt>0 (==384)
		{384, 6,  9,384,384,768,0},

		//
		// Set 3:
		// dt for the new event > the dt value of event at/near the
		// insertion point in the sequence.  
		// The cumtk at idx [0,6)==0, [6,10)==384, [10,13)==768 
		// Insert @ idx <=5 => Desire a tk_onset for the new event of
		// 385+0==385; the new event must go after event 5 but before event 
		// 9 (=> directly before event 9), and the dt of the new event 10 
		// (old event 9) must be set to 1.  
		// Insertion @ idx 6 (cumtk==384) => tk_onset == 384+385==769
		// The new event must go directly before event 12 and the dt of the 
		// new event 13 (old event 12) must be set to 1.
		{385, 0,  9, 384,1,385,383},
		{385, 1,  9, 384,1,385,383},
		{385, 5,  9, 384,1,385,383},  // 5 is the first event w/ dt>0 (==384)
		{385, 6,  12, 768,1,769,383}
	};
	
	for (const auto ctest : tests) {
		auto ev = jmid::mtrk_event_t(ctest.ev_dt,jmid::ch_event_data_t {0x90u,0,57,25});
		auto expect_ev_inserted = jmid::mtrk_event_t(ctest.inserted_dt,jmid::ch_event_data_t {0x90u,0,57,25});
		auto mtrk_tsa = make_tsa();
		auto init_nticks = mtrk_tsa.nticks();

		auto it_insert = mtrk_tsa.begin()+ctest.insert_idx;
		auto it_new = mtrk_tsa.insert_no_tkshift(it_insert,ev);
		EXPECT_EQ(mtrk_tsa.nticks(),init_nticks);
		EXPECT_EQ(*it_new,expect_ev_inserted);
		EXPECT_EQ(it_new->delta_time(),ctest.inserted_dt);
		auto it=mtrk_tsa.begin();  uint64_t cumtk = 0;
		for (true; it!=it_new; ++it) {
			cumtk += it->delta_time();
		}
		EXPECT_EQ(cumtk,ctest.inserted_cumtk);
		EXPECT_EQ(cumtk+it->delta_time(),ctest.inserted_tkonset);
		auto new_idx = it-mtrk_tsa.begin();
		EXPECT_EQ(new_idx,ctest.inserted_idx);
		EXPECT_EQ(mtrk_tsa[new_idx+1].delta_time(),ctest.next_ev_dt);
	}
	
}



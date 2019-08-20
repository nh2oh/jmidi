#include "gtest/gtest.h"
#include "mtrk_event_t_internal.h"
#include <vector>
#include <cstdint>

using namespace mtrk_event_t_internal;

std::vector<unsigned char> small_sizes {
	0,1,2,7,17,20,22,small_bytevec_t::capacity_small
};
std::vector<unsigned char> big_sizes {
	small_bytevec_t::capacity_small+1,small_bytevec_t::capacity_small+2,25,30,36
};
// size() == 6
std::vector<unsigned char> a {
	15,14,13,12,11,10
};
// size() == 23
std::vector<unsigned char> b {
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22
};
std::vector<unsigned char> c {
	24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46
};
// size() == 24
std::vector<unsigned char> d24 {
	47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70
};
// size() == 25
std::vector<unsigned char> e25 {
	1,11,21,31,41,51,61,71,81,91,110,111,112,113,114,115,116,117,118,119,
	120,121,122,121,122,123,124
};
// size() == 100
std::vector<unsigned char> f100 {
	1,11,21,31,41,51,61,71,81,91,110,111,112,113,114,115,116,117,118,119,
	120,121,122,121,122,123,124,
	47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,
	70,71,
	24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,11,141,
	21,211,221,231,241,152,162,217,182,129,210,211,212,213,214,215,216,217,
	218,219,210,211,212
};

void fill_small_bytevec_to_sbo_size(small_bytevec_t& sbo, std::vector<unsigned char>&v) {
	if (sbo.size() > v.size()) {
		std::abort();
	}
	int i=0;
	for (auto it=sbo.begin(); it!=sbo.end(); ++it) {
		int ii = i++;
		*it = v[ii];
	}
}

//
// TODO:
// copyAssignBigToBigger/BiggerToBig
// copyAssignSmallSmall
// copyAssignSelfBig/Small...
// moveAssignSmallSmall/SmallBig/BigBig/BigSmall/...
// moveAssignSelfBig/Small?!
//
TEST(mtrk_event_t_internal, defaultCtor) {
	small_bytevec_t x = small_bytevec_t();
	EXPECT_FALSE(x.debug_is_big());
	EXPECT_EQ(x.size(),0);
	EXPECT_EQ(x.capacity(),small_bytevec_t::capacity_small);
	EXPECT_EQ(x.begin(),x.end());
	EXPECT_EQ(x.end()-x.begin(),0);
}

TEST(mtrk_event_t_internal, copyCtorSmall) {
	small_bytevec_t src = small_bytevec_t();
	src.resize(a.size());
	fill_small_bytevec_to_sbo_size(src,a);
	EXPECT_FALSE(src.debug_is_big());

	small_bytevec_t dest = src;
	EXPECT_FALSE(dest.debug_is_big());
	EXPECT_EQ(src.size(),dest.size());
	EXPECT_EQ(src.capacity(),dest.capacity());
	for (int i=0; i<src.size(); ++i) {
		EXPECT_EQ(*(src.begin()+i),*(dest.begin()+i));
	}
}

TEST(mtrk_event_t_internal, copyCtorBig) {
	small_bytevec_t src = small_bytevec_t();
	src.resize(f100.size());
	fill_small_bytevec_to_sbo_size(src,f100);
	EXPECT_TRUE(src.debug_is_big());

	small_bytevec_t dest = src;
	EXPECT_TRUE(dest.debug_is_big());
	EXPECT_EQ(src.size(),dest.size());
	EXPECT_EQ(src.capacity(),dest.capacity());
	for (int i=0; i<src.size(); ++i) {
		EXPECT_EQ(*(src.begin()+i),*(dest.begin()+i));
	}
}

// Copy assign 'big' src to 'small' dest
TEST(mtrk_event_t_internal, copyAssignBigToSmall) {
	small_bytevec_t src = small_bytevec_t();
	src.resize(f100.size());
	fill_small_bytevec_to_sbo_size(src,f100);
	EXPECT_TRUE(src.debug_is_big());  // src is big

	small_bytevec_t dest = small_bytevec_t();
	dest.resize(b.size());
	fill_small_bytevec_to_sbo_size(dest,b);
	EXPECT_FALSE(dest.debug_is_big());  // dest is small

	dest = src;

	EXPECT_TRUE(dest.debug_is_big());  // dest is now big
	EXPECT_EQ(src.size(),dest.size());
	// The following is only true since initially dest.cpacity()<src.size()
	EXPECT_EQ(dest.capacity(),src.size());  
	for (int i=0; i<src.size(); ++i) {
		EXPECT_EQ(*(src.begin()+i),*(dest.begin()+i));
	}
}


// Copy assign 'small' src to 'big' dest
// Assigning a 'small' object to a 'big' object causes a big->small
// transition in the destination object.  
TEST(mtrk_event_t_internal, copyAssignSmallToBig) {
	small_bytevec_t src = small_bytevec_t();
	src.resize(b.size());
	fill_small_bytevec_to_sbo_size(src,f100);
	EXPECT_FALSE(src.debug_is_big());  // src is small

	small_bytevec_t dest = small_bytevec_t();
	dest.resize(f100.size());
	fill_small_bytevec_to_sbo_size(dest,f100);
	EXPECT_TRUE(dest.debug_is_big());  // dest is big

	dest = src;

	EXPECT_FALSE(dest.debug_is_big());  // dest is now small
	EXPECT_EQ(src.size(),dest.size());
	for (int i=0; i<src.size(); ++i) {
		EXPECT_EQ(*(src.begin()+i),*(dest.begin()+i));
	}
}

TEST(mtrk_event_t_internal, moveCtorSmall) {
	small_bytevec_t src = small_bytevec_t();
	auto data = a;
	src.resize(data.size());
	fill_small_bytevec_to_sbo_size(src,data);
	EXPECT_FALSE(src.debug_is_big());
	auto src_size = src.size();
	auto src_cap = src.capacity();

	small_bytevec_t dest = std::move(src);
	EXPECT_FALSE(dest.debug_is_big());
	EXPECT_EQ(src_size,dest.size());
	EXPECT_EQ(src_cap,dest.capacity());
	for (int i=0; i<src_size; ++i) {
		EXPECT_EQ(*(data.begin()+i),*(dest.begin()+i));
	}
}

TEST(mtrk_event_t_internal, moveCtorBig) {
	small_bytevec_t src = small_bytevec_t();
	auto data = f100;
	src.resize(data.size());
	fill_small_bytevec_to_sbo_size(src,data);
	EXPECT_TRUE(src.debug_is_big());
	auto src_size = src.size();
	auto src_cap = src.capacity();

	small_bytevec_t dest = std::move(src);
	EXPECT_TRUE(dest.debug_is_big());
	EXPECT_EQ(src_size,dest.size());
	EXPECT_EQ(src_cap,dest.capacity());
	for (int i=0; i<src_size; ++i) {
		EXPECT_EQ(*(data.begin()+i),*(dest.begin()+i));
	}
}

TEST(mtrk_event_t_internal, defaultCtorReserveToSmallMax) {
	small_bytevec_t x = small_bytevec_t();
	x.reserve(small_bytevec_t::capacity_small);
	EXPECT_FALSE(x.debug_is_big());
	EXPECT_EQ(x.size(),0);
	EXPECT_EQ(x.capacity(),small_bytevec_t::capacity_small);
	EXPECT_EQ(x.begin(),x.end());
	EXPECT_EQ(x.end()-x.begin(),0);
}

TEST(mtrk_event_t_internal, defaultCtorResizeToSmallMax) {
	small_bytevec_t x = small_bytevec_t();
	x.resize(small_bytevec_t::capacity_small);
	EXPECT_FALSE(x.debug_is_big());
	EXPECT_EQ(x.size(),small_bytevec_t::capacity_small);
	EXPECT_EQ(x.capacity(),small_bytevec_t::capacity_small);
	EXPECT_NE(x.begin(),x.end());
	EXPECT_EQ(x.end()-x.begin(),small_bytevec_t::capacity_small);
}

// Resize leaves values initially present @ indices < new_size
// unchanged.  
TEST(mtrk_event_t_internal, smallObjectMultipleResize) {
	small_bytevec_t x = small_bytevec_t();
	x.resize(b.size());
	EXPECT_FALSE(x.debug_is_big());
	EXPECT_EQ(x.size(),b.size());

	fill_small_bytevec_to_sbo_size(x,b);
	auto it = x.begin();
	for (int i=0; i<x.size(); ++i) {
		EXPECT_EQ(*it++,b[i]);
	}

	std::vector<int> resize_to {
		x.size(),
		0,1,2,17,7,20,22,small_bytevec_t::capacity_small
	};
	for (const auto& new_sz : resize_to) {
		x.resize(new_sz);
		EXPECT_FALSE(x.debug_is_big());
		EXPECT_EQ(x.size(),new_sz);
		EXPECT_EQ(x.capacity(),small_bytevec_t::capacity_small);
		EXPECT_EQ(x.end()-x.begin(),new_sz);
		auto it = x.begin();
		for (int i=0; i<x.size(); ++i) {
			EXPECT_EQ(*it++,b[i]);
		}
		// Reset & refill x
		x.resize(b.size());
		fill_small_bytevec_to_sbo_size(x,b);
	}
}

// Resize can change a small object -> a big object, and alters
// both the size and capacity
TEST(mtrk_event_t_internal, createBigObjByResize) {
	small_bytevec_t x = small_bytevec_t();
	EXPECT_FALSE(x.debug_is_big());  // Initially small
	int resize_to = d24.size();
	if (resize_to<=small_bytevec_t::capacity_small) {
		// The whole point of this test is to make a 'big' object
		std::abort();
	}
	x.resize(resize_to);

	EXPECT_TRUE(x.debug_is_big());
	EXPECT_EQ(x.size(),resize_to);
	EXPECT_TRUE(x.capacity()>=resize_to);
	EXPECT_NE(x.begin(),x.end());
	EXPECT_EQ(x.end()-x.begin(),x.size());
}

// Reserve can change a small object -> a big object, and alters
// the capacity but not the size
TEST(mtrk_event_t_internal, createBigObjByReserve) {
	small_bytevec_t x = small_bytevec_t();
	EXPECT_FALSE(x.debug_is_big());  // Initially small
	int reserve_to = e25.size();
	if (reserve_to<=small_bytevec_t::capacity_small) {
		// The whole point of this test is to make a 'big' object
		std::abort();
	}
	x.reserve(reserve_to);

	EXPECT_TRUE(x.debug_is_big());
	EXPECT_EQ(x.size(),0);
	EXPECT_TRUE(x.capacity()>=reserve_to);
	EXPECT_EQ(x.begin(),x.end());
	EXPECT_EQ(x.end()-x.begin(),x.size());
}

// Reserve leaves size() unchanged, even when called w/a value smaller
// than size().  All values initially present are stable.  
// Capacity is increased but never decreased.  
TEST(mtrk_event_t_internal, multipleReserveValueStability) {
	small_bytevec_t x = small_bytevec_t();
	auto data = e25;
	if (data.size()<=small_bytevec_t::capacity_small) {
		// The whole point of this test is to be able to make 
		// 'big' objects
		std::abort();
	}
	x.resize(data.size());
	fill_small_bytevec_to_sbo_size(x,data);
	int init_size = data.size();
	int init_cap = data.capacity();

	std::vector<int> reserve_to_set {
		x.size(),
		24,78,6,100,2,0,17,init_size,7,54,20,84,67,22,small_bytevec_t::capacity_small
	};
	for (const auto& reserve_to : reserve_to_set) {
		auto prior_sz = x.size();
		auto prior_cap = x.capacity();
		x.reserve(reserve_to);
		EXPECT_EQ(x.size(),init_size);
		EXPECT_TRUE(x.capacity()>=init_cap);
		EXPECT_EQ(x.end()-x.begin(),init_size);
		auto it = x.begin();
		for (int i=0; i<x.size(); ++i) {
			EXPECT_EQ(*it++,data[i]);
		}
		if (reserve_to>prior_cap) {
			EXPECT_EQ(x.capacity(),reserve_to);
		} else {
			EXPECT_EQ(x.capacity(),prior_cap);
		}
	}
}

// Resize can interconvert between small and big objects
TEST(mtrk_event_t_internal, interconvertSmallAndBigByResize) {
	small_bytevec_t x = small_bytevec_t();
	EXPECT_FALSE(x.debug_is_big());  // Initially small

	std::vector<unsigned char> assorted_sizes {
		small_bytevec_t::capacity_small+1,0,
		7,17,20,22,small_bytevec_t::capacity_small,
		small_bytevec_t::capacity_small+2,25,30,36,
		112,2,113,24,114,115,116,117,23,118,119
	};

	for (const auto& resize_to : assorted_sizes) {
		x.resize(resize_to);
		if (resize_to>small_bytevec_t::capacity_small) {
			EXPECT_TRUE(x.debug_is_big());
		} else {
			EXPECT_FALSE(x.debug_is_big());
		}
		EXPECT_EQ(x.size(),resize_to);
		EXPECT_TRUE(x.capacity()>=resize_to);
		if (x.size() > 0) {
			EXPECT_NE(x.begin(),x.end());
		} else {
			EXPECT_EQ(x.begin(),x.end());
		}
		EXPECT_EQ(x.end()-x.begin(),x.size());
	}
}


TEST(mtrk_event_t_internal, bigObjBasicConstFuntions) {
	small_bytevec_t x = small_bytevec_t();
	int resize_to = e25.size();
	if (resize_to<=small_bytevec_t::capacity_small) {
		// The whole point of this test is to make a 'big' object
		std::abort();
	}
	x.resize(resize_to);

	EXPECT_TRUE(x.debug_is_big());
	EXPECT_EQ(x.size(),resize_to);
	EXPECT_TRUE(x.capacity()>=resize_to);
	EXPECT_NE(x.begin(),x.end());
	EXPECT_EQ(x.end()-x.begin(),x.size());
	
	fill_small_bytevec_to_sbo_size(x,e25);
	int i=0;
	for (auto it=x.begin(); it<x.end(); ++it) {
		EXPECT_EQ(*it,e25[i++]);
	}
}


// Resize leaves values initially present @ indices < new_size
// unchanged.  
TEST(mtrk_event_t_internal, multipleResizeValueStability) {
	small_bytevec_t x = small_bytevec_t();
	auto data = f100;
	if (data.size()<=small_bytevec_t::capacity_small) {
		// The whole point of this test is to be able to make 
		// 'big' objects
		std::abort();
	}
	x.resize(data.size());
	fill_small_bytevec_to_sbo_size(x,f100);

	std::vector<int> resize_to {
		x.size(),
		24,78,6,100,2,0,17,7,54,20,84,67,22,small_bytevec_t::capacity_small
	};
	for (const auto& new_sz : resize_to) {
		x.resize(new_sz);
		if (new_sz>small_bytevec_t::capacity_small) {
			EXPECT_TRUE(x.debug_is_big());
		} else {
			EXPECT_FALSE(x.debug_is_big());
		}
		EXPECT_EQ(x.size(),new_sz);
		EXPECT_TRUE(x.capacity()>=x.size());
		EXPECT_EQ(x.end()-x.begin(),new_sz);
		auto it = x.begin();
		for (int i=0; i<x.size(); ++i) {
			EXPECT_EQ(*it++,f100[i]);
		}
		// Reset & refill x
		x.resize(f100.size());
		fill_small_bytevec_to_sbo_size(x,f100);
	}
}


// push_back() can start from a size()==0 'small' object and make a
// 'big' object w/ size()==that of the input
TEST(mtrk_event_t_internal, repeatedPushBackBigDataSrc) {
	small_bytevec_t x = small_bytevec_t();
	auto data = f100;
	EXPECT_TRUE(data.size()>small_bytevec_t::capacity_small);
	// The whole point of this test is to be able to make 
	// 'big' objects

	for (int i=0; i<data.size(); ++i) {
		EXPECT_EQ(x.size(),i);
		x.push_back(data[i]);
		EXPECT_EQ(x.size(),i+1);
		EXPECT_TRUE(x.capacity()>=x.size());
		EXPECT_EQ(*(x.end()-1),data[i]);
		if (x.size() <= small_bytevec_t::capacity_small) {
			// For an object that starts out as small, repeated push_back()'s
			// will not cause a small->big transition until the last possible
			// moment
			EXPECT_FALSE(x.debug_is_big());
			EXPECT_EQ(x.capacity(),small_bytevec_t::capacity_small);
		} else {
			EXPECT_TRUE(x.debug_is_big());
			EXPECT_TRUE(x.capacity()>small_bytevec_t::capacity_small);
		}
	}

	for (int i=0; i<data.size(); ++i) {
		EXPECT_EQ(*(x.begin()+i),data[i]);
	}

}





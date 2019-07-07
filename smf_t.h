#pragma once
#include "mthd_t.h"
#include "mtrk_event_t.h"
#include "mtrk_t.h"
#include "..\..\generic_iterator.h"
#include <string>
#include <cstdint>
#include <vector>


//
// smf_t
//
// Holds an smf; owns the underlying data.  MTrk chunks are stored as a 
// std::vector<mtrk_t>, unknown chunks as a 
// std::vector<std::vector<unsigned char>>.  Also stores the MThd chunk as
// an mthd_t, the filename, and the order in which the MTrk and Unknown
// chunks occured in the file.  
//
// Provided that the MTrk and unknown chunks validate, the only invariants
// an smf_t needs to maintain in order to be serializable are consistency
// between values of the MThd chunk parameters and the number, size etc of
// the MTrk chunks (ex mthd_.format()==0 => .ntrks()==1).  This is difficult
// to continuously, since smf_t exposes functions returning references &
// iterators directly into the member mtrks and mthd data.  Hence, any given 
// smf_t object may not be serializable to a valid smf.  The member function
// validate() returns detailed information about any problems.  The member
// function compute_mthd() sets the MThd data to be consistent with the 
// mtrk and unknown chunks.  
// 
// TODO:  Rename size() => nbytes; track_size(), track_data_size(), 
//        mthd_size(), etc.  
// TODO:  validate()
// TODO:  compute_mthd()
// TODO:  Ctors
//

struct smf_container_types_t {
	using value_type = mtrk_t;
	using size_type = int64_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
};

class smf_t {
public:
	using value_type = smf_container_types_t::value_type;
	using size_type = smf_container_types_t::size_type;
	using difference_type = smf_container_types_t::difference_type;
	using reference = smf_container_types_t::reference;
	using const_reference = smf_container_types_t::const_reference;
	using pointer = smf_container_types_t::pointer;
	using const_pointer = smf_container_types_t::const_pointer;
	using iterator = generic_ra_iterator<smf_container_types_t>;
	using const_iterator = generic_ra_const_iterator<smf_container_types_t>;

	using uchk_iterator = std::vector<std::vector<unsigned char>>::iterator;
	using uchk_const_iterator = std::vector<std::vector<unsigned char>>::const_iterator;
	using uchk_value_type = std::vector<unsigned char>;

	size_type size() const;  // Number of mtrk chunks
	size_type nchunks() const;  // Number of MTrk + Unkn chunks
	size_type ntrks() const;  // Number of MTrk chunks
	size_type nuchks() const;  // Number of MTrk chunks
	size_type nbytes() const;  // Number of bytes serialized

	iterator begin();
	iterator end();
	const_iterator cbegin() const;
	const_iterator cend() const;
	const_iterator begin() const;
	const_iterator end() const;
	
	mtrk_t& push_back(const mtrk_t&);
	iterator insert(iterator, const mtrk_t&);
	const_iterator insert(const_iterator, const mtrk_t&);

	uchk_iterator insert(uchk_iterator, const uchk_value_type&);
	uchk_const_iterator insert(uchk_const_iterator,	const uchk_value_type&);

	mtrk_t& operator[](size_type);
	const mtrk_t& operator[](size_type) const;

	int32_t format() const;  // mthd alias
	int32_t division() const;  // mthd alias
	int32_t mthd_size() const;  // mthd alias
	int32_t mthd_data_size() const;  // mthd alias
	const std::string& fname() const;

	mthd_view_t get_header_view() const;
	const mthd_t& get_header() const;
	const mtrk_t& get_track(int) const;
	mtrk_t& get_track(int);
	
	const std::string& set_fname(const std::string&);
	void set_mthd(const validate_mthd_chunk_result_t&);

	void append_mtrk(const mtrk_t&);
	void append_uchk(const std::vector<unsigned char>&);
private:
	std::string fname_ {};
	mthd_t mthd_ {};
	std::vector<mtrk_t> mtrks_ {};
	std::vector<std::vector<unsigned char>> uchks_ {};
	// Since MTrk and unknown chunks are split into mtrks_ and uchks_,
	// respectively, the order in which these chunks occured in the file
	// is lost.  chunkorder_ saves the order of interspersed MTrk and 
	// unknown chunks.  chunkorder.size()==mtrks_.size+uchks.size().  
	std::vector<int> chunkorder_ {};  // 0=>mtrk, 1=>unknown
};
std::string print(const smf_t&);

struct maybe_smf_t {
	smf_t smf;
	std::string error {"No error"};
	operator bool() const;
};
maybe_smf_t read_smf(const std::string&);

struct mtrk_event_range_t {
	mtrk_event_t::const_iterator beg;
	mtrk_event_t::const_iterator end;
};

class sequential_range_iterator {
public:
private:
	struct range_pos {
		mtrk_event_t::const_iterator beg;
		mtrk_event_t::const_iterator end;
		mtrk_event_t::const_iterator curr;
		uint32_t tkonset;
	};
	std::vector<range_pos> r_;
	std::vector<range_pos>::iterator curr_;
	uint32_t tkonset_;
};
class simultaneous_range_iterator {
public:
private:
	struct range_pos {
		mtrk_event_t::const_iterator beg;
		mtrk_event_t::const_iterator end;
		mtrk_event_t::const_iterator curr;
		uint32_t tkonset;
	};
	std::vector<range_pos> r_;
	std::vector<range_pos>::iterator curr_;
	uint32_t tkonset_;
};

/*class smf2_chrono_iterator {
public:
private:
	struct event_range_t {
		mtrk_iterator_t beg;
		mtrk_iterator_t end;
	};
	std::vector<event_range_t> trks_;
};*/


struct all_smf_events_dt_ordered_t {
	mtrk_event_t ev;
	uint32_t cumtk;
	int trackn;
};
std::vector<all_smf_events_dt_ordered_t> get_events_dt_ordered(const smf_t&);
std::string print(const std::vector<all_smf_events_dt_ordered_t>&);

/*
struct linked_pair_with_trackn_t {
	int trackn;
	linked_onoff_pair_t ev_pair;
};
struct orphan_onoff_with_trackn_t {
	int trackn;
	orphan_onoff_t orph_ev;
};
struct linked_and_orphans_with_trackn_t {
	std::vector<linked_pair_with_trackn_t> linked {};
	std::vector<orphan_onoff_with_trackn_t> orphan_on {};
	std::vector<orphan_onoff_with_trackn_t> orphan_off {};
};
linked_and_orphans_with_trackn_t get_linked_onoff_pairs(const smf_t&);
std::string print(const linked_and_orphans_with_trackn_t&);
*/




/*
struct smf_simultaneous_event_range_t {
	std::vector<simultaneous_event_range_t> trks;
};
smf_simultaneous_event_range_t 
make_smf_simultaneous_event_range(mtrk_iterator_t beg, mtrk_iterator_t end);
*/



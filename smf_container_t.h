#pragma once
#include "midi_raw.h"
#include "mthd_container_t.h"
#include "mtrk_container_t.h"
#include <string>
#include <cstdint>

//
// smf_container_t & friends
//
// As was the case for the mtrk_event_container_t and mtrk_container_t above, an 
// smf_container_t is nothing more than an assurance that the pointed to range is a valid 
// smf.  It contains exactly one MThd chunk and one or more unknown or MTrk chunks.  The
// sizes of all chunks are validated, so the chunk_container_t returned from the appropriate
// methods are guaranteed correct.  Further, for the MThd and Mtrk chunk types, all sub-data
// is validated (ie, all events in an MTrk are valid).
//
//
class smf_container_t {
public:
	smf_container_t(const validate_smf_result_t&);

	mthd_container_t get_header() const;
	mtrk_container_t get_track(int) const;
	bool get_chunk(int) const;  // unknown chunks allowed...
	std::string fname() const;

	int n_tracks() const;
	int n_chunks() const;  // Includes MThd
private:
	std::string fname_ {};
	std::vector<chunk_idx_t> chunk_idxs_ {};

	// Total number of chunks is n_unknown + n_mtrk + 1 (MThd)
	int8_t n_mtrk_ {0};
	int8_t n_unknown_ {0};
	const unsigned char *p_ {};
	int32_t size_ {0};
};

std::string print(const smf_container_t&);
bool play(const smf_container_t&);



//
// smf_t
//
// Holds an smf; owns the underlying data
//
//
class yay_mtrk_event_iterator_t;

class smf_t {
public:
	smf_t(const validate_smf_result_t&);

	uint32_t size() const;
	uint16_t nchunks() const;

	uint16_t format() const;
	uint16_t ntrks() const;

	yay_mtrk_event_iterator_t event_iterator_begin(int);  // int track number
	yay_mtrk_event_iterator_t event_iterator_end(int);  // int track number
private:
	std::string fname_ {};
	
};

//
// Operations
//
// Print a list of on and off times for each note in each track in the file 
// in order of cumtime-of-event for:
// ex:  cumtime-of-event; 'on'; C4; velocity=x;
// a) concurrent tracks
// b) sequential tracks
// 1)  Caller obtains ntrks track iterators, caller can integrate the delta-times.
//     yay_mtrk_event_iterator_t derefs to an mtrk_event_container_sbo_t:
//
//
// struct time_integrating_ymei_t {
//   yay_mtrk_event_iterator_t it;
//   midi_time_obj_t cumt;
//   operator++()...;
//   operator<()...;
// }
// std::vector<time_integrating_ymei_t> ymeis {ymei1,ymei2,ymei3};
// while (true) {
//   for (auto& curr_ymei=std::min(ymeis); (*(curr_ymei.it)).delta_t()==0; ++curr_ymei) {
//     if (cev1.type()==midi_channel) {
//       curr_timepoint.push_back({1,,cev1.onoff(),cev1.ch(),cev1.pitch(),cev1.velocity()});
//     }
//   }
// }
//   
//
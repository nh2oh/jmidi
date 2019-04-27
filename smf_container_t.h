#pragma once
#include "midi_raw.h"
#include "mthd_container_t.h"
#include "mtrk_container_t.h"
#include <string>
#include <cstdint>
#include <vector>


//
// smf_t
//
// Holds an smf; owns the underlying data.  Splits each chunk into its
// own std::vector<unsigned char>
//
//
// Alternate design:  Can hold mthd_view_t, mtrk_view_t objects as members.
//
class smf_chrono_iterator_t;

class smf_t {
public:
	smf_t(const validate_smf_result_t&);

	uint32_t size() const;
	uint16_t nchunks() const;
	uint16_t format() const;  // mthd alias
	uint16_t division() const;  // mthd alias
	uint32_t mthd_size() const;  // mthd alias
	uint32_t mthd_data_length() const;  // mthd alias
	uint16_t ntrks() const;
	std::string fname() const;

	mtrk_view_t get_track_view(int) const;
	mthd_view_t get_header_view() const;

	// Returns mtrk events in order by tonset
	smf_chrono_iterator_t event_iterator_begin();  // int track number
	smf_chrono_iterator_t event_iterator_end();  // int track number
private:
	std::string fname_ {};
	std::vector<std::vector<unsigned char>> d_ {};  // All chunks, incl ::unknown
};
std::string print(const smf_t&);

// Draft of the the chrono_iterator
std::string print_events_chrono(const smf_t&);


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


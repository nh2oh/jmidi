#pragma once
#include "midi_raw.h"
#include "mthd_t.h"
#include "mtrk_event_t.h"
#include "mtrk_t.h"
#include <string>
#include <cstdint>
#include <vector>


//
// smf2_t
//
// Holds an smf; owns the underlying data.  Stores the mtrk chunks as
// a std::vector<std::vector<mtrk_event_t>>.  
//
class smf2_t {
public:
	uint32_t size() const;
	uint16_t nchunks() const;
	uint16_t format() const;  // mthd alias
	uint16_t division() const;  // mthd alias
	uint32_t mthd_size() const;  // mthd alias
	uint32_t mthd_data_size() const;  // mthd alias
	uint32_t track_size(int) const;
	uint32_t track_data_size(int) const;
	uint16_t ntrks() const;
	std::string fname() const;

	mthd_view_t get_header_view() const;
	const mthd_t& get_header() const;
	const mtrk_t& get_track(int) const;
	
	void set_fname(const std::string&);
	void set_mthd(const validate_mthd_chunk_result_t&);
	void append_mtrk(const mtrk_t&);
	void append_uchk(const std::vector<unsigned char>&);
private:
	std::string fname_ {};
	mthd_t mthd_ {};
	std::vector<mtrk_t> mtrks_ {};
	std::vector<std::vector<unsigned char>> uchks_ {};
};
std::string print(const smf2_t&);

struct maybe_smf2_t {
	smf2_t smf;
	std::string error {"No error"};
	operator bool() const;
};
maybe_smf2_t read_smf2(const std::string&);


struct smf_simultanious_event_range_t {
	std::vector<simultanious_event_range_t> trks;
};
smf_simultanious_event_range_t 
make_smf_simultanious_event_range(mtrk_iterator_t beg, mtrk_iterator_t end);

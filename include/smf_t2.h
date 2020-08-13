#include <vector>
#include <cstdint>
#include "mtrk_t.h"
#include "midi_time.h"  // time_division_t


namespace jmid {

//
// The mthd data is encoded in fmt_, tdiv_, and mtrks_.size()+uchks.size().  
//
// The fmt <-> ntrks relationship mandated by the midi standard is _not_
// maintained.  This is to allow a user to build an object step by step by
// calling methods, which may require the object to pass through states that
// are invalid by the midi std.  
//
// TODO:
// - Does the {} construction of the std::vector members allocate?
//


class smf_t2 {
public:
	//...

	smf_t2();
	smf_t2(std::int32_t, std::int32_t, jmid::time_division_t);  // ntrks, fmt, tdiv
	smf_t2(const smf_t2&);
	smf_t2(smf_t2&&);
	smf_t2& operator=(const smf_t2&);
	smf_t2& operator=(smf_t2&&);
	~smf_t2();

	std::int64_t size() const;
	std::int64_t nchunks() const;
	std::int64_t ntrks() const;
	std::int64_t nuchks() const;
	std::int64_t nbytes() const;


	//
	// MThd data accessors/modifiers
	//
	std::int32_t format() const;
	jmid::time_division_t time_division() const;

private:
	std::vector<jmid::mtrk_t> mtrks_ {};
	std::vector<std::vector<unsigned char>> uchks_ {};
	std::vector<int> chunkorder_ {};
	std::int32_t fmt_ {0};
	// std::int32_t ntrks_;  // Not needed; mtrks_.size()
	jmid::time_division_t tdiv_ {};
};

}  // namespace jmid







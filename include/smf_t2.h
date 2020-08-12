#include <vector>
#include <cstdint>
#include "mtrk_t.h"
#include "midi_time.h"  // time_division_t


namespace jmid {

//
// The mthd data is encoded in fmt_, tdiv_, and mtrks_.size()+uchks.size().  
//
// The fmt <-> ntrks relationship mandated by the midi standard is _not_
// maintained.  
//

class smf_t2 {
public:
	//...

	smf_t2() noexcept;
	smf_t2(const smf_t2&) noexcept;
	smf_t2(smf_t2&&) noexcept;
	smf_t2& operator=(const smf_t2&) noexcept;
	smf_t2& operator=(smf_t2&&) noexcept;
	~smf_t2() noexcept;


private:
	std::vector<jmid::mtrk_t> mtrks_ {};
	std::vector<std::vector<unsigned char>> uchks_ {};
	std::vector<int> chunkorder_ {};
	std::int32_t fmt_ {0};
	// std::int32_t ntrks_;  // Not needed; mtrks_.size()
	jmid::time_division_t tdiv_ {};
};

}  // namespace jmid







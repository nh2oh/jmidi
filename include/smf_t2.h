#include <vector>
#include <cstdint>
#include "mtrk_t.h"
#include "midi_time.h"


namespace jmid {

class smf_t2 {
public:
	//...
private:
	std::vector<jmid::mtrk_t> mtrks_;
	std::vector<std::vector<unsigned char>> uchks_;
	std::vector<int> chunkorder_;
	std::int32_t fmt_;
	// std::int32_t ntrks_;  // Not needed; mtrks_.size()
	jmid::time_division_t tdiv_;
};

}  // namespace jmid







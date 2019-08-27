#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include "midi_vlq.h"
#include "midi_delta_time.h"
#include <vector>
#include <cstdlib>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
	auto dt_field = jmid::read_delta_time(Data,Data+Size);
	auto dt = jmid::to_nearest_valid_delta_time(dt_field.val);
	
	std::vector<uint8_t> vdata(Data,Data+Size);
	
	auto f0 = jmid::make_sysex_f0(dt,vdata);
	auto f7 = jmid::make_sysex_f7(dt,vdata);
	
	if (!jmid::is_sysex_f0(f0)) { std::abort(); }
	if (!jmid::is_sysex_f7(f7)) { std::abort(); }
	
	if (f0.delta_time()!=dt) { std::abort(); }
	if (f7.delta_time()!=dt) { std::abort(); }
	if (f0.status_byte()!=0xF0u) { std::abort(); }
	if (f7.status_byte()!=0xF7u) { std::abort(); }
	
	auto itf0 = f0.event_begin()+1;
	auto len_f0 = jmid::read_vlq(itf0,f0.end());
	itf0 += len_f0.N;
	for (std::size_t i=0; i<Size; ++i) { 
		if (Data[i]!=*itf0++) { std::abort(); }
	}
	auto itf0_end = f0.end();  --itf0_end;
	if (*itf0_end != 0xF7u) { std::abort(); }
	
	auto itf7 = f7.event_begin()+1;
	auto len_f7 = jmid::read_vlq(itf7,f7.end());
	itf7 += len_f7.N;
	for (std::size_t i=0; i<Size; ++i) { 
		if (Data[i]!=*itf7++) { std::abort(); }
	}
	auto itf7_end = f7.end();  --itf7_end;
	if (*itf7_end != 0xF7u) { std::abort(); }
	
	return 0;
}


#include "mtrk_event_t.h"
#include "midi_delta_time.h"
#include "mtrk_event_methods.h"
#include "midi_status_byte.h"
#include <cstdlib>


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
	if (Size < 2) {
		return 0;
	}
	
	auto rs = Data[0];
	auto beg = Data+1;
	auto end = Data+Size;
	jmid::mtrk_event_error_t mtrk_error;
	jmid::maybe_mtrk_event_t maybe_ev = make_mtrk_event(beg,end,rs,&mtrk_error,(Size-1));
	if (!maybe_ev) {
		return 0;
	}
	auto ev = maybe_ev.event;
	
	auto expect_dt = jmid::read_delta_time(beg,end);
	if (expect_dt.val != ev.delta_time()) {
		std::abort();
	}
	
	if (jmid::is_meta(ev)) {
		if (!jmid::is_meta_status_byte(ev.status_byte())) {
			std::abort();
		}
	} else if (jmid::is_sysex(ev)) {
		if (!jmid::is_sysex_status_byte(ev.status_byte())) {
			std::abort();
		}
	} else if (jmid::is_channel(ev)) {
		if (!jmid::is_channel_status_byte(ev.status_byte())) {
			std::abort();
		}
	} else {
		std::abort();
	}

	return 0;
}

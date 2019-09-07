#include "deprecated_make_mtrk_event.h"



jmid::maybe_mtrk_event_t::operator bool() const {
	auto tf = (this->error==jmid::mtrk_event_error_t::errc::no_error);
	return tf;
}



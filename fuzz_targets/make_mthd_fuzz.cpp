#include "mthd_t.h"
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
	std::string error_str;
	int n = 0;
	
	jmid::mthd_error_t mthd_error;
	jmid::maybe_mthd_t mthd_result = jmid::make_mthd(Data, Data+Size, &mthd_error, Size);
	
	if (!mthd_result) {
		error_str = jmid::explain(mthd_error);
	} else {
		n = mthd_result.mthd.size();
	}

	return 0;
}

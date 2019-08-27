#include "smf_t.h"
#include <string>


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
	std::string error_str;
	int n = 0;
	
	jmid::smf_error_t smf_error;
	jmid::maybe_smf_t smf_result;
	jmid::make_smf(Data, Data+Size, &smf_result, &smf_error, Size);
	if (!smf_result) {
		error_str = jmid::explain(smf_error);
	} else {
		n = smf_result.smf.nbytes();
	}

	return 0;
}

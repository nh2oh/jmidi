#pragma once
#include <vector>


//
// Status bytes,data bytes; valid and invalid
// "sbs" == "status bytes"
// "dbs" == "data bytes"
//
extern const std::vector<unsigned char> sbs_invalid;
extern const std::vector<unsigned char> sbs_meta_sysex;
extern const std::vector<unsigned char> sbs_unrecognized;
extern const std::vector<unsigned char> sbs_ch_mode_voice;
extern const std::vector<unsigned char> dbs_valid;
extern const std::vector<unsigned char> dbs_invalid;

#include "midi_examples.h"
#include "midi_raw.h"
#include "mthd_container_t.h"
#include "mtrk_container_t.h"
#include "smf_container_t.h"
#include "midi_container.h"
#include "midi_utils.h"
#include "dbklib\binfile.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <type_traits>

struct tinner_t {  // sizeof() == 23  TODO:  24 !
	unsigned char *p;
	uint32_t size;
	uint32_t capacity;
	uint32_t dt_fixed;
	smf_event_type sevt;  // "smf_event_type"
	unsigned char midi_status;
	uint8_t unused;
	unsigned char xxed;
};

class t1_t {  // sizeof() == 24
private:
	tinner_t x;
	unsigned char xx;
};


int midi_example() {

	//auto rawfiledata = dbk::readfile("C:\\Users\\ben\\Desktop\\scr\\CLEMENTI.MID").d;
	auto rawfiledata = dbk::readfile("C:\\Users\\ben\\Desktop\\scr\\test.mid").d;
	auto rawfile_check_result = validate_smf(&rawfiledata[0],rawfiledata.size(),
		"C:\\Users\\ben\\Desktop\\scr\\CLEMENTI.MID");

	smf_container_t mf {rawfile_check_result};
	
	std::cout << print(mf) << std::endl << std::endl;
	//std::cout << print_notelist(mf.get_track(1)) << std::endl << std::endl;

	//auto h = mf.get_header();
	//std::cout << print(h) << std::endl << std::endl;
	//auto t1 = mf.get_track(0);
	//std::cout << "TRACK 1\n" << print(t1) << std::endl << std::endl;
	//auto t2 = mf.get_track(1);
	//std::cout << "TRACK 2\n" << print(t2) << std::endl << std::endl;
	//auto t3 = mf.get_track(2);
	//std::cout << "TRACK 3\n" << print(t3) << std::endl << std::endl;


	constexpr auto ttinner = sizeof(tinner_t);
	constexpr auto t1 = sizeof(t1_t);
	constexpr auto tstr = sizeof(mtrk_event_container_sbo_t);

	return 0;
}



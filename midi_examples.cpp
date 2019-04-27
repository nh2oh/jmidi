#include "midi_examples.h"
#include "midi_raw.h"
#include "midi_raw_test_parts.h"
#include "mthd_container_t.h"
#include "mtrk_container_t.h"
#include "smf_container_t.h"
#include "midi_container.h"
#include "midi_utils.h"
#include "dbklib\binfile.h"
#include "dbklib\byte_manipulation.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <type_traits>

int midi_example() {
	//testdata::print_midi_test_cases();

	std::string fn = "C:\\Users\\ben\\Desktop\\scr\\CLEMENTI.MID";
	//std::string fn = "C:\\Users\\ben\\Desktop\\scr\\test.mid";
	//std::string fn = "C:\\Users\\ben\\Desktop\\scr\\hallelujah_joy_to_the_world.mid";
	auto rawfiledata = dbk::readfile(fn).d;
	//auto rawfiledata = dbk::readfile("C:\\Users\\ben\\Desktop\\scr\\test.mid").d;
	auto rawfile_check_result = validate_smf(&rawfiledata[0],rawfiledata.size(),fn);

	//smf_container_t mf {rawfile_check_result};
	smf_t mf(rawfile_check_result);
	
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

	return 0;
}



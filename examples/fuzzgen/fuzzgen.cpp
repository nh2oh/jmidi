#include <iostream>
#include <random>
#include <string>
#include <iterator>
#include <array>
#include "print_hexascii.h"
#include "midi_raw_test_parts.h"
#include "midi_vlq.h"
#include "make_mtrk_event.h"
#include "mtrk_event_methods.h"

int main() {
	std::random_device rdev;
	std::mt19937 re(rdev());

	std::vector<unsigned char> seq;

	for (int i=0; i<20; ++i) {
		seq.clear();
		jmid::rand::make_random_sequence(re,seq);
		
		jmid::mtrk_event_error_t curr_err;
		jmid::mtrk_event_t curr_event;
		curr_err.code = jmid::mtrk_event_error_t::errc::no_error;
		for (auto it = seq.begin(); it!=seq.end(); true) {
			it = jmid::make_mtrk_event3(it,seq.end(),
				0x00u,&curr_event,&curr_err);
			if (curr_err.code != jmid::mtrk_event_error_t::errc::no_error) {
				break;
			}
			std::cout << jmid::print(curr_event) << std::endl;
		}

		std::cout << "--------------------------------\n";
	}


	return 0;
}


void demo_vlqs() {
	std::random_device rdev;
	std::mt19937 re(rdev());

	for (int i=0; i<200; ++i) {
		auto rvlq = jmid::rand::make_random_vlq(re);
		std::string sf;  std::string scf;
		jmid::print_hexascii(rvlq.field.begin(),rvlq.field.end(),
			std::back_inserter(sf),'\0',' ');
		jmid::print_hexascii(rvlq.canonical_field.begin(),
			rvlq.canonical_field.end(),std::back_inserter(scf),'\0',' ');

		std::cout << sf
			<< ";\t encoded_value == " 
			<< std::to_string(rvlq.encoded_value) 
			<< ", field_size == " << std::to_string(rvlq.field_size)
			<< "\n" << scf
			<< ";\t canonical field_size == " 
			<< std::to_string(rvlq.canonical_field_size)
			<< "\n------------------------------------------"
			<< "--------------------------------------------\n";

		auto f = jmid::rand::read_vlq_max64(rvlq.field.begin());
		auto cf = jmid::rand::read_vlq_max64(rvlq.canonical_field.begin());
		if (f.val != cf.val) {
			std::cout << "trouble\n";
		}

		std::array<unsigned char,10> test_write {0,0,0,0,0,0,0,0,0,0};
		jmid::rand::write_vlq_max64(cf.val,test_write.begin());
		auto tf = jmid::rand::read_vlq_max64(test_write.begin());
		if ((cf.val != tf.val) || (cf.N != tf.N)) {
			std::cout << "trouble\n";
		}
	}
}



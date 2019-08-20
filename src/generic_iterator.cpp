#include "generic_iterator.h"
#include <vector>

struct gi_test_types_a_t {
	using value_type = int;
	using size_type = uint32_t;
	using difference_type = std::ptrdiff_t;  // TODO:  Inconsistent
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
};

void generic_iterator_tests() {
	std::vector<int> vi {0,1,2,3,4,5,6,7,8,9};
	int *pbeg = &vi[0];  int *pend = &vi[9]+1;
	const int *cpbeg = &vi[0];  const int *cpend = &vi[9]+1;
	std::ptrdiff_t n = 3;

	auto a = generic_ra_iterator<decltype(vi)>(pbeg);
	auto b = generic_ra_iterator<decltype(vi)>(pend);
	auto ca = generic_ra_const_iterator<decltype(vi)>(cpbeg);
	auto cb = generic_ra_const_iterator<decltype(vi)>(cpend);

	auto plus_a3 = a+3;
	auto plus_3a = 3+a;
	auto plus_ca3 = ca+3;
	auto plus_3ca = 3+ca;
	auto minus_a3 = a-3;
	//auto minus_3a = 3-a;
	auto minus_ca3 = ca-3;
	//auto minus_3ca = 3-ca;

	bool a_b = a==b;
	bool ca_cb = ca==cb;
	//bool a_cb = a==cb;
	bool cb_a = cb==a;  // a goes through the converting ctor const_it(it)
	bool ca_b = ca==b;  // a goes through the converting ctor const_it(it)

	auto vit = vi.begin();
	auto vcit = vi.cbegin();

	bool vit_vcit = (vit==vcit);
	bool vcit_vit = (vcit==vit);
};






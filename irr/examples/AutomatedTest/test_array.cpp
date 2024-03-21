#include <irrArray.h>
#include "test_helper.h"

using namespace irr;
using core::array;

static void test_basics()
{
	array<int> v;
	v.push_back(1);  // 1
	v.push_front(2); // 2, 1
	v.insert(4, 0);  // 4, 2, 1
	v.insert(3, 1);  // 4, 3, 2, 1
	v.insert(0, 4);  // 4, 3, 2, 1, 0
	UASSERTEQ(v.size(), 5);
	UASSERTEQ(v[0], 4);
	UASSERTEQ(v[1], 3);
	UASSERTEQ(v[2], 2);
	UASSERTEQ(v[3], 1);
	UASSERTEQ(v[4], 0);
	array<int> w = v;
	UASSERTEQ(w.size(), 5);
	UASSERT(w == v);
	w.clear();
	UASSERTEQ(w.size(), 0);
	UASSERTEQ(w.allocated_size(), 0);
	UASSERT(w.empty());
	w = v;
	UASSERTEQ(w.size(), 5);
	w.set_used(3);
	UASSERTEQ(w.size(), 3);
	UASSERTEQ(w[0], 4);
	UASSERTEQ(w[1], 3);
	UASSERTEQ(w[2], 2);
	UASSERTEQ(w.getLast(), 2);
	w.set_used(20);
	UASSERTEQ(w.size(), 20);
	w = v;
	w.sort();
	UASSERTEQ(w.size(), 5);
	UASSERTEQ(w[0], 0);
	UASSERTEQ(w[1], 1);
	UASSERTEQ(w[2], 2);
	UASSERTEQ(w[3], 3);
	UASSERTEQ(w[4], 4);
	w.erase(0);
	UASSERTEQ(w.size(), 4);
	UASSERTEQ(w[0], 1);
	UASSERTEQ(w[1], 2);
	UASSERTEQ(w[2], 3);
	UASSERTEQ(w[3], 4);
	w.erase(1, 2);
	UASSERTEQ(w.size(), 2);
	UASSERTEQ(w[0], 1);
	UASSERTEQ(w[1], 4);
	w.swap(v);
	UASSERTEQ(w.size(), 5);
	UASSERTEQ(v.size(), 2);
}

static void test_linear_searches()
{
	// Populate the array with 0, 1, 2, ..., 100, 100, 99, 98, 97, ..., 0
	array<int> arr;
	for (int i = 0; i <= 100; i++)
		arr.push_back(i);
	for (int i = 100; i >= 0; i--)
		arr.push_back(i);
	s32 end = arr.size() - 1;
	for (int i = 0; i <= 100; i++) {
		s32 index = arr.linear_reverse_search(i);
		UASSERTEQ(index, end - i);
	}
	for (int i = 0; i <= 100; i++) {
		s32 index = arr.linear_search(i);
		UASSERTEQ(index, i);
	}
}

static void test_binary_searches()
{
	const auto &values = {3, 5, 1, 2, 5, 10, 19, 9, 7, 1, 2, 5, 8, 15};
	array<int> arr;
	for (int value : values) {
		arr.push_back(value);
	}
	// Test the const form first, it uses a linear search without sorting
	const array<int> &carr = arr;
	UASSERTEQ(carr.binary_search(20), -1);
	UASSERTEQ(carr.binary_search(0), -1);
	UASSERTEQ(carr.binary_search(1), 2);

	// Sorted: 1, 1, 2, 2, 3, 5, 5, 5, 7, 8, 9, 10, 15, 19
	UASSERTEQ(arr.binary_search(20), -1);
	UASSERTEQ(arr.binary_search(0), -1);

	for (int value : values) {
		s32 i = arr.binary_search(value);
		UASSERTNE(i, -1);
		UASSERTEQ(arr[i], value);
	}

	s32 first, last;
	first = arr.binary_search_multi(1, last);
	UASSERTEQ(first, 0);
	UASSERTEQ(last, 1);

	first = arr.binary_search_multi(2, last);
	UASSERTEQ(first, 2);
	UASSERTEQ(last, 3);

	first = arr.binary_search_multi(3, last);
	UASSERTEQ(first, 4);
	UASSERTEQ(last, 4);

	first = arr.binary_search_multi(4, last);
	UASSERTEQ(first, -1);

	first = arr.binary_search_multi(5, last);
	UASSERTEQ(first, 5);
	UASSERTEQ(last, 7);

	first = arr.binary_search_multi(7, last);
	UASSERTEQ(first, 8);
	UASSERTEQ(last, 8);

	first = arr.binary_search_multi(19, last);
	UASSERTEQ(first, 13);
	UASSERTEQ(last, 13);
}

void test_irr_array()
{
	test_basics();
	test_linear_searches();
	test_binary_searches();
	std::cout << "    test_irr_array PASSED" << std::endl;
}

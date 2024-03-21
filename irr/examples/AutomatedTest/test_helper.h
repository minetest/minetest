#pragma once

#include <exception>
#include <iostream>

class TestFailedException : public std::exception
{
};

// Asserts the comparison specified by CMP is true, or fails the current unit test
#define UASSERTCMP(CMP, actual, expected)                                         \
	do {                                                                          \
		const auto &a = (actual);                                                 \
		const auto &e = (expected);                                               \
		if (!CMP(a, e)) {                                                         \
			std::cout                                                             \
					<< "Test assertion failed: " << #actual << " " << #CMP << " " \
					<< #expected << std::endl                                     \
					<< "    at " << __FILE__ << ":" << __LINE__ << std::endl      \
					<< "    actual:   " << a << std::endl                         \
					<< "    expected: "                                           \
					<< e << std::endl;                                            \
			throw TestFailedException();                                          \
		}                                                                         \
	} while (0)

#define CMPEQ(a, e) (a == e)
#define CMPTRUE(a, e) (a)
#define CMPNE(a, e) (a != e)

#define UASSERTEQ(actual, expected) UASSERTCMP(CMPEQ, actual, expected)
#define UASSERTNE(actual, nexpected) UASSERTCMP(CMPNE, actual, nexpected)
#define UASSERT(actual) UASSERTCMP(CMPTRUE, actual, true)

/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include <exception>
#include <vector>

#include "irrlichttypes_extrabloated.h"
#include "porting.h"
#include "filesys.h"
#include "mapnode.h"

class TestFailedException : public std::exception {
};

// Runs a unit test and reports results
#define TEST(fxn, ...) {                                                      \
	u64 t1 = porting::getTimeMs();                                            \
	try {                                                                     \
		fxn(__VA_ARGS__);                                                     \
		rawstream << "[PASS] ";                                               \
	} catch (TestFailedException &e) {                                        \
		rawstream << "[FAIL] ";                                               \
		num_tests_failed++;                                                   \
	} catch (std::exception &e) {                                             \
		rawstream << "Caught unhandled exception: " << e.what() << std::endl; \
		rawstream << "[FAIL] ";                                               \
		num_tests_failed++;                                                   \
	}                                                                         \
	num_tests_run++;                                                          \
	u64 tdiff = porting::getTimeMs() - t1;                                    \
	rawstream << #fxn << " - " << tdiff << "ms" << std::endl;                 \
}

// Asserts the specified condition is true, or fails the current unit test
#define UASSERT(x)                                              \
	if (!(x)) {                                                 \
		rawstream << "Test assertion failed: " #x << std::endl  \
			<< "    at " << fs::GetFilenameFromPath(__FILE__)   \
			<< ":" << __LINE__ << std::endl;                    \
		throw TestFailedException();                            \
	}

// Asserts the specified condition is true, or fails the current unit test
// and prints the format specifier fmt
#define UTEST(x, fmt, ...)                                               \
	if (!(x)) {                                                          \
		char utest_buf[1024];                                            \
		snprintf(utest_buf, sizeof(utest_buf), fmt, __VA_ARGS__);        \
		rawstream << "Test assertion failed: " << utest_buf << std::endl \
			<< "    at " << fs::GetFilenameFromPath(__FILE__)            \
			<< ":" << __LINE__ << std::endl;                             \
		throw TestFailedException();                                     \
	}

// Asserts the comparison specified by CMP is true, or fails the current unit test
#define UASSERTCMP(T, CMP, actual, expected) {                            \
	T a = (actual);                                                       \
	T e = (expected);                                                     \
	if (!(a CMP e)) {                                                     \
		rawstream                                                         \
			<< "Test assertion failed: " << #actual << " " << #CMP << " " \
			<< #expected << std::endl                                     \
			<< "    at " << fs::GetFilenameFromPath(__FILE__) << ":"      \
			<< __LINE__ << std::endl                                      \
			<< "    actual  : " << a << std::endl << "    expected: "     \
			<< e << std::endl;                                            \
		throw TestFailedException();                                      \
	}                                                                     \
}

#define UASSERTEQ(T, actual, expected) UASSERTCMP(T, ==, actual, expected)

// UASSERTs that the specified exception occurs
#define EXCEPTION_CHECK(EType, code) {    \
	bool exception_thrown = false;        \
	try {                                 \
		code;                             \
	} catch (EType &e) {                  \
		exception_thrown = true;          \
	}                                     \
	UASSERT(exception_thrown);            \
}

class IGameDef;

class TestBase {
public:
	bool testModule(IGameDef *gamedef);
	std::string getTestTempDirectory();
	std::string getTestTempFile();

	virtual void runTests(IGameDef *gamedef) = 0;
	virtual const char *getName() = 0;

	u32 num_tests_failed;
	u32 num_tests_run;

private:
	std::string m_test_dir;
};

class TestManager {
public:
	static std::vector<TestBase *> &getTestModules()
	{
		static std::vector<TestBase *> m_modules_to_test;
		return m_modules_to_test;
	}

	static void registerTestModule(TestBase *module)
	{
		getTestModules().push_back(module);
	}
};

// A few item and node definitions for those tests that need them
extern content_t t_CONTENT_STONE;
extern content_t t_CONTENT_GRASS;
extern content_t t_CONTENT_TORCH;
extern content_t t_CONTENT_WATER;
extern content_t t_CONTENT_LAVA;
extern content_t t_CONTENT_BRICK;

bool run_tests();

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

#include "test.h"

#include <sstream>

#include "log.h"
#include "serialization.h"
#include "nodedef.h"
#include "noise.h"

class TestFilePath : public TestBase {
public:
	TestFilePath() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestFilePath"; }

	void runTests(IGameDef *gamedef);

	void testIsDirDelimiter();
	void testPathStartsWith();
	void testRemoveLastPathComponent();
	void testRemoveLastPathComponentWithTrailingDelimiter();
	void testRemoveRelativePathComponent();
};

static TestFilePath g_test_instance;

void TestFilePath::runTests(IGameDef *gamedef)
{
	TEST(testIsDirDelimiter);
	TEST(testPathStartsWith);
	TEST(testRemoveLastPathComponent);
	TEST(testRemoveLastPathComponentWithTrailingDelimiter);
	TEST(testRemoveRelativePathComponent);
}

////////////////////////////////////////////////////////////////////////////////

// adjusts a POSIX path to system-specific conventions
// -> changes '/' to DIR_DELIM
// -> absolute paths start with "C:\\" on windows
std::string p(std::string path)
{
	for (size_t i = 0; i < path.size(); ++i) {
		if (path[i] == '/') {
			path.replace(i, 1, DIR_DELIM);
			i += std::string(DIR_DELIM).size() - 1; // generally a no-op
		}
	}

	#ifdef _WIN32
	if (path[0] == '\\')
		path = "C:" + path;
	#endif

	return path;
}


void TestFilePath::testIsDirDelimiter()
{
	UASSERT(fs::IsDirDelimiter('/') == true);
	UASSERT(fs::IsDirDelimiter('A') == false);
	UASSERT(fs::IsDirDelimiter(0) == false);
#ifdef _WIN32
	UASSERT(fs::IsDirDelimiter('\\') == true);
#else
	UASSERT(fs::IsDirDelimiter('\\') == false);
#endif
}


void TestFilePath::testPathStartsWith()
{
	const int numpaths = 12;
	std::string paths[numpaths] = {
		"",
		p("/"),
		p("/home/user/minetest"),
		p("/home/user/minetest/bin"),
		p("/home/user/.minetest"),
		p("/tmp/dir/file"),
		p("/tmp/file/"),
		p("/tmP/file"),
		p("/tmp"),
		p("/tmp/dir"),
		p("/home/user2/minetest/worlds"),
		p("/home/user2/minetest/world"),
	};
	/*
		expected fs::PathStartsWith results
		0 = returns false
		1 = returns true
		2 = returns false on windows, true elsewhere
		3 = returns true on windows, false elsewhere
		4 = returns true if and only if
			FILESYS_CASE_INSENSITIVE is true
	*/
	int expected_results[numpaths][numpaths] = {
		{1,2,0,0,0,0,0,0,0,0,0,0},
		{1,1,0,0,0,0,0,0,0,0,0,0},
		{1,1,1,0,0,0,0,0,0,0,0,0},
		{1,1,1,1,0,0,0,0,0,0,0,0},
		{1,1,0,0,1,0,0,0,0,0,0,0},
		{1,1,0,0,0,1,0,0,1,1,0,0},
		{1,1,0,0,0,0,1,4,1,0,0,0},
		{1,1,0,0,0,0,4,1,4,0,0,0},
		{1,1,0,0,0,0,0,0,1,0,0,0},
		{1,1,0,0,0,0,0,0,1,1,0,0},
		{1,1,0,0,0,0,0,0,0,0,1,0},
		{1,1,0,0,0,0,0,0,0,0,0,1},
	};

	for (int i = 0; i < numpaths; i++)
	for (int j = 0; j < numpaths; j++){
		/*verbosestream<<"testing fs::PathStartsWith(\""
			<<paths[i]<<"\", \""
			<<paths[j]<<"\")"<<std::endl;*/
		bool starts = fs::PathStartsWith(paths[i], paths[j]);
		int expected = expected_results[i][j];
		if(expected == 0){
			UASSERT(starts == false);
		}
		else if(expected == 1){
			UASSERT(starts == true);
		}
		#ifdef _WIN32
		else if(expected == 2){
			UASSERT(starts == false);
		}
		else if(expected == 3){
			UASSERT(starts == true);
		}
		#else
		else if(expected == 2){
			UASSERT(starts == true);
		}
		else if(expected == 3){
			UASSERT(starts == false);
		}
		#endif
		else if(expected == 4){
			UASSERT(starts == (bool)FILESYS_CASE_INSENSITIVE);
		}
	}
}


void TestFilePath::testRemoveLastPathComponent()
{
	std::string path, result, removed;

	UASSERT(fs::RemoveLastPathComponent("") == "");
	path = p("/home/user/minetest/bin/..//worlds/world1");
	result = fs::RemoveLastPathComponent(path, &removed, 0);
	UASSERT(result == path);
	UASSERT(removed == "");
	result = fs::RemoveLastPathComponent(path, &removed, 1);
	UASSERT(result == p("/home/user/minetest/bin/..//worlds"));
	UASSERT(removed == p("world1"));
	result = fs::RemoveLastPathComponent(path, &removed, 2);
	UASSERT(result == p("/home/user/minetest/bin/.."));
	UASSERT(removed == p("worlds/world1"));
	result = fs::RemoveLastPathComponent(path, &removed, 3);
	UASSERT(result == p("/home/user/minetest/bin"));
	UASSERT(removed == p("../worlds/world1"));
	result = fs::RemoveLastPathComponent(path, &removed, 4);
	UASSERT(result == p("/home/user/minetest"));
	UASSERT(removed == p("bin/../worlds/world1"));
	result = fs::RemoveLastPathComponent(path, &removed, 5);
	UASSERT(result == p("/home/user"));
	UASSERT(removed == p("minetest/bin/../worlds/world1"));
	result = fs::RemoveLastPathComponent(path, &removed, 6);
	UASSERT(result == p("/home"));
	UASSERT(removed == p("user/minetest/bin/../worlds/world1"));
	result = fs::RemoveLastPathComponent(path, &removed, 7);
#ifdef _WIN32
	UASSERT(result == "C:");
#else
	UASSERT(result == "");
#endif
	UASSERT(removed == p("home/user/minetest/bin/../worlds/world1"));
}


void TestFilePath::testRemoveLastPathComponentWithTrailingDelimiter()
{
	std::string path, result, removed;

	path = p("/home/user/minetest/bin/..//worlds/world1/");
	result = fs::RemoveLastPathComponent(path, &removed, 0);
	UASSERT(result == path);
	UASSERT(removed == "");
	result = fs::RemoveLastPathComponent(path, &removed, 1);
	UASSERT(result == p("/home/user/minetest/bin/..//worlds"));
	UASSERT(removed == p("world1"));
	result = fs::RemoveLastPathComponent(path, &removed, 2);
	UASSERT(result == p("/home/user/minetest/bin/.."));
	UASSERT(removed == p("worlds/world1"));
	result = fs::RemoveLastPathComponent(path, &removed, 3);
	UASSERT(result == p("/home/user/minetest/bin"));
	UASSERT(removed == p("../worlds/world1"));
	result = fs::RemoveLastPathComponent(path, &removed, 4);
	UASSERT(result == p("/home/user/minetest"));
	UASSERT(removed == p("bin/../worlds/world1"));
	result = fs::RemoveLastPathComponent(path, &removed, 5);
	UASSERT(result == p("/home/user"));
	UASSERT(removed == p("minetest/bin/../worlds/world1"));
	result = fs::RemoveLastPathComponent(path, &removed, 6);
	UASSERT(result == p("/home"));
	UASSERT(removed == p("user/minetest/bin/../worlds/world1"));
	result = fs::RemoveLastPathComponent(path, &removed, 7);
#ifdef _WIN32
	UASSERT(result == "C:");
#else
	UASSERT(result == "");
#endif
	UASSERT(removed == p("home/user/minetest/bin/../worlds/world1"));
}


void TestFilePath::testRemoveRelativePathComponent()
{
	std::string path, result;

	path = p("/home/user/minetest/bin");
	result = fs::RemoveRelativePathComponents(path);
	UASSERT(result == path);
	path = p("/home/user/minetest/bin/../worlds/world1");
	result = fs::RemoveRelativePathComponents(path);
	UASSERT(result == p("/home/user/minetest/worlds/world1"));
	path = p("/home/user/minetest/bin/../worlds/world1/");
	result = fs::RemoveRelativePathComponents(path);
	UASSERT(result == p("/home/user/minetest/worlds/world1"));
	path = p(".");
	result = fs::RemoveRelativePathComponents(path);
	UASSERT(result == "");
	path = p("../a");
	result = fs::RemoveRelativePathComponents(path);
	UASSERT(result == "");
	path = p("./subdir/../..");
	result = fs::RemoveRelativePathComponents(path);
	UASSERT(result == "");
	path = p("/a/b/c/.././../d/../e/f/g/../h/i/j/../../../..");
	result = fs::RemoveRelativePathComponents(path);
	UASSERT(result == p("/a/e"));
}

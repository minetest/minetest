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
#include <algorithm>

#include "util/numeric.h"
#include "util/serialize.h"
#include "util/string.h"

#include "irrlichttypes_extrabloated.h"
#include "debug.h"
#include "map.h"
#include "player.h"
#include "socket.h"
#include "serialization.h"
#include "voxel.h"
#include "collision.h"
#include "porting.h"
#include "content_mapnode.h"
#include "nodedef.h"
#include "mapsector.h"
#include "settings.h"
#include "log.h"
#include "filesys.h"
#include "voxelalgorithms.h"
#include "inventory.h"
#include "network/connection.h"
#include "network/networkprotocol.h" // LATEST_PROTOCOL_VERSION
#include "noise.h"
#include "profiler.h"

/*
	Asserts that the exception occurs
*/
#define EXCEPTION_CHECK(EType, code)\
{\
	bool exception_thrown = false;\
	try{ code; }\
	catch(EType &e) { exception_thrown = true; }\
	UASSERT(exception_thrown);\
}

#define UTEST(x, fmt, ...)\
{\
	if(!(x)){\
		dstream << "Test (" #x ") failed: " fmt << std::endl; \
		test_failed = true;\
	}\
}

#define UASSERT(x) UTEST(x, "UASSERT")

/*
	A few item and node definitions for those tests that need them
*/

static content_t CONTENT_STONE;
static content_t CONTENT_GRASS;
static content_t CONTENT_TORCH;

void define_some_nodes(IWritableItemDefManager *idef, IWritableNodeDefManager *ndef)
{
	ItemDefinition itemdef;
	ContentFeatures f;

	/*
		Stone
	*/
	itemdef = ItemDefinition();
	itemdef.type = ITEM_NODE;
	itemdef.name = "default:stone";
	itemdef.description = "Stone";
	itemdef.groups["cracky"] = 3;
	itemdef.inventory_image = "[inventorycube"
		"{default_stone.png"
		"{default_stone.png"
		"{default_stone.png";
	f = ContentFeatures();
	f.name = itemdef.name;
	for(int i = 0; i < 6; i++)
		f.tiledef[i].name = "default_stone.png";
	f.is_ground_content = true;
	idef->registerItem(itemdef);
	CONTENT_STONE = ndef->set(f.name, f);

	/*
		Grass
	*/
	itemdef = ItemDefinition();
	itemdef.type = ITEM_NODE;
	itemdef.name = "default:dirt_with_grass";
	itemdef.description = "Dirt with grass";
	itemdef.groups["crumbly"] = 3;
	itemdef.inventory_image = "[inventorycube"
		"{default_grass.png"
		"{default_dirt.png&default_grass_side.png"
		"{default_dirt.png&default_grass_side.png";
	f = ContentFeatures();
	f.name = itemdef.name;
	f.tiledef[0].name = "default_grass.png";
	f.tiledef[1].name = "default_dirt.png";
	for(int i = 2; i < 6; i++)
		f.tiledef[i].name = "default_dirt.png^default_grass_side.png";
	f.is_ground_content = true;
	idef->registerItem(itemdef);
	CONTENT_GRASS = ndef->set(f.name, f);

	/*
		Torch (minimal definition for lighting tests)
	*/
	itemdef = ItemDefinition();
	itemdef.type = ITEM_NODE;
	itemdef.name = "default:torch";
	f = ContentFeatures();
	f.name = itemdef.name;
	f.param_type = CPT_LIGHT;
	f.light_propagates = true;
	f.sunlight_propagates = true;
	f.light_source = LIGHT_MAX-1;
	idef->registerItem(itemdef);
	CONTENT_TORCH = ndef->set(f.name, f);
}

struct TestBase
{
	bool test_failed;
	TestBase():
		test_failed(false)
	{}
};

struct TestUtilities: public TestBase
{
	inline float ref_WrapDegrees180(float f)
	{
		// This is a slower alternative to the wrapDegrees_180() function;
		// used as a reference for testing
		float value = fmodf(f + 180, 360);
		if (value < 0)
			value += 360;
		return value - 180;
	}

	inline float ref_WrapDegrees_0_360(float f)
	{
		// This is a slower alternative to the wrapDegrees_0_360() function;
		// used as a reference for testing
		float value = fmodf(f, 360);
		if (value < 0)
			value += 360;
		return value < 0 ? value + 360 : value;
	}


	void Run()
	{
		UASSERT(fabs(modulo360f(100.0) - 100.0) < 0.001);
		UASSERT(fabs(modulo360f(720.5) - 0.5) < 0.001);
		UASSERT(fabs(modulo360f(-0.5) - (-0.5)) < 0.001);
		UASSERT(fabs(modulo360f(-365.5) - (-5.5)) < 0.001);

		for (float f = -720; f <= -360; f += 0.25) {
			UASSERT(fabs(modulo360f(f) - modulo360f(f + 360)) < 0.001);
		}

		for (float f = -1440; f <= 1440; f += 0.25) {
			UASSERT(fabs(modulo360f(f) - fmodf(f, 360)) < 0.001);
			UASSERT(fabs(wrapDegrees_180(f) - ref_WrapDegrees180(f)) < 0.001);
			UASSERT(fabs(wrapDegrees_0_360(f) - ref_WrapDegrees_0_360(f)) < 0.001);
			UASSERT(wrapDegrees_0_360(fabs(wrapDegrees_180(f) - wrapDegrees_0_360(f))) < 0.001);
		}

		UASSERT(lowercase("Foo bAR") == "foo bar");
		UASSERT(trim("\n \t\r  Foo bAR  \r\n\t\t  ") == "Foo bAR");
		UASSERT(trim("\n \t\r    \r\n\t\t  ") == "");
		UASSERT(is_yes("YeS") == true);
		UASSERT(is_yes("") == false);
		UASSERT(is_yes("FAlse") == false);
		UASSERT(is_yes("-1") == true);
		UASSERT(is_yes("0") == false);
		UASSERT(is_yes("1") == true);
		UASSERT(is_yes("2") == true);
		const char *ends[] = {"abc", "c", "bc", "", NULL};
		UASSERT(removeStringEnd("abc", ends) == "");
		UASSERT(removeStringEnd("bc", ends) == "b");
		UASSERT(removeStringEnd("12c", ends) == "12");
		UASSERT(removeStringEnd("foo", ends) == "");
		UASSERT(urlencode("\"Aardvarks lurk, OK?\"")
				== "%22Aardvarks%20lurk%2C%20OK%3F%22");
		UASSERT(urldecode("%22Aardvarks%20lurk%2C%20OK%3F%22")
				== "\"Aardvarks lurk, OK?\"");
		UASSERT(padStringRight("hello", 8) == "hello   ");
		UASSERT(str_equal(narrow_to_wide("abc"), narrow_to_wide("abc")));
		UASSERT(str_equal(narrow_to_wide("ABC"), narrow_to_wide("abc"), true));
		UASSERT(trim("  a") == "a");
		UASSERT(trim("   a  ") == "a");
		UASSERT(trim("a   ") == "a");
		UASSERT(trim("") == "");
		UASSERT(mystoi("123", 0, 1000) == 123);
		UASSERT(mystoi("123", 0, 10) == 10);
		std::string test_str;
		test_str = "Hello there";
		str_replace(test_str, "there", "world");
		UASSERT(test_str == "Hello world");
		test_str = "ThisAisAaAtest";
		str_replace(test_str, 'A', ' ');
		UASSERT(test_str == "This is a test");
		UASSERT(string_allowed("hello", "abcdefghijklmno") == true);
		UASSERT(string_allowed("123", "abcdefghijklmno") == false);
		UASSERT(string_allowed_blacklist("hello", "123") == true);
		UASSERT(string_allowed_blacklist("hello123", "123") == false);
		UASSERT(wrap_rows("12345678",4) == "1234\n5678");
		UASSERT(is_number("123") == true);
		UASSERT(is_number("") == false);
		UASSERT(is_number("123a") == false);
		UASSERT(is_power_of_two(0) == false);
		UASSERT(is_power_of_two(1) == true);
		UASSERT(is_power_of_two(2) == true);
		UASSERT(is_power_of_two(3) == false);
		for (int exponent = 2; exponent <= 31; ++exponent) {
			UASSERT(is_power_of_two((1 << exponent) - 1) == false);
			UASSERT(is_power_of_two((1 << exponent)) == true);
			UASSERT(is_power_of_two((1 << exponent) + 1) == false);
		}
		UASSERT(is_power_of_two((u32)-1) == false);
	}
};

struct TestPath: public TestBase
{
	// adjusts a POSIX path to system-specific conventions
	// -> changes '/' to DIR_DELIM
	// -> absolute paths start with "C:\\" on windows
	std::string p(std::string path)
	{
		for(size_t i = 0; i < path.size(); ++i){
			if(path[i] == '/'){
				path.replace(i, 1, DIR_DELIM);
				i += std::string(DIR_DELIM).size() - 1; // generally a no-op
			}
		}

		#ifdef _WIN32
		if(path[0] == '\\')
			path = "C:" + path;
		#endif

		return path;
	}

	void Run()
	{
		std::string path, result, removed;

		/*
			Test fs::IsDirDelimiter
		*/
		UASSERT(fs::IsDirDelimiter('/') == true);
		UASSERT(fs::IsDirDelimiter('A') == false);
		UASSERT(fs::IsDirDelimiter(0) == false);
		#ifdef _WIN32
		UASSERT(fs::IsDirDelimiter('\\') == true);
		#else
		UASSERT(fs::IsDirDelimiter('\\') == false);
		#endif

		/*
			Test fs::PathStartsWith
		*/
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

		/*
			Test fs::RemoveLastPathComponent
		*/
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

		/*
			Now repeat the test with a trailing delimiter
		*/
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

		/*
			Test fs::RemoveRelativePathComponent
		*/
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
		path = p("./subdir/../..");
		result = fs::RemoveRelativePathComponents(path);
		UASSERT(result == "");
		path = p("/a/b/c/.././../d/../e/f/g/../h/i/j/../../../..");
		result = fs::RemoveRelativePathComponents(path);
		UASSERT(result == p("/a/e"));
	}
};

#define TEST_CONFIG_TEXT_BEFORE               \
	"leet = 1337\n"                           \
	"leetleet = 13371337\n"                   \
	"leetleet_neg = -13371337\n"              \
	"floaty_thing = 1.1\n"                    \
	"stringy_thing = asd /( ¤%&(/\" BLÖÄRP\n" \
	"coord = (1, 2, 4.5)\n"                   \
	"      # this is just a comment\n"        \
	"this is an invalid line\n"               \
	"asdf = {\n"                              \
	"	a   = 5\n"                            \
	"	bb  = 2.5\n"                          \
	"	ccc = \"\"\"\n"                       \
	"testy\n"                                 \
	"   testa   \n"                           \
	"\"\"\"\n"                                \
	"\n"                                      \
	"}\n"                                     \
	"blarg = \"\"\" \n"                       \
	"some multiline text\n"                   \
	"     with leading whitespace!\n"         \
	"\"\"\"\n"                                \
	"np_terrain = 5, 40, (250, 250, 250), 12341, 5, 0.7, 2.4\n" \
	"zoop = true"

#define TEST_CONFIG_TEXT_AFTER                \
	"leet = 1337\n"                           \
	"leetleet = 13371337\n"                   \
	"leetleet_neg = -13371337\n"              \
	"floaty_thing = 1.1\n"                    \
	"stringy_thing = asd /( ¤%&(/\" BLÖÄRP\n" \
	"coord = (1, 2, 4.5)\n"                   \
	"      # this is just a comment\n"        \
	"this is an invalid line\n"               \
	"asdf = {\n"                              \
	"	a   = 5\n"                            \
	"	bb  = 2.5\n"                          \
	"	ccc = \"\"\"\n"                       \
	"testy\n"                                 \
	"   testa   \n"                           \
	"\"\"\"\n"                                \
	"\n"                                      \
	"}\n"                                     \
	"blarg = \"\"\" \n"                       \
	"some multiline text\n"                   \
	"     with leading whitespace!\n"         \
	"\"\"\"\n"                                \
	"np_terrain = {\n"                        \
	"	flags = defaults\n"                   \
	"	lacunarity = 2.4\n"                   \
	"	octaves = 6\n"                        \
	"	offset = 3.5\n"                       \
	"	persistence = 0.7\n"                  \
	"	scale = 40\n"                         \
	"	seed = 12341\n"                       \
	"	spread = (250,250,250)\n"             \
	"}\n"                                     \
	"zoop = true\n"                           \
	"coord2 = (1,2,3.3)\n"                    \
	"floaty_thing_2 = 1.2\n"                  \
	"groupy_thing = {\n"                      \
	"	animals = cute\n"                     \
	"	num_apples = 4\n"                     \
	"	num_oranges = 53\n"                   \
	"}\n"

struct TestSettings: public TestBase
{
	void Run()
	{
		try {
		Settings s;

		// Test reading of settings
		std::istringstream is(TEST_CONFIG_TEXT_BEFORE);
		s.parseConfigLines(is);

		UASSERT(s.getS32("leet") == 1337);
		UASSERT(s.getS16("leetleet") == 32767);
		UASSERT(s.getS16("leetleet_neg") == -32768);

		// Not sure if 1.1 is an exact value as a float, but doesn't matter
		UASSERT(fabs(s.getFloat("floaty_thing") - 1.1) < 0.001);
		UASSERT(s.get("stringy_thing") == "asd /( ¤%&(/\" BLÖÄRP");
		UASSERT(fabs(s.getV3F("coord").X - 1.0) < 0.001);
		UASSERT(fabs(s.getV3F("coord").Y - 2.0) < 0.001);
		UASSERT(fabs(s.getV3F("coord").Z - 4.5) < 0.001);

		// Test the setting of settings too
		s.setFloat("floaty_thing_2", 1.2);
		s.setV3F("coord2", v3f(1, 2, 3.3));
		UASSERT(s.get("floaty_thing_2").substr(0,3) == "1.2");
		UASSERT(fabs(s.getFloat("floaty_thing_2") - 1.2) < 0.001);
		UASSERT(fabs(s.getV3F("coord2").X - 1.0) < 0.001);
		UASSERT(fabs(s.getV3F("coord2").Y - 2.0) < 0.001);
		UASSERT(fabs(s.getV3F("coord2").Z - 3.3) < 0.001);

		// Test settings groups
		Settings *group = s.getGroup("asdf");
		UASSERT(group != NULL);
		UASSERT(s.getGroupNoEx("zoop", group) == false);
		UASSERT(group->getS16("a") == 5);
		UASSERT(fabs(group->getFloat("bb") - 2.5) < 0.001);

		Settings *group3 = new Settings;
		group3->set("cat", "meow");
		group3->set("dog", "woof");

		Settings *group2 = new Settings;
		group2->setS16("num_apples", 4);
		group2->setS16("num_oranges", 53);
		group2->setGroup("animals", group3);
		group2->set("animals", "cute"); //destroys group 3
		s.setGroup("groupy_thing", group2);

		// Test set failure conditions
		UASSERT(s.set("Zoop = Poop\nsome_other_setting", "false") == false);
		UASSERT(s.set("sneaky", "\"\"\"\njabberwocky = false") == false);
		UASSERT(s.set("hehe", "asdfasdf\n\"\"\"\nsomething = false") == false);

		// Test multiline settings
		UASSERT(group->get("ccc") == "testy\n   testa   ");

		UASSERT(s.get("blarg") ==
			"some multiline text\n"
			"     with leading whitespace!");

		// Test NoiseParams
		UASSERT(s.getEntry("np_terrain").is_group == false);

		NoiseParams np;
		UASSERT(s.getNoiseParams("np_terrain", np) == true);
		UASSERT(fabs(np.offset - 5) < 0.001);
		UASSERT(fabs(np.scale - 40) < 0.001);
		UASSERT(fabs(np.spread.X - 250) < 0.001);
		UASSERT(fabs(np.spread.Y - 250) < 0.001);
		UASSERT(fabs(np.spread.Z - 250) < 0.001);
		UASSERT(np.seed == 12341);
		UASSERT(np.octaves == 5);
		UASSERT(fabs(np.persist - 0.7) < 0.001);

		np.offset  = 3.5;
		np.octaves = 6;
		s.setNoiseParams("np_terrain", np);

		UASSERT(s.getEntry("np_terrain").is_group == true);

		// Test writing
		std::ostringstream os(std::ios_base::binary);
		is.clear();
		is.seekg(0);

		UASSERT(s.updateConfigObject(is, os, "", 0) == true);
		//printf(">>>> expected config:\n%s\n", TEST_CONFIG_TEXT_AFTER);
		//printf(">>>> actual config:\n%s\n", os.str().c_str());
		UASSERT(os.str() == TEST_CONFIG_TEXT_AFTER);
		} catch (SettingNotFoundException &e) {
			UASSERT(!"Setting not found!");
		}
	}
};

struct TestSerialization: public TestBase
{
	// To be used like this:
	//   mkstr("Some\0string\0with\0embedded\0nuls")
	// since std::string("...") doesn't work as expected in that case.
	template<size_t N> std::string mkstr(const char (&s)[N])
	{
		return std::string(s, N - 1);
	}

	void Run()
	{
		// Tests some serialization primitives

		UASSERT(serializeString("") == mkstr("\0\0"));
		UASSERT(serializeWideString(L"") == mkstr("\0\0"));
		UASSERT(serializeLongString("") == mkstr("\0\0\0\0"));
		UASSERT(serializeJsonString("") == "\"\"");

		std::string teststring = "Hello world!";
		UASSERT(serializeString(teststring) ==
			mkstr("\0\14Hello world!"));
		UASSERT(serializeWideString(narrow_to_wide(teststring)) ==
			mkstr("\0\14\0H\0e\0l\0l\0o\0 \0w\0o\0r\0l\0d\0!"));
		UASSERT(serializeLongString(teststring) ==
			mkstr("\0\0\0\14Hello world!"));
		UASSERT(serializeJsonString(teststring) ==
			"\"Hello world!\"");

		std::string teststring2;
		std::wstring teststring2_w;
		std::string teststring2_w_encoded;
		{
			std::ostringstream tmp_os;
			std::wostringstream tmp_os_w;
			std::ostringstream tmp_os_w_encoded;
			for(int i = 0; i < 256; i++)
			{
				tmp_os<<(char)i;
				tmp_os_w<<(wchar_t)i;
				tmp_os_w_encoded<<(char)0<<(char)i;
			}
			teststring2 = tmp_os.str();
			teststring2_w = tmp_os_w.str();
			teststring2_w_encoded = tmp_os_w_encoded.str();
		}
		UASSERT(serializeString(teststring2) ==
			mkstr("\1\0") + teststring2);
		UASSERT(serializeWideString(teststring2_w) ==
			mkstr("\1\0") + teststring2_w_encoded);
		UASSERT(serializeLongString(teststring2) ==
			mkstr("\0\0\1\0") + teststring2);
		// MSVC fails when directly using "\\\\"
		std::string backslash = "\\";
		UASSERT(serializeJsonString(teststring2) ==
			mkstr("\"") +
			"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007" +
			"\\b\\t\\n\\u000b\\f\\r\\u000e\\u000f" +
			"\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017" +
			"\\u0018\\u0019\\u001a\\u001b\\u001c\\u001d\\u001e\\u001f" +
			" !\\\"" + teststring2.substr(0x23, 0x2f-0x23) +
			"\\/" + teststring2.substr(0x30, 0x5c-0x30) +
			backslash + backslash + teststring2.substr(0x5d, 0x7f-0x5d) + "\\u007f" +
			"\\u0080\\u0081\\u0082\\u0083\\u0084\\u0085\\u0086\\u0087" +
			"\\u0088\\u0089\\u008a\\u008b\\u008c\\u008d\\u008e\\u008f" +
			"\\u0090\\u0091\\u0092\\u0093\\u0094\\u0095\\u0096\\u0097" +
			"\\u0098\\u0099\\u009a\\u009b\\u009c\\u009d\\u009e\\u009f" +
			"\\u00a0\\u00a1\\u00a2\\u00a3\\u00a4\\u00a5\\u00a6\\u00a7" +
			"\\u00a8\\u00a9\\u00aa\\u00ab\\u00ac\\u00ad\\u00ae\\u00af" +
			"\\u00b0\\u00b1\\u00b2\\u00b3\\u00b4\\u00b5\\u00b6\\u00b7" +
			"\\u00b8\\u00b9\\u00ba\\u00bb\\u00bc\\u00bd\\u00be\\u00bf" +
			"\\u00c0\\u00c1\\u00c2\\u00c3\\u00c4\\u00c5\\u00c6\\u00c7" +
			"\\u00c8\\u00c9\\u00ca\\u00cb\\u00cc\\u00cd\\u00ce\\u00cf" +
			"\\u00d0\\u00d1\\u00d2\\u00d3\\u00d4\\u00d5\\u00d6\\u00d7" +
			"\\u00d8\\u00d9\\u00da\\u00db\\u00dc\\u00dd\\u00de\\u00df" +
			"\\u00e0\\u00e1\\u00e2\\u00e3\\u00e4\\u00e5\\u00e6\\u00e7" +
			"\\u00e8\\u00e9\\u00ea\\u00eb\\u00ec\\u00ed\\u00ee\\u00ef" +
			"\\u00f0\\u00f1\\u00f2\\u00f3\\u00f4\\u00f5\\u00f6\\u00f7" +
			"\\u00f8\\u00f9\\u00fa\\u00fb\\u00fc\\u00fd\\u00fe\\u00ff" +
			"\"");

		{
			std::istringstream is(serializeString(teststring2), std::ios::binary);
			UASSERT(deSerializeString(is) == teststring2);
			UASSERT(!is.eof());
			is.get();
			UASSERT(is.eof());
		}
		{
			std::istringstream is(serializeWideString(teststring2_w), std::ios::binary);
			UASSERT(deSerializeWideString(is) == teststring2_w);
			UASSERT(!is.eof());
			is.get();
			UASSERT(is.eof());
		}
		{
			std::istringstream is(serializeLongString(teststring2), std::ios::binary);
			UASSERT(deSerializeLongString(is) == teststring2);
			UASSERT(!is.eof());
			is.get();
			UASSERT(is.eof());
		}
		{
			std::istringstream is(serializeJsonString(teststring2), std::ios::binary);
			//dstream<<serializeJsonString(deSerializeJsonString(is));
			UASSERT(deSerializeJsonString(is) == teststring2);
			UASSERT(!is.eof());
			is.get();
			UASSERT(is.eof());
		}
	}
};

struct TestNodedefSerialization: public TestBase
{
	void Run()
	{
		ContentFeatures f;
		f.name = "default:stone";
		for(int i = 0; i < 6; i++)
			f.tiledef[i].name = "default_stone.png";
		f.is_ground_content = true;
		std::ostringstream os(std::ios::binary);
		f.serialize(os, LATEST_PROTOCOL_VERSION);
		verbosestream<<"Test ContentFeatures size: "<<os.str().size()<<std::endl;
		std::istringstream is(os.str(), std::ios::binary);
		ContentFeatures f2;
		f2.deSerialize(is);
		UASSERT(f.walkable == f2.walkable);
		UASSERT(f.node_box.type == f2.node_box.type);
	}
};

struct TestCompress: public TestBase
{
	void Run()
	{
		{ // ver 0

		SharedBuffer<u8> fromdata(4);
		fromdata[0]=1;
		fromdata[1]=5;
		fromdata[2]=5;
		fromdata[3]=1;

		std::ostringstream os(std::ios_base::binary);
		compress(fromdata, os, 0);

		std::string str_out = os.str();

		infostream<<"str_out.size()="<<str_out.size()<<std::endl;
		infostream<<"TestCompress: 1,5,5,1 -> ";
		for(u32 i=0; i<str_out.size(); i++)
		{
			infostream<<(u32)str_out[i]<<",";
		}
		infostream<<std::endl;

		UASSERT(str_out.size() == 10);

		UASSERT(str_out[0] == 0);
		UASSERT(str_out[1] == 0);
		UASSERT(str_out[2] == 0);
		UASSERT(str_out[3] == 4);
		UASSERT(str_out[4] == 0);
		UASSERT(str_out[5] == 1);
		UASSERT(str_out[6] == 1);
		UASSERT(str_out[7] == 5);
		UASSERT(str_out[8] == 0);
		UASSERT(str_out[9] == 1);

		std::istringstream is(str_out, std::ios_base::binary);
		std::ostringstream os2(std::ios_base::binary);

		decompress(is, os2, 0);
		std::string str_out2 = os2.str();

		infostream<<"decompress: ";
		for(u32 i=0; i<str_out2.size(); i++)
		{
			infostream<<(u32)str_out2[i]<<",";
		}
		infostream<<std::endl;

		UASSERT(str_out2.size() == fromdata.getSize());

		for(u32 i=0; i<str_out2.size(); i++)
		{
			UASSERT(str_out2[i] == fromdata[i]);
		}

		}

		{ // ver HIGHEST

		SharedBuffer<u8> fromdata(4);
		fromdata[0]=1;
		fromdata[1]=5;
		fromdata[2]=5;
		fromdata[3]=1;

		std::ostringstream os(std::ios_base::binary);
		compress(fromdata, os, SER_FMT_VER_HIGHEST_READ);

		std::string str_out = os.str();

		infostream<<"str_out.size()="<<str_out.size()<<std::endl;
		infostream<<"TestCompress: 1,5,5,1 -> ";
		for(u32 i=0; i<str_out.size(); i++)
		{
			infostream<<(u32)str_out[i]<<",";
		}
		infostream<<std::endl;

		std::istringstream is(str_out, std::ios_base::binary);
		std::ostringstream os2(std::ios_base::binary);

		decompress(is, os2, SER_FMT_VER_HIGHEST_READ);
		std::string str_out2 = os2.str();

		infostream<<"decompress: ";
		for(u32 i=0; i<str_out2.size(); i++)
		{
			infostream<<(u32)str_out2[i]<<",";
		}
		infostream<<std::endl;

		UASSERT(str_out2.size() == fromdata.getSize());

		for(u32 i=0; i<str_out2.size(); i++)
		{
			UASSERT(str_out2[i] == fromdata[i]);
		}

		}

		// Test zlib wrapper with large amounts of data (larger than its
		// internal buffers)
		{
			infostream<<"Test: Testing zlib wrappers with a large amount "
					<<"of pseudorandom data"<<std::endl;
			u32 size = 50000;
			infostream<<"Test: Input size of large compressZlib is "
					<<size<<std::endl;
			std::string data_in;
			data_in.resize(size);
			PseudoRandom pseudorandom(9420);
			for(u32 i=0; i<size; i++)
				data_in[i] = pseudorandom.range(0,255);
			std::ostringstream os_compressed(std::ios::binary);
			compressZlib(data_in, os_compressed);
			infostream<<"Test: Output size of large compressZlib is "
					<<os_compressed.str().size()<<std::endl;
			std::istringstream is_compressed(os_compressed.str(), std::ios::binary);
			std::ostringstream os_decompressed(std::ios::binary);
			decompressZlib(is_compressed, os_decompressed);
			infostream<<"Test: Output size of large decompressZlib is "
					<<os_decompressed.str().size()<<std::endl;
			std::string str_decompressed = os_decompressed.str();
			UTEST(str_decompressed.size() == data_in.size(), "Output size not"
					" equal (output: %u, input: %u)",
					(unsigned int)str_decompressed.size(), (unsigned int)data_in.size());
			for(u32 i=0; i<size && i<str_decompressed.size(); i++){
				UTEST(str_decompressed[i] == data_in[i],
						"index out[%i]=%i differs from in[%i]=%i",
						i, str_decompressed[i], i, data_in[i]);
			}
		}
	}
};

struct TestMapNode: public TestBase
{
	void Run(INodeDefManager *nodedef)
	{
		MapNode n(CONTENT_AIR);

		UASSERT(n.getContent() == CONTENT_AIR);
		UASSERT(n.getLight(LIGHTBANK_DAY, nodedef) == 0);
		UASSERT(n.getLight(LIGHTBANK_NIGHT, nodedef) == 0);

		// Transparency
		n.setContent(CONTENT_AIR);
		UASSERT(nodedef->get(n).light_propagates == true);
		n.setContent(LEGN(nodedef, "CONTENT_STONE"));
		UASSERT(nodedef->get(n).light_propagates == false);
	}
};

struct TestVoxelManipulator: public TestBase
{
	void Run(INodeDefManager *nodedef)
	{
		/*
			VoxelArea
		*/

		VoxelArea a(v3s16(-1,-1,-1), v3s16(1,1,1));
		UASSERT(a.index(0,0,0) == 1*3*3 + 1*3 + 1);
		UASSERT(a.index(-1,-1,-1) == 0);

		VoxelArea c(v3s16(-2,-2,-2), v3s16(2,2,2));
		// An area that is 1 bigger in x+ and z-
		VoxelArea d(v3s16(-2,-2,-3), v3s16(3,2,2));

		std::list<VoxelArea> aa;
		d.diff(c, aa);

		// Correct results
		std::vector<VoxelArea> results;
		results.push_back(VoxelArea(v3s16(-2,-2,-3),v3s16(3,2,-3)));
		results.push_back(VoxelArea(v3s16(3,-2,-2),v3s16(3,2,2)));

		UASSERT(aa.size() == results.size());

		infostream<<"Result of diff:"<<std::endl;
		for(std::list<VoxelArea>::const_iterator
				i = aa.begin(); i != aa.end(); ++i)
		{
			i->print(infostream);
			infostream<<std::endl;

			std::vector<VoxelArea>::iterator j = std::find(results.begin(), results.end(), *i);
			UASSERT(j != results.end());
			results.erase(j);
		}


		/*
			VoxelManipulator
		*/

		VoxelManipulator v;

		v.print(infostream, nodedef);

		infostream<<"*** Setting (-1,0,-1)=2 ***"<<std::endl;

		v.setNodeNoRef(v3s16(-1,0,-1), MapNode(CONTENT_GRASS));

		v.print(infostream, nodedef);

 		UASSERT(v.getNode(v3s16(-1,0,-1)).getContent() == CONTENT_GRASS);

		infostream<<"*** Reading from inexistent (0,0,-1) ***"<<std::endl;

		EXCEPTION_CHECK(InvalidPositionException, v.getNode(v3s16(0,0,-1)));

		v.print(infostream, nodedef);

		infostream<<"*** Adding area ***"<<std::endl;

		v.addArea(a);

		v.print(infostream, nodedef);

		UASSERT(v.getNode(v3s16(-1,0,-1)).getContent() == CONTENT_GRASS);
		EXCEPTION_CHECK(InvalidPositionException, v.getNode(v3s16(0,1,1)));
	}
};

struct TestVoxelAlgorithms: public TestBase
{
	void Run(INodeDefManager *ndef)
	{
		/*
			voxalgo::propagateSunlight
		*/
		{
			VoxelManipulator v;
			for(u16 z=0; z<3; z++)
			for(u16 y=0; y<3; y++)
			for(u16 x=0; x<3; x++)
			{
				v3s16 p(x,y,z);
				v.setNodeNoRef(p, MapNode(CONTENT_AIR));
			}
			VoxelArea a(v3s16(0,0,0), v3s16(2,2,2));
			{
				std::set<v3s16> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, true, light_sources, ndef);
				//v.print(dstream, ndef, VOXELPRINT_LIGHT_DAY);
				UASSERT(res.bottom_sunlight_valid == true);
				UASSERT(v.getNode(v3s16(1,1,1)).getLight(LIGHTBANK_DAY, ndef)
						== LIGHT_SUN);
			}
			v.setNodeNoRef(v3s16(0,0,0), MapNode(CONTENT_STONE));
			{
				std::set<v3s16> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, true, light_sources, ndef);
				UASSERT(res.bottom_sunlight_valid == true);
				UASSERT(v.getNode(v3s16(1,1,1)).getLight(LIGHTBANK_DAY, ndef)
						== LIGHT_SUN);
			}
			{
				std::set<v3s16> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, false, light_sources, ndef);
				UASSERT(res.bottom_sunlight_valid == true);
				UASSERT(v.getNode(v3s16(2,0,2)).getLight(LIGHTBANK_DAY, ndef)
						== 0);
			}
			v.setNodeNoRef(v3s16(1,3,2), MapNode(CONTENT_STONE));
			{
				std::set<v3s16> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, true, light_sources, ndef);
				UASSERT(res.bottom_sunlight_valid == true);
				UASSERT(v.getNode(v3s16(1,1,2)).getLight(LIGHTBANK_DAY, ndef)
						== 0);
			}
			{
				std::set<v3s16> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, false, light_sources, ndef);
				UASSERT(res.bottom_sunlight_valid == true);
				UASSERT(v.getNode(v3s16(1,0,2)).getLight(LIGHTBANK_DAY, ndef)
						== 0);
			}
			{
				MapNode n(CONTENT_AIR);
				n.setLight(LIGHTBANK_DAY, 10, ndef);
				v.setNodeNoRef(v3s16(1,-1,2), n);
			}
			{
				std::set<v3s16> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, true, light_sources, ndef);
				UASSERT(res.bottom_sunlight_valid == true);
			}
			{
				std::set<v3s16> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, false, light_sources, ndef);
				UASSERT(res.bottom_sunlight_valid == true);
			}
			{
				MapNode n(CONTENT_AIR);
				n.setLight(LIGHTBANK_DAY, LIGHT_SUN, ndef);
				v.setNodeNoRef(v3s16(1,-1,2), n);
			}
			{
				std::set<v3s16> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, true, light_sources, ndef);
				UASSERT(res.bottom_sunlight_valid == false);
			}
			{
				std::set<v3s16> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, false, light_sources, ndef);
				UASSERT(res.bottom_sunlight_valid == false);
			}
			v.setNodeNoRef(v3s16(1,3,2), MapNode(CONTENT_IGNORE));
			{
				std::set<v3s16> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, true, light_sources, ndef);
				UASSERT(res.bottom_sunlight_valid == true);
			}
		}
		/*
			voxalgo::clearLightAndCollectSources
		*/
		{
			VoxelManipulator v;
			for(u16 z=0; z<3; z++)
			for(u16 y=0; y<3; y++)
			for(u16 x=0; x<3; x++)
			{
				v3s16 p(x,y,z);
				v.setNode(p, MapNode(CONTENT_AIR));
			}
			VoxelArea a(v3s16(0,0,0), v3s16(2,2,2));
			v.setNodeNoRef(v3s16(0,0,0), MapNode(CONTENT_STONE));
			v.setNodeNoRef(v3s16(1,1,1), MapNode(CONTENT_TORCH));
			{
				MapNode n(CONTENT_AIR);
				n.setLight(LIGHTBANK_DAY, 1, ndef);
				v.setNode(v3s16(1,1,2), n);
			}
			{
				std::set<v3s16> light_sources;
				std::map<v3s16, u8> unlight_from;
				voxalgo::clearLightAndCollectSources(v, a, LIGHTBANK_DAY,
						ndef, light_sources, unlight_from);
				//v.print(dstream, ndef, VOXELPRINT_LIGHT_DAY);
				UASSERT(v.getNode(v3s16(0,1,1)).getLight(LIGHTBANK_DAY, ndef)
						== 0);
				UASSERT(light_sources.find(v3s16(1,1,1)) != light_sources.end());
				UASSERT(light_sources.size() == 1);
				UASSERT(unlight_from.find(v3s16(1,1,2)) != unlight_from.end());
				UASSERT(unlight_from.size() == 1);
			}
		}
	}
};

struct TestInventory: public TestBase
{
	void Run(IItemDefManager *idef)
	{
		std::string serialized_inventory =
		"List 0 32\n"
		"Width 3\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Item default:cobble 61\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Item default:dirt 71\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Item default:dirt 99\n"
		"Item default:cobble 38\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"EndInventoryList\n"
		"EndInventory\n";

		std::string serialized_inventory_2 =
		"List main 32\n"
		"Width 5\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Item default:cobble 61\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Item default:dirt 71\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Item default:dirt 99\n"
		"Item default:cobble 38\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"EndInventoryList\n"
		"EndInventory\n";

		Inventory inv(idef);
		std::istringstream is(serialized_inventory, std::ios::binary);
		inv.deSerialize(is);
		UASSERT(inv.getList("0"));
		UASSERT(!inv.getList("main"));
		inv.getList("0")->setName("main");
		UASSERT(!inv.getList("0"));
		UASSERT(inv.getList("main"));
		UASSERT(inv.getList("main")->getWidth() == 3);
		inv.getList("main")->setWidth(5);
		std::ostringstream inv_os(std::ios::binary);
		inv.serialize(inv_os);
		UASSERT(inv_os.str() == serialized_inventory_2);
	}
};

/*
	NOTE: These tests became non-working then NodeContainer was removed.
	      These should be redone, utilizing some kind of a virtual
		  interface for Map (IMap would be fine).
*/
#if 0
struct TestMapBlock: public TestBase
{
	class TC : public NodeContainer
	{
	public:

		MapNode node;
		bool position_valid;
		core::list<v3s16> validity_exceptions;

		TC()
		{
			position_valid = true;
		}

		virtual bool isValidPosition(v3s16 p)
		{
			//return position_valid ^ (p==position_valid_exception);
			bool exception = false;
			for(core::list<v3s16>::Iterator i=validity_exceptions.begin();
					i != validity_exceptions.end(); i++)
			{
				if(p == *i)
				{
					exception = true;
					break;
				}
			}
			return exception ? !position_valid : position_valid;
		}

		virtual MapNode getNode(v3s16 p)
		{
			if(isValidPosition(p) == false)
				throw InvalidPositionException();
			return node;
		}

		virtual void setNode(v3s16 p, MapNode & n)
		{
			if(isValidPosition(p) == false)
				throw InvalidPositionException();
		};

		virtual u16 nodeContainerId() const
		{
			return 666;
		}
	};

	void Run()
	{
		TC parent;

		MapBlock b(&parent, v3s16(1,1,1));
		v3s16 relpos(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);

		UASSERT(b.getPosRelative() == relpos);

		UASSERT(b.getBox().MinEdge.X == MAP_BLOCKSIZE);
		UASSERT(b.getBox().MaxEdge.X == MAP_BLOCKSIZE*2-1);
		UASSERT(b.getBox().MinEdge.Y == MAP_BLOCKSIZE);
		UASSERT(b.getBox().MaxEdge.Y == MAP_BLOCKSIZE*2-1);
		UASSERT(b.getBox().MinEdge.Z == MAP_BLOCKSIZE);
		UASSERT(b.getBox().MaxEdge.Z == MAP_BLOCKSIZE*2-1);

		UASSERT(b.isValidPosition(v3s16(0,0,0)) == true);
		UASSERT(b.isValidPosition(v3s16(-1,0,0)) == false);
		UASSERT(b.isValidPosition(v3s16(-1,-142,-2341)) == false);
		UASSERT(b.isValidPosition(v3s16(-124,142,2341)) == false);
		UASSERT(b.isValidPosition(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1)) == true);
		UASSERT(b.isValidPosition(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE,MAP_BLOCKSIZE-1)) == false);

		/*
			TODO: this method should probably be removed
			if the block size isn't going to be set variable
		*/
		/*UASSERT(b.getSizeNodes() == v3s16(MAP_BLOCKSIZE,
				MAP_BLOCKSIZE, MAP_BLOCKSIZE));*/

		// Changed flag should be initially set
		UASSERT(b.getModified() == MOD_STATE_WRITE_NEEDED);
		b.resetModified();
		UASSERT(b.getModified() == MOD_STATE_CLEAN);

		// All nodes should have been set to
		// .d=CONTENT_IGNORE and .getLight() = 0
		for(u16 z=0; z<MAP_BLOCKSIZE; z++)
		for(u16 y=0; y<MAP_BLOCKSIZE; y++)
		for(u16 x=0; x<MAP_BLOCKSIZE; x++)
		{
			//UASSERT(b.getNode(v3s16(x,y,z)).getContent() == CONTENT_AIR);
			UASSERT(b.getNode(v3s16(x,y,z)).getContent() == CONTENT_IGNORE);
			UASSERT(b.getNode(v3s16(x,y,z)).getLight(LIGHTBANK_DAY) == 0);
			UASSERT(b.getNode(v3s16(x,y,z)).getLight(LIGHTBANK_NIGHT) == 0);
		}

		{
			MapNode n(CONTENT_AIR);
			for(u16 z=0; z<MAP_BLOCKSIZE; z++)
			for(u16 y=0; y<MAP_BLOCKSIZE; y++)
			for(u16 x=0; x<MAP_BLOCKSIZE; x++)
			{
				b.setNode(v3s16(x,y,z), n);
			}
		}

		/*
			Parent fetch functions
		*/
		parent.position_valid = false;
		parent.node.setContent(5);

		MapNode n;

		// Positions in the block should still be valid
		UASSERT(b.isValidPositionParent(v3s16(0,0,0)) == true);
		UASSERT(b.isValidPositionParent(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1)) == true);
		n = b.getNodeParent(v3s16(0,MAP_BLOCKSIZE-1,0));
		UASSERT(n.getContent() == CONTENT_AIR);

		// ...but outside the block they should be invalid
		UASSERT(b.isValidPositionParent(v3s16(-121,2341,0)) == false);
		UASSERT(b.isValidPositionParent(v3s16(-1,0,0)) == false);
		UASSERT(b.isValidPositionParent(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1,MAP_BLOCKSIZE)) == false);

		{
			bool exception_thrown = false;
			try{
				// This should throw an exception
				MapNode n = b.getNodeParent(v3s16(0,0,-1));
			}
			catch(InvalidPositionException &e)
			{
				exception_thrown = true;
			}
			UASSERT(exception_thrown);
		}

		parent.position_valid = true;
		// Now the positions outside should be valid
		UASSERT(b.isValidPositionParent(v3s16(-121,2341,0)) == true);
		UASSERT(b.isValidPositionParent(v3s16(-1,0,0)) == true);
		UASSERT(b.isValidPositionParent(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1,MAP_BLOCKSIZE)) == true);
		n = b.getNodeParent(v3s16(0,0,MAP_BLOCKSIZE));
		UASSERT(n.getContent() == 5);

		/*
			Set a node
		*/
		v3s16 p(1,2,0);
		n.setContent(4);
		b.setNode(p, n);
		UASSERT(b.getNode(p).getContent() == 4);
		//TODO: Update to new system
		/*UASSERT(b.getNodeTile(p) == 4);
		UASSERT(b.getNodeTile(v3s16(-1,-1,0)) == 5);*/

		/*
			propagateSunlight()
		*/
		// Set lighting of all nodes to 0
		for(u16 z=0; z<MAP_BLOCKSIZE; z++){
			for(u16 y=0; y<MAP_BLOCKSIZE; y++){
				for(u16 x=0; x<MAP_BLOCKSIZE; x++){
					MapNode n = b.getNode(v3s16(x,y,z));
					n.setLight(LIGHTBANK_DAY, 0);
					n.setLight(LIGHTBANK_NIGHT, 0);
					b.setNode(v3s16(x,y,z), n);
				}
			}
		}
		{
			/*
				Check how the block handles being a lonely sky block
			*/
			parent.position_valid = true;
			b.setIsUnderground(false);
			parent.node.setContent(CONTENT_AIR);
			parent.node.setLight(LIGHTBANK_DAY, LIGHT_SUN);
			parent.node.setLight(LIGHTBANK_NIGHT, 0);
			core::map<v3s16, bool> light_sources;
			// The bottom block is invalid, because we have a shadowing node
			UASSERT(b.propagateSunlight(light_sources) == false);
			UASSERT(b.getNode(v3s16(1,4,0)).getLight(LIGHTBANK_DAY) == LIGHT_SUN);
			UASSERT(b.getNode(v3s16(1,3,0)).getLight(LIGHTBANK_DAY) == LIGHT_SUN);
			UASSERT(b.getNode(v3s16(1,2,0)).getLight(LIGHTBANK_DAY) == 0);
			UASSERT(b.getNode(v3s16(1,1,0)).getLight(LIGHTBANK_DAY) == 0);
			UASSERT(b.getNode(v3s16(1,0,0)).getLight(LIGHTBANK_DAY) == 0);
			UASSERT(b.getNode(v3s16(1,2,3)).getLight(LIGHTBANK_DAY) == LIGHT_SUN);
			UASSERT(b.getFaceLight2(1000, p, v3s16(0,1,0)) == LIGHT_SUN);
			UASSERT(b.getFaceLight2(1000, p, v3s16(0,-1,0)) == 0);
			UASSERT(b.getFaceLight2(0, p, v3s16(0,-1,0)) == 0);
			// According to MapBlock::getFaceLight,
			// The face on the z+ side should have double-diminished light
			//UASSERT(b.getFaceLight(p, v3s16(0,0,1)) == diminish_light(diminish_light(LIGHT_MAX)));
			// The face on the z+ side should have diminished light
			UASSERT(b.getFaceLight2(1000, p, v3s16(0,0,1)) == diminish_light(LIGHT_MAX));
		}
		/*
			Check how the block handles being in between blocks with some non-sunlight
			while being underground
		*/
		{
			// Make neighbours to exist and set some non-sunlight to them
			parent.position_valid = true;
			b.setIsUnderground(true);
			parent.node.setLight(LIGHTBANK_DAY, LIGHT_MAX/2);
			core::map<v3s16, bool> light_sources;
			// The block below should be valid because there shouldn't be
			// sunlight in there either
			UASSERT(b.propagateSunlight(light_sources, true) == true);
			// Should not touch nodes that are not affected (that is, all of them)
			//UASSERT(b.getNode(v3s16(1,2,3)).getLight() == LIGHT_SUN);
			// Should set light of non-sunlighted blocks to 0.
			UASSERT(b.getNode(v3s16(1,2,3)).getLight(LIGHTBANK_DAY) == 0);
		}
		/*
			Set up a situation where:
			- There is only air in this block
			- There is a valid non-sunlighted block at the bottom, and
			- Invalid blocks elsewhere.
			- the block is not underground.

			This should result in bottom block invalidity
		*/
		{
			b.setIsUnderground(false);
			// Clear block
			for(u16 z=0; z<MAP_BLOCKSIZE; z++){
				for(u16 y=0; y<MAP_BLOCKSIZE; y++){
					for(u16 x=0; x<MAP_BLOCKSIZE; x++){
						MapNode n;
						n.setContent(CONTENT_AIR);
						n.setLight(LIGHTBANK_DAY, 0);
						b.setNode(v3s16(x,y,z), n);
					}
				}
			}
			// Make neighbours invalid
			parent.position_valid = false;
			// Add exceptions to the top of the bottom block
			for(u16 x=0; x<MAP_BLOCKSIZE; x++)
			for(u16 z=0; z<MAP_BLOCKSIZE; z++)
			{
				parent.validity_exceptions.push_back(v3s16(MAP_BLOCKSIZE+x, MAP_BLOCKSIZE-1, MAP_BLOCKSIZE+z));
			}
			// Lighting value for the valid nodes
			parent.node.setLight(LIGHTBANK_DAY, LIGHT_MAX/2);
			core::map<v3s16, bool> light_sources;
			// Bottom block is not valid
			UASSERT(b.propagateSunlight(light_sources) == false);
		}
	}
};

struct TestMapSector: public TestBase
{
	class TC : public NodeContainer
	{
	public:

		MapNode node;
		bool position_valid;

		TC()
		{
			position_valid = true;
		}

		virtual bool isValidPosition(v3s16 p)
		{
			return position_valid;
		}

		virtual MapNode getNode(v3s16 p)
		{
			if(position_valid == false)
				throw InvalidPositionException();
			return node;
		}

		virtual void setNode(v3s16 p, MapNode & n)
		{
			if(position_valid == false)
				throw InvalidPositionException();
		};

		virtual u16 nodeContainerId() const
		{
			return 666;
		}
	};

	void Run()
	{
		TC parent;
		parent.position_valid = false;

		// Create one with no heightmaps
		ServerMapSector sector(&parent, v2s16(1,1));

		UASSERT(sector.getBlockNoCreateNoEx(0) == 0);
		UASSERT(sector.getBlockNoCreateNoEx(1) == 0);

		MapBlock * bref = sector.createBlankBlock(-2);

		UASSERT(sector.getBlockNoCreateNoEx(0) == 0);
		UASSERT(sector.getBlockNoCreateNoEx(-2) == bref);

		//TODO: Check for AlreadyExistsException

		/*bool exception_thrown = false;
		try{
			sector.getBlock(0);
		}
		catch(InvalidPositionException &e){
			exception_thrown = true;
		}
		UASSERT(exception_thrown);*/

	}
};
#endif

struct TestCollision: public TestBase
{
	void Run()
	{
		/*
			axisAlignedCollision
		*/

		for(s16 bx = -3; bx <= 3; bx++)
		for(s16 by = -3; by <= 3; by++)
		for(s16 bz = -3; bz <= 3; bz++)
		{
			// X-
			{
				aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
				aabb3f m(bx-2, by, bz, bx-1, by+1, bz+1);
				v3f v(1, 0, 0);
				f32 dtime = 0;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == 0);
				UASSERT(fabs(dtime - 1.000) < 0.001);
			}
			{
				aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
				aabb3f m(bx-2, by, bz, bx-1, by+1, bz+1);
				v3f v(-1, 0, 0);
				f32 dtime = 0;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == -1);
			}
			{
				aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
				aabb3f m(bx-2, by+1.5, bz, bx-1, by+2.5, bz-1);
				v3f v(1, 0, 0);
				f32 dtime;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == -1);
			}
			{
				aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
				aabb3f m(bx-2, by-1.5, bz, bx-1.5, by+0.5, bz+1);
				v3f v(0.5, 0.1, 0);
				f32 dtime;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == 0);
				UASSERT(fabs(dtime - 3.000) < 0.001);
			}
			{
				aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
				aabb3f m(bx-2, by-1.5, bz, bx-1.5, by+0.5, bz+1);
				v3f v(0.5, 0.1, 0);
				f32 dtime;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == 0);
				UASSERT(fabs(dtime - 3.000) < 0.001);
			}

			// X+
			{
				aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
				aabb3f m(bx+2, by, bz, bx+3, by+1, bz+1);
				v3f v(-1, 0, 0);
				f32 dtime;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == 0);
				UASSERT(fabs(dtime - 1.000) < 0.001);
			}
			{
				aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
				aabb3f m(bx+2, by, bz, bx+3, by+1, bz+1);
				v3f v(1, 0, 0);
				f32 dtime;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == -1);
			}
			{
				aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
				aabb3f m(bx+2, by, bz+1.5, bx+3, by+1, bz+3.5);
				v3f v(-1, 0, 0);
				f32 dtime;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == -1);
			}
			{
				aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
				aabb3f m(bx+2, by-1.5, bz, bx+2.5, by-0.5, bz+1);
				v3f v(-0.5, 0.2, 0);
				f32 dtime;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == 1);  // Y, not X!
				UASSERT(fabs(dtime - 2.500) < 0.001);
			}
			{
				aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
				aabb3f m(bx+2, by-1.5, bz, bx+2.5, by-0.5, bz+1);
				v3f v(-0.5, 0.3, 0);
				f32 dtime;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == 0);
				UASSERT(fabs(dtime - 2.000) < 0.001);
			}

			// TODO: Y-, Y+, Z-, Z+

			// misc
			{
				aabb3f s(bx, by, bz, bx+2, by+2, bz+2);
				aabb3f m(bx+2.3, by+2.29, bz+2.29, bx+4.2, by+4.2, bz+4.2);
				v3f v(-1./3, -1./3, -1./3);
				f32 dtime;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == 0);
				UASSERT(fabs(dtime - 0.9) < 0.001);
			}
			{
				aabb3f s(bx, by, bz, bx+2, by+2, bz+2);
				aabb3f m(bx+2.29, by+2.3, bz+2.29, bx+4.2, by+4.2, bz+4.2);
				v3f v(-1./3, -1./3, -1./3);
				f32 dtime;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == 1);
				UASSERT(fabs(dtime - 0.9) < 0.001);
			}
			{
				aabb3f s(bx, by, bz, bx+2, by+2, bz+2);
				aabb3f m(bx+2.29, by+2.29, bz+2.3, bx+4.2, by+4.2, bz+4.2);
				v3f v(-1./3, -1./3, -1./3);
				f32 dtime;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == 2);
				UASSERT(fabs(dtime - 0.9) < 0.001);
			}
			{
				aabb3f s(bx, by, bz, bx+2, by+2, bz+2);
				aabb3f m(bx-4.2, by-4.2, bz-4.2, bx-2.3, by-2.29, bz-2.29);
				v3f v(1./7, 1./7, 1./7);
				f32 dtime;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == 0);
				UASSERT(fabs(dtime - 16.1) < 0.001);
			}
			{
				aabb3f s(bx, by, bz, bx+2, by+2, bz+2);
				aabb3f m(bx-4.2, by-4.2, bz-4.2, bx-2.29, by-2.3, bz-2.29);
				v3f v(1./7, 1./7, 1./7);
				f32 dtime;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == 1);
				UASSERT(fabs(dtime - 16.1) < 0.001);
			}
			{
				aabb3f s(bx, by, bz, bx+2, by+2, bz+2);
				aabb3f m(bx-4.2, by-4.2, bz-4.2, bx-2.29, by-2.29, bz-2.3);
				v3f v(1./7, 1./7, 1./7);
				f32 dtime;
				UASSERT(axisAlignedCollision(s, m, v, 0, dtime) == 2);
				UASSERT(fabs(dtime - 16.1) < 0.001);
			}
		}
	}
};

struct TestSocket: public TestBase
{
	void Run()
	{
		const int port = 30003;
		Address address(0, 0, 0, 0, port);
		Address bind_addr(0, 0, 0, 0, port);
		Address address6((IPv6AddressBytes*) NULL, port);

		/*
		 * Try to use the bind_address for servers with no localhost address
		 * For example: FreeBSD jails
		 */
		std::string bind_str = g_settings->get("bind_address");
		try {
			bind_addr.Resolve(bind_str.c_str());

			if (!bind_addr.isIPv6()) {
				address = bind_addr;
			}
		} catch (ResolveError &e) {
		}

		// IPv6 socket test
		if (g_settings->getBool("enable_ipv6")) {
			UDPSocket socket6;

			if (!socket6.init(true, true)) {
				/* Note: Failing to create an IPv6 socket is not technically an
				   error because the OS may not support IPv6 or it may
				   have been disabled. IPv6 is not /required/ by
				   minetest and therefore this should not cause the unit
				   test to fail
				*/
				dstream << "WARNING: IPv6 socket creation failed (unit test)"
				        << std::endl;
			} else {
				const char sendbuffer[] = "hello world!";
				IPv6AddressBytes bytes;
				bytes.bytes[15] = 1;

				socket6.Bind(address6);

				try {
					socket6.Send(Address(&bytes, port), sendbuffer, sizeof(sendbuffer));

					sleep_ms(50);

					char rcvbuffer[256] = { 0 };
					Address sender;

					for(;;) {
						if (socket6.Receive(sender, rcvbuffer, sizeof(rcvbuffer )) < 0)
							break;
					}
					//FIXME: This fails on some systems
					UASSERT(strncmp(sendbuffer, rcvbuffer, sizeof(sendbuffer)) == 0);
					UASSERT(memcmp(sender.getAddress6().sin6_addr.s6_addr,
							Address(&bytes, 0).getAddress6().sin6_addr.s6_addr, 16) == 0);
				}
				catch (SendFailedException &e) {
					errorstream << "IPv6 support enabled but not available!"
					            << std::endl;
				}
			}
		}

		// IPv4 socket test
		{
			UDPSocket socket(false);
			socket.Bind(address);

			const char sendbuffer[] = "hello world!";
			/*
			 * If there is a bind address, use it.
			 * It's useful in container environments
			 */
			if (address != Address(0, 0, 0, 0, port)) {
				socket.Send(address, sendbuffer, sizeof(sendbuffer));
			}
			else
				socket.Send(Address(127, 0, 0 ,1, port), sendbuffer, sizeof(sendbuffer));

			sleep_ms(50);

			char rcvbuffer[256] = { 0 };
			Address sender;
			for(;;) {
				if (socket.Receive(sender, rcvbuffer, sizeof(rcvbuffer)) < 0)
					break;
			}
			//FIXME: This fails on some systems
			UASSERT(strncmp(sendbuffer, rcvbuffer, sizeof(sendbuffer)) == 0);

			if (address != Address(0, 0, 0, 0, port)) {
				UASSERT(sender.getAddress().sin_addr.s_addr ==
						address.getAddress().sin_addr.s_addr);
			}
			else {
				UASSERT(sender.getAddress().sin_addr.s_addr ==
						Address(127, 0, 0, 1, 0).getAddress().sin_addr.s_addr);
			}
		}
	}
};

struct TestConnection: public TestBase
{
	void TestHelpers()
	{
		/*
			Test helper functions
		*/

		// Some constants for testing
		u32 proto_id = 0x12345678;
		u16 peer_id = 123;
		u8 channel = 2;
		SharedBuffer<u8> data1(1);
		data1[0] = 100;
		Address a(127,0,0,1, 10);
		const u16 seqnum = 34352;

		con::BufferedPacket p1 = con::makePacket(a, data1,
				proto_id, peer_id, channel);
		/*
			We should now have a packet with this data:
			Header:
				[0] u32 protocol_id
				[4] u16 sender_peer_id
				[6] u8 channel
			Data:
				[7] u8 data1[0]
		*/
		UASSERT(readU32(&p1.data[0]) == proto_id);
		UASSERT(readU16(&p1.data[4]) == peer_id);
		UASSERT(readU8(&p1.data[6]) == channel);
		UASSERT(readU8(&p1.data[7]) == data1[0]);

		//infostream<<"initial data1[0]="<<((u32)data1[0]&0xff)<<std::endl;

		SharedBuffer<u8> p2 = con::makeReliablePacket(data1, seqnum);

		/*infostream<<"p2.getSize()="<<p2.getSize()<<", data1.getSize()="
				<<data1.getSize()<<std::endl;
		infostream<<"readU8(&p2[3])="<<readU8(&p2[3])
				<<" p2[3]="<<((u32)p2[3]&0xff)<<std::endl;
		infostream<<"data1[0]="<<((u32)data1[0]&0xff)<<std::endl;*/

		UASSERT(p2.getSize() == 3 + data1.getSize());
		UASSERT(readU8(&p2[0]) == TYPE_RELIABLE);
		UASSERT(readU16(&p2[1]) == seqnum);
		UASSERT(readU8(&p2[3]) == data1[0]);
	}

	struct Handler : public con::PeerHandler
	{
		Handler(const char *a_name)
		{
			count = 0;
			last_id = 0;
			name = a_name;
		}
		void peerAdded(con::Peer *peer)
		{
			infostream<<"Handler("<<name<<")::peerAdded(): "
					"id="<<peer->id<<std::endl;
			last_id = peer->id;
			count++;
		}
		void deletingPeer(con::Peer *peer, bool timeout)
		{
			infostream<<"Handler("<<name<<")::deletingPeer(): "
					"id="<<peer->id
					<<", timeout="<<timeout<<std::endl;
			last_id = peer->id;
			count--;
		}

		s32 count;
		u16 last_id;
		const char *name;
	};

	void Run()
	{
		DSTACK("TestConnection::Run");

		TestHelpers();

		/*
			Test some real connections

			NOTE: This mostly tests the legacy interface.
		*/

		u32 proto_id = 0xad26846a;

		Handler hand_server("server");
		Handler hand_client("client");

		Address address(0, 0, 0, 0, 30001);
		Address bind_addr(0, 0, 0, 0, 30001);
		/*
		 * Try to use the bind_address for servers with no localhost address
		 * For example: FreeBSD jails
		 */
		std::string bind_str = g_settings->get("bind_address");
		try {
			bind_addr.Resolve(bind_str.c_str());

			if (!bind_addr.isIPv6()) {
				address = bind_addr;
			}
		} catch (ResolveError &e) {
		}

		infostream<<"** Creating server Connection"<<std::endl;
		con::Connection server(proto_id, 512, 5.0, false, &hand_server);
		server.Serve(address);

		infostream<<"** Creating client Connection"<<std::endl;
		con::Connection client(proto_id, 512, 5.0, false, &hand_client);

		UASSERT(hand_server.count == 0);
		UASSERT(hand_client.count == 0);

		sleep_ms(50);

		Address server_address(127, 0, 0, 1, 30001);
		if (address != Address(0, 0, 0, 0, 30001)) {
			server_address = bind_addr;
		}

		infostream<<"** running client.Connect()"<<std::endl;
		client.Connect(server_address);

		sleep_ms(50);

		// Client should not have added client yet
		UASSERT(hand_client.count == 0);

		try
		{
			NetworkPacket pkt;
			infostream << "** running client.Receive()" << std::endl;
			client.Receive(&pkt);
			infostream << "** Client received: peer_id=" << pkt.getPeerId()
					<< ", size=" << pkt.getSize()
					<< std::endl;
		}
		catch(con::NoIncomingDataException &e)
		{
		}

		// Client should have added server now
		UASSERT(hand_client.count == 1);
		UASSERT(hand_client.last_id == 1);
		// Server should not have added client yet
		UASSERT(hand_server.count == 0);

		sleep_ms(100);

		try
		{
			NetworkPacket pkt;
			infostream << "** running server.Receive()" << std::endl;
			server.Receive(&pkt);
			infostream<<"** Server received: peer_id=" << pkt.getPeerId()
					<< ", size=" << pkt.getSize()
					<< std::endl;
		}
		catch(con::NoIncomingDataException &e)
		{
			// No actual data received, but the client has
			// probably been connected
		}

		// Client should be the same
		UASSERT(hand_client.count == 1);
		UASSERT(hand_client.last_id == 1);
		// Server should have the client
		UASSERT(hand_server.count == 1);
		UASSERT(hand_server.last_id == 2);

		//sleep_ms(50);

		while(client.Connected() == false)
		{
			try
			{
				NetworkPacket pkt;
				infostream << "** running client.Receive()" << std::endl;
				client.Receive(&pkt);
				infostream << "** Client received: peer_id=" << pkt.getPeerId()
						<< ", size=" << pkt.getSize()
						<< std::endl;
			}
			catch(con::NoIncomingDataException &e)
			{
			}
			sleep_ms(50);
		}

		sleep_ms(50);

		try
		{
			NetworkPacket pkt;
			infostream << "** running server.Receive()" << std::endl;
			server.Receive(&pkt);
			infostream << "** Server received: peer_id=" << pkt.getPeerId()
					<< ", size=" << pkt.getSize()
					<< std::endl;
		}
		catch(con::NoIncomingDataException &e)
		{
		}

		/*
			Simple send-receive test
		*/
		{
			NetworkPacket pkt;
			pkt.putRawPacket((u8*) "Hello World !", 14, 0);

			Buffer<u8> sentdata = pkt.oldForgePacket();

			infostream<<"** running client.Send()"<<std::endl;
			client.Send(PEER_ID_SERVER, 0, &pkt, true);

			sleep_ms(50);

			NetworkPacket recvpacket;
			infostream << "** running server.Receive()" << std::endl;
			server.Receive(&recvpacket);
			infostream << "** Server received: peer_id=" << pkt.getPeerId()
					<< ", size=" << pkt.getSize()
					<< ", data=" << (const char*)pkt.getU8Ptr(0)
					<< std::endl;

			Buffer<u8> recvdata = pkt.oldForgePacket();

			UASSERT(memcmp(*sentdata, *recvdata, recvdata.getSize()) == 0);
		}

		u16 peer_id_client = 2;
		/*
			Send a large packet
		*/
		{
			const int datasize = 30000;
			NetworkPacket pkt(0, datasize);
			for (u16 i=0; i<datasize; i++) {
				pkt << (u8) i/4;
			}

			infostream<<"Sending data (size="<<datasize<<"):";
			for(int i=0; i<datasize && i<20; i++) {
				if(i%2==0) infostream<<" ";
				char buf[10];
				snprintf(buf, 10, "%.2X", ((int)((const char*)pkt.getU8Ptr(0))[i])&0xff);
				infostream<<buf;
			}
			if(datasize > 20)
				infostream << "...";
			infostream << std::endl;

			Buffer<u8> sentdata = pkt.oldForgePacket();

			server.Send(peer_id_client, 0, &pkt, true);

			//sleep_ms(3000);

			Buffer<u8> recvdata;
			infostream << "** running client.Receive()" << std::endl;
			u16 peer_id = 132;
			u16 size = 0;
			bool received = false;
			u32 timems0 = porting::getTimeMs();
			for(;;) {
				if(porting::getTimeMs() - timems0 > 5000 || received)
					break;
				try {
					NetworkPacket pkt;
					client.Receive(&pkt);
					size = pkt.getSize();
					peer_id = pkt.getPeerId();
					recvdata = pkt.oldForgePacket();
					received = true;
				} catch(con::NoIncomingDataException &e) {
				}
				sleep_ms(10);
			}
			UASSERT(received);
			infostream<<"** Client received: peer_id="<<peer_id
					<<", size="<<size
					<<std::endl;

			infostream<<"Received data (size="<<size<<"): ";
			for(int i=0; i<size && i<20; i++){
				if(i%2==0) infostream<<" ";
				char buf[10];
				snprintf(buf, 10, "%.2X", ((int)(recvdata[i]))&0xff);
				infostream<<buf;
			}
			if(size>20)
				infostream<<"...";
			infostream<<std::endl;

			UASSERT(memcmp(*sentdata, *recvdata, recvdata.getSize()) == 0);
			UASSERT(peer_id == PEER_ID_SERVER);
		}

		// Check peer handlers
		UASSERT(hand_client.count == 1);
		UASSERT(hand_client.last_id == 1);
		UASSERT(hand_server.count == 1);
		UASSERT(hand_server.last_id == 2);
	}
};

static const float expected_3d_results[10 * 10 * 10] = {
	19.11726, 18.01059, 16.90392, 15.79725, 16.37154, 17.18597, 18.00040,
	18.33467, 18.50889, 18.68311, 17.85386, 16.90585, 15.95785, 15.00985,
	15.61132, 16.43415, 17.25697, 17.95415, 18.60942, 19.26471, 16.59046,
	15.80112, 15.01178, 14.22244, 14.85110, 15.68232, 16.51355, 17.57361,
	18.70996, 19.84631, 15.32705, 14.69638, 14.06571, 13.43504, 14.09087,
	14.93050, 15.77012, 17.19309, 18.81050, 20.42790, 15.06729, 14.45855,
	13.84981, 13.24107, 14.39364, 15.79782, 17.20201, 18.42640, 19.59085,
	20.75530, 14.95090, 14.34456, 13.73821, 13.13187, 14.84825, 16.89645,
	18.94465, 19.89025, 20.46832, 21.04639, 14.83452, 14.23057, 13.62662,
	13.02267, 15.30287, 17.99508, 20.68730, 21.35411, 21.34580, 21.33748,
	15.39817, 15.03590, 14.67364, 14.31137, 16.78242, 19.65824, 22.53405,
	22.54626, 21.60395, 20.66164, 16.18850, 16.14768, 16.10686, 16.06603,
	18.60362, 21.50956, 24.41549, 23.64784, 21.65566, 19.66349, 16.97884,
	17.25946, 17.54008, 17.82069, 20.42482, 23.36088, 26.29694, 24.74942,
	21.70738, 18.66534, 18.78506, 17.51834, 16.25162, 14.98489, 15.14217,
	15.50287, 15.86357, 16.40597, 17.00895, 17.61193, 18.20160, 16.98795,
	15.77430, 14.56065, 14.85059, 15.35533, 15.86007, 16.63399, 17.49763,
	18.36128, 17.61814, 16.45757, 15.29699, 14.13641, 14.55902, 15.20779,
	15.85657, 16.86200, 17.98632, 19.11064, 17.03468, 15.92718, 14.81968,
	13.71218, 14.26744, 15.06026, 15.85306, 17.09001, 18.47501, 19.86000,
	16.67870, 15.86256, 15.04641, 14.23026, 15.31397, 16.66909, 18.02420,
	18.89042, 19.59369, 20.29695, 16.35522, 15.86447, 15.37372, 14.88297,
	16.55165, 18.52883, 20.50600, 20.91547, 20.80237, 20.68927, 16.03174,
	15.86639, 15.70103, 15.53568, 17.78933, 20.38857, 22.98780, 22.94051,
	22.01105, 21.08159, 16.42434, 16.61407, 16.80381, 16.99355, 19.16133,
	21.61169, 24.06204, 23.65252, 22.28970, 20.92689, 17.05562, 17.61035,
	18.16508, 18.71981, 20.57809, 22.62260, 24.66711, 23.92686, 22.25835,
	20.58984, 17.68691, 18.60663, 19.52635, 20.44607, 21.99486, 23.63352,
	25.27217, 24.20119, 22.22699, 20.25279, 18.45285, 17.02608, 15.59931,
	14.17254, 13.91279, 13.81976, 13.72674, 14.47727, 15.50900, 16.54073,
	18.54934, 17.07005, 15.59076, 14.11146, 14.08987, 14.27651, 14.46316,
	15.31383, 16.38584, 17.45785, 18.64582, 17.11401, 15.58220, 14.05039,
	14.26694, 14.73326, 15.19958, 16.15038, 17.26268, 18.37498, 18.74231,
	17.15798, 15.57364, 13.98932, 14.44402, 15.19001, 15.93600, 16.98694,
	18.13952, 19.29210, 18.29012, 17.26656, 16.24301, 15.21946, 16.23430,
	17.54035, 18.84639, 19.35445, 19.59653, 19.83860, 17.75954, 17.38438,
	17.00923, 16.63407, 18.25505, 20.16120, 22.06734, 21.94068, 21.13642,
	20.33215, 17.22896, 17.50220, 17.77544, 18.04868, 20.27580, 22.78205,
	25.28829, 24.52691, 22.67631, 20.82571, 17.45050, 18.19224, 18.93398,
	19.67573, 21.54024, 23.56514, 25.59004, 24.75878, 22.97546, 21.19213,
	17.92274, 19.07302, 20.22330, 21.37358, 22.55256, 23.73565, 24.91873,
	24.20587, 22.86103, 21.51619, 18.39499, 19.95381, 21.51263, 23.07145,
	23.56490, 23.90615, 24.24741, 23.65296, 22.74660, 21.84024, 18.12065,
	16.53382, 14.94700, 13.36018, 12.68341, 12.13666, 11.58990, 12.54858,
	14.00906, 15.46955, 18.89708, 17.15214, 15.40721, 13.66227, 13.32914,
	13.19769, 13.06625, 13.99367, 15.27405, 16.55443, 19.67351, 17.77046,
	15.86741, 13.96436, 13.97486, 14.25873, 14.54260, 15.43877, 16.53904,
	17.63931, 20.44994, 18.38877, 16.32761, 14.26645, 14.62059, 15.31977,
	16.01895, 16.88387, 17.80403, 18.72419, 19.90153, 18.67057, 17.43962,
	16.20866, 17.15464, 18.41161, 19.66858, 19.81848, 19.59936, 19.38024,
	19.16386, 18.90429, 18.64473, 18.38517, 19.95845, 21.79357, 23.62868,
	22.96589, 21.47046, 19.97503, 18.42618, 19.13802, 19.84985, 20.56168,
	22.76226, 25.17553, 27.58879, 26.11330, 23.34156, 20.56982, 18.47667,
	19.77041, 21.06416, 22.35790, 23.91914, 25.51859, 27.11804, 25.86504,
	23.66121, 21.45738, 18.78986, 20.53570, 22.28153, 24.02736, 24.52704,
	24.84869, 25.17035, 24.48488, 23.46371, 22.44254, 19.10306, 21.30098,
	23.49890, 25.69682, 25.13494, 24.17879, 23.22265, 23.10473, 23.26621,
	23.42769, 17.93453, 16.72707, 15.51962, 14.31216, 12.96039, 11.58800,
	10.21561, 11.29675, 13.19573, 15.09471, 18.05853, 16.85308, 15.64762,
	14.44216, 13.72634, 13.08047, 12.43459, 13.48912, 15.11045, 16.73179,
	18.18253, 16.97908, 15.77562, 14.57217, 14.49229, 14.57293, 14.65357,
	15.68150, 17.02518, 18.36887, 18.30654, 17.10508, 15.90363, 14.70217,
	15.25825, 16.06540, 16.87255, 17.87387, 18.93991, 20.00595, 17.54117,
	17.32369, 17.10622, 16.88875, 18.07494, 19.46166, 20.84837, 21.12988,
	21.04298, 20.95609, 16.64874, 17.55554, 18.46234, 19.36913, 21.18461,
	23.12989, 25.07517, 24.53784, 23.17297, 21.80810, 15.75632, 17.78738,
	19.81845, 21.84951, 24.29427, 26.79812, 29.30198, 27.94580, 25.30295,
	22.66010, 15.98046, 18.43027, 20.88008, 23.32989, 25.21976, 27.02964,
	28.83951, 27.75863, 25.71416, 23.66970, 16.57679, 19.21017, 21.84355,
	24.47693, 25.41719, 26.11557, 26.81396, 26.37308, 25.55245, 24.73182,
	17.17313, 19.99008, 22.80702, 25.62397, 25.61462, 25.20151, 24.78840,
	24.98753, 25.39074, 25.79395, 17.76927, 17.01824, 16.26722, 15.51620,
	13.45256, 11.20141,  8.95025, 10.14162, 12.48049, 14.81936, 17.05051,
	16.49955, 15.94860, 15.39764, 14.28896, 13.10061, 11.91225, 13.10109,
	15.08232, 17.06355, 16.33175, 15.98086, 15.62998, 15.27909, 15.12537,
	14.99981, 14.87425, 16.06056, 17.68415, 19.30775, 15.61299, 15.46217,
	15.31136, 15.16054, 15.96177, 16.89901, 17.83625, 19.02003, 20.28599,
	21.55194, 14.61341, 15.58383, 16.55426, 17.52469, 18.99524, 20.53725,
	22.07925, 22.56233, 22.69243, 22.82254, 13.57371, 15.79697, 18.02024,
	20.24351, 22.34258, 24.42392, 26.50526, 26.18790, 25.07097, 23.95404,
	12.53401, 16.01011, 19.48622, 22.96232, 25.68993, 28.31060, 30.93126,
	29.81347, 27.44951, 25.08555, 12.98106, 16.67323, 20.36540, 24.05756,
	26.36633, 28.47748, 30.58862, 29.76471, 27.96244, 26.16016, 13.92370,
	17.48634, 21.04897, 24.61161, 26.15244, 27.40443, 28.65643, 28.49117,
	27.85349, 27.21581, 14.86633, 18.29944, 21.73255, 25.16566, 25.93854,
	26.33138, 26.72423, 27.21763, 27.74455, 28.27147, 17.60401, 17.30942,
	17.01482, 16.72023, 13.94473, 10.81481,  7.68490,  8.98648, 11.76524,
	14.54400, 16.04249, 16.14603, 16.24958, 16.35312, 14.85158, 13.12075,
	11.38991, 12.71305, 15.05418, 17.39531, 14.48097, 14.98265, 15.48433,
	15.98602, 15.75844, 15.42668, 15.09493, 16.43962, 18.34312, 20.24663,
	12.91945, 13.81927, 14.71909, 15.61891, 16.66530, 17.73262, 18.79995,
	20.16619, 21.63206, 23.09794, 11.68565, 13.84398, 16.00230, 18.16062,
	19.91554, 21.61284, 23.31013, 23.99478, 24.34188, 24.68898, 10.49868,
	14.03841, 17.57814, 21.11788, 23.50056, 25.71795, 27.93534, 27.83796,
	26.96897, 26.09999,  9.31170, 14.23284, 19.15399, 24.07513, 27.08558,
	29.82307, 32.56055, 31.68113, 29.59606, 27.51099,  9.98166, 14.91619,
	19.85071, 24.78524, 27.51291, 29.92532, 32.33773, 31.77077, 30.21070,
	28.65063, 11.27060, 15.76250, 20.25440, 24.74629, 26.88768, 28.69329,
	30.49889, 30.60925, 30.15453, 29.69981, 12.55955, 16.60881, 20.65808,
	24.70735, 26.26245, 27.46126, 28.66005, 29.44773, 30.09835, 30.74898,
	15.20134, 15.53016, 15.85898, 16.18780, 13.53087, 10.44740,  7.36393,
	 8.95806, 12.11139, 15.26472, 13.87432, 14.52378, 15.17325, 15.82272,
	14.49093, 12.87611, 11.26130, 12.73342, 15.23453, 17.73563, 12.54730,
	13.51741, 14.48752, 15.45763, 15.45100, 15.30483, 15.15867, 16.50878,
	18.35766, 20.20654, 11.22027, 12.51103, 13.80179, 15.09254, 16.41106,
	17.73355, 19.05603, 20.28415, 21.48080, 22.67745, 10.27070, 12.53633,
	14.80195, 17.06758, 19.04654, 20.98454, 22.92254, 23.63840, 23.94687,
	24.25534,  9.37505, 12.70901, 16.04297, 19.37693, 21.92136, 24.35300,
	26.78465, 26.93249, 26.31907, 25.70565,  8.47939, 12.88168, 17.28398,
	21.68627, 24.79618, 27.72146, 30.64674, 30.22658, 28.69127, 27.15597,
	 9.77979, 13.97583, 18.17186, 22.36790, 25.18828, 27.81215, 30.43601,
	30.34293, 29.34420, 28.34548, 11.81220, 15.37712, 18.94204, 22.50695,
	24.75282, 26.81024, 28.86766, 29.40003, 29.42404, 29.44806, 13.84461,
	16.77841, 19.71221, 22.64601, 24.31735, 25.80833, 27.29932, 28.45713,
	29.50388, 30.55064, 12.05287, 13.06077, 14.06866, 15.07656, 12.81500,
	10.08638,  7.35776,  9.30520, 12.81134, 16.31747, 11.31943, 12.47863,
	13.63782, 14.79702, 13.82253, 12.54323, 11.26392, 12.88993, 15.48436,
	18.07880, 10.58600, 11.89649, 13.20698, 14.51747, 14.83005, 15.00007,
	15.17009, 16.47465, 18.15739, 19.84013,  9.85256, 11.31435, 12.77614,
	14.23793, 15.83757, 17.45691, 19.07625, 20.05937, 20.83042, 21.60147,
	 9.36002, 11.37275, 13.38548, 15.39822, 17.58109, 19.78828, 21.99546,
	22.68573, 22.87036, 23.05500,  8.90189, 11.52266, 14.14343, 16.76420,
	19.42976, 22.10172, 24.77368, 25.17519, 24.81987, 24.46455,  8.44375,
	11.67256, 14.90137, 18.13018, 21.27843, 24.41516, 27.55190, 27.66464,
	26.76937, 25.87411, 10.51042, 13.30769, 16.10496, 18.90222, 21.70659,
	24.51197, 27.31734, 27.77045, 27.43945, 27.10846, 13.41869, 15.43789,
	17.45709, 19.47628, 21.66124, 23.86989, 26.07853, 27.08170, 27.68305,
	28.28440, 16.32697, 17.56809, 18.80922, 20.05033, 21.61590, 23.22781,
	24.83972, 26.39296, 27.92665, 29.46033,  8.90439, 10.59137, 12.27835,
	13.96532, 12.09914,  9.72536,  7.35159,  9.65235, 13.51128, 17.37022,
	 8.76455, 10.43347, 12.10239, 13.77132, 13.15412, 12.21033, 11.26655,
	13.04643, 15.73420, 18.42198,  8.62470, 10.27557, 11.92644, 13.57731,
	14.20910, 14.69531, 15.18151, 16.44051, 17.95712, 19.47373,  8.48485,
	10.11767, 11.75049, 13.38331, 15.26408, 17.18027, 19.09647, 19.83460,
	20.18004, 20.52548,  8.44933, 10.20917, 11.96901, 13.72885, 16.11565,
	18.59202, 21.06838, 21.73307, 21.79386, 21.85465,  8.42872, 10.33631,
	12.24389, 14.15147, 16.93816, 19.85044, 22.76272, 23.41788, 23.32067,
	23.22346,  8.40812, 10.46344, 12.51877, 14.57409, 17.76068, 21.10886,
	24.45705, 25.10269, 24.84748, 24.59226, 11.24106, 12.63955, 14.03805,
	15.43654, 18.22489, 21.21178, 24.19868, 25.19796, 25.53469, 25.87143,
	15.02519, 15.49866, 15.97213, 16.44560, 18.56967, 20.92953, 23.28940,
	24.76337, 25.94205, 27.12073, 18.80933, 18.35777, 17.90622, 17.45466,
	18.91445, 20.64729, 22.38013, 24.32880, 26.34941, 28.37003,
};

static const float expected_2d_results[10 * 10] = {
	19.11726, 18.49626, 16.48476, 15.02135, 14.75713, 16.26008, 17.54822,
	18.06860, 18.57016, 18.48407, 18.49649, 17.89160, 15.94162, 14.54901,
	14.31298, 15.72643, 16.94669, 17.55494, 18.58796, 18.87925, 16.08101,
	15.53764, 13.83844, 12.77139, 12.73648, 13.95632, 14.97904, 15.81829,
	18.37694, 19.73759, 13.19182, 12.71924, 11.34560, 10.78025, 11.18980,
	12.52303, 13.45012, 14.30001, 17.43298, 19.15244, 10.93217, 10.48625,
	 9.30923,  9.18632, 10.16251, 12.11264, 13.19697, 13.80801, 16.39567,
	17.66203, 10.40222,  9.86070,  8.47223,  8.45471, 10.04780, 13.54730,
	15.33709, 15.48503, 16.46177, 16.52508, 10.80333, 10.19045,  8.59420,
	 8.47646, 10.22676, 14.43173, 16.48353, 16.24859, 16.20863, 15.52847,
	11.01179, 10.45209,  8.98678,  8.83986, 10.43004, 14.46054, 16.29387,
	15.73521, 15.01744, 13.85542, 10.55201, 10.33375,  9.85102, 10.07821,
	11.58235, 15.62046, 17.35505, 16.13181, 12.66011,  9.51853, 11.50994,
	11.54074, 11.77989, 12.29790, 13.76139, 17.81982, 19.49008, 17.79470,
	12.34344,  7.78363,
};

struct TestNoise: public TestBase
{
	void TestNoise2dPoint()
	{
		NoiseParams np_normal(20, 40, v3f(50, 50, 50), 9,  5, 0.6, 2.0);

		u32 i = 0;
		for (u32 y = 0; y != 10; y++)
		for (u32 x = 0; x != 10; x++, i++) {
			float actual   = NoisePerlin2D(&np_normal, x, y, 1337);
			float expected = expected_2d_results[i];
			UASSERT(fabs(actual - expected) <= 0.00001);
		}
	}

	void TestNoise2dBulk()
	{
		NoiseParams np_normal(20, 40, v3f(50, 50, 50), 9,  5, 0.6, 2.0);
		Noise noise_normal_2d(&np_normal, 1337, 10, 10);
		float *noisevals = noise_normal_2d.perlinMap2D(0, 0, NULL);

		for (u32 i = 0; i != 10 * 10; i++) {
			float actual   = noisevals[i];
			float expected = expected_2d_results[i];
			UASSERT(fabs(actual - expected) <= 0.00001);
		}
	}

	void TestNoise3dPoint()
	{
		NoiseParams np_normal(20, 40, v3f(50, 50, 50), 9,  5, 0.6, 2.0);

		u32 i = 0;
		for (u32 z = 0; z != 10; z++)
		for (u32 y = 0; y != 10; y++)
		for (u32 x = 0; x != 10; x++, i++) {
			float actual   = NoisePerlin3D(&np_normal, x, y, z, 1337);
			float expected = expected_3d_results[i];
			UASSERT(fabs(actual - expected) <= 0.00001);
		}
	}

	void TestNoise3dBulk()
	{
		NoiseParams np_normal(20, 40, v3f(50, 50, 50), 9, 5, 0.6, 2.0);
		Noise noise_normal_3d(&np_normal, 1337, 10, 10, 10);
		float *noisevals = noise_normal_3d.perlinMap3D(0, 0, 0, NULL);

		for (u32 i = 0; i != 10 * 10 * 10; i++) {
			float actual   = noisevals[i];
			float expected = expected_3d_results[i];
			UASSERT(fabs(actual - expected) <= 0.00001);
		}
	}

	void TestNoiseInvalidParams()
	{
		bool exception_thrown = false;

		try {
			NoiseParams np_highmem(4, 70, v3f(1, 1, 1), 5, 60, 0.7, 10.0);
			Noise noise_highmem_3d(&np_highmem, 1337, 200, 200, 200);
			noise_highmem_3d.perlinMap3D(0, 0, 0, NULL);
		} catch (InvalidNoiseParamsException) {
			exception_thrown = true;
		}

		UASSERT(exception_thrown);
	}

	void Run()
	{
		TestNoise2dPoint();
		TestNoise2dBulk();
		TestNoise3dPoint();
		TestNoise3dBulk();
		TestNoiseInvalidParams();
	}
};

struct TestProfiler : public TestBase
{
	void Run()
	{
		Profiler p;

		p.avg("Test1", 1.f);
		UASSERT(p.getValue("Test1") == 1.f);

		p.avg("Test1", 2.f);
		UASSERT(p.getValue("Test1") == 1.5f);

		p.avg("Test1", 3.f);
		UASSERT(p.getValue("Test1") == 2.f);

		p.avg("Test1", 486.f);
		UASSERT(p.getValue("Test1") == 123.f);

		p.avg("Test1", 8);
		UASSERT(p.getValue("Test1") == 100.f);

		p.avg("Test1", 700);
		UASSERT(p.getValue("Test1") == 200.f);

		p.avg("Test1", 10000);
		UASSERT(p.getValue("Test1") == 1600.f);

		p.avg("Test2", 123.56);
		p.avg("Test2", 123.58);

		UASSERT(p.getValue("Test2") == 123.57f);
	}
};

#define TEST(X) do {\
	X x;\
	infostream<<"Running " #X <<std::endl;\
	x.Run();\
	tests_run++;\
	tests_failed += x.test_failed ? 1 : 0;\
} while (0)

#define TESTPARAMS(X, ...) do {\
	X x;\
	infostream<<"Running " #X <<std::endl;\
	x.Run(__VA_ARGS__);\
	tests_run++;\
	tests_failed += x.test_failed ? 1 : 0;\
} while (0)

void run_tests()
{
	DSTACK(__FUNCTION_NAME);

	int tests_run = 0;
	int tests_failed = 0;

	// Create item and node definitions
	IWritableItemDefManager *idef = createItemDefManager();
	IWritableNodeDefManager *ndef = createNodeDefManager();
	define_some_nodes(idef, ndef);

	log_set_lev_silence(LMT_ERROR, true);

	infostream<<"run_tests() started"<<std::endl;
	TEST(TestUtilities);
	TEST(TestPath);
	TEST(TestSettings);
	TEST(TestCompress);
	TEST(TestSerialization);
	TEST(TestNodedefSerialization);
	TEST(TestProfiler);
	TEST(TestNoise);
	TESTPARAMS(TestMapNode, ndef);
	TESTPARAMS(TestVoxelManipulator, ndef);
	TESTPARAMS(TestVoxelAlgorithms, ndef);
	TESTPARAMS(TestInventory, idef);
	//TEST(TestMapBlock);
	//TEST(TestMapSector);
	TEST(TestCollision);
	if(INTERNET_SIMULATOR == false){
		TEST(TestSocket);
		dout_con << "=== BEGIN RUNNING UNIT TESTS FOR CONNECTION ===" << std::endl;
		TEST(TestConnection);
		dout_con << "=== END RUNNING UNIT TESTS FOR CONNECTION ===" << std::endl;
	}

	log_set_lev_silence(LMT_ERROR, false);

	delete idef;
	delete ndef;

	if(tests_failed == 0) {
		actionstream << "run_tests(): " << tests_failed << " / " << tests_run << " tests failed." << std::endl;
		actionstream << "run_tests() passed." << std::endl;
		return;
	} else {
		errorstream << "run_tests(): " << tests_failed << " / " << tests_run << " tests failed." << std::endl;
		errorstream << "run_tests() aborting." << std::endl;
		abort();
	}
}


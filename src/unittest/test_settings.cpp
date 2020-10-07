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

#include <cmath>
#include "settings.h"
#include "noise.h"

class TestSettings : public TestBase {
public:
	TestSettings() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestSettings"; }

	void runTests(IGameDef *gamedef);

	void testAllSettings();
	void testFlagDesc();

	static const char *config_text_before;
	static const std::string config_text_after;
};

static TestSettings g_test_instance;

void TestSettings::runTests(IGameDef *gamedef)
{
	TEST(testAllSettings);
	TEST(testFlagDesc);
}

////////////////////////////////////////////////////////////////////////////////

const char *TestSettings::config_text_before =
	"leet = 1337\n"
	"leetleet = 13371337\n"
	"leetleet_neg = -13371337\n"
	"floaty_thing = 1.1\n"
	"stringy_thing = asd /( ¤%&(/\" BLÖÄRP\n"
	"coord = (1, 2, 4.5)\n"
	"      # this is just a comment\n"
	"this is an invalid line\n"
	"asdf = {\n"
	"	a   = 5\n"
	"	bb  = 2.5\n"
	"	ccc = \"\"\"\n"
	"testy\n"
	"   testa   \n"
	"\"\"\"\n"
	"\n"
	"}\n"
	"blarg = \"\"\" \n"
	"some multiline text\n"
	"     with leading whitespace!\n"
	"\"\"\"\n"
	"np_terrain = 5, 40, (250, 250, 250), 12341, 5, 0.7, 2.4\n"
	"zoop = true";

const std::string TestSettings::config_text_after =
	"leet = 1337\n"
	"leetleet = 13371337\n"
	"leetleet_neg = -13371337\n"
	"floaty_thing = 1.1\n"
	"stringy_thing = asd /( ¤%&(/\" BLÖÄRP\n"
	"coord = (1, 2, 4.5)\n"
	"      # this is just a comment\n"
	"this is an invalid line\n"
	"asdf = {\n"
	"	a   = 5\n"
	"	bb  = 2.5\n"
	"	ccc = \"\"\"\n"
	"testy\n"
	"   testa   \n"
	"\"\"\"\n"
	"\n"
	"}\n"
	"blarg = \"\"\" \n"
	"some multiline text\n"
	"     with leading whitespace!\n"
	"\"\"\"\n"
	"np_terrain = {\n"
	"	flags = defaults\n"
	"	lacunarity = 2.4\n"
	"	octaves = 6\n"
	"	offset = 3.5\n"
	"	persistence = 0.7\n"
	"	scale = 40\n"
	"	seed = 12341\n"
	"	spread = (250,250,250)\n"
	"}\n"
	"zoop = true\n"
	"coord2 = (1,2,3.3)\n"
	"floaty_thing_2 = 1.2\n"
	"groupy_thing = {\n"
	"	animals = cute\n"
	"	num_apples = 4\n"
	"	num_oranges = 53\n"
	"}\n";

void TestSettings::testAllSettings()
{
	try {
	Settings s;

	// Test reading of settings
	std::istringstream is(config_text_before);
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

	Settings group3;
	group3.set("cat", "meow");
	group3.set("dog", "woof");

	Settings group2;
	group2.setS16("num_apples", 4);
	group2.setS16("num_oranges", 53);
	group2.setGroup("animals", group3);
	group2.set("animals", "cute"); //destroys group 3
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
	UASSERT(std::fabs(np.offset - 5) < 0.001f);
	UASSERT(std::fabs(np.scale - 40) < 0.001f);
	UASSERT(std::fabs(np.spread.X - 250) < 0.001f);
	UASSERT(std::fabs(np.spread.Y - 250) < 0.001f);
	UASSERT(std::fabs(np.spread.Z - 250) < 0.001f);
	UASSERT(np.seed == 12341);
	UASSERT(np.octaves == 5);
	UASSERT(std::fabs(np.persist - 0.7) < 0.001f);

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
#if __cplusplus < 201103L
	// This test only works in older C++ versions than C++11 because we use unordered_map
	UASSERT(os.str() == config_text_after);
#endif
	} catch (SettingNotFoundException &e) {
		UASSERT(!"Setting not found!");
	}
}

void TestSettings::testFlagDesc()
{
	Settings s;
	FlagDesc flagdesc[] = {
		{ "biomes",  0x01 },
		{ "trees",   0x02 },
		{ "jungles", 0x04 },
		{ "oranges", 0x08 },
		{ "tables",  0x10 },
		{ nullptr,      0 }
	};

	// Enabled: biomes, jungles, oranges (default)
	s.setDefault("test_desc", flagdesc, readFlagString(
		"biomes,notrees,jungles,oranges", flagdesc, nullptr));
	UASSERT(s.getFlagStr("test_desc", flagdesc, nullptr) == (0x01 | 0x04 | 0x08));

	// Enabled: jungles, oranges, tables
	s.set("test_desc", "nobiomes,tables");
	UASSERT(s.getFlagStr("test_desc", flagdesc, nullptr) == (0x04 | 0x08 | 0x10));

	// Enabled: (nothing)
	s.set("test_desc", "nobiomes,nojungles,nooranges,notables");
	UASSERT(s.getFlagStr("test_desc", flagdesc, nullptr) == 0x00);

	// Numeric flag tests (override)
	// Enabled: trees, tables
	s.setDefault("test_flags", flagdesc, 0x02 | 0x10);
	UASSERT(s.getFlagStr("test_flags", flagdesc, nullptr) == (0x02 | 0x10));

	// Enabled: tables
	s.set("test_flags", "16");
	UASSERT(s.getFlagStr("test_flags", flagdesc, nullptr) == 0x10);
}

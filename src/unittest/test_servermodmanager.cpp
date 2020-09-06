/*
Minetest
Copyright (C) 2018 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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
#include <algorithm>
#include "server/mods.h"
#include "settings.h"
#include "test_config.h"

class TestServerModManager : public TestBase
{
public:
	TestServerModManager() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestServerModManager"; }

	void runTests(IGameDef *gamedef);

	void testCreation();
	void testIsConsistent();
	void testUnsatisfiedMods();
	void testGetMods();
	void testGetModsWrongDir();
	void testGetModspec();
	void testGetModNamesWrongDir();
	void testGetModNames();
	void testGetModMediaPathsWrongDir();
	void testGetModMediaPaths();
};

static TestServerModManager g_test_instance;

void TestServerModManager::runTests(IGameDef *gamedef)
{
	const char *saved_env_mt_subgame_path = getenv("MINETEST_SUBGAME_PATH");
#ifdef WIN32
	{
		std::string subgame_path("MINETEST_SUBGAME_PATH=");
		subgame_path.append(TEST_SUBGAME_PATH);
		_putenv(subgame_path.c_str());
	}
#else
	setenv("MINETEST_SUBGAME_PATH", TEST_SUBGAME_PATH, 1);
#endif

	TEST(testCreation);
	TEST(testIsConsistent);
	TEST(testGetModsWrongDir);
	TEST(testUnsatisfiedMods);
	TEST(testGetMods);
	TEST(testGetModspec);
	TEST(testGetModNamesWrongDir);
	TEST(testGetModNames);
	TEST(testGetModMediaPathsWrongDir);
	TEST(testGetModMediaPaths);

#ifdef WIN32
	{
		std::string subgame_path("MINETEST_SUBGAME_PATH=");
		if (saved_env_mt_subgame_path)
			subgame_path.append(saved_env_mt_subgame_path);
		_putenv(subgame_path.c_str());
	}
#else
	if (saved_env_mt_subgame_path)
		setenv("MINETEST_SUBGAME_PATH", saved_env_mt_subgame_path, 1);
	else
		unsetenv("MINETEST_SUBGAME_PATH");
#endif
}

void TestServerModManager::testCreation()
{
	std::string path = std::string(TEST_WORLDDIR) + DIR_DELIM + "world.mt";
	Settings world_config;
	world_config.set("gameid", "devtest");
	UASSERTEQ(bool, world_config.updateConfigFile(path.c_str()), true);
	ServerModManager sm(TEST_WORLDDIR);
}

void TestServerModManager::testGetModsWrongDir()
{
	// Test in non worlddir to ensure no mods are found
	ServerModManager sm(std::string(TEST_WORLDDIR) + DIR_DELIM + "..");
	UASSERTEQ(bool, sm.getMods().empty(), true);
}

void TestServerModManager::testUnsatisfiedMods()
{
	ServerModManager sm(std::string(TEST_WORLDDIR));
	UASSERTEQ(bool, sm.getUnsatisfiedMods().empty(), true);
}

void TestServerModManager::testIsConsistent()
{
	ServerModManager sm(std::string(TEST_WORLDDIR));
	UASSERTEQ(bool, sm.isConsistent(), true);
}

void TestServerModManager::testGetMods()
{
	ServerModManager sm(std::string(TEST_WORLDDIR));
	const auto &mods = sm.getMods();
	UASSERTEQ(bool, mods.empty(), false);

	// Ensure we found basenodes mod (part of devtest)
	bool default_found = false;
	for (const auto &m : mods) {
		if (m.name == "basenodes")
			default_found = true;

		// Verify if paths are not empty
		UASSERTEQ(bool, m.path.empty(), false);
	}

	UASSERTEQ(bool, default_found, true);
}

void TestServerModManager::testGetModspec()
{
	ServerModManager sm(std::string(TEST_WORLDDIR));
	UASSERTEQ(const ModSpec *, sm.getModSpec("wrongmod"), NULL);
	UASSERT(sm.getModSpec("basenodes") != NULL);
}

void TestServerModManager::testGetModNamesWrongDir()
{
	ServerModManager sm(std::string(TEST_WORLDDIR) + DIR_DELIM + "..");
	std::vector<std::string> result;
	sm.getModNames(result);
	UASSERTEQ(bool, result.empty(), true);
}

void TestServerModManager::testGetModNames()
{
	ServerModManager sm(std::string(TEST_WORLDDIR));
	std::vector<std::string> result;
	sm.getModNames(result);
	UASSERTEQ(bool, result.empty(), false);
	UASSERT(std::find(result.begin(), result.end(), "basenodes") != result.end());
}

void TestServerModManager::testGetModMediaPathsWrongDir()
{
	ServerModManager sm(std::string(TEST_WORLDDIR) + DIR_DELIM + "..");
	std::vector<std::string> result;
	sm.getModsMediaPaths(result);
	UASSERTEQ(bool, result.empty(), true);
}

void TestServerModManager::testGetModMediaPaths()
{
	ServerModManager sm(std::string(TEST_WORLDDIR));
	std::vector<std::string> result;
	sm.getModsMediaPaths(result);
	UASSERTEQ(bool, result.empty(), false);
}

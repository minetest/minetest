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
	void testModDependencies();
};

static TestServerModManager g_test_instance;

void TestServerModManager::runTests(IGameDef *gamedef)
{
	const char *saved_env_mt_subgame_path = getenv("MINETEST_SUBGAME_PATH");
	const char *saved_env_mt_mod_path = getenv("MINETEST_MOD_PATH");
#ifdef WIN32
	{
		std::string subgame_path("MINETEST_SUBGAME_PATH=");
		subgame_path.append(TEST_SUBGAME_PATH);
		_putenv(subgame_path.c_str());

		std::string mod_path("MINETEST_MOD_PATH=");
		mod_path.append(TEST_MOD_PATH);
		_putenv(mod_path.c_str());
	}
#else
	setenv("MINETEST_SUBGAME_PATH", TEST_SUBGAME_PATH, 1);
	setenv("MINETEST_MOD_PATH", TEST_MOD_PATH, 1);
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
	TEST(testModDependencies);

#ifdef WIN32
	{
		std::string subgame_path("MINETEST_SUBGAME_PATH=");
		if (saved_env_mt_subgame_path)
			subgame_path.append(saved_env_mt_subgame_path);
		_putenv(subgame_path.c_str());

		std::string mod_path("MINETEST_MOD_PATH=");
		if (saved_env_mt_mod_path)
			mod_path.append(saved_env_mt_mod_path);
		_putenv(mod_path.c_str());
	}
#else
	if (saved_env_mt_subgame_path)
		setenv("MINETEST_SUBGAME_PATH", saved_env_mt_subgame_path, 1);
	else
		unsetenv("MINETEST_SUBGAME_PATH");
	if (saved_env_mt_mod_path)
		setenv("MINETEST_MOD_PATH", saved_env_mt_mod_path, 1);
	else
		unsetenv("MINETEST_MOD_PATH");
#endif
}

void TestServerModManager::testCreation()
{
	std::string path = std::string(TEST_WORLDDIR) + DIR_DELIM + "world.mt";
	Settings world_config;
	world_config.set("gameid", "devtest");
	world_config.set("load_mod_test_mod", "true");
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
	// and test_mod (for testing MINETEST_MOD_PATH).
	bool default_found = false;
	bool test_mod_found = false;
	for (const auto &m : mods) {
		if (m.name == "basenodes")
			default_found = true;
		if (m.name == "test_mod")
			test_mod_found = true;

		// Verify if paths are not empty
		UASSERTEQ(bool, m.path.empty(), false);
	}

	UASSERTEQ(bool, default_found, true);
	UASSERTEQ(bool, test_mod_found, true);
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

void TestServerModManager::testModDependencies()
{
	ModConfiguration mc(TEST_WORLDDIR);

	/* Dependencies
		NAME       HARD-DEPEND         OPT-DEPEND
		default    (none)
		dye        default
		wool       default, dye
		mesecons   default, invalid
		pipeworks  default             mesecons
		technic    pipeworks, mesecons
		circ_1     default             circ_2
		circ_2     circ_3
		circ_3     circ_1

		Expected:
			mesecons:  unsatisfied dependencies (error)
			pipeworks: opt-depend "mesecons" unsatisfied (ok)
			technic:   unsatisfied dependencies (error)
			circ_1:    circular reference. circ_2 not met
	*/
	std::vector<ModSpec> mods;
	mods.emplace_back(ModSpec("default"));
	mods.emplace_back(ModSpec("dye"));
	mods[mods.size() - 1].depends.insert("default");

	mods.emplace_back(ModSpec("wool"));
	mods[mods.size() - 1].depends.insert("default");
	mods[mods.size() - 1].depends.insert("dye");

	mods.emplace_back(ModSpec("mesecons"));
	mods[mods.size() - 1].depends.insert("default");
	mods[mods.size() - 1].depends.insert("invalid"); // unknown mod

	mods.emplace_back(ModSpec("pipeworks"));
	mods[mods.size() - 1].depends.insert("default");
	mods[mods.size() - 1].optdepends.insert("mesecons");

	mods.emplace_back(ModSpec("technic"));
	mods[mods.size() - 1].depends.insert("pipeworks");
	mods[mods.size() - 1].depends.insert("mesecons");

	mods.emplace_back(ModSpec("circ_1"));
	mods[mods.size() - 1].depends.insert("default");
	mods[mods.size() - 1].optdepends.insert("circ_2");

	mods.emplace_back(ModSpec("circ_2"));
	mods[mods.size() - 1].depends.insert("circ_3");
	mods.emplace_back(ModSpec("circ_3"));
	mods[mods.size() - 1].depends.insert("circ_1");

	mc.addMods(mods);
	mc.resolveDependencies();

	const auto &unsatisfied = mc.getUnsatisfiedMods();
	const auto &sorted = mc.getMods();
	UASSERT(unsatisfied.size() == 2); // mesecons, technic
	UASSERT(sorted.size() == mods.size() - 2); // the rest

	auto find_mod = [] (const std::vector<ModSpec> &where, const std::string &name) {
		return std::find_if(where.begin(), where.end(), [&] (const ModSpec &spec) {
			return spec.name == name;
		});
	};
	UASSERT(find_mod(unsatisfied, "mesecons") != unsatisfied.end());
	UASSERT(find_mod(unsatisfied, "technic") != unsatisfied.end());

	// Check whether mods are correctly loaded after each other
	auto pos_default = find_mod(sorted, "default");
	auto pos_dye     = find_mod(sorted, "dye");
	auto pos_wool    = find_mod(sorted, "wool");
	UASSERT(pos_dye > pos_default);
	UASSERT(pos_wool > pos_dye);

	auto pos_circ_1 = find_mod(sorted, "circ_1");
	auto pos_circ_3 = find_mod(sorted, "circ_3");
	UASSERT(pos_circ_1 > pos_default);
	UASSERT(pos_circ_3 > pos_circ_1);
}

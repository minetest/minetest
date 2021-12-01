/*
Minetest
Copyright (C) 2018 bendeutsch, Ben Deutsch <ben@bendeutsch.de>
Copyright (C) 2021 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

// This file is an edited copy of test_authdatabase.cpp

#include "test.h"

#include <algorithm>
#include "database/database-files.h"
#include "database/database-sqlite3.h"
#include "filesys.h"

namespace
{
// Anonymous namespace to create classes that are only
// visible to this file
//
// These are helpers that return a *ModMetadataDatabase and
// allow us to run the same tests on different databases and
// database acquisition strategies.

class ModMetadataDatabaseProvider
{
public:
	virtual ~ModMetadataDatabaseProvider() = default;
	virtual ModMetadataDatabase *getModMetadataDatabase() = 0;
};

class FixedProvider : public ModMetadataDatabaseProvider
{
public:
	FixedProvider(ModMetadataDatabase *mod_meta_db) : mod_meta_db(mod_meta_db){};
	virtual ~FixedProvider(){};
	virtual ModMetadataDatabase *getModMetadataDatabase() { return mod_meta_db; };

private:
	ModMetadataDatabase *mod_meta_db;
};

class FilesProvider : public ModMetadataDatabaseProvider
{
public:
	FilesProvider(const std::string &dir) : dir(dir){};
	virtual ~FilesProvider()
	{
		if (mod_meta_db)
			mod_meta_db->endSave();
		delete mod_meta_db;
	}
	virtual ModMetadataDatabase *getModMetadataDatabase()
	{
		if (mod_meta_db)
			mod_meta_db->endSave();
		delete mod_meta_db;
		mod_meta_db = new ModMetadataDatabaseFiles(dir);
		mod_meta_db->beginSave();
		return mod_meta_db;
	};

private:
	std::string dir;
	ModMetadataDatabase *mod_meta_db = nullptr;
};

class SQLite3Provider : public ModMetadataDatabaseProvider
{
public:
	SQLite3Provider(const std::string &dir) : dir(dir){};
	virtual ~SQLite3Provider()
	{
		if (mod_meta_db)
			mod_meta_db->endSave();
		delete mod_meta_db;
	}
	virtual ModMetadataDatabase *getModMetadataDatabase()
	{
		if (mod_meta_db)
			mod_meta_db->endSave();
		delete mod_meta_db;
		mod_meta_db = new ModMetadataDatabaseSQLite3(dir);
		mod_meta_db->beginSave();
		return mod_meta_db;
	};

private:
	std::string dir;
	ModMetadataDatabase *mod_meta_db = nullptr;
};
}

class TestModMetadataDatabase : public TestBase
{
public:
	TestModMetadataDatabase() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestModMetadataDatabase"; }

	void runTests(IGameDef *gamedef);
	void runTestsForCurrentDB();

	void testRecallFail();
	void testCreate();
	void testRecall();
	void testChange();
	void testRecallChanged();
	void testListMods();
	void testRemove();

private:
	ModMetadataDatabaseProvider *mod_meta_provider;
};

static TestModMetadataDatabase g_test_instance;

void TestModMetadataDatabase::runTests(IGameDef *gamedef)
{
	// fixed directory, for persistence
	thread_local const std::string test_dir = getTestTempDirectory();

	// Each set of tests is run twice for each database type:
	// one where we reuse the same ModMetadataDatabase object (to test local caching),
	// and one where we create a new ModMetadataDatabase object for each call
	// (to test actual persistence).

	rawstream << "-------- Files database (same object)" << std::endl;

	ModMetadataDatabase *mod_meta_db = new ModMetadataDatabaseFiles(test_dir);
	mod_meta_provider = new FixedProvider(mod_meta_db);

	runTestsForCurrentDB();

	delete mod_meta_db;
	delete mod_meta_provider;

	// reset database
	fs::RecursiveDelete(test_dir + DIR_DELIM + "mod_storage");

	rawstream << "-------- Files database (new objects)" << std::endl;

	mod_meta_provider = new FilesProvider(test_dir);

	runTestsForCurrentDB();

	delete mod_meta_provider;

	rawstream << "-------- SQLite3 database (same object)" << std::endl;

	mod_meta_db = new ModMetadataDatabaseSQLite3(test_dir);
	mod_meta_provider = new FixedProvider(mod_meta_db);

	runTestsForCurrentDB();

	delete mod_meta_db;
	delete mod_meta_provider;

	// reset database
	fs::DeleteSingleFileOrEmptyDirectory(test_dir + DIR_DELIM + "mod_storage.sqlite");

	rawstream << "-------- SQLite3 database (new objects)" << std::endl;

	mod_meta_provider = new SQLite3Provider(test_dir);

	runTestsForCurrentDB();

	delete mod_meta_provider;
}

////////////////////////////////////////////////////////////////////////////////

void TestModMetadataDatabase::runTestsForCurrentDB()
{
	TEST(testRecallFail);
	TEST(testCreate);
	TEST(testRecall);
	TEST(testChange);
	TEST(testRecallChanged);
	TEST(testListMods);
	TEST(testRemove);
	TEST(testRecallFail);
}

void TestModMetadataDatabase::testRecallFail()
{
	ModMetadataDatabase *mod_meta_db = mod_meta_provider->getModMetadataDatabase();
	StringMap recalled;
	mod_meta_db->getModEntries("mod1", &recalled);
	UASSERT(recalled.empty());
}

void TestModMetadataDatabase::testCreate()
{
	ModMetadataDatabase *mod_meta_db = mod_meta_provider->getModMetadataDatabase();
	StringMap recalled;
	UASSERT(mod_meta_db->setModEntry("mod1", "key1", "value1"));
}

void TestModMetadataDatabase::testRecall()
{
	ModMetadataDatabase *mod_meta_db = mod_meta_provider->getModMetadataDatabase();
	StringMap recalled;
	mod_meta_db->getModEntries("mod1", &recalled);
	UASSERT(recalled.size() == 1);
	UASSERT(recalled["key1"] == "value1");
}

void TestModMetadataDatabase::testChange()
{
	ModMetadataDatabase *mod_meta_db = mod_meta_provider->getModMetadataDatabase();
	StringMap recalled;
	UASSERT(mod_meta_db->setModEntry("mod1", "key1", "value2"));
}

void TestModMetadataDatabase::testRecallChanged()
{
	ModMetadataDatabase *mod_meta_db = mod_meta_provider->getModMetadataDatabase();
	StringMap recalled;
	mod_meta_db->getModEntries("mod1", &recalled);
	UASSERT(recalled.size() == 1);
	UASSERT(recalled["key1"] == "value2");
}

void TestModMetadataDatabase::testListMods()
{
	ModMetadataDatabase *mod_meta_db = mod_meta_provider->getModMetadataDatabase();
	UASSERT(mod_meta_db->setModEntry("mod2", "key1", "value1"));
	std::vector<std::string> mod_list;
	mod_meta_db->listMods(&mod_list);
	UASSERT(mod_list.size() == 2);
	UASSERT(std::find(mod_list.cbegin(), mod_list.cend(), "mod1") != mod_list.cend());
	UASSERT(std::find(mod_list.cbegin(), mod_list.cend(), "mod2") != mod_list.cend());
}

void TestModMetadataDatabase::testRemove()
{
	ModMetadataDatabase *mod_meta_db = mod_meta_provider->getModMetadataDatabase();
	UASSERT(mod_meta_db->removeModEntry("mod1", "key1"));
}

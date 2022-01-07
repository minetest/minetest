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

#include "irrlichttypes.h"
#include <list>
#include <set>
#include <vector>
#include <string>
#include <map>
#include <json/json.h>
#include <unordered_set>
#include "util/basic_macros.h"
#include "config.h"
#include "metadata.h"

class ModMetadataDatabase;

#define MODNAME_ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyz0123456789_"

struct ModSpec
{
	std::string name;
	std::string author;
	std::string path;
	std::string desc;
	int release = 0;

	// if normal mod:
	std::unordered_set<std::string> depends;
	std::unordered_set<std::string> optdepends;
	std::unordered_set<std::string> unsatisfied_depends;

	bool part_of_modpack = false;
	bool is_modpack = false;

	// For logging purposes
	std::vector<const char *> deprecation_msgs;

	// if modpack:
	std::map<std::string, ModSpec> modpack_content;
	ModSpec(const std::string &name = "", const std::string &path = "") :
			name(name), path(path)
	{
	}
	ModSpec(const std::string &name, const std::string &path, bool part_of_modpack) :
			name(name), path(path), part_of_modpack(part_of_modpack)
	{
	}

	void checkAndLog() const;
};

// Retrieves depends, optdepends, is_modpack and modpack_content
void parseModContents(ModSpec &mod);

std::map<std::string, ModSpec> getModsInPath(
		const std::string &path, bool part_of_modpack = false);

// replaces modpack Modspecs with their content
std::vector<ModSpec> flattenMods(const std::map<std::string, ModSpec> &mods);

// a ModConfiguration is a subset of installed mods, expected to have
// all dependencies fullfilled, so it can be used as a list of mods to
// load when the game starts.
class ModConfiguration
{
public:
	// checks if all dependencies are fullfilled.
	bool isConsistent() const { return m_unsatisfied_mods.empty(); }

	const std::vector<ModSpec> &getMods() const { return m_sorted_mods; }

	const std::vector<ModSpec> &getUnsatisfiedMods() const
	{
		return m_unsatisfied_mods;
	}

	void printUnsatisfiedModsError() const;

protected:
	ModConfiguration(const std::string &worldpath);
	// adds all mods in the given path. used for games, modpacks
	// and world-specific mods (worldmods-folders)
	void addModsInPath(const std::string &path);

	// adds all mods in the set.
	void addMods(const std::vector<ModSpec> &new_mods);

	void addModsFromConfig(const std::string &settings_path,
			const std::set<std::string> &mods);

	void checkConflictsAndDeps();

protected:
	// list of mods sorted such that they can be loaded in the
	// given order with all dependencies being fullfilled. I.e.,
	// every mod in this list has only dependencies on mods which
	// appear earlier in the vector.
	std::vector<ModSpec> m_sorted_mods;

private:
	// move mods from m_unsatisfied_mods to m_sorted_mods
	// in an order that satisfies dependencies
	void resolveDependencies();

	// mods with unmet dependencies. Before dependencies are resolved,
	// this is where all mods are stored. Afterwards this contains
	// only the ones with really unsatisfied dependencies.
	std::vector<ModSpec> m_unsatisfied_mods;

	// set of mod names for which an unresolved name conflict
	// exists. A name conflict happens when two or more mods
	// at the same level have the same name but different paths.
	// Levels (mods in higher levels override mods in lower levels):
	// 1. game mod in modpack; 2. game mod;
	// 3. world mod in modpack; 4. world mod;
	// 5. addon mod in modpack; 6. addon mod.
	std::unordered_set<std::string> m_name_conflicts;

	// Deleted default constructor
	ModConfiguration() = default;
};

#ifndef SERVER
class ClientModConfiguration : public ModConfiguration
{
public:
	ClientModConfiguration(const std::string &path);
};
#endif

class ModMetadata : public Metadata
{
public:
	ModMetadata() = delete;
	ModMetadata(const std::string &mod_name, ModMetadataDatabase *database);
	~ModMetadata() = default;

	virtual void clear();

	const std::string &getModName() const { return m_mod_name; }

	virtual bool setString(const std::string &name, const std::string &var);

private:
	std::string m_mod_name;
	ModMetadataDatabase *m_database;
};

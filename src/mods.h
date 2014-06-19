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

#ifndef MODS_HEADER
#define MODS_HEADER

#include "irrlichttypes.h"
#include <list>
#include <set>
#include <vector>
#include <string>
#include <map>
#include <exception>
#include "json/json.h"
#include "config.h"

#define MODNAME_ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyz0123456789_"

class ModError : public std::exception
{
public:
	ModError(const std::string &s)
	{
		m_s = "ModError: ";
		m_s += s;
	}
	virtual ~ModError() throw()
	{}
	virtual const char * what() const throw()
	{
		return m_s.c_str();
	}
	std::string m_s;
};

struct ModSpec
{
	std::string name;
	std::string path;
	//if normal mod:
	std::set<std::string> depends;
	std::set<std::string> optdepends;
	std::set<std::string> unsatisfied_depends;

	bool part_of_modpack;
	bool is_modpack;
	// if modpack:
	std::map<std::string,ModSpec> modpack_content;
	ModSpec(const std::string &name_="", const std::string &path_=""):
		name(name_),
		path(path_),
		depends(),
		optdepends(),
		unsatisfied_depends(),
		part_of_modpack(false),
		is_modpack(false),
		modpack_content()
	{}
};

// Retrieves depends, optdepends, is_modpack and modpack_content
void parseModContents(ModSpec &mod);

std::map<std::string,ModSpec> getModsInPath(std::string path, bool part_of_modpack = false);

// If failed, returned modspec has name==""
ModSpec findCommonMod(const std::string &modname);

// expands modpack contents, but does not replace them.
std::map<std::string, ModSpec> flattenModTree(std::map<std::string, ModSpec> mods);

// replaces modpack Modspecs with their content
std::vector<ModSpec> flattenMods(std::map<std::string,ModSpec> mods);

// a ModConfiguration is a subset of installed mods, expected to have
// all dependencies fullfilled, so it can be used as a list of mods to
// load when the game starts.
class ModConfiguration
{
public:
	ModConfiguration():
		m_unsatisfied_mods(),
		m_sorted_mods(),
		m_name_conflicts()
	{}


	ModConfiguration(std::string worldpath);

	// checks if all dependencies are fullfilled.
	bool isConsistent()
	{
		return m_unsatisfied_mods.empty();
	}

	std::vector<ModSpec> getMods()
	{
		return m_sorted_mods;
	}

	std::vector<ModSpec> getUnsatisfiedMods()
	{
		return m_unsatisfied_mods;
	}

private:
	// adds all mods in the given path. used for games, modpacks
	// and world-specific mods (worldmods-folders)
	void addModsInPath(std::string path);

	// adds all mods in the set.
	void addMods(std::vector<ModSpec> new_mods);

	// move mods from m_unsatisfied_mods to m_sorted_mods
	// in an order that satisfies dependencies
	void resolveDependencies();

	// mods with unmet dependencies. Before dependencies are resolved,
	// this is where all mods are stored. Afterwards this contains
	// only the ones with really unsatisfied dependencies.
	std::vector<ModSpec> m_unsatisfied_mods;

	// list of mods sorted such that they can be loaded in the
	// given order with all dependencies being fullfilled. I.e.,
	// every mod in this list has only dependencies on mods which
	// appear earlier in the vector.
	std::vector<ModSpec> m_sorted_mods;

	// set of mod names for which an unresolved name conflict
	// exists. A name conflict happens when two or more mods
	// at the same level have the same name but different paths.
	// Levels (mods in higher levels override mods in lower levels):
	// 1. game mod in modpack; 2. game mod;
	// 3. world mod in modpack; 4. world mod;
	// 5. addon mod in modpack; 6. addon mod.
	std::set<std::string> m_name_conflicts;

};

#if USE_CURL
Json::Value getModstoreUrl(std::string url);
#else
inline Json::Value getModstoreUrl(std::string url) {
	return Json::Value();
}
#endif

struct ModLicenseInfo {
	int id;
	std::string shortinfo;
	std::string url;
};

struct ModAuthorInfo {
	int id;
	std::string username;
};

struct ModStoreMod {
	int id;
	std::string title;
	std::string basename;
	ModAuthorInfo author;
	float rating;
	bool valid;
};

struct ModStoreCategoryInfo {
	int id;
	std::string name;
};

struct ModStoreVersionEntry {
	int id;
	std::string date;
	std::string file;
	bool approved;
	//ugly version number
	int mtversion;
};

struct ModStoreTitlePic {
	int id;
	std::string file;
	std::string description;
	int mod;
};

struct ModStoreModDetails {
	/* version_set?? */
	std::vector<ModStoreCategoryInfo> categories;
	ModAuthorInfo author;
	ModLicenseInfo license;
	ModStoreTitlePic titlepic;
	int id;
	std::string title;
	std::string basename;
	std::string description;
	std::string repository;
	float rating;
	std::vector<std::string> depends;
	std::vector<std::string> softdeps;

	std::string download_url;
	std::string screenshot_url;
	std::vector<ModStoreVersionEntry> versions;
	bool valid;
};

#endif

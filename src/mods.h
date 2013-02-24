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
#include <irrList.h>
#include <list>
#include <set>
#include <vector>
#include <string>
#include <map>
#include <exception>

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
	std::set<std::string> unsatisfied_depends;

	bool is_modpack;
	// if modpack:
	std::map<std::string,ModSpec> modpack_content;
	ModSpec(const std::string name_="", const std::string path_="",
			const std::set<std::string> depends_=std::set<std::string>()):
		name(name_),
		path(path_),
		depends(depends_),
		unsatisfied_depends(depends_),
		is_modpack(false),	
		modpack_content()	
	{}
};


std::map<std::string,ModSpec> getModsInPath(std::string path);

// expands modpack contents, but does not replace them.
std::map<std::string, ModSpec> flattenModTree(std::map<std::string, ModSpec> mods);

// replaces modpack Modspecs with their content
std::vector<ModSpec> flattenMods(std::map<std::string,ModSpec> mods);

// removes Mods mentioned in exclude_mod_names
std::vector<ModSpec> filterMods(std::vector<ModSpec> mods,
								std::set<std::string> exclude_mod_names);

// a ModConfiguration is a subset of installed mods, expected to have
// all dependencies fullfilled, so it can be used as a list of mods to
// load when the game starts.
class ModConfiguration
{
public:
	ModConfiguration():
		m_unsatisfied_mods(),
		m_sorted_mods()
	{}

		
	ModConfiguration(std::string worldpath);

	// adds all mods in the given path. used for games, modpacks
	// and world-specific mods (worldmods-folders)
	void addModsInPath(std::string path)
	{
		addMods(flattenMods(getModsInPath(path)));
	}

	// adds all mods in the given path whose name does not appear
	// in the exclude_mods set. 
	void addModsInPathFiltered(std::string path, std::set<std::string> exclude_mods);

	// adds all mods in the set.
	void addMods(std::vector<ModSpec> mods);

	// checks if all dependencies are fullfilled.
	bool isConsistent()
	{
		return m_unsatisfied_mods.empty();
	}

	std::vector<ModSpec> getMods()
	{
		return m_sorted_mods;
	}

	std::list<ModSpec> getUnsatisfiedMods()
	{
		return m_unsatisfied_mods;
	}

private:

	// mods with unmet dependencies. This is a list and not a
	// vector because we want easy removal of elements at every
	// position.
	std::list<ModSpec> m_unsatisfied_mods;

	// list of mods sorted such that they can be loaded in the
	// given order with all dependencies being fullfilled. I.e.,
	// every mod in this list has only dependencies on mods which
	// appear earlier in the vector.
	std::vector<ModSpec> m_sorted_mods;

};


#endif


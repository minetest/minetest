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

#include "mods.h"
#include "filesys.h"
#include "strfnd.h"
#include "log.h"
#include "subgame.h"
#include "settings.h"
#include "strfnd.h"

std::map<std::string, ModSpec> getModsInPath(std::string path)
{
	std::map<std::string, ModSpec> result;
	std::vector<fs::DirListNode> dirlist = fs::GetDirListing(path);
	for(u32 j=0; j<dirlist.size(); j++){
		if(!dirlist[j].dir)
			continue;
		std::string modname = dirlist[j].name;
		// Ignore all directories beginning with a ".", especially
		// VCS directories like ".git" or ".svn"
		if(modname[0] == '.')
			continue;
		std::string modpath = path + DIR_DELIM + modname;

		// Handle modpacks (defined by containing modpack.txt)
		std::ifstream modpack_is((modpath+DIR_DELIM+"modpack.txt").c_str(),
					std::ios_base::binary);
		if(modpack_is.good()) //a modpack, recursively get the mods in it
		{
			modpack_is.close(); // We don't actually need the file
			ModSpec spec(modname,modpath);
			spec.modpack_content = getModsInPath(modpath);
			spec.is_modpack = true;
			result.insert(std::make_pair(modname,spec));
		}
		else // not a modpack, add the modspec
		{
			std::set<std::string> depends;
			std::ifstream is((modpath+DIR_DELIM+"depends.txt").c_str(),
				std::ios_base::binary);
			while(is.good())
			{
				std::string dep;
				std::getline(is, dep);
				dep = trim(dep);	
				if(dep != "")
					depends.insert(dep);
			}

			ModSpec spec(modname, modpath, depends);
			result.insert(std::make_pair(modname,spec));
		}
	}
	return result;
}

std::map<std::string, ModSpec> flattenModTree(std::map<std::string, ModSpec> mods)
{
	std::map<std::string, ModSpec> result;
	for(std::map<std::string,ModSpec>::iterator it = mods.begin();
		it != mods.end(); ++it)
	{
		ModSpec mod = (*it).second;
		if(mod.is_modpack)
		{
			std::map<std::string, ModSpec> content = 
				flattenModTree(mod.modpack_content);
			result.insert(content.begin(),content.end());
			result.insert(std::make_pair(mod.name,mod));
		} 
		else //not a modpack
		{
			result.insert(std::make_pair(mod.name,mod));
		}
	}
	return result;
}

std::vector<ModSpec> flattenMods(std::map<std::string, ModSpec> mods)
{
	std::vector<ModSpec> result;
	for(std::map<std::string,ModSpec>::iterator it = mods.begin();
		it != mods.end(); ++it)
	{
		ModSpec mod = (*it).second;
		if(mod.is_modpack)
		{
			std::vector<ModSpec> content = flattenMods(mod.modpack_content);
			result.reserve(result.size() + content.size());
			result.insert(result.end(),content.begin(),content.end());
			
		} 
		else //not a modpack
		{
			// infostream << "inserting mod " << mod.name << std::endl;
			result.push_back(mod);
		}
	}
	return result;
}

std::vector<ModSpec> filterMods(std::vector<ModSpec> mods,
								std::set<std::string> exclude_mod_names)
{
	std::vector<ModSpec> result;
	for(std::vector<ModSpec>::iterator it = mods.begin();
		it != mods.end(); ++it)
	{
		ModSpec& mod = *it;
		if(exclude_mod_names.count(mod.name) == 0)
			result.push_back(mod);
	}	
	return result;
}

void ModConfiguration::addModsInPathFiltered(std::string path, std::set<std::string> exclude_mods)
{
	addMods(filterMods(flattenMods(getModsInPath(path)),exclude_mods));
}


void ModConfiguration::addMods(std::vector<ModSpec> new_mods)
{
	// Step 1: remove mods in sorted_mods from unmet dependencies
	// of new_mods. new mods without unmet dependencies are
	// temporarily stored in satisfied_mods
	std::vector<ModSpec> satisfied_mods;
	for(std::vector<ModSpec>::iterator it = m_sorted_mods.begin();
		it != m_sorted_mods.end(); ++it)
	{
		ModSpec mod = *it;
		for(std::vector<ModSpec>::iterator it_new = new_mods.begin();
			it_new != new_mods.end(); ++it_new)
		{
			ModSpec& mod_new = *it_new;
			//infostream << "erasing dependency " << mod.name << " from " << mod_new.name << std::endl;
			mod_new.unsatisfied_depends.erase(mod.name);
		}
	}

	// split new mods into satisfied and unsatisfied
	for(std::vector<ModSpec>::iterator it = new_mods.begin();
		it != new_mods.end(); ++it)
	{
		ModSpec mod_new = *it;
		if(mod_new.unsatisfied_depends.empty())
			satisfied_mods.push_back(mod_new);
		else
			m_unsatisfied_mods.push_back(mod_new);
	}

	// Step 2: mods without unmet dependencies can be appended to
	// the sorted list.
	while(!satisfied_mods.empty())
	{
		ModSpec mod = satisfied_mods.back();
		m_sorted_mods.push_back(mod);
		satisfied_mods.pop_back();
		for(std::list<ModSpec>::iterator it = m_unsatisfied_mods.begin();
			it != m_unsatisfied_mods.end(); )
		{
			ModSpec& mod2 = *it;
			mod2.unsatisfied_depends.erase(mod.name);
			if(mod2.unsatisfied_depends.empty())
			{
				satisfied_mods.push_back(mod2);
				it = m_unsatisfied_mods.erase(it);
			}
			else
				++it;
		}	
	}
}

// If failed, returned modspec has name==""
static ModSpec findCommonMod(const std::string &modname)
{
	// Try to find in {$user,$share}/games/common/$modname
	std::vector<std::string> find_paths;
	find_paths.push_back(porting::path_user + DIR_DELIM + "games" +
			DIR_DELIM + "common" + DIR_DELIM + "mods" + DIR_DELIM + modname);
	find_paths.push_back(porting::path_share + DIR_DELIM + "games" +
			DIR_DELIM + "common" + DIR_DELIM + "mods" + DIR_DELIM + modname);
	for(u32 i=0; i<find_paths.size(); i++){
		const std::string &try_path = find_paths[i];
		if(fs::PathExists(try_path))
			return ModSpec(modname, try_path);
	}
	// Failed to find mod
	return ModSpec();
}

ModConfiguration::ModConfiguration(std::string worldpath)
{
	SubgameSpec gamespec = findWorldSubgame(worldpath);

	// Add common mods without dependency handling
	std::vector<std::string> inexistent_common_mods;
	Settings gameconf;
	if(getGameConfig(gamespec.path, gameconf)){
		if(gameconf.exists("common_mods")){
			Strfnd f(gameconf.get("common_mods"));
			while(!f.atend()){
				std::string modname = trim(f.next(","));
				if(modname.empty())
					continue;
				ModSpec spec = findCommonMod(modname);
				if(spec.name.empty())
					inexistent_common_mods.push_back(modname);
				else
					m_sorted_mods.push_back(spec);
			}
		}
	}
	if(!inexistent_common_mods.empty()){
		std::string s = "Required common mods ";
		for(u32 i=0; i<inexistent_common_mods.size(); i++){
			if(i != 0) s += ", ";
			s += std::string("\"") + inexistent_common_mods[i] + "\"";
		}
		s += " could not be found.";
		throw ModError(s);
	}

	// Add all world mods and all game mods
	addModsInPath(worldpath + DIR_DELIM + "worldmods");
	addModsInPath(gamespec.gamemods_path);

	// check world.mt file for mods explicitely declared to be
	// loaded or not by a load_mod_<modname> = ... line.
	std::string worldmt = worldpath+DIR_DELIM+"world.mt";
	Settings worldmt_settings;
	worldmt_settings.readConfigFile(worldmt.c_str());
	std::vector<std::string> names = worldmt_settings.getNames();
	std::set<std::string> exclude_mod_names;
	for(std::vector<std::string>::iterator it = names.begin(); 
		it != names.end(); ++it)
	{	
		std::string name = *it;  
		// for backwards compatibility: exclude only mods which are
		// explicitely excluded. if mod is not mentioned at all, it is
		// enabled. So by default, all installed mods are enabled.
		if (name.compare(0,9,"load_mod_") == 0 &&
			!worldmt_settings.getBool(name))
		{
			exclude_mod_names.insert(name.substr(9));
		}
	}

	for(std::set<std::string>::const_iterator i = gamespec.addon_mods_paths.begin();
			i != gamespec.addon_mods_paths.end(); ++i)
		addModsInPathFiltered((*i),exclude_mod_names);
}

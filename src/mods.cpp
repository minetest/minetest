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

#include <cctype>
#include <fstream>
#include "mods.h"
#include "filesys.h"
#include "log.h"
#include "subgame.h"
#include "settings.h"
#include "convert_json.h"
#include "exceptions.h"
#include "porting.h"

static bool parseDependsLine(std::istream &is,
		std::string &dep, std::set<char> &symbols)
{
	std::getline(is, dep);
	dep = trim(dep);
	symbols.clear();
	size_t pos = dep.size();
	while(pos > 0 && !string_allowed(dep.substr(pos-1, 1), MODNAME_ALLOWED_CHARS)){
		// last character is a symbol, not part of the modname
		symbols.insert(dep[pos-1]);
		--pos;
	}
	dep = trim(dep.substr(0, pos));
	return dep != "";
}

void parseModContents(ModSpec &spec)
{
	// NOTE: this function works in mutual recursion with getModsInPath
	Settings info;
	info.readConfigFile((spec.path+DIR_DELIM+"mod.conf").c_str());

	if (info.exists("name"))
		spec.name = info.get("name");

	spec.depends.clear();
	spec.optdepends.clear();
	spec.is_modpack = false;
	spec.modpack_content.clear();

	// Handle modpacks (defined by containing modpack.txt)
	std::ifstream modpack_is((spec.path+DIR_DELIM+"modpack.txt").c_str());
	if(modpack_is.good()){ //a modpack, recursively get the mods in it
		modpack_is.close(); // We don't actually need the file
		spec.is_modpack = true;
		spec.modpack_content = getModsInPath(spec.path, true);

		// modpacks have no dependencies; they are defined and
		// tracked separately for each mod in the modpack
	}
	else{ // not a modpack, parse the dependencies
		std::ifstream is((spec.path+DIR_DELIM+"depends.txt").c_str());
		while(is.good()){
			std::string dep;
			std::set<char> symbols;
			if(parseDependsLine(is, dep, symbols)){
				if(symbols.count('?') != 0){
					spec.optdepends.insert(dep);
				}
				else{
					spec.depends.insert(dep);
				}
			}
		}
	}
}

std::map<std::string, ModSpec> getModsInPath(std::string path, bool part_of_modpack)
{
	// NOTE: this function works in mutual recursion with parseModContents

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

		ModSpec spec(modname, modpath);
		spec.part_of_modpack = part_of_modpack;
		parseModContents(spec);
		result.insert(std::make_pair(modname, spec));
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
			result.push_back(mod);
		}
	}
	return result;
}

ModConfiguration::ModConfiguration(const std::string &worldpath):
	m_unsatisfied_mods(),
	m_sorted_mods(),
	m_name_conflicts()
{
}

void ModConfiguration::printUnsatisfiedModsError() const
{
	for (std::vector<ModSpec>::const_iterator it = m_unsatisfied_mods.begin();
		it != m_unsatisfied_mods.end(); ++it) {
		ModSpec mod = *it;
		errorstream << "mod \"" << mod.name << "\" has unsatisfied dependencies: ";
		for (UNORDERED_SET<std::string>::iterator dep_it = mod.unsatisfied_depends.begin();
			dep_it != mod.unsatisfied_depends.end(); ++dep_it)
			errorstream << " \"" << *dep_it << "\"";
		errorstream << std::endl;
	}
}

void ModConfiguration::addModsInPath(const std::string &path)
{
	addMods(flattenMods(getModsInPath(path)));
}

void ModConfiguration::addMods(const std::vector<ModSpec> &new_mods)
{
	// Maintain a map of all existing m_unsatisfied_mods.
	// Keys are mod names and values are indices into m_unsatisfied_mods.
	std::map<std::string, u32> existing_mods;
	for(u32 i = 0; i < m_unsatisfied_mods.size(); ++i){
		existing_mods[m_unsatisfied_mods[i].name] = i;
	}

	// Add new mods
	for(int want_from_modpack = 1; want_from_modpack >= 0; --want_from_modpack){
		// First iteration:
		// Add all the mods that come from modpacks
		// Second iteration:
		// Add all the mods that didn't come from modpacks

		std::set<std::string> seen_this_iteration;

		for (std::vector<ModSpec>::const_iterator it = new_mods.begin();
				it != new_mods.end(); ++it) {
			const ModSpec &mod = *it;
			if(mod.part_of_modpack != (bool)want_from_modpack)
				continue;
			if(existing_mods.count(mod.name) == 0){
				// GOOD CASE: completely new mod.
				m_unsatisfied_mods.push_back(mod);
				existing_mods[mod.name] = m_unsatisfied_mods.size() - 1;
			}
			else if(seen_this_iteration.count(mod.name) == 0){
				// BAD CASE: name conflict in different levels.
				u32 oldindex = existing_mods[mod.name];
				const ModSpec &oldmod = m_unsatisfied_mods[oldindex];
				warningstream<<"Mod name conflict detected: \""
					<<mod.name<<"\""<<std::endl
					<<"Will not load: "<<oldmod.path<<std::endl
					<<"Overridden by: "<<mod.path<<std::endl;
				m_unsatisfied_mods[oldindex] = mod;

				// If there was a "VERY BAD CASE" name conflict
				// in an earlier level, ignore it.
				m_name_conflicts.erase(mod.name);
			}
			else{
				// VERY BAD CASE: name conflict in the same level.
				u32 oldindex = existing_mods[mod.name];
				const ModSpec &oldmod = m_unsatisfied_mods[oldindex];
				warningstream<<"Mod name conflict detected: \""
					<<mod.name<<"\""<<std::endl
					<<"Will not load: "<<oldmod.path<<std::endl
					<<"Will not load: "<<mod.path<<std::endl;
				m_unsatisfied_mods[oldindex] = mod;
				m_name_conflicts.insert(mod.name);
			}
			seen_this_iteration.insert(mod.name);
		}
	}
}

void ModConfiguration::checkConflictsAndDeps()
{
	// report on name conflicts
	if (!m_name_conflicts.empty()) {
		std::string s = "Unresolved name conflicts for mods ";
		for (UNORDERED_SET<std::string>::const_iterator it = m_name_conflicts.begin();
			it != m_name_conflicts.end(); ++it) {
			if (it != m_name_conflicts.begin()) s += ", ";
			s += std::string("\"") + (*it) + "\"";
		}
		s += ".";
		throw ModError(s);
	}

	// get the mods in order
	resolveDependencies();
}

void ModConfiguration::resolveDependencies()
{
	// Step 1: Compile a list of the mod names we're working with
	std::set<std::string> modnames;
	for(std::vector<ModSpec>::iterator it = m_unsatisfied_mods.begin();
		it != m_unsatisfied_mods.end(); ++it){
		modnames.insert((*it).name);
	}

	// Step 2: get dependencies (including optional dependencies)
	// of each mod, split mods into satisfied and unsatisfied
	std::list<ModSpec> satisfied;
	std::list<ModSpec> unsatisfied;
	for (std::vector<ModSpec>::iterator it = m_unsatisfied_mods.begin();
			it != m_unsatisfied_mods.end(); ++it) {
		ModSpec mod = *it;
		mod.unsatisfied_depends = mod.depends;
		// check which optional dependencies actually exist
		for (UNORDERED_SET<std::string>::iterator it_optdep = mod.optdepends.begin();
				it_optdep != mod.optdepends.end(); ++it_optdep) {
			std::string optdep = *it_optdep;
			if (modnames.count(optdep) != 0)
				mod.unsatisfied_depends.insert(optdep);
		}
		// if a mod has no depends it is initially satisfied
		if (mod.unsatisfied_depends.empty())
			satisfied.push_back(mod);
		else
			unsatisfied.push_back(mod);
	}

	// Step 3: mods without unmet dependencies can be appended to
	// the sorted list.
	while(!satisfied.empty()){
		ModSpec mod = satisfied.back();
		m_sorted_mods.push_back(mod);
		satisfied.pop_back();
		for(std::list<ModSpec>::iterator it = unsatisfied.begin();
				it != unsatisfied.end(); ){
			ModSpec& mod2 = *it;
			mod2.unsatisfied_depends.erase(mod.name);
			if(mod2.unsatisfied_depends.empty()){
				satisfied.push_back(mod2);
				it = unsatisfied.erase(it);
			}
			else{
				++it;
			}
		}
	}

	// Step 4: write back list of unsatisfied mods
	m_unsatisfied_mods.assign(unsatisfied.begin(), unsatisfied.end());
}

ServerModConfiguration::ServerModConfiguration(const std::string &worldpath):
	ModConfiguration(worldpath)
{
	SubgameSpec gamespec = findWorldSubgame(worldpath);

	// Add all game mods and all world mods
	addModsInPath(gamespec.gamemods_path);
	addModsInPath(worldpath + DIR_DELIM + "worldmods");

	// check world.mt file for mods explicitely declared to be
	// loaded or not by a load_mod_<modname> = ... line.
	std::string worldmt = worldpath+DIR_DELIM+"world.mt";
	Settings worldmt_settings;
	worldmt_settings.readConfigFile(worldmt.c_str());
	std::vector<std::string> names = worldmt_settings.getNames();
	std::set<std::string> include_mod_names;
	for (std::vector<std::string>::const_iterator it = names.begin();
		it != names.end(); ++it) {
		std::string name = *it;
		// for backwards compatibility: exclude only mods which are
		// explicitely excluded. if mod is not mentioned at all, it is
		// enabled. So by default, all installed mods are enabled.
		if (name.compare(0,9,"load_mod_") == 0 &&
			worldmt_settings.getBool(name)) {
			include_mod_names.insert(name.substr(9));
		}
	}

	// Collect all mods that are also in include_mod_names
	std::vector<ModSpec> addon_mods;
	for (std::set<std::string>::const_iterator it_path = gamespec.addon_mods_paths.begin();
		it_path != gamespec.addon_mods_paths.end(); ++it_path) {
		std::vector<ModSpec> addon_mods_in_path = flattenMods(getModsInPath(*it_path));
		for (std::vector<ModSpec>::const_iterator it = addon_mods_in_path.begin();
			it != addon_mods_in_path.end(); ++it) {
			const ModSpec& mod = *it;
			if (include_mod_names.count(mod.name) != 0)
				addon_mods.push_back(mod);
			else
				worldmt_settings.setBool("load_mod_" + mod.name, false);
		}
	}
	worldmt_settings.updateConfigFile(worldmt.c_str());

	addMods(addon_mods);

	checkConflictsAndDeps();
}

#ifndef SERVER
ClientModConfiguration::ClientModConfiguration(const std::string &path):
	ModConfiguration(path)
{
	addModsInPath(path);
	addModsInPath(porting::path_user + DIR_DELIM + "clientmods");
	checkConflictsAndDeps();
}
#endif

#if USE_CURL
Json::Value getModstoreUrl(const std::string &url)
{
	std::vector<std::string> extra_headers;

	bool special_http_header = true;

	try {
		special_http_header = g_settings->getBool("modstore_disable_special_http_header");
	} catch (SettingNotFoundException) {}

	if (special_http_header) {
		extra_headers.push_back("Accept: application/vnd.minetest.mmdb-v1+json");
	}
	return fetchJsonValue(url, special_http_header ? &extra_headers : NULL);
}

#endif

ModMetadata::ModMetadata(const std::string &mod_name):
	m_mod_name(mod_name),
	m_modified(false)
{
	m_stringvars.clear();
}

void ModMetadata::clear()
{
	Metadata::clear();
	m_modified = true;
}

bool ModMetadata::save(const std::string &root_path)
{
	Json::Value json;
	for (StringMap::const_iterator it = m_stringvars.begin();
			it != m_stringvars.end(); ++it) {
		json[it->first] = it->second;
	}

	if (!fs::PathExists(root_path)) {
		if (!fs::CreateAllDirs(root_path)) {
			errorstream << "ModMetadata[" << m_mod_name << "]: Unable to save. '"
				<< root_path << "' tree cannot be created." << std::endl;
			return false;
		}
	} else if (!fs::IsDir(root_path)) {
		errorstream << "ModMetadata[" << m_mod_name << "]: Unable to save. '"
			<< root_path << "' is not a directory." << std::endl;
		return false;
	}

	bool w_ok = fs::safeWriteToFile(root_path + DIR_DELIM + m_mod_name,
		Json::FastWriter().write(json));

	if (w_ok) {
		m_modified = false;
	} else {
		errorstream << "ModMetadata[" << m_mod_name << "]: failed write file." << std::endl;
	}
	return w_ok;
}

bool ModMetadata::load(const std::string &root_path)
{
	m_stringvars.clear();

	std::ifstream is((root_path + DIR_DELIM + m_mod_name).c_str(), std::ios_base::binary);
	if (!is.good()) {
		return false;
	}

	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(is, root)) {
		errorstream << "ModMetadata[" << m_mod_name << "]: failed read data "
			"(Json decoding failure)." << std::endl;
		return false;
	}

	const Json::Value::Members attr_list = root.getMemberNames();
	for (Json::Value::Members::const_iterator it = attr_list.begin();
			it != attr_list.end(); ++it) {
		Json::Value attr_value = root[*it];
		m_stringvars[*it] = attr_value.asString();
	}

	return true;
}

bool ModMetadata::setString(const std::string &name, const std::string &var)
{
	m_modified = Metadata::setString(name, var);
	return m_modified;
}

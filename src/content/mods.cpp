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
#include <json/json.h>
#include <algorithm>
#include "content/mods.h"
#include "database/database.h"
#include "filesys.h"
#include "log.h"
#include "content/subgames.h"
#include "settings.h"
#include "porting.h"
#include "convert_json.h"
#include "script/common/c_internal.h"

void ModSpec::checkAndLog() const
{
	if (!string_allowed(name, MODNAME_ALLOWED_CHARS)) {
		throw ModError("Error loading mod \"" + name +
			"\": Mod name does not follow naming conventions: "
				"Only characters [a-z0-9_] are allowed.");
	}

	// Log deprecation messages
	auto handling_mode = get_deprecated_handling_mode();
	if (!deprecation_msgs.empty() && handling_mode != DeprecatedHandlingMode::Ignore) {
		std::ostringstream os;
		os << "Mod " << name << " at " << path << ":" << std::endl;
		for (auto msg : deprecation_msgs)
			os << "\t" << msg << std::endl;

		if (handling_mode == DeprecatedHandlingMode::Error)
			throw ModError(os.str());
		else
			warningstream << os.str();
	}
}

bool parseDependsString(std::string &dep, std::unordered_set<char> &symbols)
{
	dep = trim(dep);
	symbols.clear();
	size_t pos = dep.size();
	while (pos > 0 &&
			!string_allowed(dep.substr(pos - 1, 1), MODNAME_ALLOWED_CHARS)) {
		// last character is a symbol, not part of the modname
		symbols.insert(dep[pos - 1]);
		--pos;
	}
	dep = trim(dep.substr(0, pos));
	return !dep.empty();
}

void parseModContents(ModSpec &spec)
{
	// NOTE: this function works in mutual recursion with getModsInPath

	spec.depends.clear();
	spec.optdepends.clear();
	spec.is_modpack = false;
	spec.modpack_content.clear();

	// Handle modpacks (defined by containing modpack.txt)
	std::ifstream modpack_is((spec.path + DIR_DELIM + "modpack.txt").c_str());
	std::ifstream modpack2_is((spec.path + DIR_DELIM + "modpack.conf").c_str());
	if (modpack_is.good() || modpack2_is.good()) {
		if (modpack_is.good())
			modpack_is.close();

		if (modpack2_is.good())
			modpack2_is.close();

		spec.is_modpack = true;
		spec.modpack_content = getModsInPath(spec.path, spec.virtual_path, true);

	} else {
		Settings info;
		info.readConfigFile((spec.path + DIR_DELIM + "mod.conf").c_str());

		if (info.exists("name"))
			spec.name = info.get("name");
		else
			spec.deprecation_msgs.push_back("Mods not having a mod.conf file with the name is deprecated.");

		if (info.exists("author"))
			spec.author = info.get("author");

		if (info.exists("release"))
			spec.release = info.getS32("release");

		// Attempt to load dependencies from mod.conf
		bool mod_conf_has_depends = false;
		if (info.exists("depends")) {
			mod_conf_has_depends = true;
			std::string dep = info.get("depends");
			// clang-format off
			dep.erase(std::remove_if(dep.begin(), dep.end(),
					static_cast<int (*)(int)>(&std::isspace)), dep.end());
			// clang-format on
			for (const auto &dependency : str_split(dep, ',')) {
				spec.depends.insert(dependency);
			}
		}

		if (info.exists("optional_depends")) {
			mod_conf_has_depends = true;
			std::string dep = info.get("optional_depends");
			// clang-format off
			dep.erase(std::remove_if(dep.begin(), dep.end(),
					static_cast<int (*)(int)>(&std::isspace)), dep.end());
			// clang-format on
			for (const auto &dependency : str_split(dep, ',')) {
				spec.optdepends.insert(dependency);
			}
		}

		// Fallback to depends.txt
		if (!mod_conf_has_depends) {
			std::vector<std::string> dependencies;

			std::ifstream is((spec.path + DIR_DELIM + "depends.txt").c_str());

			if (is.good())
				spec.deprecation_msgs.push_back("depends.txt is deprecated, please use mod.conf instead.");

			while (is.good()) {
				std::string dep;
				std::getline(is, dep);
				dependencies.push_back(dep);
			}

			for (auto &dependency : dependencies) {
				std::unordered_set<char> symbols;
				if (parseDependsString(dependency, symbols)) {
					if (symbols.count('?') != 0) {
						spec.optdepends.insert(dependency);
					} else {
						spec.depends.insert(dependency);
					}
				}
			}
		}

		if (info.exists("description"))
			spec.desc = info.get("description");
		else if (fs::ReadFile(spec.path + DIR_DELIM + "description.txt", spec.desc))
			spec.deprecation_msgs.push_back("description.txt is deprecated, please use mod.conf instead.");
	}
}

std::map<std::string, ModSpec> getModsInPath(
		const std::string &path, const std::string &virtual_path, bool part_of_modpack)
{
	// NOTE: this function works in mutual recursion with parseModContents

	std::map<std::string, ModSpec> result;
	std::vector<fs::DirListNode> dirlist = fs::GetDirListing(path);
	std::string mod_path;
	std::string mod_virtual_path;

	for (const fs::DirListNode &dln : dirlist) {
		if (!dln.dir)
			continue;

		const std::string &modname = dln.name;
		// Ignore all directories beginning with a ".", especially
		// VCS directories like ".git" or ".svn"
		if (modname[0] == '.')
			continue;

		mod_path.clear();
		mod_path.append(path).append(DIR_DELIM).append(modname);

		mod_virtual_path.clear();
		// Intentionally uses / to keep paths same on different platforms
		mod_virtual_path.append(virtual_path).append("/").append(modname);

		ModSpec spec(modname, mod_path, part_of_modpack, mod_virtual_path);
		parseModContents(spec);
		result.insert(std::make_pair(modname, spec));
	}
	return result;
}

std::vector<ModSpec> flattenMods(const std::map<std::string, ModSpec> &mods)
{
	std::vector<ModSpec> result;
	for (const auto &it : mods) {
		const ModSpec &mod = it.second;
		if (mod.is_modpack) {
			std::vector<ModSpec> content = flattenMods(mod.modpack_content);
			result.reserve(result.size() + content.size());
			result.insert(result.end(), content.begin(), content.end());

		} else // not a modpack
		{
			result.push_back(mod);
		}
	}
	return result;
}

// Look ma, an inline class!
class ModsResolver {
public:
	// the mods here is a copy of the ModConfiguration unsatisifed mods
	ModsResolver(std::vector<ModSpec> mods) {
	  for (ModSpec mod : mods) {
	    modsByName[mod.name] = mod;
	  }
	};

	// the main entry point to start resolving the graph
	void run();

	// all the valid modspecs already sorted neatly for loading
	std::vector<ModSpec> sortedMods;
	// mods that could not be satisfied
	std::list<ModSpec> unsatisfiedMods;
	// mods that are valid, but some or all of its optional dependencies
	// could not be resolved
	std::list<ModSpec> modsWithUnsatisfiedOptionals;
private:
	// resolve the mod by name, not spec, since a mod may not exist
	void resolveMod(std::string &modname);

	// mods that have already been resolved
	std::list<std::string> resolvedMods;
	// mods that have been seen (not neccessarily resolved)
	std::set<std::string> seenMods;
	// inner mapping
	std::map<std::string, ModSpec> modsByName;
};

void ModsResolver::run()
{
	// Step 1: Resolve a mod's dependencies
	for (auto mod_iter = modsByName.begin(); mod_iter != modsByName.end();) {
		ModSpec &mod = (*mod_iter).second;
		resolveMod(mod.name);
		++mod_iter;
	}

	// Step 2: Clear any satisfied dep
	for (auto mod_iter = modsByName.begin(); mod_iter != modsByName.end();) {
		ModSpec &mod = (*mod_iter).second;

		// Setup unsatisfied mandatory lists
		mod.unsatisfied_depends = mod.depends;

		// Setup the optional list based on what was actually seen
		for (std::string modname : mod.optdepends) {
			if (seenMods.count(modname) > 0) {
				mod.unsatisfied_optdepends.insert(modname);
			}
		}

		// resolvedMods only contain mods that are known to exist
		// This loop removes the mod from the unsatisfied deps of the
		// target mod
		for (std::string modname : resolvedMods) {
			mod.unsatisfied_depends.erase(modname);
			mod.unsatisfied_optdepends.erase(modname);
		}

		// increment iterator
		++mod_iter;
	}

	// Step 3: Check that the mod's deps have been properly satisfied
	for (std::string modname : resolvedMods) {
		auto mod_iter = modsByName.find(modname);
		// sanity check
		if (mod_iter != modsByName.end()) {
			ModSpec &mod = (*mod_iter).second;

			// Check if the mod has its mandatory mods satisfied
			if (mod.unsatisfied_depends.empty()) {
				// if it has, it can be pushed unto the sortedMods list
				sortedMods.push_back(mod);
				// now to give the user some feedback, check if the mod also
				// satisfied it's optional dependencies
				if (!mod.unsatisfied_optdepends.empty()) {
					// if not, then we push it unto the optionals list
					modsWithUnsatisfiedOptionals.push_back(mod);
				}
			} else {
				// the mod failed one or more mandatory dependencies
				unsatisfiedMods.push_back(mod);
			}
		}
	}
}

void ModsResolver::resolveMod(std::string &modname)
{
	// behold recursion - the funny thing about this, it sorts itself
	// based on dependencies, mods that require other mods will naturally
	// try to load them before itself, ultimately performing the best
	// sorting possible.
	// And with recursion the dependencies of the dependencies can be
	// resolved just the same
	//
	// Now the only caveat I can think of is punching a whole in the stack with
	// a deep dependency graph.
	//
	// Circular dependencies are also ignored for the most part due to the
	// presence of the seenMods set, if we've seen a mod before,
	// no need to resolve it again, even if it's being resolved further up.
	//
	// If circular dependency checks are required, then an additional set
	// can be added to the resolver class that tracks the mods that are
	// currently being resolved.
	if (seenMods.count(modname) == 0) {
		// immediately mark the mod as seen, to avoid circular deps later on
		seenMods.insert(modname);

		auto mod_iter = modsByName.find(modname);
		// Chances are the mod may or may not exist
		if (mod_iter != modsByName.end()) {
			ModSpec &mod = (*mod_iter).second;

			// resolve all hard dependencies
			for (std::string depname : mod.depends) {
				resolveMod(depname);
			}

			// resolve all soft dependencies
			for (std::string depname : mod.optdepends) {
				resolveMod(depname);
			}

			// the mod is known to be resolved (as best as it could)
			resolvedMods.push_back(modname);
		}
	}
}

ModConfiguration::ModConfiguration(const std::string &worldpath)
{
}

void ModConfiguration::printUnsatisfiedModsError() const
{
	for (const ModSpec &mod : m_unsatisfied_mods) {
		errorstream << "mod \"" << mod.name
			    << "\" has unsatisfied dependencies: ";
		for (const std::string &unsatisfied_depend : mod.unsatisfied_depends)
			errorstream << " \"" << unsatisfied_depend << "\"";
		errorstream << std::endl;
	}
}

void ModConfiguration::printModsWithUnsatisfiedOptionalsWarning() const
{
	for (const ModSpec &mod : m_mods_with_unsatisfied_optionals) {
		warningstream << "mod \"" << mod.name
			    << "\" has unsatisfied dependencies (optional): ";
		for (const std::string &unsatisfied_depend : mod.unsatisfied_optdepends)
			warningstream << " \"" << unsatisfied_depend << "\"";
		warningstream << std::endl;
	}
}

void ModConfiguration::addModsInPath(const std::string &path, const std::string &virtual_path)
{
	addMods(flattenMods(getModsInPath(path, virtual_path)));
}

void ModConfiguration::addMods(const std::vector<ModSpec> &new_mods)
{
	// Maintain a map of all existing m_unsatisfied_mods.
	// Keys are mod names and values are indices into m_unsatisfied_mods.
	std::map<std::string, u32> existing_mods;
	for (u32 i = 0; i < m_unsatisfied_mods.size(); ++i) {
		existing_mods[m_unsatisfied_mods[i].name] = i;
	}

	// Add new mods
	for (int want_from_modpack = 1; want_from_modpack >= 0; --want_from_modpack) {
		// First iteration:
		// Add all the mods that come from modpacks
		// Second iteration:
		// Add all the mods that didn't come from modpacks

		std::set<std::string> seen_this_iteration;

		for (const ModSpec &mod : new_mods) {
			if (mod.part_of_modpack != (bool)want_from_modpack)
				continue;

			if (existing_mods.count(mod.name) == 0) {
				// GOOD CASE: completely new mod.
				m_unsatisfied_mods.push_back(mod);
				existing_mods[mod.name] = m_unsatisfied_mods.size() - 1;
			} else if (seen_this_iteration.count(mod.name) == 0) {
				// BAD CASE: name conflict in different levels.
				u32 oldindex = existing_mods[mod.name];
				const ModSpec &oldmod = m_unsatisfied_mods[oldindex];
				warningstream << "Mod name conflict detected: \""
					      << mod.name << "\"" << std::endl
					      << "Will not load: " << oldmod.path
					      << std::endl
					      << "Overridden by: " << mod.path
					      << std::endl;
				m_unsatisfied_mods[oldindex] = mod;

				// If there was a "VERY BAD CASE" name conflict
				// in an earlier level, ignore it.
				m_name_conflicts.erase(mod.name);
			} else {
				// VERY BAD CASE: name conflict in the same level.
				u32 oldindex = existing_mods[mod.name];
				const ModSpec &oldmod = m_unsatisfied_mods[oldindex];
				warningstream << "Mod name conflict detected: \""
					      << mod.name << "\"" << std::endl
					      << "Will not load: " << oldmod.path
					      << std::endl
					      << "Will not load: " << mod.path
					      << std::endl;
				m_unsatisfied_mods[oldindex] = mod;
				m_name_conflicts.insert(mod.name);
			}

			seen_this_iteration.insert(mod.name);
		}
	}
}

void ModConfiguration::addModsFromConfig(
		const std::string &settings_path,
		const std::unordered_map<std::string, std::string> &modPaths)
{
	Settings conf;
	std::unordered_map<std::string, std::string> load_mod_names;

	conf.readConfigFile(settings_path.c_str());
	std::vector<std::string> names = conf.getNames();
	for (const std::string &name : names) {
		const auto &value = conf.get(name);
		if (name.compare(0, 9, "load_mod_") == 0 && value != "false" &&
				value != "nil")
			load_mod_names[name.substr(9)] = value;
	}

	std::vector<ModSpec> addon_mods;
	std::unordered_map<std::string, std::vector<std::string>> candidates;

	for (const auto &modPath : modPaths) {
		std::vector<ModSpec> addon_mods_in_path = flattenMods(getModsInPath(modPath.second, modPath.first));
		for (std::vector<ModSpec>::const_iterator it = addon_mods_in_path.begin();
				it != addon_mods_in_path.end(); ++it) {
			const ModSpec &mod = *it;
			const auto &pair = load_mod_names.find(mod.name);
			if (pair != load_mod_names.end()) {
				if (is_yes(pair->second) || pair->second == mod.virtual_path) {
					addon_mods.push_back(mod);
				} else {
					candidates[pair->first].emplace_back(mod.virtual_path);
				}
			} else {
				conf.setBool("load_mod_" + mod.name, false);
			}
		}
	}
	conf.updateConfigFile(settings_path.c_str());

	addMods(addon_mods);
	checkConflictsAndDeps();

	// complain about mods declared to be loaded, but not found
	for (const ModSpec &addon_mod : addon_mods)
		load_mod_names.erase(addon_mod.name);

	std::vector<ModSpec> unsatisfiedMods = getUnsatisfiedMods();

	for (const ModSpec &unsatisfiedMod : unsatisfiedMods)
		load_mod_names.erase(unsatisfiedMod.name);

	if (!load_mod_names.empty()) {
		errorstream << "The following mods could not be found:";
		for (const auto &pair : load_mod_names)
			errorstream << " \"" << pair.first << "\"";
		errorstream << std::endl;

		for (const auto &pair : load_mod_names) {
			const auto &candidate = candidates.find(pair.first);
			if (candidate != candidates.end()) {
				errorstream << "Unable to load " << pair.first << " as the specified path "
					<< pair.second << " could not be found. "
					<< "However, it is available in the following locations:"
					<< std::endl;
				for (const auto &path : candidate->second) {
					errorstream << " - " << path << std::endl;
				}
			}
		}
	}
}

void ModConfiguration::checkConflictsAndDeps()
{
	// report on name conflicts
	if (!m_name_conflicts.empty()) {
		std::string s = "Unresolved name conflicts for mods ";
		for (std::unordered_set<std::string>::const_iterator it =
						m_name_conflicts.begin();
				it != m_name_conflicts.end(); ++it) {
			if (it != m_name_conflicts.begin())
				s += ", ";
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
	ModsResolver resolver(m_unsatisfied_mods);

	resolver.run();

	m_sorted_mods.assign(resolver.sortedMods.begin(), resolver.sortedMods.end());
	m_unsatisfied_mods.assign(resolver.unsatisfiedMods.begin(), resolver.unsatisfiedMods.end());
	m_mods_with_unsatisfied_optionals.assign(resolver.modsWithUnsatisfiedOptionals.begin(), resolver.modsWithUnsatisfiedOptionals.end());
}

#ifndef SERVER
ClientModConfiguration::ClientModConfiguration(const std::string &path) :
		ModConfiguration(path)
{
	std::unordered_map<std::string, std::string> paths;
	std::string path_user = porting::path_user + DIR_DELIM + "clientmods";
	if (path != path_user) {
		paths["share"] = path;
	}
	paths["mods"] = path_user;

	std::string settings_path = path_user + DIR_DELIM + "mods.conf";
	addModsFromConfig(settings_path, paths);
}
#endif

ModMetadata::ModMetadata(const std::string &mod_name, ModMetadataDatabase *database):
	m_mod_name(mod_name), m_database(database)
{
	m_database->getModEntries(m_mod_name, &m_stringvars);
}

void ModMetadata::clear()
{
	for (const auto &pair : m_stringvars) {
		m_database->removeModEntry(m_mod_name, pair.first);
	}
	Metadata::clear();
}

bool ModMetadata::setString(const std::string &name, const std::string &var)
{
	if (Metadata::setString(name, var)) {
		if (var.empty()) {
			m_database->removeModEntry(m_mod_name, name);
		} else {
			m_database->setModEntry(m_mod_name, name, var);
		}
		return true;
	}
	return false;
}

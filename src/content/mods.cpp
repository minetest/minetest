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
#include "filesys.h"
#include "log.h"
#include "content/subgames.h"
#include "settings.h"
#include "porting.h"
#include "convert_json.h"

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
	Settings info;
	info.readConfigFile((spec.path + DIR_DELIM + "mod.conf").c_str());

	if (info.exists("name"))
		spec.name = info.get("name");

	if (info.exists("author"))
		spec.author = info.get("author");

	if (info.exists("release"))
		spec.release = info.getS32("release");

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
		spec.modpack_content = getModsInPath(spec.path, true);

	} else {
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

		if (info.exists("description")) {
			spec.desc = info.get("description");
		} else {
			std::ifstream is((spec.path + DIR_DELIM + "description.txt")
							 .c_str());
			spec.desc = std::string((std::istreambuf_iterator<char>(is)),
					std::istreambuf_iterator<char>());
		}
	}
}

std::map<std::string, ModSpec> getModsInPath(
		const std::string &path, bool part_of_modpack)
{
	// NOTE: this function works in mutual recursion with parseModContents

	std::map<std::string, ModSpec> result;
	std::vector<fs::DirListNode> dirlist = fs::GetDirListing(path);
	std::string modpath;

	for (const fs::DirListNode &dln : dirlist) {
		if (!dln.dir)
			continue;

		const std::string &modname = dln.name;
		// Ignore all directories beginning with a ".", especially
		// VCS directories like ".git" or ".svn"
		if (modname[0] == '.')
			continue;

		modpath.clear();
		modpath.append(path).append(DIR_DELIM).append(modname);

		ModSpec spec(modname, modpath, part_of_modpack);
		parseModContents(spec);
		result.insert(std::make_pair(modname, spec));
	}
	return result;
}

std::vector<ModSpec> flattenMods(std::map<std::string, ModSpec> mods)
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

void ModConfiguration::addModsInPath(const std::string &path)
{
	addMods(flattenMods(getModsInPath(path)));
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
		const std::string &settings_path, const std::set<std::string> &mods)
{
	Settings conf;
	std::set<std::string> load_mod_names;

	conf.readConfigFile(settings_path.c_str());
	std::vector<std::string> names = conf.getNames();
	for (const std::string &name : names) {
		if (name.compare(0, 9, "load_mod_") == 0 && conf.get(name) != "false" &&
				conf.get(name) != "nil")
			load_mod_names.insert(name.substr(9));
	}

	std::vector<ModSpec> addon_mods;
	for (const std::string &i : mods) {
		std::vector<ModSpec> addon_mods_in_path = flattenMods(getModsInPath(i));
		for (std::vector<ModSpec>::const_iterator it = addon_mods_in_path.begin();
				it != addon_mods_in_path.end(); ++it) {
			const ModSpec &mod = *it;
			if (load_mod_names.count(mod.name) != 0)
				addon_mods.push_back(mod);
			else
				conf.setBool("load_mod_" + mod.name, false);
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
		for (const std::string &mod : load_mod_names)
			errorstream << " \"" << mod << "\"";
		errorstream << std::endl;
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

struct PrioritySortedMod
{
	PrioritySortedMod(ModSpec *mod) : spec(mod) {}

	static bool sorter(const PrioritySortedMod *a, const PrioritySortedMod *b)
	{
		// Return true means: move 'a' towards begin() in std::sort
		return a->deps_chain.size() > b->deps_chain.size();
	}

	void addDependingMod(const std::string &modname,
			std::unordered_map<std::string, PrioritySortedMod> &mods_by_name);

	std::unordered_set<std::string> deps_chain;
	ModSpec *spec;
};

void PrioritySortedMod::addDependingMod(const std::string &modname,
		std::unordered_map<std::string, PrioritySortedMod> &mods_by_name)
{
	if (deps_chain.find(modname) != deps_chain.end()) {
		// clang-format off
		if (modname == spec->name)
			warningstream << "PrioritySortedMod::addDependingMod(): Detected" <<
					" circular dependencies in mod '" << modname << "'" << std::endl;
		// clang-format on
		return;
	}

	deps_chain.emplace(modname);

	for (const std::string &dependency : spec->depends) {
		auto &mod = mods_by_name.find(dependency)->second;
		mod.addDependingMod(modname, mods_by_name);
	}

	for (const std::string &dependency : spec->optdepends) {
		auto &mod = mods_by_name.find(dependency)->second;
		mod.addDependingMod(modname, mods_by_name);
	}
}

void ModConfiguration::resolveDependencies()
{
	std::unordered_map<std::string, PrioritySortedMod> mods_by_name;
	for (ModSpec &mod : m_unsatisfied_mods) {
		// Reduce memory copy time by passing 'mod' as pointer
		// 'm_unsatisfied_mods' owns the value!
		mods_by_name.emplace(std::pair<std::string, PrioritySortedMod>(
				mod.name, &mod));
	}

	std::vector<ModSpec> unsatisfied;

	// Step 1: Cleanup missing mods
	size_t old_list_size;
	do {
		old_list_size = mods_by_name.size();

		for (auto mod_it = mods_by_name.begin(); mod_it != mods_by_name.end();) {
			ModSpec &mod = *mod_it->second.spec;

			for (auto it = mod.optdepends.begin();
					it != mod.optdepends.end();) {
				// Remove what cannot be satisfied anyway
				if (mods_by_name.find(*it) == mods_by_name.end())
					mod.optdepends.erase(it++);
				else
					++it;
			}

			bool ok = true;
			for (const std::string &dependency : mod.depends) {
				if (mods_by_name.find(dependency) == mods_by_name.end()) {
					mod.unsatisfied_depends.emplace(dependency);
					ok = false;
				}
			}

			if (ok) {
				++mod_it;
			} else {
				unsatisfied.push_back(mod);
				mods_by_name.erase(mod_it++);
			}
		}
	} while (mods_by_name.size() != old_list_size);

	// Step 2: Recursively add the current mod to the dependency lists
	// of its depending mods.
	for (auto &mod : mods_by_name)
		mod.second.addDependingMod(mod.first, mods_by_name);

	// Step 3: Sort it according to the new priorities
	// The mod with the most hard dependencies must be loaded first
	std::vector<const PrioritySortedMod *> mods;
	for (const auto &mod : mods_by_name)
		mods.push_back(&mod.second);

	std::sort(mods.begin(), mods.end(), PrioritySortedMod::sorter);
	for (auto &mod : mods) {
		m_sorted_mods.push_back(*mod->spec);
		// clang-format off
		verbosestream << "Sorted mod: " << mod->spec->name <<
				"   hard_deps=" << mod->deps_chain.size() << ", soft_deps=" <<
				mod->deps_chain.size() << std::endl;
		// clang-format on
	}

	// Step 4: Update the list of unsatisfied mods
	mods.clear();
	mods_by_name.clear();
	// Don't even try to dereference ModSpec now
	m_unsatisfied_mods.assign(unsatisfied.begin(), unsatisfied.end());
}

#ifndef SERVER
ClientModConfiguration::ClientModConfiguration(const std::string &path) :
		ModConfiguration(path)
{
	std::set<std::string> paths;
	std::string path_user = porting::path_user + DIR_DELIM + "clientmods";
	paths.insert(path);
	paths.insert(path_user);

	std::string settings_path = path_user + DIR_DELIM + "mods.conf";
	addModsFromConfig(settings_path, paths);
}
#endif

ModMetadata::ModMetadata(const std::string &mod_name) : m_mod_name(mod_name)
{
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
			errorstream << "ModMetadata[" << m_mod_name
				    << "]: Unable to save. '" << root_path
				    << "' tree cannot be created." << std::endl;
			return false;
		}
	} else if (!fs::IsDir(root_path)) {
		errorstream << "ModMetadata[" << m_mod_name << "]: Unable to save. '"
			    << root_path << "' is not a directory." << std::endl;
		return false;
	}

	bool w_ok = fs::safeWriteToFile(
			root_path + DIR_DELIM + m_mod_name, fastWriteJson(json));

	if (w_ok) {
		m_modified = false;
	} else {
		errorstream << "ModMetadata[" << m_mod_name << "]: failed write file."
			    << std::endl;
	}
	return w_ok;
}

bool ModMetadata::load(const std::string &root_path)
{
	m_stringvars.clear();

	std::ifstream is((root_path + DIR_DELIM + m_mod_name).c_str(),
			std::ios_base::binary);
	if (!is.good()) {
		return false;
	}

	Json::Value root;
	Json::CharReaderBuilder builder;
	builder.settings_["collectComments"] = false;
	std::string errs;

	if (!Json::parseFromStream(builder, is, &root, &errs)) {
		errorstream << "ModMetadata[" << m_mod_name
			    << "]: failed read data "
			       "(Json decoding failure). Message: "
			    << errs << std::endl;
		return false;
	}

	const Json::Value::Members attr_list = root.getMemberNames();
	for (const auto &it : attr_list) {
		Json::Value attr_value = root[it];
		m_stringvars[it] = attr_value.asString();
	}

	return true;
}

bool ModMetadata::setString(const std::string &name, const std::string &var)
{
	m_modified = Metadata::setString(name, var);
	return m_modified;
}

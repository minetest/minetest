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

#include "mods.h"
#include "filesys.h"
#include "log.h"
#include "scripting_server.h"
#include "content/subgames.h"
#include "porting.h"
#include "util/metricsbackend.h"

/**
 * Manage server mods
 *
 * All new calls to this class must be tested in test_servermodmanager.cpp
 */

/**
 * Creates a ServerModManager which targets worldpath
 * @param worldpath
 */
ServerModManager::ServerModManager(const std::string &worldpath) :
		ModConfiguration(worldpath)
{
	SubgameSpec gamespec = findWorldSubgame(worldpath);

	// Add all game mods and all world mods
	addModsInPath(gamespec.gamemods_path);
	addModsInPath(worldpath + DIR_DELIM + "worldmods");

	// Load normal mods
	std::string worldmt = worldpath + DIR_DELIM + "world.mt";
	addModsFromConfig(worldmt, gamespec.addon_mods_paths);
}

// clang-format off
// This function cannot be currenctly easily tested but it should be ASAP
void ServerModManager::loadMods(ServerScripting *script)
{
	// Print mods
	infostream << "Server: Loading mods: ";
	for (const ModSpec &mod : m_sorted_mods) {
		infostream << mod.name << " ";
	}
	infostream << std::endl;
	// Load and run "mod" scripts
	for (const ModSpec &mod : m_sorted_mods) {
		if (!string_allowed(mod.name, MODNAME_ALLOWED_CHARS)) {
			throw ModError("Error loading mod \"" + mod.name +
					"\": Mod name does not follow naming "
					"conventions: "
					"Only characters [a-z0-9_] are allowed.");
		}
		std::string script_path = mod.path + DIR_DELIM + "init.lua";
		auto t = porting::getTimeMs();
		script->loadMod(script_path, mod.name);
		infostream << "Mod \"" << mod.name << "\" loaded after "
			<< (porting::getTimeMs() - t) << " ms" << std::endl;
	}

	// Run a callback when mods are loaded
	script->on_mods_loaded();
}

// clang-format on
const ModSpec *ServerModManager::getModSpec(const std::string &modname) const
{
	std::vector<ModSpec>::const_iterator it;
	for (it = m_sorted_mods.begin(); it != m_sorted_mods.end(); ++it) {
		const ModSpec &mod = *it;
		if (mod.name == modname)
			return &mod;
	}
	return NULL;
}

void ServerModManager::getModNames(std::vector<std::string> &modlist) const
{
	for (const ModSpec &spec : m_sorted_mods)
		modlist.push_back(spec.name);
}

void ServerModManager::getModsMediaPaths(std::vector<std::string> &paths) const
{
	for (const ModSpec &spec : m_sorted_mods) {
		fs::GetRecursiveDirs(paths, spec.path + DIR_DELIM + "textures");
		fs::GetRecursiveDirs(paths, spec.path + DIR_DELIM + "sounds");
		fs::GetRecursiveDirs(paths, spec.path + DIR_DELIM + "media");
		fs::GetRecursiveDirs(paths, spec.path + DIR_DELIM + "models");
		fs::GetRecursiveDirs(paths, spec.path + DIR_DELIM + "locale");
	}
}

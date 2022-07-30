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

/**
 * Manage server mods
 *
 * All new calls to this class must be tested in test_servermodmanager.cpp
 */

/**
 * Creates a ServerModManager which targets worldpath
 * @param worldpath
 */
ServerModManager::ServerModManager(const std::string &worldpath):
	configuration()
{
	SubgameSpec gamespec = findWorldSubgame(worldpath);

	// Add all game mods and all world mods
	configuration.addGameMods(gamespec);
	configuration.addModsInPath(worldpath + DIR_DELIM + "worldmods", "worldmods");

	// Load normal mods
	std::string worldmt = worldpath + DIR_DELIM + "world.mt";
	configuration.addModsFromConfig(worldmt, gamespec.addon_mods_paths);
	configuration.checkConflictsAndDeps();
}

// clang-format off
// This function cannot be currenctly easily tested but it should be ASAP
void ServerModManager::loadMods(ServerScripting *script)
{
	// Print mods
	infostream << "Server: Loading mods: ";
	for (const ModSpec &mod : configuration.getMods()) {
		infostream << mod.name << " ";
	}

	infostream << std::endl;
	// Load and run "mod" scripts
	for (const ModSpec &mod : configuration.getMods()) {
		mod.checkAndLog();

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
	for (const auto &mod : configuration.getMods()) {
		if (mod.name == modname)
			return &mod;
	}

	return nullptr;
}

void ServerModManager::getModNames(std::vector<std::string> &modlist) const
{
	for (const ModSpec &spec : configuration.getMods())
		modlist.push_back(spec.name);
}

void ServerModManager::getModsMediaPaths(std::vector<std::string> &paths) const
{
	// Iterate mods in reverse load order: Media loading expects higher priority media files first
	// and mods loading later should be able to override media of already loaded mods
	const auto &mods = configuration.getMods();
	for (auto it = mods.crbegin(); it != mods.crend(); it++) {
		const ModSpec &spec = *it;
		fs::GetRecursiveDirs(paths, spec.path + DIR_DELIM + "textures");
		fs::GetRecursiveDirs(paths, spec.path + DIR_DELIM + "sounds");
		fs::GetRecursiveDirs(paths, spec.path + DIR_DELIM + "media");
		fs::GetRecursiveDirs(paths, spec.path + DIR_DELIM + "models");
		fs::GetRecursiveDirs(paths, spec.path + DIR_DELIM + "locale");
	}
}

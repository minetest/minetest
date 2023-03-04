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

#pragma once

#include "content/mod_configuration.h"
#include <memory>

class MetricsBackend;
class MetricCounter;
class ServerScripting;

/**
 * Manage server mods
 *
 * All new calls to this class must be tested in test_servermodmanager.cpp
 */
class ServerModManager
{
	ModConfiguration configuration;

public:
	/**
	 * Creates a ServerModManager which targets worldpath
	 * @param worldpath
	 */
	ServerModManager(const std::string &worldpath);
	void loadMods(ServerScripting *script);
	const ModSpec *getModSpec(const std::string &modname) const;
	void getModNames(std::vector<std::string> &modlist) const;

	inline const std::vector<ModSpec> &getMods() const {
		return configuration.getMods();
	}

	inline const std::vector<ModSpec> &getUnsatisfiedMods() const {
		return configuration.getUnsatisfiedMods();
	}

	inline bool isConsistent() const {
		return configuration.isConsistent();
	}

	inline std::string getUnsatisfiedModsError() const {
		return configuration.getUnsatisfiedModsError();
	}

	/**
	 * Recursively gets all paths of mod folders that can contain media files.
	 *
	 * Result is ordered in descending priority, ie. files from an earlier path
	 * should not be replaced by files from a latter one.
	 *
	 * @param paths result vector
	 */
	void getModsMediaPaths(std::vector<std::string> &paths) const;
};

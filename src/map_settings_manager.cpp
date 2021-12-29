/*
Minetest
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#include "debug.h"
#include "filesys.h"
#include "log.h"
#include "mapgen/mapgen.h"
#include "settings.h"

#include "map_settings_manager.h"

MapSettingsManager::MapSettingsManager(const std::string &map_meta_path):
	m_map_meta_path(map_meta_path),
	m_hierarchy(g_settings)
{
	/*
	 * We build our own hierarchy which falls back to the global one.
	 * It looks as follows: (lowest prio first)
	 * 0: whatever is picked up from g_settings (incl. engine defaults)
	 * 1: defaults set by scripts (override_meta = false)
	 * 2: settings present in map_meta.txt or overriden by scripts
	 */
	m_defaults = new Settings("", &m_hierarchy, 1);
	m_map_settings = new Settings("[end_of_params]", &m_hierarchy, 2);
}


MapSettingsManager::~MapSettingsManager()
{
	delete m_defaults;
	delete m_map_settings;
	delete mapgen_params;
}


bool MapSettingsManager::getMapSetting(
	const std::string &name, std::string *value_out)
{
	return m_map_settings->getNoEx(name, *value_out);
}


bool MapSettingsManager::getMapSettingNoiseParams(
	const std::string &name, NoiseParams *value_out)
{
	// TODO: Rename to "getNoiseParams"
	return m_map_settings->getNoiseParams(name, *value_out);
}


bool MapSettingsManager::setMapSetting(
	const std::string &name, const std::string &value, bool override_meta)
{
	if (mapgen_params)
		return false;

	if (override_meta)
		m_map_settings->set(name, value);
	else
		m_defaults->set(name, value);

	return true;
}


bool MapSettingsManager::setMapSettingNoiseParams(
	const std::string &name, const NoiseParams *value, bool override_meta)
{
	if (mapgen_params)
		return false;

	if (override_meta)
		m_map_settings->setNoiseParams(name, *value);
	else
		m_defaults->setNoiseParams(name, *value);

	return true;
}


bool MapSettingsManager::loadMapMeta()
{
	std::ifstream is(m_map_meta_path.c_str(), std::ios_base::binary);

	if (!is.good()) {
		errorstream << "loadMapMeta: could not open "
			<< m_map_meta_path << std::endl;
		return false;
	}

	if (!m_map_settings->parseConfigLines(is)) {
		errorstream << "loadMapMeta: Format error. '[end_of_params]' missing?" << std::endl;
		return false;
	}

	return true;
}


bool MapSettingsManager::saveMapMeta()
{
	// If mapgen params haven't been created yet; abort
	if (!mapgen_params) {
		infostream << "saveMapMeta: mapgen_params not present! "
			<< "Server startup was probably interrupted." << std::endl;
		return false;
	}

	// Paths set up by subgames.cpp, but not in unittests
	if (!fs::CreateAllDirs(fs::RemoveLastPathComponent(m_map_meta_path))) {
		errorstream << "saveMapMeta: could not create dirs to "
			<< m_map_meta_path;
		return false;
	}

	mapgen_params->MapgenParams::writeParams(m_map_settings);
	mapgen_params->writeParams(m_map_settings);

	if (!m_map_settings->updateConfigFile(m_map_meta_path.c_str())) {
		errorstream << "saveMapMeta: could not write "
			<< m_map_meta_path << std::endl;
		return false;
	}

	return true;
}


MapgenParams *MapSettingsManager::makeMapgenParams()
{
	if (mapgen_params)
		return mapgen_params;

	assert(m_map_settings);
	assert(m_defaults);

	// Now, get the mapgen type so we can create the appropriate MapgenParams
	std::string mg_name;
	MapgenType mgtype = getMapSetting("mg_name", &mg_name) ?
		Mapgen::getMapgenType(mg_name) : MAPGEN_DEFAULT;

	if (mgtype == MAPGEN_INVALID) {
		errorstream << "EmergeManager: mapgen '" << mg_name <<
			"' not valid; falling back to " <<
			Mapgen::getMapgenName(MAPGEN_DEFAULT) << std::endl;
		mgtype = MAPGEN_DEFAULT;
	}

	// Create our MapgenParams
	MapgenParams *params = Mapgen::createMapgenParams(mgtype);
	if (!params)
		return nullptr;

	params->mgtype = mgtype;

	// Load the rest of the mapgen params from our active settings
	params->MapgenParams::readParams(m_map_settings);
	params->readParams(m_map_settings);

	// Hold onto our params
	mapgen_params = params;

	return params;
}

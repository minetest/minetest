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

MapSettingsManager::MapSettingsManager(Settings *user_settings,
		const std::string &map_meta_path):
	m_map_meta_path(map_meta_path),
	m_map_settings(new Settings()),
	m_user_settings(user_settings)
{
	assert(m_user_settings != NULL);

	Mapgen::setDefaultSettings(m_map_settings);
	// This inherits the combined defaults provided by loadGameConfAndInitWorld.
	m_map_settings->overrideDefaults(user_settings);
}


MapSettingsManager::~MapSettingsManager()
{
	delete m_map_settings;
	delete mapgen_params;
}


bool MapSettingsManager::getMapSetting(
	const std::string &name, std::string *value_out)
{
	if (m_map_settings->getNoEx(name, *value_out))
		return true;

	// Compatibility kludge
	if (m_user_settings == g_settings && name == "seed")
		return m_user_settings->getNoEx("fixed_map_seed", *value_out);

	return m_user_settings->getNoEx(name, *value_out);
}


bool MapSettingsManager::getMapSettingNoiseParams(
	const std::string &name, NoiseParams *value_out)
{
	return m_map_settings->getNoiseParams(name, *value_out) ||
		m_user_settings->getNoiseParams(name, *value_out);
}


bool MapSettingsManager::setMapSetting(
	const std::string &name, const std::string &value, bool override_meta)
{
	if (mapgen_params)
		return false;

	if (override_meta)
		m_map_settings->set(name, value);
	else
		m_map_settings->setDefault(name, value);

	return true;
}


bool MapSettingsManager::setMapSettingNoiseParams(
	const std::string &name, const NoiseParams *value, bool override_meta)
{
	if (mapgen_params)
		return false;

	m_map_settings->setNoiseParams(name, *value, !override_meta);
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

	if (!m_map_settings->parseConfigLines(is, "[end_of_params]")) {
		errorstream << "loadMapMeta: [end_of_params] not found!" << std::endl;
		return false;
	}

	return true;
}


bool MapSettingsManager::saveMapMeta()
{
	// If mapgen params haven't been created yet; abort
	if (!mapgen_params)
		return false;

	if (!fs::CreateAllDirs(fs::RemoveLastPathComponent(m_map_meta_path))) {
		errorstream << "saveMapMeta: could not create dirs to "
			<< m_map_meta_path;
		return false;
	}

	std::ostringstream oss(std::ios_base::binary);
	Settings conf;

	mapgen_params->MapgenParams::writeParams(&conf);
	mapgen_params->writeParams(&conf);
	conf.writeLines(oss);

	// NOTE: If there are ever types of map settings other than
	// those relating to map generation, save them here

	oss << "[end_of_params]\n";

	if (!fs::safeWriteToFile(m_map_meta_path, oss.str())) {
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

	assert(m_user_settings != NULL);
	assert(m_map_settings != NULL);

	// At this point, we have (in order of precedence):
	// 1). m_mapgen_settings->m_settings containing map_meta.txt settings or
	//     explicit overrides from scripts
	// 2). m_mapgen_settings->m_defaults containing script-set mgparams without
	//     overrides
	// 3). g_settings->m_settings containing all user-specified config file
	//     settings
	// 4). g_settings->m_defaults containing any low-priority settings from
	//     scripts, e.g. mods using Lua as an enhanced config file)

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

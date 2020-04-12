/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013-2020 Minetest core developers & community

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

#include "worldsettings.h"
#include "filesys.h"
#include "log.h"

#define MAP_BACKEND_KEY "backend"
#define DEFAULT_BACKEND "sqlite3"
#define MAP_READONLY_BACKEND_KEY "readonly_backend"
#define AUTH_BACKEND_KEY "auth_backend"
#define PLAYER_BACKEND_KEY "player_backend"

#if USE_REDIS
#define REDIS_ADDR_KEY "redis_address"
#define REDIS_HASH_KEY "redis_hash"
#define REDIS_PASSWORD_KEY "redis_password"
#define REDIS_PORT_KEY "redis_port"
#endif
#if USE_POSTGRESQL
#define PGSQL_CONN_KEY "pgsql_connection"
#define PGSQL_PLAYER_CONN_KEY "pgsql_player_connection"
#endif

WorldSettings::WorldSettings(const std::string &savedir) :
		m_conf_path(savedir + DIR_DELIM + "world.mt"), m_savedir(savedir)
{
}

bool WorldSettings::load(bool return_on_config_read)
{
	bool succeeded = readConfigFile(m_conf_path.c_str());

	// player backend is not set, assume it's legacy file backend.
	if (!exists(PLAYER_BACKEND_KEY)) {
		// fall back to files
		set(PLAYER_BACKEND_KEY, "files");
		infostream << "No " << PLAYER_BACKEND_KEY << " key found, assuming files."
			   << std::endl;

		if (!updateConfigFile()) {
			errorstream << "WorldSettings::load(): "
				    << "Failed to update world.mt!" << std::endl;
		}
	}

	// auth backend is not set, assume it's legacy file backend.
	if (!exists(AUTH_BACKEND_KEY)) {
		set(AUTH_BACKEND_KEY, "files");
		infostream << "No " << AUTH_BACKEND_KEY << " key found, assuming files."
			   << std::endl;

		if (!updateConfigFile()) {
			errorstream << "WorldSettings::load(): "
				    << "Failed to update world.mt!" << std::endl;
		}
	}

	// Now read env vars
	if (const char *backend = std::getenv("WORLD_BACKEND"))
		set(MAP_BACKEND_KEY, std::string(backend));

	if (const char *readonly_backend = std::getenv("WORLD_READONLY_BACKEND"))
		set(MAP_READONLY_BACKEND_KEY, std::string(readonly_backend));

	if (const char *backend = std::getenv("WORLD_AUTH_BACKEND"))
		set(AUTH_BACKEND_KEY, std::string(backend));

	if (const char *backend = std::getenv("WORLD_PLAYER_BACKEND"))
		set(PLAYER_BACKEND_KEY, std::string(backend));

#if USE_REDIS
	if (const char *redis_addr = std::getenv("WORLD_REDIS_ADDRESS"))
		set(REDIS_ADDR_KEY, std::string(redis_addr));

	if (const char *redis_hash = std::getenv("WORLD_REDIS_HASH"))
		set(REDIS_HASH_KEY, std::string(redis_hash));

	if (const char *redis_password = std::getenv("WORLD_REDIS_PASSWORD"))
		set(REDIS_PASSWORD_KEY, std::string(redis_password));

	if (const char *redis_port = std::getenv("WORLD_REDIS_PORT")) {
		setU16(REDIS_PORT_KEY, std::atoi(redis_port));
	}
#endif

#if USE_POSTGRESQL
	if (const char *pgsql_conn_str = std::getenv("WORLD_POSTGRESQL_CONNECTION"))
		set(PGSQL_CONN_KEY, std::string(pgsql_conn_str));

	if (const char *pgsql_conn_str =
					std::getenv("WORLD_PLAYER_POSTGRESQL_CONNECTION"))
		set(PGSQL_PLAYER_CONN_KEY, std::string(pgsql_conn_str));
#endif

	if (return_on_config_read) {
		return succeeded;
	}

	if (!succeeded || !exists(MAP_BACKEND_KEY)) {
		// fall back to sqlite3
		set(MAP_BACKEND_KEY, DEFAULT_BACKEND);
	}

	m_backend = get(MAP_BACKEND_KEY);

	if (exists(MAP_READONLY_BACKEND_KEY)) {
		m_readonly_backend = get(MAP_READONLY_BACKEND_KEY);
	}

	return true;
}

std::string WorldSettings::getReadOnlyDir() const
{
	return m_savedir + DIR_DELIM + "readonly";
}

std::string WorldSettings::getAuthBackend() const
{
	std::string backend_name;
	getNoEx(AUTH_BACKEND_KEY, backend_name);
	return backend_name;
}

void WorldSettings::setMapBackend(const std::string &backend, bool save)
{
	m_backend = backend;
	set(MAP_BACKEND_KEY, backend);
	if (save) {
		if (!updateConfigFile())
			errorstream << "Failed to update world.mt!" << std::endl;
		else
			actionstream << "world.mt updated" << std::endl;
	}
}
void WorldSettings::setAuthBackend(const std::string &backend, bool save)
{
	set(AUTH_BACKEND_KEY, backend);
	if (save) {
		if (!updateConfigFile())
			errorstream << "Failed to update world.mt!" << std::endl;
		else
			actionstream << "world.mt updated" << std::endl;
	}
}

void WorldSettings::setPlayerBackend(const std::string &backend, bool save)
{
	set(PLAYER_BACKEND_KEY, backend);
	if (save) {
		if (!updateConfigFile())
			errorstream << "Failed to update world.mt!" << std::endl;
		else
			actionstream << "world.mt updated" << std::endl;
	}
}

std::string WorldSettings::getPlayerBackend() const
{
	std::string backend_name;
	getNoEx(PLAYER_BACKEND_KEY, backend_name);
	return backend_name;
}

#if USE_POSTGRESQL
std::string WorldSettings::getMapPostgreSQLConnectionString() const
{
	std::string connection_string;
	getNoEx(PGSQL_CONN_KEY, connection_string);
	return connection_string;
}

std::string WorldSettings::getPlayerPostgresConnectionString() const
{
	std::string connection_string;
	getNoEx(PGSQL_PLAYER_CONN_KEY, connection_string);
	return connection_string;
}
#endif
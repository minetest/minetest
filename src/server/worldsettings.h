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

#pragma once

#include "settings.h"
#include "cmake_config.h"

class WorldSettings : private Settings
{
public:
	WorldSettings() = delete;
	WorldSettings(const std::string &savedir);
	bool load(bool return_on_config_read = false);
	inline const std::string &getMapBackend() const { return m_backend; }
	void setMapBackend(const std::string &backend, bool save = false);
	void setAuthBackend(const std::string &backend, bool save = false);
	void setPlayerBackend(const std::string &backend, bool save = false);
	bool isPlayerBackendDefined() const { return exists("player_backend"); }
	std::string getPlayerBackend() const;
	std::string getAuthBackend() const;
	inline const std::string &getReadOnlyBackend() const
	{
		return m_readonly_backend;
	}
	bool isBackendSet() const { return exists("backend"); }
	std::string getReadOnlyDir() const;
	bool updateConfigFile()
	{
		return Settings::updateConfigFile(m_conf_path.c_str());
	}
#if USE_REDIS
	const std::string &getRedisAddr() const { return get("redis_address"); }
	const std::string &getRedisHash() const { return get("redis_hash"); }
	std::string getRedisPassword() const
	{
		return exists("redis_password") ? get("redis_password") : "";
	}
	u16 getRedisPort() const
	{
		return exists("redis_port") ? getU16("redis_port") : 6379;
	}
#endif
#if USE_POSTGRESQL
	std::string getMapPostgreSQLConnectionString() const;
	std::string getPlayerPostgresConnectionString() const;
#endif
private:
	std::string m_conf_path;
	std::string m_savedir;
	std::string m_backend;
	std::string m_readonly_backend;
};
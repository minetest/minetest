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

#pragma once

#include "util/string.h"
#include "threading/thread.h"
#include "exceptions.h"
#include <map>
#include <string>
#include <mutex>

class BanManager
{
public:
	BanManager(const std::string &banfilepath);
	~BanManager();
	void load();
	void save();
	bool isIpBanned(const std::string &ip);
	// Supplying ip_or_name = "" lists all bans.
	std::string getBanDescription(const std::string &ip_or_name);
	std::string getBanName(const std::string &ip);
	void add(const std::string &ip, const std::string &name);
	void remove(const std::string &ip_or_name);
	bool isModified();

private:
	std::mutex m_mutex;
	std::string m_banfilepath = "";
	StringMap m_ips;
	bool m_modified = false;
};

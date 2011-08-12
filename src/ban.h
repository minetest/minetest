/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef BAN_HEADER
#define BAN_HEADER

#include <set>
#include <string>
#include <jthread.h>
#include <jmutex.h>
#include "common_irrlicht.h"
#include "exceptions.h"

class BanManager
{
public:
	BanManager(const std::string &bannfilepath);
	~BanManager();
	void load();
	void save();
	void add(std::string ip);
	void remove(std::string ip);
	bool isIpBanned(std::string ip);
	bool isModified();
private:
	JMutex m_mutex;
	std::string m_banfilepath;
	std::set<std::string> m_ips;
	bool m_modified;

};

#endif

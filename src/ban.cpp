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

#include "ban.h"
#include <fstream>
#include <jmutexautolock.h>
#include <sstream>
#include "strfnd.h"
#include "debug.h"

BanManager::BanManager(const std::string &banfilepath):
		m_banfilepath(banfilepath),
		m_modified(false)
{
	m_mutex.Init();
	try{
		load();
	}
	catch(SerializationError &e)
	{
		dstream<<"WARNING: BanManager: creating "
				<<m_banfilepath<<std::endl;
	}
}

BanManager::~BanManager()
{
	save();
}

void BanManager::load()
{
	JMutexAutoLock lock(m_mutex);
	dstream<<"BanManager: loading from "<<m_banfilepath<<std::endl;
	std::ifstream is(m_banfilepath.c_str(), std::ios::binary);
	if(is.good() == false)
	{
		dstream<<"BanManager: failed loading from "<<m_banfilepath<<std::endl;
		throw SerializationError("BanManager::load(): Couldn't open file");
	}
	
	for(;;)
	{
		if(is.eof() || is.good() == false)
			break;
		std::string ip;
		std::getline(is, ip, '\n');
		m_ips.insert(ip);
	}
	m_modified = false;
}

void BanManager::save()
{
	JMutexAutoLock lock(m_mutex);
	dstream<<"BanManager: saving to "<<m_banfilepath<<std::endl;
	std::ofstream os(m_banfilepath.c_str(), std::ios::binary);
	
	if(os.good() == false)
	{
		dstream<<"BanManager: failed loading from "<<m_banfilepath<<std::endl;
		throw SerializationError("BanManager::load(): Couldn't open file");
	}

	for(std::set<std::string>::iterator
			i = m_ips.begin();
			i != m_ips.end(); i++)
	{
		if(*i == "")
			continue;
		os<<*i<<"\n";
	}
	m_modified = false;
}

bool BanManager::isIpBanned(std::string ip)
{
	JMutexAutoLock lock(m_mutex);
	return m_ips.find(ip) != m_ips.end();
}

void BanManager::add(std::string ip)
{
	JMutexAutoLock lock(m_mutex);
	m_ips.insert(ip);
	m_modified = true;
}

void BanManager::remove(std::string ip)
{
	JMutexAutoLock lock(m_mutex);
	m_ips.erase(m_ips.find(ip));
	m_modified = true;
}
	

bool BanManager::isModified()
{
	JMutexAutoLock lock(m_mutex);
	return m_modified;
}


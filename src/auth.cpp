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

#include "auth.h"
#include <fstream>
#include <jmutexautolock.h>
//#include "main.h" // for g_settings
#include <sstream>
#include "strfnd.h"
#include "debug.h"

// Convert a privileges value into a human-readable string,
// with each component separated by a comma.
std::string privsToString(u64 privs)
{
	std::ostringstream os(std::ios_base::binary);
	if(privs & PRIV_BUILD)
		os<<"build,";
	if(privs & PRIV_TELEPORT)
		os<<"teleport,";
	if(privs & PRIV_SETTIME)
		os<<"settime,";
	if(privs & PRIV_PRIVS)
		os<<"privs,";
	if(privs & PRIV_SHOUT)
		os<<"shout,";
	if(os.tellp())
	{
		// Drop the trailing comma. (Why on earth can't
		// you truncate a C++ stream anyway???)
		std::string tmp = os.str();
		return tmp.substr(0, tmp.length() -1);
	}
	return os.str();
}

// Converts a comma-seperated list of privilege values into a
// privileges value. The reverse of privsToString(). Returns
// PRIV_INVALID if there is anything wrong with the input.
u64 stringToPrivs(std::string str)
{
	u64 privs=0;
	Strfnd f(str);
	while(f.atend() == false)
	{
		std::string s = trim(f.next(","));
		if(s == "build")
			privs |= PRIV_BUILD;
		else if(s == "teleport")
			privs |= PRIV_TELEPORT;
		else if(s == "settime")
			privs |= PRIV_SETTIME;
		else if(s == "privs")
			privs |= PRIV_PRIVS;
		else if(s == "shout")
			privs |= PRIV_SHOUT;
		else
			return PRIV_INVALID;
	}
	return privs;
}

AuthManager::AuthManager(const std::string &authfilepath):
		m_authfilepath(authfilepath)
{
	m_mutex.Init();
	
	try{
		load();
	}
	catch(SerializationError &e)
	{
		dstream<<"WARNING: AuthManager: creating "
				<<m_authfilepath<<std::endl;
	}
}

AuthManager::~AuthManager()
{
	save();
}

void AuthManager::load()
{
	JMutexAutoLock lock(m_mutex);
	
	dstream<<"AuthManager: loading from "<<m_authfilepath<<std::endl;
	std::ifstream is(m_authfilepath.c_str(), std::ios::binary);
	if(is.good() == false)
	{
		dstream<<"AuthManager: failed loading from "<<m_authfilepath<<std::endl;
		throw SerializationError("AuthManager::load(): Couldn't open file");
	}

	for(;;)
	{
		if(is.eof() || is.good() == false)
			break;

		// Read a line
		std::string line;
		std::getline(is, line, '\n');

		std::istringstream iss(line);
		
		// Read name
		std::string name;
		std::getline(iss, name, ':');

		// Read password
		std::string pwd;
		std::getline(iss, pwd, ':');

		// Read privileges
		std::string stringprivs;
		std::getline(iss, stringprivs, ':');
		u64 privs = stringToPrivs(stringprivs);
		
		// Store it
		AuthData ad;
		ad.pwd = pwd;
		ad.privs = privs;
		m_authdata[name] = ad;
	}
}

void AuthManager::save()
{
	JMutexAutoLock lock(m_mutex);
	
	dstream<<"AuthManager: saving to "<<m_authfilepath<<std::endl;
	std::ofstream os(m_authfilepath.c_str(), std::ios::binary);
	if(os.good() == false)
	{
		dstream<<"AuthManager: failed saving to "<<m_authfilepath<<std::endl;
		throw SerializationError("AuthManager::save(): Couldn't open file");
	}
	
	for(core::map<std::string, AuthData>::Iterator
			i = m_authdata.getIterator();
			i.atEnd()==false; i++)
	{
		std::string name = i.getNode()->getKey();
		if(name == "")
			continue;
		AuthData ad = i.getNode()->getValue();
		os<<name<<":"<<ad.pwd<<":"<<privsToString(ad.privs)<<"\n";
	}
}

bool AuthManager::exists(const std::string &username)
{
	JMutexAutoLock lock(m_mutex);
	
	core::map<std::string, AuthData>::Node *n;
	n = m_authdata.find(username);
	if(n == NULL)
		return false;
	return true;
}

void AuthManager::set(const std::string &username, AuthData ad)
{
	JMutexAutoLock lock(m_mutex);
	
	m_authdata[username] = ad;
}

void AuthManager::add(const std::string &username)
{
	JMutexAutoLock lock(m_mutex);
	
	m_authdata[username] = AuthData();
}

std::string AuthManager::getPassword(const std::string &username)
{
	JMutexAutoLock lock(m_mutex);
	
	core::map<std::string, AuthData>::Node *n;
	n = m_authdata.find(username);
	if(n == NULL)
		throw AuthNotFoundException("");
	
	return n->getValue().pwd;
}

void AuthManager::setPassword(const std::string &username,
		const std::string &password)
{
	JMutexAutoLock lock(m_mutex);
	
	core::map<std::string, AuthData>::Node *n;
	n = m_authdata.find(username);
	if(n == NULL)
		throw AuthNotFoundException("");
	
	AuthData ad = n->getValue();
	ad.pwd = password;
	n->setValue(ad);
}

u64 AuthManager::getPrivs(const std::string &username)
{
	JMutexAutoLock lock(m_mutex);
	
	core::map<std::string, AuthData>::Node *n;
	n = m_authdata.find(username);
	if(n == NULL)
		throw AuthNotFoundException("");
	
	return n->getValue().privs;
}

void AuthManager::setPrivs(const std::string &username, u64 privs)
{
	JMutexAutoLock lock(m_mutex);
	
	core::map<std::string, AuthData>::Node *n;
	n = m_authdata.find(username);
	if(n == NULL)
		throw AuthNotFoundException("");
	
	AuthData ad = n->getValue();
	ad.privs = privs;
	n->setValue(ad);
}


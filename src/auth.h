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

#ifndef AUTH_HEADER
#define AUTH_HEADER

#include <string>
#include <jthread.h>
#include <jmutex.h>
#include "common_irrlicht.h"
#include "exceptions.h"

// Player privileges. These form a bitmask stored in the privs field
// of the player, and define things they're allowed to do. See also
// the static methods Player::privsToString and stringToPrivs that
// convert these to human-readable form.
const u64 PRIV_BUILD = 1;            // Can build - i.e. modify the world
const u64 PRIV_TELEPORT = 2;         // Can teleport
const u64 PRIV_SETTIME = 4;          // Can set the time
const u64 PRIV_PRIVS = 8;            // Can grant and revoke privileges
const u64 PRIV_SERVER = 16;          // Can manage the server (e.g. shutodwn
                                     // ,settings)
const u64 PRIV_SHOUT = 32;           // Can broadcast chat messages to all
                                     // players
const u64 PRIV_BAN = 64;             // Can ban players

// Default privileges - these can be overriden for new players using the
// config option "default_privs" - however, this value still applies for
// players that existed before the privileges system was added.
const u64 PRIV_DEFAULT = PRIV_BUILD|PRIV_SHOUT;
const u64 PRIV_ALL = 0x7FFFFFFFFFFFFFFFULL;
const u64 PRIV_INVALID = 0x8000000000000000ULL;

// Convert a privileges value into a human-readable string,
// with each component separated by a comma.
std::string privsToString(u64 privs);

// Converts a comma-seperated list of privilege values into a
// privileges value. The reverse of privsToString(). Returns
// PRIV_INVALID if there is anything wrong with the input.
u64 stringToPrivs(std::string str);

struct AuthData
{
	std::string pwd;
	u64 privs;

	AuthData():
		privs(PRIV_DEFAULT)
	{
	}
};

class AuthNotFoundException : public BaseException
{
public:
	AuthNotFoundException(const char *s):
		BaseException(s)
	{}
};

class AuthManager
{
public:
	AuthManager(const std::string &authfilepath);
	~AuthManager();
	void load();
	void save();
	bool exists(const std::string &username);
	void set(const std::string &username, AuthData ad);
	void add(const std::string &username);
	std::string getPassword(const std::string &username);
	void setPassword(const std::string &username,
			const std::string &password);
	u64 getPrivs(const std::string &username);
	void setPrivs(const std::string &username, u64 privs);
	bool isModified();
private:
	JMutex m_mutex;
	std::string m_authfilepath;
	core::map<std::string, AuthData> m_authdata;
	bool m_modified;
};

#endif


/*
Minetest
Copyright (C) 2024 SFENCE <sfence.software@gmail.com>

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

#include "irrlichttypes.h"
#include "util/basic_macros.h"
#include "network/address.h"
#include <string>
#include <map>

struct AnotherServer
{
	std::string address_string;
	Address address;
	u16 port;
	std::string auth_send;
	std::string auth_receive;
};

class ServerNetwork
{
public:
	ServerNetwork() = default;
	~ServerNetwork() = default;
	DISABLE_CLASS_COPY(ServerNetwork);

	// get name of server
	const std::string *getName(const Address &address, const std::string &auth) const;

	// add/set server
	void setServer(const std::string &name, const AnotherServer &server);
	// get server
	const AnotherServer *getServer(const std::string &name);
	// remove server
	void removeServer(const std::string &name);

private:
	std::map<std::string, AnotherServer> m_servers;
};


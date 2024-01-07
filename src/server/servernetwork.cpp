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

#include "servernetwork.h"
#include "log.h"
#include <stdexcept>

	
// get name of server
const std::string *ServerNetwork::getName(const Address &address, const std::string &auth) const
{
	for (auto it = m_servers.begin(); it != m_servers.end(); ++it)
	{
		if ((it->second.auth_receive == auth) && (it->second.address == address)) {
			return &it->first;
		}
	}
	return nullptr;
}

// add/set server
void ServerNetwork::setServer(const std::string &name, const AnotherServer &server_def)
{
	m_servers[name] = server_def;

	AnotherServer &server = m_servers[name];
	
	server.address.setPort(server.port);

	server.address.Resolve(server.address_string.c_str());

	if (server.address.isZero()) { // i.e. INADDR_ANY, IN6ADDR_ANY
		throw ResolveError("Resolved as zero address.");
	}

	verbosestream << "Another server address " << server.address_string << " resolved as " << server.address.serializeString();
}
// get server
const AnotherServer* ServerNetwork::getServer(const std::string &name)
{
	try {
		const AnotherServer &server = m_servers.at(name);
		return &server;
	}
	catch (std::out_of_range &key) {
	}
	return nullptr;
}
// remove server
void ServerNetwork::removeServer(const std::string &name)
{
	m_servers.erase(name);
}

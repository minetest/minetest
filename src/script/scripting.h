/*
Minetest
Copyright (C) 2017 Krock <mk939@ymail.com>

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

#ifndef SCRIPTING_H_
#define SCRIPTING_H_

#include "scripting_client.h"
#include "scripting_server.h"

enum ScriptingType {
	SCT_NONE,
	SCT_SERVER,
	SCT_CLIENT
};

class Scripting
{
public:
	Scripting()
	{
		m_type = SCT_NONE;
		m_dst.client = NULL;
	}
	~Scripting()
	{
		m_type = SCT_NONE;
	}

	Scripting(ServerScripting *server)
	{
		m_type = (server != NULL) ? SCT_SERVER : SCT_NONE;
		m_dst.server = server;
	}
	Scripting(ClientScripting *client)
	{
		m_type = (client != NULL) ? SCT_CLIENT : SCT_NONE;
		m_dst.client = client;
	}
	inline ServerScripting *getServerScripting()
	{
		return (m_type == SCT_SERVER) ? m_dst.server : NULL;
	}
	inline ClientScripting *getClientScripting()
	{
		return (m_type == SCT_CLIENT) ? m_dst.client : NULL;
	}

private:
	ScriptingType m_type;
	union {
		ServerScripting *server;
		ClientScripting *client;
	} m_dst;
};

#endif

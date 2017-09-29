/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "clientobject.h"
#include "debug.h"
#include "porting.h"

/*
	ClientActiveObject
*/

ClientActiveObject::ClientActiveObject(u16 id, Client *client,
		ClientEnvironment *env):
	ActiveObject(id),
	m_client(client),
	m_env(env)
{
}

ClientActiveObject::~ClientActiveObject()
{
	removeFromScene(true);
}

ClientActiveObject* ClientActiveObject::create(ActiveObjectType type,
		Client *client, ClientEnvironment *env)
{
	// Find factory function
	auto n = m_types.find(type);
	if (n == m_types.end()) {
		// If factory is not found, just return.
		warningstream << "ClientActiveObject: No factory for type="
				<< (int)type << std::endl;
		return NULL;
	}

	Factory f = n->second;
	ClientActiveObject *object = (*f)(client, env);
	return object;
}

void ClientActiveObject::registerType(u16 type, Factory f)
{
	auto n = m_types.find(type);
	if (n != m_types.end())
		return;
	m_types[type] = f;
}



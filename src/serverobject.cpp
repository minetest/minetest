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

#include "serverobject.h"
#include <fstream>
#include "inventory.h"
#include "constants.h" // BS
#include "log.h"

ServerActiveObject::ServerActiveObject(ServerEnvironment *env, v3f pos):
	ActiveObject(0),
	m_env(env),
	m_base_position(pos)
{
}

ServerActiveObject* ServerActiveObject::create(ActiveObjectType type,
		ServerEnvironment *env, u16 id, v3f pos,
		const std::string &data)
{
	// Find factory function
	std::map<u16, Factory>::iterator n;
	n = m_types.find(type);
	if(n == m_types.end()) {
		// These are 0.3 entity types, return without error.
		if (ACTIVEOBJECT_TYPE_ITEM <= type && type <= ACTIVEOBJECT_TYPE_MOBV2) {
			return NULL;
		}

		// If factory is not found, just return.
		warningstream<<"ServerActiveObject: No factory for type="
				<<type<<std::endl;
		return NULL;
	}

	Factory f = n->second;
	ServerActiveObject *object = (*f)(env, pos, data);
	return object;
}

void ServerActiveObject::registerType(u16 type, Factory f)
{
	std::map<u16, Factory>::iterator n;
	n = m_types.find(type);
	if(n != m_types.end())
		return;
	m_types[type] = f;
}

float ServerActiveObject::getMinimumSavedMovement()
{
	return 2.0*BS;
}

ItemStack ServerActiveObject::getWieldedItem(ItemStack *selected, ItemStack *hand) const
{
	*selected = ItemStack();
	if (hand)
		*hand = ItemStack();

	return ItemStack();
}

bool ServerActiveObject::setWieldedItem(const ItemStack &item)
{
	return false;
}

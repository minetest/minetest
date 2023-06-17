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
#include "irrlichttypes_bloated.h"
#include "shootline.h"
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


void ClientActiveObject::selectIfInRange(const Shootline &shootline,
		std::vector<DistanceSortedActiveObject> &dest)
{
	// Imagine a not-axis-aligned cuboid oriented into the direction of the shootline,
	// with the width of the object's selection box radius * 2 and with length of the
	// shootline (+selection box radius forwards and backwards). We check whether
	// the selection box center is inside this cuboid.
	aabb3f selection_box;
	if (!getSelectionBox(&selection_box))
			return;

	// possible optimization: get rid of the sqrt here
	f32 selection_box_radius = selection_box.getRadius();

	v3f pos_diff = getPosition() + selection_box.getCenter() - shootline.getLine().start;

	f32 d = shootline.getDir().dotProduct(pos_diff);

	if (shootline.mayHitSelectionBox(pos_diff, d, selection_box_radius)) {
		dest.emplace_back(this, d);
	}
}

void ClientActiveObject::registerType(u16 type, Factory f)
{
	auto n = m_types.find(type);
	if (n != m_types.end())
		return;
	m_types[type] = f;
}



/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "clientobject.h"
#include "debug.h"
#include "porting.h"
#include "constants.h"

/*
	ClientActiveObject
*/

ClientActiveObject::ClientActiveObject(u16 id):
	ActiveObject(id)
{
}

ClientActiveObject::~ClientActiveObject()
{
	removeFromScene();
}

ClientActiveObject* ClientActiveObject::create(u8 type)
{
	// Find factory function
	core::map<u16, Factory>::Node *n;
	n = m_types.find(type);
	if(n == NULL)
	{
		// If factory is not found, just return.
		dstream<<"WARNING: ClientActiveObject: No factory for type="
				<<(int)type<<std::endl;
		return NULL;
	}

	Factory f = n->getValue();
	ClientActiveObject *object = (*f)();
	return object;
}

void ClientActiveObject::registerType(u16 type, Factory f)
{
	core::map<u16, Factory>::Node *n;
	n = m_types.find(type);
	if(n)
		return;
	m_types.insert(type, f);
}



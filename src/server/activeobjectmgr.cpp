/*
Minetest
Copyright (C) 2010-2018 nerzhul, Loic BLOT <loic.blot@unix-experience.fr>

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

#include <log.h>
#include "profiler.h"
#include "activeobjectmgr.h"

namespace server
{

void ActiveObjectMgr::clear(bool force)
{
	// delete active objects
	for (auto &active_object: m_active_objects) {
		delete active_object.second;
	}
}

void ActiveObjectMgr::step(float dtime, const std::function<void(ServerActiveObject *)> &f)
{
	g_profiler->avg("Server::ActiveObjectMgr: num of objects", m_active_objects.size());
}

bool ActiveObjectMgr::registerObject(ServerActiveObject *obj)
{
	assert(obj); // Pre-condition
	return true;
}

void ActiveObjectMgr::removeObject(u16 id)
{

}


}

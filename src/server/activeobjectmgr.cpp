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
#include "mapblock.h"
#include "profiler.h"
#include "activeobjectmgr.h"

namespace server
{

void ActiveObjectMgr::clear(const std::function<bool(ServerActiveObject *, u16)> &cb)
{
	std::vector<u16> objects_to_remove;
	for (auto &it : m_active_objects) {
		if (cb(it.second, it.first)) {
			// Id to be removed from m_active_objects
			objects_to_remove.push_back(it.first);
		}
	}

	// Remove references from m_active_objects
	for (u16 i : objects_to_remove) {
		m_active_objects.erase(i);
	}
}

void ActiveObjectMgr::step(
		float dtime, const std::function<void(ServerActiveObject *)> &f)
{
	g_profiler->avg("ActiveObjectMgr: SAO count [#]", m_active_objects.size());
	for (auto &ao_it : m_active_objects) {
		f(ao_it.second);
	}
}

// clang-format off
bool ActiveObjectMgr::registerObject(ServerActiveObject *obj)
{
	assert(obj); // Pre-condition
	if (obj->getId() == 0) {
		u16 new_id = getFreeId();
		if (new_id == 0) {
			errorstream << "Server::ActiveObjectMgr::addActiveObjectRaw(): "
					<< "no free id available" << std::endl;
			if (obj->environmentDeletes())
				delete obj;
			return false;
		}
		obj->setId(new_id);
	} else {
		verbosestream << "Server::ActiveObjectMgr::addActiveObjectRaw(): "
				<< "supplied with id " << obj->getId() << std::endl;
	}

	if (!isFreeId(obj->getId())) {
		errorstream << "Server::ActiveObjectMgr::addActiveObjectRaw(): "
				<< "id is not free (" << obj->getId() << ")" << std::endl;
		if (obj->environmentDeletes())
			delete obj;
		return false;
	}

	if (objectpos_over_limit(obj->getBasePosition())) {
		v3f p = obj->getBasePosition();
		warningstream << "Server::ActiveObjectMgr::addActiveObjectRaw(): "
				<< "object position (" << p.X << "," << p.Y << "," << p.Z
				<< ") outside maximum range" << std::endl;
		if (obj->environmentDeletes())
			delete obj;
		return false;
	}

	m_active_objects[obj->getId()] = obj;

	verbosestream << "Server::ActiveObjectMgr::addActiveObjectRaw(): "
			<< "Added id=" << obj->getId() << "; there are now "
			<< m_active_objects.size() << " active objects." << std::endl;
	return true;
}

void ActiveObjectMgr::removeObject(u16 id)
{
	verbosestream << "Server::ActiveObjectMgr::removeObject(): "
			<< "id=" << id << std::endl;
	ServerActiveObject *obj = getActiveObject(id);
	if (!obj) {
		infostream << "Server::ActiveObjectMgr::removeObject(): "
				<< "id=" << id << " not found" << std::endl;
		return;
	}

	m_active_objects.erase(id);
	delete obj;
}

// clang-format on
void ActiveObjectMgr::getObjectsInsideRadius(const v3f &pos, float radius,
		std::vector<ServerActiveObject *> &result,
		std::function<bool(ServerActiveObject *obj)> include_obj_cb)
{
	float r2 = radius * radius;
	for (auto &activeObject : m_active_objects) {
		ServerActiveObject *obj = activeObject.second;
		const v3f &objectpos = obj->getBasePosition();
		if (objectpos.getDistanceFromSQ(pos) > r2)
			continue;

		if (!include_obj_cb || include_obj_cb(obj))
			result.push_back(obj);
	}
}

void ActiveObjectMgr::getObjectsInArea(const aabb3f &box,
		std::vector<ServerActiveObject *> &result,
		std::function<bool(ServerActiveObject *obj)> include_obj_cb)
{
	for (auto &activeObject : m_active_objects) {
		ServerActiveObject *obj = activeObject.second;
		const v3f &objectpos = obj->getBasePosition();
		if (!box.isPointInside(objectpos))
			continue;

		if (!include_obj_cb || include_obj_cb(obj))
			result.push_back(obj);
	}
}

void ActiveObjectMgr::getAddedActiveObjectsAroundPos(const v3f &player_pos, f32 radius,
		f32 player_radius, std::set<u16> &current_objects,
		std::queue<u16> &added_objects)
{
	/*
		Go through the object list,
		- discard removed/deactivated objects,
		- discard objects that are too far away,
		- discard objects that are found in current_objects.
		- add remaining objects to added_objects
	*/
	for (auto &ao_it : m_active_objects) {
		u16 id = ao_it.first;

		// Get object
		ServerActiveObject *object = ao_it.second;
		if (!object)
			continue;

		if (object->isGone())
			continue;

		f32 distance_f = object->getBasePosition().getDistanceFrom(player_pos);
		if (object->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
			// Discard if too far
			if (distance_f > player_radius && player_radius != 0)
				continue;
		} else if (distance_f > radius)
			continue;

		// Discard if already on current_objects
		auto n = current_objects.find(id);
		if (n != current_objects.end())
			continue;
		// Add to added_objects
		added_objects.push(id);
	}
}

} // namespace server

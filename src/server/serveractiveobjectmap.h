/*
Minetest
Copyright (C) 2018 numZero, Lobachevsky Vitaly <numzer0@yandex.com>

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
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "irr_v3d.h"
#include "irr_aabb3d.h"

class ServerActiveObject;

/*!
 * The class to speed up collision tests.
 *
 * @note It stores any objects but only those that has valid collision box
 * (`physical` Lua entities) are actually processed.
 * @note It uses world coordinate units, i.e. node size is always BS.
 */
struct ServerActiveObjectMap
{
	struct Wrapper
	{
		ServerActiveObject *object;
		aabb3s16 box;
		v3s16 pos;
		bool has_box;
	};

	/*!
	 * Adds object to the map. It must have valid ID.
	 *
	 * If an object with the same ID already exists in the map,
	 * std::logic_error is thrown.
	 */
	void addObject(ServerActiveObject *object);

	/*!
	 * Removes object from the map. The pointer must be valid.
	 * See `removeObject(u16)` for details.
	 */
	void removeObject(ServerActiveObject *object);

	/*!
	 * Removes object from the map.
	 *
	 * If the object is not found, the call is ignored.
	 * The function never throws, unless the underlying container throws.
	 */
	ServerActiveObject *removeObject(u16 id);

	/*!
	 * Updates object metadata stored in the map.
	 * See `updateObject(u16)` for details.
	 */
	void updateObject(ServerActiveObject *object);

	/*!
	 * Updates object metadata stored in the map.
	 *
	 * The metadata includes (approximate) absolute collision box and
	 * its existence (`physical` property for Lua entities).
	 * This function must be called after each change of these properties,
	 * including each object movement.
	 */
	void updateObject(u16 id);

	/*!
	 * Returns the object with given ID, if any.
	 * Returns NULL otherwise.
	 */
	ServerActiveObject *getObject(u16 id) const;

	/*!
	 * Checks if the given ID is free and valid (i.e. non-zero).
	 */
	bool isFreeId(u16 id);

	/*!
	 * Returns a free ID, if any. Returns 0 in the case of failure.
	 *
	 * @note This function doesn't reserve the ID; it remains free until
	 * an object with that ID is added.
	 * @note This function tries to reclaim freed IDs as late as possible.
	 * However, there is no guarantee.
	 */
	u16 getFreeId();

	/*!
	 * Returns a list of objects whose base position is at distance less
	 * than @p radius from @p pos.
	 *
	 * @note Due to inexact nature of floating-point computations, it is
	 * undefined whether an object lying exactly at the boundary is included
	 * in the list or not.
	 */
	std::vector<u16> getObjectsInsideRadius(v3f pos, float radius);

	/*!
	 * Returns a list of objects whose collision box intersects with @p box
	 *
	 * @note Due to inexact nature of floating-point computations, it is
	 * undefined whether an object lying exactly at the boundary is included
	 * in the list or not.
	 */
	std::vector<u16> getObjectsTouchingBox(const aabb3f &box);

	/*!
	 * Returns count of objects in the map.
	 */
	std::size_t size() const { return objects.size(); }

	/*!
	 * Returns reference to the underlying container.
	 */
	const std::unordered_map<u16, Wrapper> &getObjects() const { return objects; }

private:
	void addObjectRef(u16 id, v3s16 pos);
	void removeObjectRef(u16 id, v3s16 pos);
	void addObjectRefs(u16 id, const aabb3s16 &box);
	void removeObjectRefs(u16 id, const aabb3s16 &box);
	std::unordered_set<u16> getObjectsNearBox(const aabb3s16 &box);

	std::unordered_map<u16, Wrapper> objects;
	std::unordered_multimap<v3s16, u16> refmap;
};

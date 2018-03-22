/*
Minetest
Copyright (C) 2010-2017 numZero, Lobachevsky Vitaly <numzer0@yandex.com>

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

struct ServerActiveObjectMap
{
	struct Wrapper
	{
		ServerActiveObject *object;
		aabb3s16 box;
		bool has_box;
	};

	void addObject(ServerActiveObject *object);
	void removeObject(ServerActiveObject *object);
	ServerActiveObject *removeObject(u16 id);
	void updateObject(ServerActiveObject *object);
	void updateObject(u16 id);
	ServerActiveObject *getObject(u16 id) const;

	bool isFreeId(u16 id);
	u16 getFreeId();
	std::vector<u16> getObjectsInsideRadius(v3f pos, float radius);
	std::vector<u16> getObjectsTouchingBox(const aabb3f &box);

	std::size_t size() const
	{
		return objects.size();
	}

	const std::unordered_map<u16, Wrapper> &getObjects() const
	{
		return objects;
	}

private:
	void addObjectRefs(u16 id, const aabb3s16 &box);
	void removeObjectRefs(u16 id, const aabb3s16 &box);
	std::unordered_set<u16> getObjectsNearBox(const aabb3s16 &box);

	std::unordered_map<u16, Wrapper> objects;
	std::unordered_multimap<v3s16, u16> refmap;
};

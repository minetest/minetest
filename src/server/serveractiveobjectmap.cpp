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

#include "serveractiveobjectmap.h"
#include <cmath>
#include "constants.h"
#include "log.h"
#include "serverobject.h"

static constexpr float granularity = 16.0 * BS;

static v3s16 getChunkPos(const v3f &pos)
{
	return v3s16(
			std::floor(pos.X / granularity),
			std::floor(pos.Y / granularity),
			std::floor(pos.Z / granularity));
}

static aabb3s16 calcBox(const aabb3f &cb)
{
	return { getChunkPos(cb.MinEdge), getChunkPos(cb.MaxEdge) };
}

void ServerActiveObjectMap::addObject(ServerActiveObject *object)
{
	aabb3f cb;
	Wrapper w;
	u16 id = object->getId();
	if (!isFreeId(id))
		throw std::logic_error("ServerActiveObjectMap::addObject: "
			"object ID in use: " + std::to_string(id));
	w.object = object;
	w.has_box = w.object->getCollisionBox(&cb);
	w.pos = getChunkPos(w.object->getBasePosition());
	if (w.has_box) {
		w.box = calcBox(cb);
		addObjectRefs(id, w.box);
	}
	addObjectRef(id, w.pos);
	objects.emplace(id, w);
}

ServerActiveObject *ServerActiveObjectMap::removeObject(u16 id)
{
	auto pw = objects.find(id);
	if (pw == objects.end())
		return nullptr; // silently ignore non-existent object
	Wrapper w = pw->second;
	if (w.has_box)
		removeObjectRefs(id, w.box);
	removeObjectRef(id, w.pos);
	objects.erase(pw);
	return w.object;
}

void ServerActiveObjectMap::removeObject(ServerActiveObject *object)
{
	removeObject(object->getId());
}

void ServerActiveObjectMap::updateObject(u16 id)
{
	auto pw = objects.find(id);
	if (pw == objects.end()) {
		warningstream << "Trying to update non-existent object: " << id
				<< std::endl;
		return;
	}
	Wrapper &w = pw->second;
	v3s16 pos = getChunkPos(w.object->getBasePosition());
	aabb3f cb;
	aabb3s16 box;
	bool has_box = w.object->getCollisionBox(&cb);
	if (has_box)
		box = calcBox(cb);
	if (w.has_box && has_box && w.box == box && pos == w.pos)
		return;
	if (w.has_box)
		removeObjectRefs(id, w.box);
	removeObjectRef(id, w.pos);
	w.box = box;
	w.has_box = has_box;
	if (w.has_box)
		addObjectRefs(id, w.box);
	addObjectRef(id, w.pos);
}

void ServerActiveObjectMap::updateObject(ServerActiveObject *object)
{
	updateObject(object->getId());
}

ServerActiveObject *ServerActiveObjectMap::getObject(u16 id) const
{
	auto pw = objects.find(id);
	if (pw == objects.end())
		return nullptr;
	return pw->second.object;
}

std::vector<u16> ServerActiveObjectMap::getObjectsInsideRadius(v3f pos, float radius)
{
	std::vector<u16> result;
	auto nearby = getObjectsNearBox(calcBox({pos - radius, pos + radius}));
	for (auto &id : nearby) {
		ServerActiveObject *obj = getObject(id);
		v3f objectpos = obj->getBasePosition();
		if (objectpos.getDistanceFrom(pos) > radius)
			continue;
		result.push_back(id);
	}
	return result;
}

std::vector<u16> ServerActiveObjectMap::getObjectsTouchingBox(const aabb3f &box)
{
	std::vector<u16> result;
	auto nearby = getObjectsNearBox(calcBox(box));
	for (auto &id : nearby) {
		ServerActiveObject *obj = getObject(id);
		aabb3f cb;
		if (!obj->getCollisionBox(&cb))
			continue;
		if (!box.intersectsWithBox(cb))
			continue;
		result.push_back(id);
	}
	return result;
}

std::unordered_set<u16> ServerActiveObjectMap::getObjectsNearBox(const aabb3s16 &box)
{
	std::unordered_set<u16> result;
	v3s16 p;
	for (p.Z = box.MinEdge.Z; p.Z <= box.MaxEdge.Z; p.Z++)
	for (p.Y = box.MinEdge.Y; p.Y <= box.MaxEdge.Y; p.Y++)
	for (p.X = box.MinEdge.X; p.X <= box.MaxEdge.X; p.X++) {
		auto bounds = refmap.equal_range(p);
		for (auto iter = bounds.first; iter != bounds.second; ++iter)
			result.insert(iter->second);
	}
	return result;
}

void ServerActiveObjectMap::addObjectRef(u16 id, v3s16 pos)
{
	refmap.emplace(pos, id);
}

void ServerActiveObjectMap::removeObjectRef(u16 id, v3s16 pos)
{
	auto bounds = refmap.equal_range(pos);
	for (auto iter = bounds.first; iter != bounds.second;) {
		if (iter->second == id)
			iter = refmap.erase(iter);
		else
			++iter;
	}
}

void ServerActiveObjectMap::addObjectRefs(u16 id, const aabb3s16 &box)
{
	v3s16 p;
	for (p.Z = box.MinEdge.Z; p.Z <= box.MaxEdge.Z; p.Z++)
	for (p.Y = box.MinEdge.Y; p.Y <= box.MaxEdge.Y; p.Y++)
	for (p.X = box.MinEdge.X; p.X <= box.MaxEdge.X; p.X++)
		addObjectRef(id, p);
}

void ServerActiveObjectMap::removeObjectRefs(u16 id, const aabb3s16 &box)
{
	v3s16 p;
	for (p.Z = box.MinEdge.Z; p.Z <= box.MaxEdge.Z; p.Z++)
	for (p.Y = box.MinEdge.Y; p.Y <= box.MaxEdge.Y; p.Y++)
	for (p.X = box.MinEdge.X; p.X <= box.MaxEdge.X; p.X++)
		removeObjectRef(id, p);
}

bool ServerActiveObjectMap::isFreeId(u16 id)
{
	if (id == 0)
		return false;
	return objects.find(id) == objects.end();
}

u16 ServerActiveObjectMap::getFreeId()
{
	// try to reuse id's as late as possible
	static u16 last_used_id = 0;
	u16 startid = last_used_id;
	for (;;) {
		last_used_id++;
		if (isFreeId(last_used_id))
			return last_used_id;
		if (last_used_id == startid) // wrapped around
			return 0;
	}
}

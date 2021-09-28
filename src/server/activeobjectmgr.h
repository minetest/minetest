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

#pragma once

#include <functional>
#include <vector>
#include "../activeobjectmgr.h"
#include "serveractiveobject.h"

namespace server
{
class ActiveObjectMgr : public ::ActiveObjectMgr<ServerActiveObject>
{
public:
	void clear(const std::function<bool(ServerActiveObject *, u16)> &cb);
	void step(float dtime,
			const std::function<void(ServerActiveObject *)> &f) override;
	bool registerObject(ServerActiveObject *obj) override;
	void removeObject(u16 id) override;

	void getObjectsInsideRadius(const v3f &pos, float radius,
			std::vector<ServerActiveObject *> &result,
			std::function<bool(ServerActiveObject *obj)> include_obj_cb);
	void getObjectsInArea(const aabb3f &box,
			std::vector<ServerActiveObject *> &result,
			std::function<bool(ServerActiveObject *obj)> include_obj_cb);

	void getAddedActiveObjectsAroundPos(const v3f &player_pos, f32 radius,
			f32 player_radius, std::set<u16> &current_objects,
			std::queue<u16> &added_objects);
};
} // namespace server

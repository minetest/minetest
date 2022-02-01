/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "cpp_api/s_base.h"
#include "irr_v3d.h"

struct ObjectProperties;
struct ToolCapabilities;
struct collisionMoveResult;

class ScriptApiEntity
		: virtual public ScriptApiBase
{
public:
	bool luaentity_Add(u16 id, const char *name);
	void luaentity_Activate(u16 id,
			const std::string &staticdata, u32 dtime_s);
	void luaentity_Deactivate(u16 id);
	void luaentity_Remove(u16 id);
	std::string luaentity_GetStaticdata(u16 id);
	void luaentity_GetProperties(u16 id,
			ServerActiveObject *self, ObjectProperties *prop);
	void luaentity_Step(u16 id, float dtime,
		const collisionMoveResult *moveresult);
	bool luaentity_Punch(u16 id,
			ServerActiveObject *puncher, float time_from_last_punch,
			const ToolCapabilities *toolcap, v3f dir, s32 damage);
	bool luaentity_on_death(u16 id, ServerActiveObject *killer);
	void luaentity_Rightclick(u16 id, ServerActiveObject *clicker);
	void luaentity_on_attach_child(u16 id, ServerActiveObject *child);
	void luaentity_on_detach_child(u16 id, ServerActiveObject *child);
	void luaentity_on_detach(u16 id, ServerActiveObject *parent);
private:
	bool luaentity_run_simple_callback(u16 id, ServerActiveObject *sao,
		const char *field);
};

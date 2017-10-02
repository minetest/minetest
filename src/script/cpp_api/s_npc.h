/*
Minetest
Copyright (C) 2017 Vincent Glize, Dumbeldor <vincent.glize@live.fr>

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

#include "s_base.h"
#include "irr_v3d.h"

struct ObjectProperties;
struct ToolCapabilities;

class ScriptApiNpc : virtual public ScriptApiBase
{
public:
	bool luanpc_Add(u16 id, const char *name);
	void luanpc_Activate(u16 id,
							const std::string &staticdata, u32 dtime_s);
	void luanpc_Remove(u16 id);
	std::string luanpc_GetStaticdata(u16 id);
	void luanpc_GetProperties(u16 id,
								 ObjectProperties *prop);
	void luanpc_Step(u16 id, float dtime);
	bool luanpc_Punch(u16 id,
						 ServerActiveObject *puncher, float time_from_last_punch,
						 const ToolCapabilities *toolcap, v3f dir, s16 damage);
	bool luanpc_on_death(u16 id, ServerActiveObject *killer);
	void luanpc_Rightclick(u16 id,
							  ServerActiveObject *clicker);
};

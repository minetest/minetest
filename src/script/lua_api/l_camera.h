/*
Minetest
Copyright (C) 2013-2017 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "l_base.h"
#include "l_internal.h"

class Camera;

class LuaCamera : public ModApiBase
{
private:
	static const char className[];
	static const luaL_Reg methods[];

	// garbage collector
	ENTRY_POINT_DECL(gc_object);

	ENTRY_POINT_DECL(l_set_camera_mode);
	ENTRY_POINT_DECL(l_get_camera_mode);

	ENTRY_POINT_DECL(l_get_fov);

	ENTRY_POINT_DECL(l_get_pos);
	ENTRY_POINT_DECL(l_get_offset);
	ENTRY_POINT_DECL(l_get_look_dir);
	ENTRY_POINT_DECL(l_get_look_vertical);
	ENTRY_POINT_DECL(l_get_look_horizontal);
	ENTRY_POINT_DECL(l_get_aspect_ratio);

	Camera *m_camera = nullptr;

public:
	LuaCamera(Camera *m);
	~LuaCamera() = default;

	static void create(lua_State *L, Camera *m);

	static LuaCamera *checkobject(lua_State *L, int narg);
	static Camera *getobject(LuaCamera *ref);
	static Camera *getobject(lua_State *L, int narg);

	static void Register(lua_State *L);
};

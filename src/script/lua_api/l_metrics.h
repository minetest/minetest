/*
Minetest
Copyright (C) 2021  rubenwardy

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

#include "lua_api/l_base.h"
#include "config.h"

#if USE_PROMETHEUS

class ModApiMetrics : public ModApiBase
{
	// create_metric(type, name, help)
	static int l_create_metric(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};

#else

class ModApiMetrics : public ModApiBase
{
public:
	static void Initialize(lua_State *L, int top) {}
};

#endif

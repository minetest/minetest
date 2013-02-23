/*
Minetest-c55
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

#ifndef LUA_ENUM_H_
#define LUA_ENUM_H_

#include <iostream>

extern "C" {
#include <lua.h>
}

struct EnumString
{
	int num;
	const char *str;
};

extern struct EnumString es_ItemType[];
extern struct EnumString es_DrawType[];
extern struct EnumString es_ContentParamType[];
extern struct EnumString es_ContentParamType2[];
extern struct EnumString es_LiquidType[];
extern struct EnumString es_NodeBoxType[];
extern struct EnumString es_CraftMethod[];
extern struct EnumString es_TileAnimationType[];
extern struct EnumString es_BiomeTerrainType[];

bool string_to_enum(const EnumString *spec, int &result,
		const std::string &str);

int getenumfield(lua_State *L, int table,
		const char *fieldname, const EnumString *spec, int default_);
#endif /* LUA_ENUM_H_ */

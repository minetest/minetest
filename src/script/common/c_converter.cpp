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

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include "util/numeric.h"
#include "common/c_converter.h"
#include "constants.h"

void push_v3f(lua_State *L, v3f p)
{
	lua_newtable(L);
	lua_pushnumber(L, p.X);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, p.Y);
	lua_setfield(L, -2, "y");
	lua_pushnumber(L, p.Z);
	lua_setfield(L, -2, "z");
}

void push_v2f(lua_State *L, v2f p)
{
	lua_newtable(L);
	lua_pushnumber(L, p.X);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, p.Y);
	lua_setfield(L, -2, "y");
}

v2s16 read_v2s16(lua_State *L, int index)
{
	v2s16 p;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "x");
	p.X = lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "y");
	p.Y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return p;
}

v2s32 read_v2s32(lua_State *L, int index)
{
	v2s32 p;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "x");
	p.X = lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "y");
	p.Y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return p;
}

v2f read_v2f(lua_State *L, int index)
{
	v2f p;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "x");
	p.X = lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "y");
	p.Y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return p;
}

v3f read_v3f(lua_State *L, int index)
{
	v3f pos;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "x");
	pos.X = lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "y");
	pos.Y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "z");
	pos.Z = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return pos;
}

v3f check_v3f(lua_State *L, int index)
{
	v3f pos;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "x");
	pos.X = luaL_checknumber(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "y");
	pos.Y = luaL_checknumber(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "z");
	pos.Z = luaL_checknumber(L, -1);
	lua_pop(L, 1);
	return pos;
}

void pushFloatPos(lua_State *L, v3f p)
{
	p /= BS;
	push_v3f(L, p);
}

v3f checkFloatPos(lua_State *L, int index)
{
	return check_v3f(L, index) * BS;
}

void push_v3s16(lua_State *L, v3s16 p)
{
	lua_newtable(L);
	lua_pushnumber(L, p.X);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, p.Y);
	lua_setfield(L, -2, "y");
	lua_pushnumber(L, p.Z);
	lua_setfield(L, -2, "z");
}

v3s16 read_v3s16(lua_State *L, int index)
{
	// Correct rounding at <0
	v3f pf = read_v3f(L, index);
	return floatToInt(pf, 1.0);
}

v3s16 check_v3s16(lua_State *L, int index)
{
	// Correct rounding at <0
	v3f pf = check_v3f(L, index);
	return floatToInt(pf, 1.0);
}

video::SColor readARGB8(lua_State *L, int index)
{
	video::SColor color(0);
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "a");
	if(lua_isnumber(L, -1))
		color.setAlpha(lua_tonumber(L, -1));
	lua_pop(L, 1);
	lua_getfield(L, index, "r");
	color.setRed(lua_tonumber(L, -1));
	lua_pop(L, 1);
	lua_getfield(L, index, "g");
	color.setGreen(lua_tonumber(L, -1));
	lua_pop(L, 1);
	lua_getfield(L, index, "b");
	color.setBlue(lua_tonumber(L, -1));
	lua_pop(L, 1);
	return color;
}

aabb3f read_aabb3f(lua_State *L, int index, f32 scale)
{
	aabb3f box;
	if(lua_istable(L, index)){
		lua_rawgeti(L, index, 1);
		box.MinEdge.X = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, index, 2);
		box.MinEdge.Y = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, index, 3);
		box.MinEdge.Z = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, index, 4);
		box.MaxEdge.X = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, index, 5);
		box.MaxEdge.Y = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, index, 6);
		box.MaxEdge.Z = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
	}
	return box;
}

std::vector<aabb3f> read_aabb3f_vector(lua_State *L, int index, f32 scale)
{
	std::vector<aabb3f> boxes;
	if(lua_istable(L, index)){
		int n = lua_objlen(L, index);
		// Check if it's a single box or a list of boxes
		bool possibly_single_box = (n == 6);
		for(int i = 1; i <= n && possibly_single_box; i++){
			lua_rawgeti(L, index, i);
			if(!lua_isnumber(L, -1))
				possibly_single_box = false;
			lua_pop(L, 1);
		}
		if(possibly_single_box){
			// Read a single box
			boxes.push_back(read_aabb3f(L, index, scale));
		} else {
			// Read a list of boxes
			for(int i = 1; i <= n; i++){
				lua_rawgeti(L, index, i);
				boxes.push_back(read_aabb3f(L, -1, scale));
				lua_pop(L, 1);
			}
		}
	}
	return boxes;
}

bool read_stringlist(lua_State *L, int index, std::vector<const char *> &result)
{
	if (index < 0)
		index = lua_gettop(L) + 1 + index;

	if (lua_istable(L, index)) {
		lua_pushnil(L);
		while (lua_next(L, index)) {
			result.push_back(lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	} else if (lua_isstring(L, index)) {
		result.push_back(lua_tostring(L, index));
	} else {
		return false;
	}
	return true;
}

/*
	Table field getters
*/

bool getstringfield(lua_State *L, int table,
		const char *fieldname, std::string &result)
{
	lua_getfield(L, table, fieldname);
	bool got = false;
	if(lua_isstring(L, -1)){
		size_t len = 0;
		const char *ptr = lua_tolstring(L, -1, &len);
		if (ptr) {
			result.assign(ptr, len);
			got = true;
		}
	}
	lua_pop(L, 1);
	return got;
}

bool getintfield(lua_State *L, int table,
		const char *fieldname, int &result)
{
	lua_getfield(L, table, fieldname);
	bool got = false;
	if(lua_isnumber(L, -1)){
		result = lua_tonumber(L, -1);
		got = true;
	}
	lua_pop(L, 1);
	return got;
}

bool getfloatfield(lua_State *L, int table,
		const char *fieldname, float &result)
{
	lua_getfield(L, table, fieldname);
	bool got = false;
	if(lua_isnumber(L, -1)){
		result = lua_tonumber(L, -1);
		got = true;
	}
	lua_pop(L, 1);
	return got;
}

bool getboolfield(lua_State *L, int table,
		const char *fieldname, bool &result)
{
	lua_getfield(L, table, fieldname);
	bool got = false;
	if(lua_isboolean(L, -1)){
		result = lua_toboolean(L, -1);
		got = true;
	}
	lua_pop(L, 1);
	return got;
}

bool getstringlistfield(lua_State *L, int table, const char *fieldname,
		std::vector<const char *> &result)
{
	lua_getfield(L, table, fieldname);

	bool got = read_stringlist(L, -1, result);

	lua_pop(L, 1);
	return got;
}

std::string checkstringfield(lua_State *L, int table,
		const char *fieldname)
{
	lua_getfield(L, table, fieldname);
	std::string s = luaL_checkstring(L, -1);
	lua_pop(L, 1);
	return s;
}

std::string getstringfield_default(lua_State *L, int table,
		const char *fieldname, const std::string &default_)
{
	std::string result = default_;
	getstringfield(L, table, fieldname, result);
	return result;
}

int getintfield_default(lua_State *L, int table,
		const char *fieldname, int default_)
{
	int result = default_;
	getintfield(L, table, fieldname, result);
	return result;
}

float getfloatfield_default(lua_State *L, int table,
		const char *fieldname, float default_)
{
	float result = default_;
	getfloatfield(L, table, fieldname, result);
	return result;
}

bool getboolfield_default(lua_State *L, int table,
		const char *fieldname, bool default_)
{
	bool result = default_;
	getboolfield(L, table, fieldname, result);
	return result;
}

void setintfield(lua_State *L, int table,
		const char *fieldname, int value)
{
	lua_pushinteger(L, value);
	if(table < 0)
		table -= 1;
	lua_setfield(L, table, fieldname);
}

void setfloatfield(lua_State *L, int table,
		const char *fieldname, float value)
{
	lua_pushnumber(L, value);
	if(table < 0)
		table -= 1;
	lua_setfield(L, table, fieldname);
}

void setboolfield(lua_State *L, int table,
		const char *fieldname, bool value)
{
	lua_pushboolean(L, value);
	if(table < 0)
		table -= 1;
	lua_setfield(L, table, fieldname);
}



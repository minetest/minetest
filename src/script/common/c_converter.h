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


/******************************************************************************/
/******************************************************************************/
/* WARNING!!!! do NOT add this header in any include file or any code file    */
/*             not being a script/modapi file!!!!!!!!                         */
/******************************************************************************/
/******************************************************************************/
#pragma once

#include <vector>
#include <unordered_map>

#include "irrlichttypes_bloated.h"
#include "common/c_types.h"

extern "C" {
#include <lua.h>
}

std::string        getstringfield_default(lua_State *L, int table,
                             const char *fieldname, const std::string &default_);
bool               getboolfield_default(lua_State *L, int table,
                             const char *fieldname, bool default_);
float              getfloatfield_default(lua_State *L, int table,
                             const char *fieldname, float default_);
int                getintfield_default(lua_State *L, int table,
                             const char *fieldname, int default_);

template<typename T>
bool getintfield(lua_State *L, int table,
		const char *fieldname, T &result)
{
	lua_getfield(L, table, fieldname);
	bool got = false;
	if (lua_isnumber(L, -1)){
		result = lua_tointeger(L, -1);
		got = true;
	}
	lua_pop(L, 1);
	return got;
}

template<class T>
bool getv3intfield(lua_State *L, int index,
		const char *fieldname, T &result)
{
	lua_getfield(L, index, fieldname);
	bool got = false;
	if (lua_istable(L, -1)) {
		got |= getintfield(L, -1, "x", result.X);
		got |= getintfield(L, -1, "y", result.Y);
		got |= getintfield(L, -1, "z", result.Z);
	}
	lua_pop(L, 1);
	return got;
}

v3s16              getv3s16field_default(lua_State *L, int table,
                             const char *fieldname, v3s16 default_);
bool               getstringfield(lua_State *L, int table,
                             const char *fieldname, std::string &result);
size_t             getstringlistfield(lua_State *L, int table,
                             const char *fieldname,
                             std::vector<std::string> *result);
void               read_groups(lua_State *L, int index,
                             std::unordered_map<std::string, int> &result);
bool               getboolfield(lua_State *L, int table,
                             const char *fieldname, bool &result);
bool               getfloatfield(lua_State *L, int table,
                             const char *fieldname, float &result);
std::string        checkstringfield(lua_State *L, int table,
                             const char *fieldname);

void               setstringfield(lua_State *L, int table,
                             const char *fieldname, const std::string &value);
void               setintfield(lua_State *L, int table,
                             const char *fieldname, int value);
void               setfloatfield(lua_State *L, int table,
                             const char *fieldname, float value);
void               setboolfield(lua_State *L, int table,
                             const char *fieldname, bool value);

v3f                 checkFloatPos       (lua_State *L, int index);
v3f                 check_v3f           (lua_State *L, int index);
v3s16               check_v3s16         (lua_State *L, int index);

v3f                 read_v3f            (lua_State *L, int index);
v2f                 read_v2f            (lua_State *L, int index);
v2s16               read_v2s16          (lua_State *L, int index);
v2s32               read_v2s32          (lua_State *L, int index);
video::SColor       read_ARGB8          (lua_State *L, int index);
bool                read_color          (lua_State *L, int index,
                                         video::SColor *color);
bool                is_color_table      (lua_State *L, int index);

aabb3f              read_aabb3f         (lua_State *L, int index, f32 scale);
v3s16               read_v3s16          (lua_State *L, int index);
std::vector<aabb3f> read_aabb3f_vector  (lua_State *L, int index, f32 scale);
size_t              read_stringlist     (lua_State *L, int index,
                                         std::vector<std::string> *result);

void                push_float_string   (lua_State *L, float value);
void                push_v3_float_string(lua_State *L, v3f p);
void                push_v2_float_string(lua_State *L, v2f p);
void                push_v2s16          (lua_State *L, v2s16 p);
void                push_v2s32          (lua_State *L, v2s32 p);
void                push_v3s16          (lua_State *L, v3s16 p);
void                push_aabb3f         (lua_State *L, aabb3f box);
void                push_ARGB8          (lua_State *L, video::SColor color);
void                pushFloatPos        (lua_State *L, v3f p);
void                push_v3f            (lua_State *L, v3f p);
void                push_v2f            (lua_State *L, v2f p);

void                warn_if_field_exists(lua_State *L, int table,
                                         const char *fieldname,
                                         const std::string &message);

size_t write_array_slice_float(lua_State *L, int table_index, float *data,
	v3u16 data_size, v3u16 slice_offset, v3u16 slice_size);
size_t write_array_slice_u16(lua_State *L, int table_index, u16 *data,
	v3u16 data_size, v3u16 slice_offset, v3u16 slice_size);

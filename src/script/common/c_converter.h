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
#ifndef C_CONVERTER_H_
#define C_CONVERTER_H_

#include <vector>
#include <map>

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
int                getintfield_default           (lua_State *L, int table,
                             const char *fieldname, int default_);

bool               getstringfield(lua_State *L, int table,
                             const char *fieldname, std::string &result);
bool               getstringlistfield(lua_State *L, int table,
                             const char *fieldname,
                             std::vector<const char *> &result);
bool               getintfield(lua_State *L, int table,
                             const char *fieldname, int &result);
void               read_groups(lua_State *L, int index,
                             std::map<std::string, int> &result);
bool               getboolfield(lua_State *L, int table,
                             const char *fieldname, bool &result);
bool               getfloatfield(lua_State *L, int table,
                             const char *fieldname, float &result);

std::string        checkstringfield(lua_State *L, int table,
                             const char *fieldname);

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
video::SColor       readARGB8           (lua_State *L, int index);
aabb3f              read_aabb3f         (lua_State *L, int index, f32 scale);
v3s16               read_v3s16          (lua_State *L, int index);
std::vector<aabb3f> read_aabb3f_vector  (lua_State *L, int index, f32 scale);
bool                read_stringlist     (lua_State *L, int index,
                                         std::vector<const char *> &result);

void                push_v3s16          (lua_State *L, v3s16 p);
void                pushFloatPos        (lua_State *L, v3f p);
void                push_v3f            (lua_State *L, v3f p);
void                push_v2f            (lua_State *L, v2f p);



void                warn_if_field_exists      (lua_State *L,
                                              int table,
                                              const char *fieldname,
                                              const std::string &message);

#endif /* C_CONVERTER_H_ */

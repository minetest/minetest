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
#include "util/cpp11_container.h"

#include "irrlichttypes_bloated.h"
#include "common/c_types.h"
#include "util/numeric.h"
#include "util/serialize.h"

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
size_t             getstringlistfield(lua_State *L, int table,
                             const char *fieldname,
                             std::vector<std::string> *result);
bool               getintfield(lua_State *L, int table,
                             const char *fieldname, int &result);
bool               getintfield(lua_State *L, int table,
                             const char *fieldname, u8 &result);
bool               getintfield(lua_State *L, int table,
                             const char *fieldname, u16 &result);
bool               getintfield(lua_State *L, int table,
                             const char *fieldname, u32 &result);
void               read_groups(lua_State *L, int index,
                             UNORDERED_MAP<std::string, int> &result);
bool               getboolfield(lua_State *L, int table,
                             const char *fieldname, bool &result);
bool               getfloatfield(lua_State *L, int table,
                             const char *fieldname, float &result);

std::string        checkstringfield(lua_State *L, int table,
                             const char *fieldname);

void               setstringfield(lua_State *L, int table,
                             const char *fieldname, const char *value);
void               setintfield(lua_State *L, int table,
                             const char *fieldname, int value);
void               setfloatfield(lua_State *L, int table,
                             const char *fieldname, float value);
void               setboolfield(lua_State *L, int table,
                             const char *fieldname, bool value);

v3f                 checkFloatPos       (lua_State *L, int index);
v2f                 check_v2f           (lua_State *L, int index, float range_factor = 1);
v2s16               check_v2s16         (lua_State *L, int index);
v3f                 check_v3f           (lua_State *L, int index, float range_factor = 1);
v3s16               check_v3s16         (lua_State *L, int index);

void                check_range_f (lua_State *L, float f,
				float min, float max, float rmin, float rmax,
				const char *what = NULL);

inline void         check_range_f (lua_State *L, float f, float min, float max,
				const char *what = NULL)
			{ check_range_f(L, f, min, max, min, max, what); }

inline void         check_range_v2f (lua_State *L, v2f p,
				float min, float max, float rmin, float rmax)
			{
				check_range_f(L, p.X, min, max, rmin, rmax, "x");
				check_range_f(L, p.Y, min, max, rmin, rmax, "y");
			}

inline void         check_range_v2f (lua_State *L, v2f p, float min, float max)
			{ check_range_v2f(L, p, min, max, min, max); }

inline void         check_range_v3f (lua_State *L, v3f p,
				float min, float max, float rmin, float rmax)
			{
				check_range_f(L, p.X, min, max, rmin, rmax, "x");
				check_range_f(L, p.Y, min, max, rmin, rmax, "y");
				check_range_f(L, p.Z, min, max, rmin, rmax, "z");
			}

inline void         check_range_v3f (lua_State *L, v3f p, float min, float max)
			{ check_range_v3f(L, p, min, max, min, max); }

// Set pitch and yaw to 0 if abs. value larger than 1e5, so that limiting the
//     range does not suffer from floating point precision issues.
//     The value shouldn't be (much) larger than 360 anyway...
inline float        sanitize_yaw (float yaw)
			{ return (fabs(yaw) > 1e5 || fabs(yaw) < 1e-3) ? 0
				: (yaw - trunc(yaw / 360) * 360 + (yaw < 0 ? 360 : 0)); }
inline float        sanitize_pitch (float pitch)
			{ return (fabs(pitch) > 1e5 || fabs(pitch) < 1e-3) ? 0
				: pitch - trunc(pitch / 90) * 90; }

v3f                 read_v3f            (lua_State *L, int index);
v2f                 read_v2f            (lua_State *L, int index);
v2s16               read_v2s16          (lua_State *L, int index);
v2s32               read_v2s32          (lua_State *L, int index);
video::SColor       read_ARGB8          (lua_State *L, int index);
bool                read_color          (lua_State *L, int index,
                                         video::SColor *color);

aabb3f              read_aabb3f         (lua_State *L, int index, f32 scale);
v3s16               read_v3s16          (lua_State *L, int index);
std::vector<aabb3f> read_aabb3f_vector  (lua_State *L, int index, f32 scale);
size_t              read_stringlist     (lua_State *L, int index,
                                         std::vector<std::string> *result);

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

#endif /* C_CONVERTER_H_ */

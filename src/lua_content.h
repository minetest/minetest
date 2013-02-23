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

#ifndef LUA_CONTENT_H_
#define LUA_CONTENT_H_

extern "C" {
#include <lua.h>
}

#include "nodedef.h"

ContentFeatures    read_content_features         (lua_State *L, int index);
TileDef            read_tiledef                  (lua_State *L, int index);
void               read_soundspec                (lua_State *L, int index,
		SimpleSoundSpec &spec);
NodeBox            read_nodebox                  (lua_State *L, int index);

extern struct EnumString es_TileAnimationType[];

#endif /* LUA_CONTENT_H_ */

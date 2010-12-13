/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef MAIN_HEADER
#define MAIN_HEADER

#include <string>
extern std::string getTimestamp();
#define DTIME (getTimestamp()+": ")

#include <jmutex.h>

extern JMutex g_range_mutex;
extern s16 g_viewing_range_nodes;
//extern s16 g_actual_viewing_range_nodes;
extern bool g_viewing_range_all;

// Settings
extern Settings g_settings;

#include <fstream>

// Debug streams
extern std::ostream *dout_con_ptr;
extern std::ostream *derr_con_ptr;
extern std::ostream *dout_client_ptr;
extern std::ostream *derr_client_ptr;
extern std::ostream *dout_server_ptr;
extern std::ostream *derr_server_ptr;

#define dout_con (*dout_con_ptr)
#define derr_con (*derr_con_ptr)
#define dout_client (*dout_client_ptr)
#define derr_client (*derr_client_ptr)
#define dout_server (*dout_server_ptr)
#define derr_server (*derr_server_ptr)

// TODO: Move somewhere else? materials.h?
// This header is only for MATERIALS_COUNT
#include "mapnode.h"
extern video::SMaterial g_materials[MATERIALS_COUNT];

#include "utility.h"
extern TextureCache g_texturecache;

extern IrrlichtDevice *g_device;

#endif


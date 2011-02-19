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

#include "utility.h"

extern Settings g_settings;

void set_default_settings()
{
	// Client and server
	g_settings.setDefault("footprints", "false");

	// Client stuff
	g_settings.setDefault("wanted_fps", "30");
	g_settings.setDefault("fps_max", "60");
	g_settings.setDefault("viewing_range_nodes_max", "300");
	g_settings.setDefault("viewing_range_nodes_min", "35");
	g_settings.setDefault("screenW", "800");
	g_settings.setDefault("screenH", "600");
	g_settings.setDefault("port", "");
	g_settings.setDefault("address", "");
	g_settings.setDefault("name", "");
	g_settings.setDefault("random_input", "false");
	g_settings.setDefault("client_delete_unused_sectors_timeout", "1200");
	g_settings.setDefault("enable_fog", "true");
	g_settings.setDefault("new_style_water", "false");
	g_settings.setDefault("new_style_leaves", "true");
	g_settings.setDefault("frametime_graph", "false");
	g_settings.setDefault("enable_texture_atlas", "true");
	g_settings.setDefault("texture_path", "");
	g_settings.setDefault("video_driver", "opengl");
	
	g_settings.setDefault("free_move", "false");
	g_settings.setDefault("continuous_forward", "false");
	g_settings.setDefault("fast_move", "false");

	// Server stuff
	g_settings.setDefault("creative_mode", "false");

	g_settings.setDefault("objectdata_interval", "0.2");
	g_settings.setDefault("active_object_range", "2");
	g_settings.setDefault("max_simultaneous_block_sends_per_client", "1");
	//g_settings.setDefault("max_simultaneous_block_sends_per_client", "2");
	g_settings.setDefault("max_simultaneous_block_sends_server_total", "4");
	g_settings.setDefault("max_block_send_distance", "7");
	g_settings.setDefault("max_block_generate_distance", "7");
	g_settings.setDefault("time_send_interval", "20");
	g_settings.setDefault("time_speed", "96");
	g_settings.setDefault("server_unload_unused_sectors_timeout", "60");
	g_settings.setDefault("server_map_save_interval", "60");
	g_settings.setDefault("full_block_send_enable_min_time_from_building", "2.0");
}


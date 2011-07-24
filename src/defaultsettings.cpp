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

	g_settings.setDefault("port", "");
	g_settings.setDefault("name", "");
	g_settings.setDefault("footprints", "false");

	// Client stuff

	g_settings.setDefault("keymap_forward", "KEY_KEY_W");
	g_settings.setDefault("keymap_backward", "KEY_KEY_S");
	g_settings.setDefault("keymap_left", "KEY_KEY_A");
	g_settings.setDefault("keymap_right", "KEY_KEY_D");
	g_settings.setDefault("keymap_jump", "KEY_SPACE");
	g_settings.setDefault("keymap_sneak", "KEY_LSHIFT");
	g_settings.setDefault("keymap_inventory", "KEY_KEY_I");
	g_settings.setDefault("keymap_chat", "KEY_KEY_T");
	g_settings.setDefault("keymap_rangeselect", "KEY_KEY_R");
	g_settings.setDefault("keymap_freemove", "KEY_KEY_K");
	g_settings.setDefault("keymap_fastmove", "KEY_KEY_J");
	g_settings.setDefault("keymap_frametime_graph", "KEY_F1");
	g_settings.setDefault("keymap_screenshot", "KEY_F12");
	// Some (temporary) keys for debugging
	g_settings.setDefault("keymap_special1", "KEY_KEY_E");
	g_settings.setDefault("keymap_print_debug_stacks", "KEY_KEY_P");

	g_settings.setDefault("wanted_fps", "30");
	g_settings.setDefault("fps_max", "60");
	g_settings.setDefault("viewing_range_nodes_max", "300");
	g_settings.setDefault("viewing_range_nodes_min", "15");
	g_settings.setDefault("screenW", "800");
	g_settings.setDefault("screenH", "600");
	g_settings.setDefault("address", "");
	g_settings.setDefault("random_input", "false");
	g_settings.setDefault("client_unload_unused_data_timeout", "600");
	g_settings.setDefault("enable_fog", "true");
	g_settings.setDefault("new_style_water", "false");
	g_settings.setDefault("new_style_leaves", "true");
	g_settings.setDefault("smooth_lighting", "true");
	g_settings.setDefault("frametime_graph", "false");
	g_settings.setDefault("enable_texture_atlas", "true");
	g_settings.setDefault("texture_path", "");
	g_settings.setDefault("video_driver", "opengl");
	g_settings.setDefault("free_move", "false");
	g_settings.setDefault("continuous_forward", "false");
	g_settings.setDefault("fast_move", "false");
	g_settings.setDefault("invert_mouse", "false");
	g_settings.setDefault("enable_farmesh", "false");
	g_settings.setDefault("farmesh_trees", "true");
	g_settings.setDefault("farmesh_distance", "40");
	g_settings.setDefault("enable_clouds", "true");
	g_settings.setDefault("invisible_stone", "false");
	g_settings.setDefault("screenshot_path", ".");

	// Server stuff
	g_settings.setDefault("enable_experimental", "false");
	g_settings.setDefault("creative_mode", "false");
	g_settings.setDefault("enable_damage", "false"); //TODO: Set to true when healing is possible
	g_settings.setDefault("give_initial_stuff", "false");
	g_settings.setDefault("default_password", "");
	g_settings.setDefault("default_privs", "build, shout");
	g_settings.setDefault("profiler_print_interval", "0");
	g_settings.setDefault("enable_mapgen_debug_info", "false");

	g_settings.setDefault("objectdata_interval", "0.2");
	g_settings.setDefault("active_object_range", "2");
	//g_settings.setDefault("max_simultaneous_block_sends_per_client", "1");
	// This causes frametime jitter on client side, or does it?
	g_settings.setDefault("max_simultaneous_block_sends_per_client", "2");
	g_settings.setDefault("max_simultaneous_block_sends_server_total", "8");
	g_settings.setDefault("max_block_send_distance", "8");
	g_settings.setDefault("max_block_generate_distance", "8");
	g_settings.setDefault("time_send_interval", "20");
	g_settings.setDefault("time_speed", "96");
	g_settings.setDefault("server_unload_unused_data_timeout", "60");
	g_settings.setDefault("server_map_save_interval", "60");
	g_settings.setDefault("full_block_send_enable_min_time_from_building", "2.0");
	//g_settings.setDefault("dungeon_rarity", "0.025");
}


/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "settings.h"

void set_default_settings(Settings *settings)
{
	// Client and server

	settings->setDefault("port", "");
	settings->setDefault("name", "");

	// Client stuff

	settings->setDefault("keymap_forward", "KEY_KEY_W");
	settings->setDefault("keymap_backward", "KEY_KEY_S");
	settings->setDefault("keymap_left", "KEY_KEY_A");
	settings->setDefault("keymap_right", "KEY_KEY_D");
	settings->setDefault("keymap_jump", "KEY_SPACE");
	settings->setDefault("keymap_sneak", "KEY_LSHIFT");
	settings->setDefault("keymap_drop", "KEY_KEY_Q");
	settings->setDefault("keymap_inventory", "KEY_KEY_I");
	settings->setDefault("keymap_special1", "KEY_KEY_E");
	settings->setDefault("keymap_chat", "KEY_KEY_T");
	settings->setDefault("keymap_cmd", "/");
	settings->setDefault("keymap_console", "KEY_F10");
	settings->setDefault("keymap_rangeselect", "KEY_KEY_R");
	settings->setDefault("keymap_freemove", "KEY_KEY_K");
	settings->setDefault("keymap_fastmove", "KEY_KEY_J");
	settings->setDefault("keymap_screenshot", "KEY_F12");
	settings->setDefault("keymap_toggle_hud", "KEY_F1");
	settings->setDefault("keymap_toggle_chat", "KEY_F2");
	settings->setDefault("keymap_toggle_force_fog_off", "KEY_F3");
	settings->setDefault("keymap_toggle_update_camera", "KEY_F4");
	settings->setDefault("keymap_toggle_debug", "KEY_F5");
	settings->setDefault("keymap_toggle_profiler", "KEY_F6");
	settings->setDefault("keymap_increase_viewing_range_min", "+");
	settings->setDefault("keymap_decrease_viewing_range_min", "-");
	settings->setDefault("aux1_descends", "false");
	// Some (temporary) keys for debugging
	settings->setDefault("keymap_print_debug_stacks", "KEY_KEY_P");
	settings->setDefault("keymap_quicktune_prev", "KEY_HOME");
	settings->setDefault("keymap_quicktune_next", "KEY_END");
	settings->setDefault("keymap_quicktune_dec", "KEY_NEXT");
	settings->setDefault("keymap_quicktune_inc", "KEY_PRIOR");

	// Show debug info by default?
	#ifdef NDEBUG
	settings->setDefault("show_debug", "false");
	#else
	settings->setDefault("show_debug", "true");
	#endif

	settings->setDefault("wanted_fps", "30");
	settings->setDefault("fps_max", "60");
	// A bit more than the server will send around the player, to make fog blend well
	settings->setDefault("viewing_range_nodes_max", "160");
	settings->setDefault("viewing_range_nodes_min", "35");
	settings->setDefault("screenW", "800");
	settings->setDefault("screenH", "600");
	settings->setDefault("fullscreen", "false");
	settings->setDefault("fullscreen_bpp", "24");
	settings->setDefault("fsaa", "0");
	settings->setDefault("vsync", "false");
	settings->setDefault("address", "");
	settings->setDefault("random_input", "false");
	settings->setDefault("client_unload_unused_data_timeout", "600");
	settings->setDefault("enable_fog", "true");
	settings->setDefault("fov", "72");
	settings->setDefault("view_bobbing", "true");
	settings->setDefault("new_style_water", "false");
	settings->setDefault("new_style_leaves", "true");
	settings->setDefault("smooth_lighting", "true");
	settings->setDefault("enable_texture_atlas", "true");
	settings->setDefault("texture_path", "");
	settings->setDefault("video_driver", "opengl");
	settings->setDefault("free_move", "false");
	settings->setDefault("continuous_forward", "false");
	settings->setDefault("fast_move", "false");
	settings->setDefault("invert_mouse", "false");
	settings->setDefault("enable_farmesh", "false");
	settings->setDefault("enable_clouds", "true");
	settings->setDefault("screenshot_path", ".");
	settings->setDefault("view_bobbing_amount", "1.0");
	settings->setDefault("enable_3d_clouds", "true");
	settings->setDefault("opaque_water", "false");
	settings->setDefault("console_color", "(0,0,0)");
	settings->setDefault("console_alpha", "200");
	settings->setDefault("enable_sound", "true");
	settings->setDefault("sound_volume", "0.8");
	settings->setDefault("desynchronize_mapblock_texture_animation", "true");

	// Server stuff
	// "map-dir" doesn't exist by default.
	settings->setDefault("default_game", "minetest");
	settings->setDefault("motd", "");
	settings->setDefault("max_users", "100");
	settings->setDefault("strict_protocol_version_checking", "true");
	settings->setDefault("creative_mode", "false");
	settings->setDefault("enable_damage", "true");
	settings->setDefault("only_peaceful_mobs", "false");
	settings->setDefault("fixed_map_seed", "");
	settings->setDefault("give_initial_stuff", "false");
	settings->setDefault("default_password", "");
	settings->setDefault("default_privs", "interact, shout");
	settings->setDefault("unlimited_player_transfer_distance", "true");
	settings->setDefault("enable_pvp", "true");
	settings->setDefault("disallow_empty_password", "false");
	settings->setDefault("disable_anticheat", "false");
	settings->setDefault("enable_rollback_recording", "false");

	settings->setDefault("profiler_print_interval", "0");
	settings->setDefault("enable_mapgen_debug_info", "false");
	settings->setDefault("active_object_send_range_blocks", "3");
	settings->setDefault("active_block_range", "2");
	//settings->setDefault("max_simultaneous_block_sends_per_client", "1");
	// This causes frametime jitter on client side, or does it?
	settings->setDefault("max_simultaneous_block_sends_per_client", "4");
	settings->setDefault("max_simultaneous_block_sends_server_total", "20");
	settings->setDefault("max_block_send_distance", "9");
	settings->setDefault("max_block_generate_distance", "7");
	settings->setDefault("time_send_interval", "5");
	settings->setDefault("time_speed", "72");
	settings->setDefault("server_unload_unused_data_timeout", "29");
	settings->setDefault("server_map_save_interval", "5.3");
	settings->setDefault("full_block_send_enable_min_time_from_building", "2.0");
	settings->setDefault("dedicated_server_step", "0.05");
	settings->setDefault("ignore_world_load_errors", "false");
}


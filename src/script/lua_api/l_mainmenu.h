/*
Minetest
Copyright (C) 2013 sapier

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

#pragma once

#include "lua_api/l_base.h"
#include "lua_api/l_internal.h"

class AsyncEngine;

/** Implementation of lua api support for mainmenu */
class ModApiMainMenu: public ModApiBase
{

private:
	/**
	 * read a text variable from gamedata table within lua stack
	 * @param L stack to read variable from
	 * @param name name of variable to read
	 * @return string value of requested variable
	 */
	static std::string getTextData(lua_State *L, std::string name);

	/**
	 * read a integer variable from gamedata table within lua stack
	 * @param L stack to read variable from
	 * @param name name of variable to read
	 * @return integer value of requested variable
	 */
	static int getIntegerData(lua_State *L, std::string name,bool& valid);

	/**
	 * read a bool variable from gamedata table within lua stack
	 * @param L stack to read variable from
	 * @param name name of variable to read
	 * @return bool value of requested variable
	 */
	static int getBoolData(lua_State *L, std::string name,bool& valid);

	/**
	 * Checks if a path may be modified. Paths in the temp directory or the user
	 * games, mods, textures, or worlds directories may be modified.
	 * @param path path to check
	 * @return true if the path may be modified
	 */
	static bool mayModifyPath(std::string path);

	//api calls

	ENTRY_POINT_DECL(l_start);

	ENTRY_POINT_DECL(l_close);

	ENTRY_POINT_DECL(l_create_world);

	ENTRY_POINT_DECL(l_delete_world);

	ENTRY_POINT_DECL(l_get_worlds);

	ENTRY_POINT_DECL(l_get_mapgen_names);

	ENTRY_POINT_DECL(l_gettext);

	//packages

	ENTRY_POINT_DECL(l_get_games);

	ENTRY_POINT_DECL(l_get_content_info);

	//gui

	ENTRY_POINT_DECL(l_show_keys_menu);

	ENTRY_POINT_DECL(l_show_path_select_dialog);

	ENTRY_POINT_DECL(l_set_topleft_text);

	ENTRY_POINT_DECL(l_set_clouds);

	ENTRY_POINT_DECL(l_get_textlist_index);

	ENTRY_POINT_DECL(l_get_table_index);

	ENTRY_POINT_DECL(l_set_background);

	ENTRY_POINT_DECL(l_update_formspec);

	ENTRY_POINT_DECL(l_set_formspec_prepend);

	ENTRY_POINT_DECL(l_get_screen_info);

	//filesystem

	ENTRY_POINT_DECL(l_get_mainmenu_path);

	ENTRY_POINT_DECL(l_get_user_path);

	ENTRY_POINT_DECL(l_get_modpath);

	ENTRY_POINT_DECL(l_get_modpaths);

	ENTRY_POINT_DECL(l_get_clientmodpath);

	ENTRY_POINT_DECL(l_get_gamepath);

	ENTRY_POINT_DECL(l_get_texturepath);

	ENTRY_POINT_DECL(l_get_texturepath_share);

	ENTRY_POINT_DECL(l_get_cache_path);

	ENTRY_POINT_DECL(l_get_temp_path);

	ENTRY_POINT_DECL(l_create_dir);

	ENTRY_POINT_DECL(l_delete_dir);

	ENTRY_POINT_DECL(l_copy_dir);

	ENTRY_POINT_DECL(l_is_dir);

	ENTRY_POINT_DECL(l_extract_zip);

	ENTRY_POINT_DECL(l_may_modify_path);

	ENTRY_POINT_DECL(l_download_file);

	ENTRY_POINT_DECL(l_get_video_drivers);

	//version compatibility
	ENTRY_POINT_DECL(l_get_min_supp_proto);

	ENTRY_POINT_DECL(l_get_max_supp_proto);

	// other
	ENTRY_POINT_DECL(l_open_url);

	ENTRY_POINT_DECL(l_open_dir);


	// async
	ENTRY_POINT_DECL(l_do_async_callback);

public:

	/**
	 * initialize this API module
	 * @param L lua stack to initialize
	 * @param top index (in lua stack) of global API table
	 */
	static void Initialize(lua_State *L, int top);

	static void InitializeAsync(lua_State *L, int top);

};

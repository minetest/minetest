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

#pragma once

#include "lua_api/l_base.h"
#include "lua_api/l_internal.h"

class AsyncEngine;

class ModApiUtil : public ModApiBase
{
private:
	/*
		NOTE:
		The functions in this module are available in the in-game API
		as well as in the mainmenu API.

		All functions that don't require either a Server or
		GUIEngine instance should be in here.
	*/

	// log([level,] text)
	// Writes a line to the logger.
	// The one-argument version logs to LL_NONE.
	// The two-argument version accepts a log level.
	ENTRY_POINT_DECL(l_log);

	// get us precision time
	ENTRY_POINT_DECL(l_get_us_time);

	// parse_json(str[, nullvalue])
	ENTRY_POINT_DECL(l_parse_json);

	// write_json(data[, styled])
	ENTRY_POINT_DECL(l_write_json);

	// get_dig_params(groups, tool_capabilities[, wear])
	ENTRY_POINT_DECL(l_get_dig_params);

	// get_hit_params(groups, tool_capabilities[, time_from_last_punch[, wear]])
	ENTRY_POINT_DECL(l_get_hit_params);

	// check_password_entry(name, entry, password)
	ENTRY_POINT_DECL(l_check_password_entry);

	// get_password_hash(name, raw_password)
	ENTRY_POINT_DECL(l_get_password_hash);

	// is_yes(arg)
	ENTRY_POINT_DECL(l_is_yes);

	// get_builtin_path()
	ENTRY_POINT_DECL(l_get_builtin_path);

	// get_user_path()
	ENTRY_POINT_DECL(l_get_user_path);

	// compress(data, method, ...)
	ENTRY_POINT_DECL(l_compress);

	// decompress(data, method, ...)
	ENTRY_POINT_DECL(l_decompress);

	// mkdir(path)
	ENTRY_POINT_DECL(l_mkdir);

	// rmdir(path, recursive)
	ENTRY_POINT_DECL(l_rmdir);

	// cpdir(source, destination, remove_source)
	ENTRY_POINT_DECL(l_cpdir);

	// mvdir(source, destination)
	ENTRY_POINT_DECL(l_mvdir);

	// get_dir_list(path, is_dir)
	ENTRY_POINT_DECL(l_get_dir_list);

	// safe_file_write(path, content)
	ENTRY_POINT_DECL(l_safe_file_write);

	// request_insecure_environment()
	ENTRY_POINT_DECL(l_request_insecure_environment);

	// encode_base64(string)
	ENTRY_POINT_DECL(l_encode_base64);

	// decode_base64(string)
	ENTRY_POINT_DECL(l_decode_base64);

	// get_version()
	ENTRY_POINT_DECL(l_get_version);

	// sha1(string, raw)
	ENTRY_POINT_DECL(l_sha1);

	// colorspec_to_colorstring(colorspec)
	ENTRY_POINT_DECL(l_colorspec_to_colorstring);

	// colorspec_to_bytes(colorspec)
	ENTRY_POINT_DECL(l_colorspec_to_bytes);

	// encode_png(w, h, data, level)
	ENTRY_POINT_DECL(l_encode_png);

	// get_last_run_mod()
	ENTRY_POINT_DECL(l_get_last_run_mod);

	// set_last_run_mod(modname)
	ENTRY_POINT_DECL(l_set_last_run_mod);

public:
	static void Initialize(lua_State *L, int top);
	static void InitializeAsync(lua_State *L, int top);
	static void InitializeClient(lua_State *L, int top);

	static void InitializeAsync(AsyncEngine &engine);
};

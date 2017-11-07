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

#ifndef L_UTIL_H_
#define L_UTIL_H_

#include "lua_api/l_base.h"

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
	// The one-argument version logs to infostream.
	// The two-argument version accepts a log level.
	static int l_log(lua_State *L);

	// get us precision time
	static int l_get_us_time(lua_State *L);

	// parse_json(str[, nullvalue])
	static int l_parse_json(lua_State *L);

	// write_json(data[, styled])
	static int l_write_json(lua_State *L);

	// get_dig_params(groups, tool_capabilities[, time_from_last_punch])
	static int l_get_dig_params(lua_State *L);

	// get_hit_params(groups, tool_capabilities[, time_from_last_punch])
	static int l_get_hit_params(lua_State *L);

	// check_password_entry(name, entry, password)
	static int l_check_password_entry(lua_State *L);

	// get_password_hash(name, raw_password)
	static int l_get_password_hash(lua_State *L);

	// is_yes(arg)
	static int l_is_yes(lua_State *L);

	// get_builtin_path()
	static int l_get_builtin_path(lua_State *L);

	// compress(data, method, ...)
	static int l_compress(lua_State *L);

	// decompress(data, method, ...)
	static int l_decompress(lua_State *L);

	// mkdir(path)
	static int l_mkdir(lua_State *L);

	// get_dir_list(path, is_dir)
	static int l_get_dir_list(lua_State *L);

	// safe_file_write(path, content)
	static int l_safe_file_write(lua_State *L);

	// request_insecure_environment()
	static int l_request_insecure_environment(lua_State *L);

	// encode_base64(string)
	static int l_encode_base64(lua_State *L);

	// decode_base64(string)
	static int l_decode_base64(lua_State *L);

	// get_version()
	static int l_get_version(lua_State *L);

	// sha1(string, raw)
	static int l_sha1(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
	static void InitializeAsync(lua_State *L, int top);
	static void InitializeClient(lua_State *L, int top);

	static void InitializeAsync(AsyncEngine &engine);
};

#endif /* L_UTIL_H_ */

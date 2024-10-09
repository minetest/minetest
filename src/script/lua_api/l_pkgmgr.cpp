/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "../../content/pkgmgr.h"
#include "cpp_api/s_base.h"
#include "common/c_content.h"
#include "common/c_converter.h"
#include "cpp_api/s_base.h"
#include "gettext.h"
#include "l_internal.h"
#include "l_pkgmgr.h"

using namespace content;

int ModApiPkgMgr::l_get_folder_type(lua_State* L)
{
	const std::string path = luaL_checkstring(L, 1);
	
	ContentType type = getContentType(path);
	
	/* The original pkgmgr.lua returned nil, but UNKNOWN was unused, so we
	 *  just ignore that entirely */
	if (type != ContentType::UNKNOWN)
	{
		lua_newtable(L);
		setstringfield(L, -1, "type", content_type_to_string(type).c_str());
		setstringfield(L, -1, "path", path.c_str());
	}
	else
		lua_pushnil(L);
	return 1;
}

int ModApiPkgMgr::l_is_valid_modname(lua_State *L)
{
	lua_pushboolean(L, PkgMgr::isValidModname(luaL_checkstring(L, 1)));
	return 1;
}

int ModApiPkgMgr::l_get_contentdb_id(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	
	ContentSpec spec;
	spec.type = getstringfield_default(L, 1, "type", "");
	spec.author = getstringfield_default(L, 1, "author", "");
	spec.release = getintfield_default(L, 1, "release", 0);
	spec.id = getstringfield_default(L, 1, "id", "");
	spec.name = getstringfield_default(L, 1, "name", "");
	
	std::string res = PkgMgr::getContentdbId(spec);
	
	if (!res.empty())
		lua_pushstring(L, res.c_str());
	else
		lua_pushnil(L);
		
	return 1;
}

void ModApiPkgMgr::Initialize(lua_State* L)
{
	lua_getglobal(L, "pkgmgr");
	int top = lua_gettop(L);
	
	API_FCT(get_folder_type);
	API_FCT(is_valid_modname);
	API_FCT(get_contentdb_id);
}


void ModApiPkgMgr::InitializeAsync(lua_State *L, int top)
{
	
}

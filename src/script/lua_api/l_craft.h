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

#include <string>
#include <vector>

#include "lua_api/l_base.h"
#include "lua_api/l_internal.h"

struct CraftReplacements;

class ModApiCraft : public ModApiBase {
private:
	ENTRY_POINT_DECL(l_register_craft);
	ENTRY_POINT_DECL(l_get_craft_recipe);
	ENTRY_POINT_DECL(l_get_all_craft_recipes);
	ENTRY_POINT_DECL(l_get_craft_result);
	ENTRY_POINT_DECL(l_clear_craft);

	static bool readCraftReplacements(lua_State *L, int index,
			CraftReplacements &replacements);
	static bool readCraftRecipeShapeless(lua_State *L, int index,
			std::vector<std::string> &recipe);
	static bool readCraftRecipeShaped(lua_State *L, int index,
			int &width, std::vector<std::string> &recipe);

	static struct EnumString es_CraftMethod[];

public:
	static void Initialize(lua_State *L, int top);
};

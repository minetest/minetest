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


#include "lua_api/l_craft.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_item.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "server.h"
#include "craftdef.h"

struct EnumString ModApiCraft::es_CraftMethod[] =
{
	{CRAFT_METHOD_NORMAL, "normal"},
	{CRAFT_METHOD_COOKING, "cooking"},
	{CRAFT_METHOD_FUEL, "fuel"},
	{0, NULL},
};

// register_craft({output=item, recipe={{item00,item10},{item01,item11}})
int ModApiCraft::l_register_craft(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	//infostream<<"register_craft"<<std::endl;


	// Get the writable craft definition manager from the server
	IWritableCraftDefManager *craftdef =
			getServer(L)->getWritableCraftDefManager();

	CraftDefinition* def = read_craft(L,1);

	if (def != 0)
		craftdef->registerCraft(def);

	lua_pop(L, 1);
	return 0; /* number of results */
}

// remove_craft({output=item, recipe={{item00,item10},{item01,item11}})
int ModApiCraft::l_remove_craft(lua_State *L)
{
	// Get the writable craft definition manager from the server
	IWritableCraftDefManager *craftdef =
			getServer(L)->getWritableCraftDefManager();

	CraftDefinition* def = read_craft(L,1);

	if (def != 0)
		craftdef->removeCraft(def);

	lua_pop(L, 1);
	return 0; /* number of results */
}

// get_craft_result(input)
int ModApiCraft::l_get_craft_result(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	int input_i = 1;
	std::string method_s = getstringfield_default(L, input_i, "method", "normal");
	enum CraftMethod method = (CraftMethod)getenumfield(L, input_i, "method",
				es_CraftMethod, CRAFT_METHOD_NORMAL);
	int width = 1;
	lua_getfield(L, input_i, "width");
	if(lua_isnumber(L, -1))
		width = luaL_checkinteger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, input_i, "items");
	std::vector<ItemStack> items = read_items(L, -1,getServer(L));
	lua_pop(L, 1); // items

	IGameDef *gdef = getServer(L);
	ICraftDefManager *cdef = gdef->cdef();
	CraftInput input(method, width, items);
	CraftOutput output;
	bool got = cdef->getCraftResult(input, output, true, gdef);
	lua_newtable(L); // output table
	if(got){
		ItemStack item;
		item.deSerialize(output.item, gdef->idef());
		LuaItemStack::create(L, item);
		lua_setfield(L, -2, "item");
		setintfield(L, -1, "time", output.time);
	} else {
		LuaItemStack::create(L, ItemStack());
		lua_setfield(L, -2, "item");
		setintfield(L, -1, "time", 0);
	}
	lua_newtable(L); // decremented input table
	lua_pushstring(L, method_s.c_str());
	lua_setfield(L, -2, "method");
	lua_pushinteger(L, width);
	lua_setfield(L, -2, "width");
	push_items(L, input.items);
	lua_setfield(L, -2, "items");
	return 2;
}

// get_craft_recipe(result item)
int ModApiCraft::l_get_craft_recipe(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	int k = 1;
	int input_i = 1;
	std::string o_item = luaL_checkstring(L,input_i);

	IGameDef *gdef = getServer(L);
	ICraftDefManager *cdef = gdef->cdef();
	CraftInput input;
	CraftOutput output(o_item,0);
	bool got = cdef->getCraftRecipe(input, output, gdef);
	lua_newtable(L); // output table
	if(got){
		lua_newtable(L);
		for(std::vector<ItemStack>::const_iterator
			i = input.items.begin();
			i != input.items.end(); i++, k++)
		{
			if (i->empty())
			{
				continue;
			}
			lua_pushinteger(L,k);
			lua_pushstring(L,i->name.c_str());
			lua_settable(L, -3);
		}
		lua_setfield(L, -2, "items");
		setintfield(L, -1, "width", input.width);
		switch (input.method) {
		case CRAFT_METHOD_NORMAL:
			lua_pushstring(L,"normal");
			break;
		case CRAFT_METHOD_COOKING:
			lua_pushstring(L,"cooking");
			break;
		case CRAFT_METHOD_FUEL:
			lua_pushstring(L,"fuel");
			break;
		default:
			lua_pushstring(L,"unknown");
		}
		lua_setfield(L, -2, "type");
	} else {
		lua_pushnil(L);
		lua_setfield(L, -2, "items");
		setintfield(L, -1, "width", 0);
	}
	return 1;
}

// get_all_craft_recipes(result item)
int ModApiCraft::l_get_all_craft_recipes(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	std::string o_item = luaL_checkstring(L,1);
	IGameDef *gdef = getServer(L);
	ICraftDefManager *cdef = gdef->cdef();
	CraftInput input;
	CraftOutput output(o_item,0);
	std::vector<CraftDefinition*> recipes_list = cdef->getCraftRecipes(output, gdef);
	if (recipes_list.empty())
	{
		lua_pushnil(L);
		return 1;
	}
	// Get the table insert function
	lua_getglobal(L, "table");
	lua_getfield(L, -1, "insert");
	int table_insert = lua_gettop(L);
	lua_newtable(L);
	int table = lua_gettop(L);
	for (std::vector<CraftDefinition*>::const_iterator
		i = recipes_list.begin();
		i != recipes_list.end(); i++)
	{
		CraftOutput tmpout;
		tmpout.item = "";
		tmpout.time = 0;
		CraftDefinition *def = *i;
		tmpout = def->getOutput(input, gdef);
		std::string query = tmpout.item;
		char *fmtpos, *fmt = &query[0];
		if (strtok_r(fmt, " ", &fmtpos) == output.item)
		{
			input = def->getInput(output, gdef);
			lua_pushvalue(L, table_insert);
			lua_pushvalue(L, table);
			lua_newtable(L);
			int k = 1;
			lua_newtable(L);
			for(std::vector<ItemStack>::const_iterator
				i = input.items.begin();
				i != input.items.end(); i++, k++)
			{
				if (i->empty())
					continue;
				lua_pushinteger(L,k);
				lua_pushstring(L,i->name.c_str());
				lua_settable(L, -3);
			}
			lua_setfield(L, -2, "items");
			setintfield(L, -1, "width", input.width);
			switch (input.method) {
				case CRAFT_METHOD_NORMAL:
					lua_pushstring(L,"normal");
					break;
				case CRAFT_METHOD_COOKING:
					lua_pushstring(L,"cooking");
					break;
				case CRAFT_METHOD_FUEL:
					lua_pushstring(L,"fuel");
					break;
				default:
					lua_pushstring(L,"unknown");
				}
			lua_setfield(L, -2, "type");
			lua_pushstring(L, &tmpout.item[0]);
			lua_setfield(L, -2, "output");
			if (lua_pcall(L, 2, 0, 0))
				script_error(L, "error: %s", lua_tostring(L, -1));
		}
	}
	return 1;
}

void ModApiCraft::Initialize(lua_State *L, int top)
{
	API_FCT(get_all_craft_recipes);
	API_FCT(get_craft_recipe);
	API_FCT(get_craft_result);
	API_FCT(register_craft);
}

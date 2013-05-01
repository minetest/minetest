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

#include "scriptapi.h"
#include "scriptapi_node.h"
#include "util/pointedthing.h"
#include "script.h"
#include "scriptapi_common.h"
#include "scriptapi_types.h"
#include "scriptapi_item.h"
#include "scriptapi_object.h"


struct EnumString es_DrawType[] =
{
	{NDT_NORMAL, "normal"},
	{NDT_AIRLIKE, "airlike"},
	{NDT_LIQUID, "liquid"},
	{NDT_FLOWINGLIQUID, "flowingliquid"},
	{NDT_GLASSLIKE, "glasslike"},
	{NDT_GLASSLIKE_FRAMED, "glasslike_framed"},
	{NDT_ALLFACES, "allfaces"},
	{NDT_ALLFACES_OPTIONAL, "allfaces_optional"},
	{NDT_TORCHLIKE, "torchlike"},
	{NDT_SIGNLIKE, "signlike"},
	{NDT_PLANTLIKE, "plantlike"},
	{NDT_FENCELIKE, "fencelike"},
	{NDT_RAILLIKE, "raillike"},
	{NDT_NODEBOX, "nodebox"},
	{0, NULL},
};

struct EnumString es_ContentParamType[] =
{
	{CPT_NONE, "none"},
	{CPT_LIGHT, "light"},
	{0, NULL},
};

struct EnumString es_ContentParamType2[] =
{
	{CPT2_NONE, "none"},
	{CPT2_FULL, "full"},
	{CPT2_FLOWINGLIQUID, "flowingliquid"},
	{CPT2_FACEDIR, "facedir"},
	{CPT2_WALLMOUNTED, "wallmounted"},
	{0, NULL},
};

struct EnumString es_LiquidType[] =
{
	{LIQUID_NONE, "none"},
	{LIQUID_FLOWING, "flowing"},
	{LIQUID_SOURCE, "source"},
	{0, NULL},
};

struct EnumString es_NodeBoxType[] =
{
	{NODEBOX_REGULAR, "regular"},
	{NODEBOX_FIXED, "fixed"},
	{NODEBOX_WALLMOUNTED, "wallmounted"},
	{0, NULL},
};


bool scriptapi_node_on_punch(lua_State *L, v3s16 p, MapNode node,
		ServerActiveObject *puncher)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(), "on_punch"))
		return false;

	// Call function
	push_v3s16(L, p);
	pushnode(L, node, ndef);
	objectref_get_or_create(L, puncher);
	if(lua_pcall(L, 3, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	return true;
}

bool scriptapi_node_on_dig(lua_State *L, v3s16 p, MapNode node,
		ServerActiveObject *digger)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(), "on_dig"))
		return false;

	// Call function
	push_v3s16(L, p);
	pushnode(L, node, ndef);
	objectref_get_or_create(L, digger);
	if(lua_pcall(L, 3, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	return true;
}

void scriptapi_node_on_construct(lua_State *L, v3s16 p, MapNode node)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(), "on_construct"))
		return;

	// Call function
	push_v3s16(L, p);
	if(lua_pcall(L, 1, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

void scriptapi_node_on_destruct(lua_State *L, v3s16 p, MapNode node)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(), "on_destruct"))
		return;

	// Call function
	push_v3s16(L, p);
	if(lua_pcall(L, 1, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

void scriptapi_node_after_destruct(lua_State *L, v3s16 p, MapNode node)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(), "after_destruct"))
		return;

	// Call function
	push_v3s16(L, p);
	pushnode(L, node, ndef);
	if(lua_pcall(L, 2, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

bool scriptapi_node_on_timer(lua_State *L, v3s16 p, MapNode node, f32 dtime)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(), "on_timer"))
		return false;

	// Call function
	push_v3s16(L, p);
	lua_pushnumber(L,dtime);
	if(lua_pcall(L, 2, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if((bool)lua_isboolean(L,-1) && (bool)lua_toboolean(L,-1) == true)
		return true;

	return false;
}

void scriptapi_node_on_receive_fields(lua_State *L, v3s16 p,
		const std::string &formname,
		const std::map<std::string, std::string> &fields,
		ServerActiveObject *sender)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = get_env(L)->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return;

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(), "on_receive_fields"))
		return;

	// Call function
	// param 1
	push_v3s16(L, p);
	// param 2
	lua_pushstring(L, formname.c_str());
	// param 3
	lua_newtable(L);
	for(std::map<std::string, std::string>::const_iterator
			i = fields.begin(); i != fields.end(); i++){
		const std::string &name = i->first;
		const std::string &value = i->second;
		lua_pushstring(L, name.c_str());
		lua_pushlstring(L, value.c_str(), value.size());
		lua_settable(L, -3);
	}
	// param 4
	objectref_get_or_create(L, sender);
	if(lua_pcall(L, 4, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

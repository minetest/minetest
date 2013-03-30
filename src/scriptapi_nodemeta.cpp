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
#include "scriptapi_nodemeta.h"
#include "map.h"
#include "script.h"
#include "scriptapi_types.h"
#include "scriptapi_inventory.h"
#include "scriptapi_common.h"
#include "scriptapi_item.h"
#include "scriptapi_object.h"


/*
	NodeMetaRef
*/
NodeMetaRef* NodeMetaRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if(!ud) luaL_typerror(L, narg, className);
	return *(NodeMetaRef**)ud;  // unbox pointer
}

NodeMetadata* NodeMetaRef::getmeta(NodeMetaRef *ref, bool auto_create)
{
	NodeMetadata *meta = ref->m_env->getMap().getNodeMetadata(ref->m_p);
	if(meta == NULL && auto_create)
	{
		meta = new NodeMetadata(ref->m_env->getGameDef());
		ref->m_env->getMap().setNodeMetadata(ref->m_p, meta);
	}
	return meta;
}

void NodeMetaRef::reportMetadataChange(NodeMetaRef *ref)
{
	// NOTE: This same code is in rollback_interface.cpp
	// Inform other things that the metadata has changed
	v3s16 blockpos = getNodeBlockPos(ref->m_p);
	MapEditEvent event;
	event.type = MEET_BLOCK_NODE_METADATA_CHANGED;
	event.p = blockpos;
	ref->m_env->getMap().dispatchEvent(&event);
	// Set the block to be saved
	MapBlock *block = ref->m_env->getMap().getBlockNoCreateNoEx(blockpos);
	if(block)
		block->raiseModified(MOD_STATE_WRITE_NEEDED,
				"NodeMetaRef::reportMetadataChange");
}

// Exported functions

// garbage collector
int NodeMetaRef::gc_object(lua_State *L) {
	NodeMetaRef *o = *(NodeMetaRef **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

// get_string(self, name)
int NodeMetaRef::l_get_string(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	std::string name = luaL_checkstring(L, 2);

	NodeMetadata *meta = getmeta(ref, false);
	if(meta == NULL){
		lua_pushlstring(L, "", 0);
		return 1;
	}
	std::string str = meta->getString(name);
	lua_pushlstring(L, str.c_str(), str.size());
	return 1;
}

// set_string(self, name, var)
int NodeMetaRef::l_set_string(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	std::string name = luaL_checkstring(L, 2);
	size_t len = 0;
	const char *s = lua_tolstring(L, 3, &len);
	std::string str(s, len);

	NodeMetadata *meta = getmeta(ref, !str.empty());
	if(meta == NULL || str == meta->getString(name))
		return 0;
	meta->setString(name, str);
	reportMetadataChange(ref);
	return 0;
}

// get_int(self, name)
int NodeMetaRef::l_get_int(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	std::string name = lua_tostring(L, 2);

	NodeMetadata *meta = getmeta(ref, false);
	if(meta == NULL){
		lua_pushnumber(L, 0);
		return 1;
	}
	std::string str = meta->getString(name);
	lua_pushnumber(L, stoi(str));
	return 1;
}

// set_int(self, name, var)
int NodeMetaRef::l_set_int(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	std::string name = lua_tostring(L, 2);
	int a = lua_tointeger(L, 3);
	std::string str = itos(a);

	NodeMetadata *meta = getmeta(ref, true);
	if(meta == NULL || str == meta->getString(name))
		return 0;
	meta->setString(name, str);
	reportMetadataChange(ref);
	return 0;
}

// get_float(self, name)
int NodeMetaRef::l_get_float(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	std::string name = lua_tostring(L, 2);

	NodeMetadata *meta = getmeta(ref, false);
	if(meta == NULL){
		lua_pushnumber(L, 0);
		return 1;
	}
	std::string str = meta->getString(name);
	lua_pushnumber(L, stof(str));
	return 1;
}

// set_float(self, name, var)
int NodeMetaRef::l_set_float(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	std::string name = lua_tostring(L, 2);
	float a = lua_tonumber(L, 3);
	std::string str = ftos(a);

	NodeMetadata *meta = getmeta(ref, true);
	if(meta == NULL || str == meta->getString(name))
		return 0;
	meta->setString(name, str);
	reportMetadataChange(ref);
	return 0;
}

// get_inventory(self)
int NodeMetaRef::l_get_inventory(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	getmeta(ref, true);  // try to ensure the metadata exists
	InvRef::createNodeMeta(L, ref->m_p);
	return 1;
}

// to_table(self)
int NodeMetaRef::l_to_table(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);

	NodeMetadata *meta = getmeta(ref, true);
	if(meta == NULL){
		lua_pushnil(L);
		return 1;
	}
	lua_newtable(L);
	// fields
	lua_newtable(L);
	{
		std::map<std::string, std::string> fields = meta->getStrings();
		for(std::map<std::string, std::string>::const_iterator
				i = fields.begin(); i != fields.end(); i++){
			const std::string &name = i->first;
			const std::string &value = i->second;
			lua_pushlstring(L, name.c_str(), name.size());
			lua_pushlstring(L, value.c_str(), value.size());
			lua_settable(L, -3);
		}
	}
	lua_setfield(L, -2, "fields");
	// inventory
	lua_newtable(L);
	Inventory *inv = meta->getInventory();
	if(inv){
		std::vector<const InventoryList*> lists = inv->getLists();
		for(std::vector<const InventoryList*>::const_iterator
				i = lists.begin(); i != lists.end(); i++){
			inventory_get_list_to_lua(inv, (*i)->getName().c_str(), L);
			lua_setfield(L, -2, (*i)->getName().c_str());
		}
	}
	lua_setfield(L, -2, "inventory");
	return 1;
}

// from_table(self, table)
int NodeMetaRef::l_from_table(lua_State *L)
{
	NodeMetaRef *ref = checkobject(L, 1);
	int base = 2;

	if(lua_isnil(L, base)){
		// No metadata
		ref->m_env->getMap().removeNodeMetadata(ref->m_p);
		lua_pushboolean(L, true);
		return 1;
	}

	// Has metadata; clear old one first
	ref->m_env->getMap().removeNodeMetadata(ref->m_p);
	// Create new metadata
	NodeMetadata *meta = getmeta(ref, true);
	// Set fields
	lua_getfield(L, base, "fields");
	int fieldstable = lua_gettop(L);
	lua_pushnil(L);
	while(lua_next(L, fieldstable) != 0){
		// key at index -2 and value at index -1
		std::string name = lua_tostring(L, -2);
		size_t cl;
		const char *cs = lua_tolstring(L, -1, &cl);
		std::string value(cs, cl);
		meta->setString(name, value);
		lua_pop(L, 1); // removes value, keeps key for next iteration
	}
	// Set inventory
	Inventory *inv = meta->getInventory();
	lua_getfield(L, base, "inventory");
	int inventorytable = lua_gettop(L);
	lua_pushnil(L);
	while(lua_next(L, inventorytable) != 0){
		// key at index -2 and value at index -1
		std::string name = lua_tostring(L, -2);
		inventory_set_list_from_lua(inv, name.c_str(), L, -1);
		lua_pop(L, 1); // removes value, keeps key for next iteration
	}
	reportMetadataChange(ref);
	lua_pushboolean(L, true);
	return 1;
}


NodeMetaRef::NodeMetaRef(v3s16 p, ServerEnvironment *env):
	m_p(p),
	m_env(env)
{
}

NodeMetaRef::~NodeMetaRef()
{
}

// Creates an NodeMetaRef and leaves it on top of stack
// Not callable from Lua; all references are created on the C side.
void NodeMetaRef::create(lua_State *L, v3s16 p, ServerEnvironment *env)
{
	NodeMetaRef *o = new NodeMetaRef(p, env);
	//infostream<<"NodeMetaRef::create: o="<<o<<std::endl;
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void NodeMetaRef::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);  // hide metatable from Lua getmetatable()

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1);  // drop metatable

	luaL_openlib(L, 0, methods, 0);  // fill methodtable
	lua_pop(L, 1);  // drop methodtable

	// Cannot be created from Lua
	//lua_register(L, className, create_object);
}

const char NodeMetaRef::className[] = "NodeMetaRef";
const luaL_reg NodeMetaRef::methods[] = {
	luamethod(NodeMetaRef, get_string),
	luamethod(NodeMetaRef, set_string),
	luamethod(NodeMetaRef, get_int),
	luamethod(NodeMetaRef, set_int),
	luamethod(NodeMetaRef, get_float),
	luamethod(NodeMetaRef, set_float),
	luamethod(NodeMetaRef, get_inventory),
	luamethod(NodeMetaRef, to_table),
	luamethod(NodeMetaRef, from_table),
	{0,0}
};

/*
	Node metadata inventory callbacks
*/

// Return number of accepted items to be moved
int scriptapi_nodemeta_inventory_allow_move(lua_State *L, v3s16 p,
		const std::string &from_list, int from_index,
		const std::string &to_list, int to_index,
		int count, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = get_env(L)->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return 0;

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(),
			"allow_metadata_inventory_move"))
		return count;

	// function(pos, from_list, from_index, to_list, to_index, count, player)
	// pos
	push_v3s16(L, p);
	// from_list
	lua_pushstring(L, from_list.c_str());
	// from_index
	lua_pushinteger(L, from_index + 1);
	// to_list
	lua_pushstring(L, to_list.c_str());
	// to_index
	lua_pushinteger(L, to_index + 1);
	// count
	lua_pushinteger(L, count);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 7, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_metadata_inventory_move should return a number");
	return luaL_checkinteger(L, -1);
}

// Return number of accepted items to be put
int scriptapi_nodemeta_inventory_allow_put(lua_State *L, v3s16 p,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = get_env(L)->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return 0;

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(),
			"allow_metadata_inventory_put"))
		return stack.count;

	// Call function(pos, listname, index, stack, player)
	// pos
	push_v3s16(L, p);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_metadata_inventory_put should return a number");
	return luaL_checkinteger(L, -1);
}

// Return number of accepted items to be taken
int scriptapi_nodemeta_inventory_allow_take(lua_State *L, v3s16 p,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = get_env(L)->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return 0;

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(),
			"allow_metadata_inventory_take"))
		return stack.count;

	// Call function(pos, listname, index, count, player)
	// pos
	push_v3s16(L, p);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_metadata_inventory_take should return a number");
	return luaL_checkinteger(L, -1);
}

// Report moved items
void scriptapi_nodemeta_inventory_on_move(lua_State *L, v3s16 p,
		const std::string &from_list, int from_index,
		const std::string &to_list, int to_index,
		int count, ServerActiveObject *player)
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
	if(!get_item_callback(L, ndef->get(node).name.c_str(),
			"on_metadata_inventory_move"))
		return;

	// function(pos, from_list, from_index, to_list, to_index, count, player)
	// pos
	push_v3s16(L, p);
	// from_list
	lua_pushstring(L, from_list.c_str());
	// from_index
	lua_pushinteger(L, from_index + 1);
	// to_list
	lua_pushstring(L, to_list.c_str());
	// to_index
	lua_pushinteger(L, to_index + 1);
	// count
	lua_pushinteger(L, count);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 7, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

// Report put items
void scriptapi_nodemeta_inventory_on_put(lua_State *L, v3s16 p,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
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
	if(!get_item_callback(L, ndef->get(node).name.c_str(),
			"on_metadata_inventory_put"))
		return;

	// Call function(pos, listname, index, stack, player)
	// pos
	push_v3s16(L, p);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

// Report taken items
void scriptapi_nodemeta_inventory_on_take(lua_State *L, v3s16 p,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
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
	if(!get_item_callback(L, ndef->get(node).name.c_str(),
			"on_metadata_inventory_take"))
		return;

	// Call function(pos, listname, index, stack, player)
	// pos
	push_v3s16(L, p);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

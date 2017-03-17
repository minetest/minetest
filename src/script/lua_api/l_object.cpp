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

#include "lua_api/l_object.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_inventory.h"
#include "lua_api/l_item.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "log.h"
#include "tool.h"
#include "serverobject.h"
#include "content_sao.h"
#include "server.h"
#include "hud.h"
#include "scripting_server.h"

struct EnumString es_HudElementType[] =
{
	{HUD_ELEM_IMAGE,     "image"},
	{HUD_ELEM_TEXT,      "text"},
	{HUD_ELEM_STATBAR,   "statbar"},
	{HUD_ELEM_INVENTORY, "inventory"},
	{HUD_ELEM_WAYPOINT,  "waypoint"},
{0, NULL},
};

struct EnumString es_HudElementStat[] =
{
	{HUD_STAT_POS,    "position"},
	{HUD_STAT_POS,    "pos"}, /* Deprecated, only for compatibility's sake */
	{HUD_STAT_NAME,   "name"},
	{HUD_STAT_SCALE,  "scale"},
	{HUD_STAT_TEXT,   "text"},
	{HUD_STAT_NUMBER, "number"},
	{HUD_STAT_ITEM,   "item"},
	{HUD_STAT_DIR,    "direction"},
	{HUD_STAT_ALIGN,  "alignment"},
	{HUD_STAT_OFFSET, "offset"},
	{HUD_STAT_WORLD_POS, "world_pos"},
	{0, NULL},
};

struct EnumString es_HudBuiltinElement[] =
{
	{HUD_FLAG_HOTBAR_VISIBLE,    "hotbar"},
	{HUD_FLAG_HEALTHBAR_VISIBLE, "healthbar"},
	{HUD_FLAG_CROSSHAIR_VISIBLE, "crosshair"},
	{HUD_FLAG_WIELDITEM_VISIBLE, "wielditem"},
	{HUD_FLAG_BREATHBAR_VISIBLE, "breathbar"},
	{HUD_FLAG_MINIMAP_VISIBLE,   "minimap"},
	{0, NULL},
};

/*
	ObjectRef
*/


ObjectRef* ObjectRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if (!ud) luaL_typerror(L, narg, className);
	return *(ObjectRef**)ud;  // unbox pointer
}

ServerActiveObject* ObjectRef::getobject(ObjectRef *ref)
{
	ServerActiveObject *co = ref->m_object;
	return co;
}

LuaEntitySAO* ObjectRef::getluaobject(ObjectRef *ref)
{
	ServerActiveObject *obj = getobject(ref);
	if (obj == NULL)
		return NULL;
	if (obj->getType() != ACTIVEOBJECT_TYPE_LUAENTITY)
		return NULL;
	return (LuaEntitySAO*)obj;
}

PlayerSAO* ObjectRef::getplayersao(ObjectRef *ref)
{
	ServerActiveObject *obj = getobject(ref);
	if (obj == NULL)
		return NULL;
	if (obj->getType() != ACTIVEOBJECT_TYPE_PLAYER)
		return NULL;
	return (PlayerSAO*)obj;
}

RemotePlayer *ObjectRef::getplayer(ObjectRef *ref)
{
	PlayerSAO *playersao = getplayersao(ref);
	if (playersao == NULL)
		return NULL;
	return playersao->getPlayer();
}

// Exported functions

// garbage collector
int ObjectRef::gc_object(lua_State *L) {
	ObjectRef *o = *(ObjectRef **)(lua_touserdata(L, 1));
	//infostream<<"ObjectRef::gc_object: o="<<o<<std::endl;
	delete o;
	return 0;
}

// remove(self)
int ObjectRef::l_remove(lua_State *L)
{
	GET_ENV_PTR;

	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL)
		return 0;
	if (co->getType() == ACTIVEOBJECT_TYPE_PLAYER)
		return 0;

	const UNORDERED_SET<int> &child_ids = co->getAttachmentChildIds();
	UNORDERED_SET<int>::const_iterator it;
	for (it = child_ids.begin(); it != child_ids.end(); ++it) {
		// Child can be NULL if it was deleted earlier
		if (ServerActiveObject *child = env->getActiveObject(*it))
			child->setAttachment(0, "", v3f(0, 0, 0), v3f(0, 0, 0));
	}

	verbosestream<<"ObjectRef::l_remove(): id="<<co->getId()<<std::endl;
	co->m_removed = true;
	return 0;
}

// get_pos(self)
// returns: {x=num, y=num, z=num}
int ObjectRef::l_get_pos(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL) return 0;
	v3f pos = co->getBasePosition() / BS;
	lua_newtable(L);
	lua_pushnumber(L, pos.X);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, pos.Y);
	lua_setfield(L, -2, "y");
	lua_pushnumber(L, pos.Z);
	lua_setfield(L, -2, "z");
	return 1;
}

// set_pos(self, pos)
int ObjectRef::l_set_pos(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	//LuaEntitySAO *co = getluaobject(ref);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL) return 0;
	// pos
	v3f pos = checkFloatPos(L, 2);
	// Do it
	co->setPos(pos);
	return 0;
}

// move_to(self, pos, continuous=false)
int ObjectRef::l_move_to(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	//LuaEntitySAO *co = getluaobject(ref);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL) return 0;
	// pos
	v3f pos = checkFloatPos(L, 2);
	// continuous
	bool continuous = lua_toboolean(L, 3);
	// Do it
	co->moveTo(pos, continuous);
	return 0;
}

// punch(self, puncher, time_from_last_punch, tool_capabilities, dir)
int ObjectRef::l_punch(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ObjectRef *puncher_ref = checkobject(L, 2);
	ServerActiveObject *co = getobject(ref);
	ServerActiveObject *puncher = getobject(puncher_ref);
	if (co == NULL) return 0;
	if (puncher == NULL) return 0;
	v3f dir;
	if (lua_type(L, 5) != LUA_TTABLE)
		dir = co->getBasePosition() - puncher->getBasePosition();
	else
		dir = read_v3f(L, 5);
	float time_from_last_punch = 1000000;
	if (lua_isnumber(L, 3))
		time_from_last_punch = lua_tonumber(L, 3);
	ToolCapabilities toolcap = read_tool_capabilities(L, 4);
	dir.normalize();

	s16 src_original_hp = co->getHP();
	s16 dst_origin_hp = puncher->getHP();

	// Do it
	co->punch(dir, &toolcap, puncher, time_from_last_punch);

	// If the punched is a player, and its HP changed
	if (src_original_hp != co->getHP() &&
			co->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
		getServer(L)->SendPlayerHPOrDie((PlayerSAO *)co);
	}

	// If the puncher is a player, and its HP changed
	if (dst_origin_hp != puncher->getHP() &&
			puncher->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
		getServer(L)->SendPlayerHPOrDie((PlayerSAO *)puncher);
	}
	return 0;
}

// right_click(self, clicker); clicker = an another ObjectRef
int ObjectRef::l_right_click(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ObjectRef *ref2 = checkobject(L, 2);
	ServerActiveObject *co = getobject(ref);
	ServerActiveObject *co2 = getobject(ref2);
	if (co == NULL) return 0;
	if (co2 == NULL) return 0;
	// Do it
	co->rightClick(co2);
	return 0;
}

// set_hp(self, hp)
// hp = number of hitpoints (2 * number of hearts)
// returns: nil
int ObjectRef::l_set_hp(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	luaL_checknumber(L, 2);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL) return 0;
	int hp = lua_tonumber(L, 2);
	/*infostream<<"ObjectRef::l_set_hp(): id="<<co->getId()
			<<" hp="<<hp<<std::endl;*/
	// Do it
	co->setHP(hp);
	if (co->getType() == ACTIVEOBJECT_TYPE_PLAYER)
		getServer(L)->SendPlayerHPOrDie((PlayerSAO *)co);

	// Return
	return 0;
}

// get_hp(self)
// returns: number of hitpoints (2 * number of hearts)
// 0 if not applicable to this type of object
int ObjectRef::l_get_hp(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL) {
		// Default hp is 1
		lua_pushnumber(L, 1);
		return 1;
	}
	int hp = co->getHP();
	/*infostream<<"ObjectRef::l_get_hp(): id="<<co->getId()
			<<" hp="<<hp<<std::endl;*/
	// Return
	lua_pushnumber(L, hp);
	return 1;
}

// get_inventory(self)
int ObjectRef::l_get_inventory(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL) return 0;
	// Do it
	InventoryLocation loc = co->getInventoryLocation();
	if (getServer(L)->getInventory(loc) != NULL)
		InvRef::create(L, loc);
	else
		lua_pushnil(L); // An object may have no inventory (nil)
	return 1;
}

// get_wield_list(self)
int ObjectRef::l_get_wield_list(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL) return 0;
	// Do it
	lua_pushstring(L, co->getWieldList().c_str());
	return 1;
}

// get_wield_index(self)
int ObjectRef::l_get_wield_index(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL) return 0;
	// Do it
	lua_pushinteger(L, co->getWieldIndex() + 1);
	return 1;
}

// get_wielded_item(self)
int ObjectRef::l_get_wielded_item(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL) {
		// Empty ItemStack
		LuaItemStack::create(L, ItemStack());
		return 1;
	}
	// Do it
	LuaItemStack::create(L, co->getWieldedItem());
	return 1;
}

// set_wielded_item(self, itemstack or itemstring or table or nil)
int ObjectRef::l_set_wielded_item(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL) return 0;
	// Do it
	ItemStack item = read_item(L, 2, getServer(L)->idef());
	bool success = co->setWieldedItem(item);
	if (success && co->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
		getServer(L)->SendInventory(((PlayerSAO*)co));
	}
	lua_pushboolean(L, success);
	return 1;
}

// set_armor_groups(self, groups)
int ObjectRef::l_set_armor_groups(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL) return 0;
	// Do it
	ItemGroupList groups;
	read_groups(L, 2, groups);
	co->setArmorGroups(groups);
	return 0;
}

// get_armor_groups(self)
int ObjectRef::l_get_armor_groups(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL)
		return 0;
	// Do it
	push_groups(L, co->getArmorGroups());
	return 1;
}

// set_physics_override(self, physics_override_speed, physics_override_jump,
//                      physics_override_gravity, sneak, sneak_glitch, new_move)
int ObjectRef::l_set_physics_override(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO *co = (PlayerSAO *) getobject(ref);
	if (co == NULL) return 0;
	// Do it
	if (lua_istable(L, 2)) {
		co->m_physics_override_speed = getfloatfield_default(
				L, 2, "speed", co->m_physics_override_speed);
		co->m_physics_override_jump = getfloatfield_default(
				L, 2, "jump", co->m_physics_override_jump);
		co->m_physics_override_gravity = getfloatfield_default(
				L, 2, "gravity", co->m_physics_override_gravity);
		co->m_physics_override_sneak = getboolfield_default(
				L, 2, "sneak", co->m_physics_override_sneak);
		co->m_physics_override_sneak_glitch = getboolfield_default(
				L, 2, "sneak_glitch", co->m_physics_override_sneak_glitch);
		co->m_physics_override_new_move = getboolfield_default(
				L, 2, "new_move", co->m_physics_override_new_move);
		co->m_physics_override_sent = false;
	} else {
		// old, non-table format
		if (!lua_isnil(L, 2)) {
			co->m_physics_override_speed = lua_tonumber(L, 2);
			co->m_physics_override_sent = false;
		}
		if (!lua_isnil(L, 3)) {
			co->m_physics_override_jump = lua_tonumber(L, 3);
			co->m_physics_override_sent = false;
		}
		if (!lua_isnil(L, 4)) {
			co->m_physics_override_gravity = lua_tonumber(L, 4);
			co->m_physics_override_sent = false;
		}
	}
	return 0;
}

// get_physics_override(self)
int ObjectRef::l_get_physics_override(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO *co = (PlayerSAO *)getobject(ref);
	if (co == NULL)
		return 0;
	// Do it
	lua_newtable(L);
	lua_pushnumber(L, co->m_physics_override_speed);
	lua_setfield(L, -2, "speed");
	lua_pushnumber(L, co->m_physics_override_jump);
	lua_setfield(L, -2, "jump");
	lua_pushnumber(L, co->m_physics_override_gravity);
	lua_setfield(L, -2, "gravity");
	lua_pushboolean(L, co->m_physics_override_sneak);
	lua_setfield(L, -2, "sneak");
	lua_pushboolean(L, co->m_physics_override_sneak_glitch);
	lua_setfield(L, -2, "sneak_glitch");
	lua_pushboolean(L, co->m_physics_override_new_move);
	lua_setfield(L, -2, "new_move");
	return 1;
}

// set_animation(self, frame_range, frame_speed, frame_blend, frame_loop)
int ObjectRef::l_set_animation(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL) return 0;
	// Do it
	v2f frames = v2f(1, 1);
	if (!lua_isnil(L, 2))
		frames = read_v2f(L, 2);
	float frame_speed = 15;
	if (!lua_isnil(L, 3))
		frame_speed = lua_tonumber(L, 3);
	float frame_blend = 0;
	if (!lua_isnil(L, 4))
		frame_blend = lua_tonumber(L, 4);
	bool frame_loop = true;
	if (lua_isboolean(L, 5))
		frame_loop = lua_toboolean(L, 5);
	co->setAnimation(frames, frame_speed, frame_blend, frame_loop);
	return 0;
}

// get_animation(self)
int ObjectRef::l_get_animation(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL)
		return 0;
	// Do it
	v2f frames = v2f(1,1);
	float frame_speed = 15;
	float frame_blend = 0;
	bool frame_loop = true;
	co->getAnimation(&frames, &frame_speed, &frame_blend, &frame_loop);

	push_v2f(L, frames);
	lua_pushnumber(L, frame_speed);
	lua_pushnumber(L, frame_blend);
	lua_pushboolean(L, frame_loop);
	return 4;
}

// set_local_animation(self, {stand/idle}, {walk}, {dig}, {walk+dig}, frame_speed)
int ObjectRef::l_set_local_animation(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;
	// Do it
	v2s32 frames[4];
	for (int i=0;i<4;i++) {
		if (!lua_isnil(L, 2+1))
			frames[i] = read_v2s32(L, 2+i);
	}
	float frame_speed = 30;
	if (!lua_isnil(L, 6))
		frame_speed = lua_tonumber(L, 6);

	if (!getServer(L)->setLocalPlayerAnimations(player, frames, frame_speed))
		return 0;

	lua_pushboolean(L, true);
	return 0;
}

// get_local_animation(self)
int ObjectRef::l_get_local_animation(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	v2s32 frames[4];
	float frame_speed;
	player->getLocalAnimations(frames, &frame_speed);

	for (int i = 0; i < 4; i++) {
		push_v2s32(L, frames[i]);
	}

	lua_pushnumber(L, frame_speed);
	return 5;
}

// set_eye_offset(self, v3f first pv, v3f third pv)
int ObjectRef::l_set_eye_offset(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;
	// Do it
	v3f offset_first = v3f(0, 0, 0);
	v3f offset_third = v3f(0, 0, 0);

	if (!lua_isnil(L, 2))
		offset_first = read_v3f(L, 2);
	if (!lua_isnil(L, 3))
		offset_third = read_v3f(L, 3);

	// Prevent abuse of offset values (keep player always visible)
	offset_third.X = rangelim(offset_third.X,-10,10);
	offset_third.Z = rangelim(offset_third.Z,-5,5);
	/* TODO: if possible: improve the camera colision detetion to allow Y <= -1.5) */
	offset_third.Y = rangelim(offset_third.Y,-10,15); //1.5*BS

	if (!getServer(L)->setPlayerEyeOffset(player, offset_first, offset_third))
		return 0;

	lua_pushboolean(L, true);
	return 0;
}

// get_eye_offset(self)
int ObjectRef::l_get_eye_offset(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;
	// Do it
	push_v3f(L, player->eye_offset_first);
	push_v3f(L, player->eye_offset_third);
	return 2;
}

// set_bone_position(self, std::string bone, v3f position, v3f rotation)
int ObjectRef::l_set_bone_position(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL) return 0;
	// Do it
	std::string bone = "";
	if (!lua_isnil(L, 2))
		bone = lua_tostring(L, 2);
	v3f position = v3f(0, 0, 0);
	if (!lua_isnil(L, 3))
		position = check_v3f(L, 3);
	v3f rotation = v3f(0, 0, 0);
	if (!lua_isnil(L, 4))
		rotation = check_v3f(L, 4);
	co->setBonePosition(bone, position, rotation);
	return 0;
}

// get_bone_position(self, bone)
int ObjectRef::l_get_bone_position(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL)
		return 0;
	// Do it
	std::string bone = "";
	if (!lua_isnil(L, 2))
		bone = lua_tostring(L, 2);

	v3f position = v3f(0, 0, 0);
	v3f rotation = v3f(0, 0, 0);
	co->getBonePosition(bone, &position, &rotation);

	push_v3f(L, position);
	push_v3f(L, rotation);
	return 2;
}

// set_attach(self, parent, bone, position, rotation)
int ObjectRef::l_set_attach(lua_State *L)
{
	GET_ENV_PTR;

	ObjectRef *ref = checkobject(L, 1);
	ObjectRef *parent_ref = checkobject(L, 2);
	ServerActiveObject *co = getobject(ref);
	ServerActiveObject *parent = getobject(parent_ref);
	if (co == NULL)
		return 0;
	if (parent == NULL)
		return 0;
	// Do it
	int parent_id = 0;
	std::string bone = "";
	v3f position = v3f(0, 0, 0);
	v3f rotation = v3f(0, 0, 0);
	co->getAttachment(&parent_id, &bone, &position, &rotation);
	if (parent_id) {
		ServerActiveObject *old_parent = env->getActiveObject(parent_id);
		old_parent->removeAttachmentChild(co->getId());
	}

	bone = "";
	if (!lua_isnil(L, 3))
		bone = lua_tostring(L, 3);
	position = v3f(0, 0, 0);
	if (!lua_isnil(L, 4))
		position = read_v3f(L, 4);
	rotation = v3f(0, 0, 0);
	if (!lua_isnil(L, 5))
		rotation = read_v3f(L, 5);
	co->setAttachment(parent->getId(), bone, position, rotation);
	parent->addAttachmentChild(co->getId());
	return 0;
}

// get_attach(self)
int ObjectRef::l_get_attach(lua_State *L)
{
	GET_ENV_PTR;

	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL)
		return 0;

	// Do it
	int parent_id = 0;
	std::string bone = "";
	v3f position = v3f(0, 0, 0);
	v3f rotation = v3f(0, 0, 0);
	co->getAttachment(&parent_id, &bone, &position, &rotation);
	if (!parent_id)
		return 0;
	ServerActiveObject *parent = env->getActiveObject(parent_id);

	getScriptApiBase(L)->objectrefGetOrCreate(L, parent);
	lua_pushlstring(L, bone.c_str(), bone.size());
	push_v3f(L, position);
	push_v3f(L, rotation);
	return 4;
}

// set_detach(self)
int ObjectRef::l_set_detach(lua_State *L)
{
	GET_ENV_PTR;

	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL)
		return 0;

	int parent_id = 0;
	std::string bone = "";
	v3f position;
	v3f rotation;
	co->getAttachment(&parent_id, &bone, &position, &rotation);
	ServerActiveObject *parent = NULL;
	if (parent_id) {
		parent = env->getActiveObject(parent_id);
		co->setAttachment(0, "", position, rotation);
	} else {
		co->setAttachment(0, "", v3f(0, 0, 0), v3f(0, 0, 0));
	}
	// Do it
	if (parent != NULL)
		parent->removeAttachmentChild(co->getId());
	return 0;
}

// set_properties(self, properties)
int ObjectRef::l_set_properties(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL) return 0;
	ObjectProperties *prop = co->accessObjectProperties();
	if (!prop)
		return 0;
	read_object_properties(L, 2, prop, getServer(L)->idef());
	co->notifyObjectPropertiesModified();
	return 0;
}

// get_properties(self)
int ObjectRef::l_get_properties(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);
	if (co == NULL)
		return 0;
	ObjectProperties *prop = co->accessObjectProperties();
	if (!prop)
		return 0;
	push_object_properties(L, prop);
	return 1;
}

// is_player(self)
int ObjectRef::l_is_player(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	lua_pushboolean(L, (player != NULL));
	return 1;
}

// set_nametag_attributes(self, attributes)
int ObjectRef::l_set_nametag_attributes(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);

	if (co == NULL)
		return 0;
	ObjectProperties *prop = co->accessObjectProperties();
	if (!prop)
		return 0;

	lua_getfield(L, 2, "color");
	if (!lua_isnil(L, -1)) {
		video::SColor color = prop->nametag_color;
		read_color(L, -1, &color);
		prop->nametag_color = color;
	}
	lua_pop(L, 1);

	std::string nametag = getstringfield_default(L, 2, "text", "");
	prop->nametag = nametag;

	co->notifyObjectPropertiesModified();
	lua_pushboolean(L, true);
	return 1;
}

// get_nametag_attributes(self)
int ObjectRef::l_get_nametag_attributes(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *co = getobject(ref);

	if (co == NULL)
		return 0;
	ObjectProperties *prop = co->accessObjectProperties();
	if (!prop)
		return 0;

	video::SColor color = prop->nametag_color;

	lua_newtable(L);
	push_ARGB8(L, color);
	lua_setfield(L, -2, "color");
	lua_pushstring(L, prop->nametag.c_str());
	lua_setfield(L, -2, "text");
	return 1;
}

/* LuaEntitySAO-only */

// set_velocity(self, {x=num, y=num, z=num})
int ObjectRef::l_set_velocity(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *co = getluaobject(ref);
	if (co == NULL) return 0;
	v3f pos = checkFloatPos(L, 2);
	// Do it
	co->setVelocity(pos);
	return 0;
}

// get_velocity(self)
int ObjectRef::l_get_velocity(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *co = getluaobject(ref);
	if (co == NULL) return 0;
	// Do it
	v3f v = co->getVelocity();
	pushFloatPos(L, v);
	return 1;
}

// set_acceleration(self, {x=num, y=num, z=num})
int ObjectRef::l_set_acceleration(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *co = getluaobject(ref);
	if (co == NULL) return 0;
	// pos
	v3f pos = checkFloatPos(L, 2);
	// Do it
	co->setAcceleration(pos);
	return 0;
}

// get_acceleration(self)
int ObjectRef::l_get_acceleration(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *co = getluaobject(ref);
	if (co == NULL) return 0;
	// Do it
	v3f v = co->getAcceleration();
	pushFloatPos(L, v);
	return 1;
}

// set_yaw(self, radians)
int ObjectRef::l_set_yaw(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *co = getluaobject(ref);
	if (co == NULL) return 0;
	float yaw = luaL_checknumber(L, 2) * core::RADTODEG;
	// Do it
	co->setYaw(yaw);
	return 0;
}

// get_yaw(self)
int ObjectRef::l_get_yaw(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *co = getluaobject(ref);
	if (co == NULL) return 0;
	// Do it
	float yaw = co->getYaw() * core::DEGTORAD;
	lua_pushnumber(L, yaw);
	return 1;
}

// set_texture_mod(self, mod)
int ObjectRef::l_set_texture_mod(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *co = getluaobject(ref);
	if (co == NULL) return 0;
	// Do it
	std::string mod = luaL_checkstring(L, 2);
	co->setTextureMod(mod);
	return 0;
}

// get_texture_mod(self)
int ObjectRef::l_get_texture_mod(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *co = getluaobject(ref);
	if (co == NULL) return 0;
	// Do it
	std::string mod = co->getTextureMod();
	lua_pushstring(L, mod.c_str());
	return 1;
}

// set_sprite(self, p={x=0,y=0}, num_frames=1, framelength=0.2,
//           select_horiz_by_yawpitch=false)
int ObjectRef::l_set_sprite(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *co = getluaobject(ref);
	if (co == NULL) return 0;
	// Do it
	v2s16 p(0,0);
	if (!lua_isnil(L, 2))
		p = read_v2s16(L, 2);
	int num_frames = 1;
	if (!lua_isnil(L, 3))
		num_frames = lua_tonumber(L, 3);
	float framelength = 0.2;
	if (!lua_isnil(L, 4))
		framelength = lua_tonumber(L, 4);
	bool select_horiz_by_yawpitch = false;
	if (!lua_isnil(L, 5))
		select_horiz_by_yawpitch = lua_toboolean(L, 5);
	co->setSprite(p, num_frames, framelength, select_horiz_by_yawpitch);
	return 0;
}

// DEPRECATED
// get_entity_name(self)
int ObjectRef::l_get_entity_name(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *co = getluaobject(ref);
	log_deprecated(L,"Deprecated call to \"get_entity_name");
	if (co == NULL) return 0;
	// Do it
	std::string name = co->getName();
	lua_pushstring(L, name.c_str());
	return 1;
}

// get_luaentity(self)
int ObjectRef::l_get_luaentity(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *co = getluaobject(ref);
	if (co == NULL) return 0;
	// Do it
	luaentity_get(L, co->getId());
	return 1;
}

/* Player-only */

// is_player_connected(self)
int ObjectRef::l_is_player_connected(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	lua_pushboolean(L, (player != NULL && player->peer_id != 0));
	return 1;
}

// get_player_name(self)
int ObjectRef::l_get_player_name(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL) {
		lua_pushlstring(L, "", 0);
		return 1;
	}
	// Do it
	lua_pushstring(L, player->getName());
	return 1;
}

// get_player_velocity(self)
int ObjectRef::l_get_player_velocity(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL) {
		lua_pushnil(L);
		return 1;
	}
	// Do it
	push_v3f(L, player->getSpeed() / BS);
	return 1;
}

// get_look_dir(self)
int ObjectRef::l_get_look_dir(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* co = getplayersao(ref);
	if (co == NULL) return 0;
	// Do it
	float pitch = co->getRadPitchDep();
	float yaw = co->getRadYawDep();
	v3f v(cos(pitch)*cos(yaw), sin(pitch), cos(pitch)*sin(yaw));
	push_v3f(L, v);
	return 1;
}

// DEPRECATED
// get_look_pitch(self)
int ObjectRef::l_get_look_pitch(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	log_deprecated(L,
		"Deprecated call to get_look_pitch, use get_look_vertical instead");

	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* co = getplayersao(ref);
	if (co == NULL) return 0;
	// Do it
	lua_pushnumber(L, co->getRadPitchDep());
	return 1;
}

// DEPRECATED
// get_look_yaw(self)
int ObjectRef::l_get_look_yaw(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	log_deprecated(L,
		"Deprecated call to get_look_yaw, use get_look_horizontal instead");

	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* co = getplayersao(ref);
	if (co == NULL) return 0;
	// Do it
	lua_pushnumber(L, co->getRadYawDep());
	return 1;
}

// get_look_pitch2(self)
int ObjectRef::l_get_look_vertical(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* co = getplayersao(ref);
	if (co == NULL) return 0;
	// Do it
	lua_pushnumber(L, co->getRadPitch());
	return 1;
}

// get_look_yaw2(self)
int ObjectRef::l_get_look_horizontal(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* co = getplayersao(ref);
	if (co == NULL) return 0;
	// Do it
	lua_pushnumber(L, co->getRadYaw());
	return 1;
}

// set_look_vertical(self, radians)
int ObjectRef::l_set_look_vertical(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* co = getplayersao(ref);
	if (co == NULL) return 0;
	float pitch = luaL_checknumber(L, 2) * core::RADTODEG;
	// Do it
	co->setPitchAndSend(pitch);
	return 1;
}

// set_look_horizontal(self, radians)
int ObjectRef::l_set_look_horizontal(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* co = getplayersao(ref);
	if (co == NULL) return 0;
	float yaw = luaL_checknumber(L, 2) * core::RADTODEG;
	// Do it
	co->setYawAndSend(yaw);
	return 1;
}

// DEPRECATED
// set_look_pitch(self, radians)
int ObjectRef::l_set_look_pitch(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	log_deprecated(L,
		"Deprecated call to set_look_pitch, use set_look_vertical instead.");

	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* co = getplayersao(ref);
	if (co == NULL) return 0;
	float pitch = luaL_checknumber(L, 2) * core::RADTODEG;
	// Do it
	co->setPitchAndSend(pitch);
	return 1;
}

// DEPRECATED
// set_look_yaw(self, radians)
int ObjectRef::l_set_look_yaw(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	log_deprecated(L,
		"Deprecated call to set_look_yaw, use set_look_horizontal instead.");

	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* co = getplayersao(ref);
	if (co == NULL) return 0;
	float yaw = luaL_checknumber(L, 2) * core::RADTODEG;
	// Do it
	co->setYawAndSend(yaw);
	return 1;
}

// set_breath(self, breath)
int ObjectRef::l_set_breath(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* co = getplayersao(ref);
	if (co == NULL) return 0;
	u16 breath = luaL_checknumber(L, 2);
	co->setBreath(breath);

	return 0;
}

// get_breath(self)
int ObjectRef::l_get_breath(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* co = getplayersao(ref);
	if (co == NULL) return 0;
	// Do it
	u16 breath = co->getBreath();
	lua_pushinteger (L, breath);
	return 1;
}

// set_attribute(self, attribute, value)
int ObjectRef::l_set_attribute(lua_State *L)
{
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* co = getplayersao(ref);
	if (co == NULL) {
		return 0;
	}

	std::string attr = luaL_checkstring(L, 2);
	std::string value = luaL_checkstring(L, 3);

	if (co->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
		co->setExtendedAttribute(attr, value);
	}
	return 1;
}

// get_attribute(self, attribute)
int ObjectRef::l_get_attribute(lua_State *L)
{
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* co = getplayersao(ref);
	if (co == NULL) {
		return 0;
	}

	std::string attr = luaL_checkstring(L, 2);

	std::string value = "";
	if (co->getExtendedAttribute(attr, &value)) {
		lua_pushstring(L, value.c_str());
		return 1;
	}

	return 0;
}


// set_inventory_formspec(self, formspec)
int ObjectRef::l_set_inventory_formspec(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL) return 0;
	std::string formspec = luaL_checkstring(L, 2);

	player->inventory_formspec = formspec;
	getServer(L)->reportInventoryFormspecModified(player->getName());
	lua_pushboolean(L, true);
	return 1;
}

// get_inventory_formspec(self) -> formspec
int ObjectRef::l_get_inventory_formspec(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL) return 0;

	std::string formspec = player->inventory_formspec;
	lua_pushlstring(L, formspec.c_str(), formspec.size());
	return 1;
}

// get_player_control(self)
int ObjectRef::l_get_player_control(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL) {
		lua_pushlstring(L, "", 0);
		return 1;
	}

	const PlayerControl &control = player->getPlayerControl();
	lua_newtable(L);
	lua_pushboolean(L, control.up);
	lua_setfield(L, -2, "up");
	lua_pushboolean(L, control.down);
	lua_setfield(L, -2, "down");
	lua_pushboolean(L, control.left);
	lua_setfield(L, -2, "left");
	lua_pushboolean(L, control.right);
	lua_setfield(L, -2, "right");
	lua_pushboolean(L, control.jump);
	lua_setfield(L, -2, "jump");
	lua_pushboolean(L, control.aux1);
	lua_setfield(L, -2, "aux1");
	lua_pushboolean(L, control.sneak);
	lua_setfield(L, -2, "sneak");
	lua_pushboolean(L, control.LMB);
	lua_setfield(L, -2, "LMB");
	lua_pushboolean(L, control.RMB);
	lua_setfield(L, -2, "RMB");
	return 1;
}

// get_player_control_bits(self)
int ObjectRef::l_get_player_control_bits(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL) {
		lua_pushlstring(L, "", 0);
		return 1;
	}
	// Do it
	lua_pushnumber(L, player->keyPressed);
	return 1;
}

// hud_add(self, form)
int ObjectRef::l_hud_add(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	HudElement *elem = new HudElement;

	elem->type = (HudElementType)getenumfield(L, 2, "hud_elem_type",
								es_HudElementType, HUD_ELEM_TEXT);

	lua_getfield(L, 2, "position");
	elem->pos = lua_istable(L, -1) ? read_v2f(L, -1) : v2f();
	lua_pop(L, 1);

	lua_getfield(L, 2, "scale");
	elem->scale = lua_istable(L, -1) ? read_v2f(L, -1) : v2f();
	lua_pop(L, 1);

	lua_getfield(L, 2, "size");
	elem->size = lua_istable(L, -1) ? read_v2s32(L, -1) : v2s32();
	lua_pop(L, 1);

	elem->name   = getstringfield_default(L, 2, "name", "");
	elem->text   = getstringfield_default(L, 2, "text", "");
	elem->number = getintfield_default(L, 2, "number", 0);
	elem->item   = getintfield_default(L, 2, "item", 0);
	elem->dir    = getintfield_default(L, 2, "direction", 0);

	// Deprecated, only for compatibility's sake
	if (elem->dir == 0)
		elem->dir = getintfield_default(L, 2, "dir", 0);

	lua_getfield(L, 2, "alignment");
	elem->align = lua_istable(L, -1) ? read_v2f(L, -1) : v2f();
	lua_pop(L, 1);

	lua_getfield(L, 2, "offset");
	elem->offset = lua_istable(L, -1) ? read_v2f(L, -1) : v2f();
	lua_pop(L, 1);

	lua_getfield(L, 2, "world_pos");
	elem->world_pos = lua_istable(L, -1) ? read_v3f(L, -1) : v3f();
	lua_pop(L, 1);

	/* check for known deprecated element usage */
	if ((elem->type  == HUD_ELEM_STATBAR) && (elem->size == v2s32())) {
		log_deprecated(L,"Deprecated usage of statbar without size!");
	}

	u32 id = getServer(L)->hudAdd(player, elem);
	if (id == U32_MAX) {
		delete elem;
		return 0;
	}

	lua_pushnumber(L, id);
	return 1;
}

// hud_remove(self, id)
int ObjectRef::l_hud_remove(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	u32 id = -1;
	if (!lua_isnil(L, 2))
		id = lua_tonumber(L, 2);

	if (!getServer(L)->hudRemove(player, id))
		return 0;

	lua_pushboolean(L, true);
	return 1;
}

// hud_change(self, id, stat, data)
int ObjectRef::l_hud_change(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	u32 id = lua_isnumber(L, 2) ? lua_tonumber(L, 2) : -1;

	HudElement *e = player->getHud(id);
	if (!e)
		return 0;

	HudElementStat stat = HUD_STAT_NUMBER;
	if (lua_isstring(L, 3)) {
		int statint;
		std::string statstr = lua_tostring(L, 3);
		stat = string_to_enum(es_HudElementStat, statint, statstr) ?
				(HudElementStat)statint : HUD_STAT_NUMBER;
	}

	void *value = NULL;
	switch (stat) {
		case HUD_STAT_POS:
			e->pos = read_v2f(L, 4);
			value = &e->pos;
			break;
		case HUD_STAT_NAME:
			e->name = luaL_checkstring(L, 4);
			value = &e->name;
			break;
		case HUD_STAT_SCALE:
			e->scale = read_v2f(L, 4);
			value = &e->scale;
			break;
		case HUD_STAT_TEXT:
			e->text = luaL_checkstring(L, 4);
			value = &e->text;
			break;
		case HUD_STAT_NUMBER:
			e->number = luaL_checknumber(L, 4);
			value = &e->number;
			break;
		case HUD_STAT_ITEM:
			e->item = luaL_checknumber(L, 4);
			value = &e->item;
			break;
		case HUD_STAT_DIR:
			e->dir = luaL_checknumber(L, 4);
			value = &e->dir;
			break;
		case HUD_STAT_ALIGN:
			e->align = read_v2f(L, 4);
			value = &e->align;
			break;
		case HUD_STAT_OFFSET:
			e->offset = read_v2f(L, 4);
			value = &e->offset;
			break;
		case HUD_STAT_WORLD_POS:
			e->world_pos = read_v3f(L, 4);
			value = &e->world_pos;
			break;
		case HUD_STAT_SIZE:
			e->size = read_v2s32(L, 4);
			value = &e->size;
			break;
	}

	getServer(L)->hudChange(player, id, stat, value);

	lua_pushboolean(L, true);
	return 1;
}

// hud_get(self, id)
int ObjectRef::l_hud_get(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	u32 id = lua_tonumber(L, -1);

	HudElement *e = player->getHud(id);
	if (!e)
		return 0;

	lua_newtable(L);

	lua_pushstring(L, es_HudElementType[(u8)e->type].str);
	lua_setfield(L, -2, "type");

	push_v2f(L, e->pos);
	lua_setfield(L, -2, "position");

	lua_pushstring(L, e->name.c_str());
	lua_setfield(L, -2, "name");

	push_v2f(L, e->scale);
	lua_setfield(L, -2, "scale");

	lua_pushstring(L, e->text.c_str());
	lua_setfield(L, -2, "text");

	lua_pushnumber(L, e->number);
	lua_setfield(L, -2, "number");

	lua_pushnumber(L, e->item);
	lua_setfield(L, -2, "item");

	lua_pushnumber(L, e->dir);
	lua_setfield(L, -2, "direction");

	// Deprecated, only for compatibility's sake
	lua_pushnumber(L, e->dir);
	lua_setfield(L, -2, "dir");

	push_v3f(L, e->world_pos);
	lua_setfield(L, -2, "world_pos");

	return 1;
}

// hud_set_flags(self, flags)
int ObjectRef::l_hud_set_flags(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	u32 flags = 0;
	u32 mask  = 0;
	bool flag;

	const EnumString *esp = es_HudBuiltinElement;
	for (int i = 0; esp[i].str; i++) {
		if (getboolfield(L, 2, esp[i].str, flag)) {
			flags |= esp[i].num * flag;
			mask  |= esp[i].num;
		}
	}
	if (!getServer(L)->hudSetFlags(player, flags, mask))
		return 0;

	lua_pushboolean(L, true);
	return 1;
}

int ObjectRef::l_hud_get_flags(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	lua_newtable(L);
	lua_pushboolean(L, player->hud_flags & HUD_FLAG_HOTBAR_VISIBLE);
	lua_setfield(L, -2, "hotbar");
	lua_pushboolean(L, player->hud_flags & HUD_FLAG_HEALTHBAR_VISIBLE);
	lua_setfield(L, -2, "healthbar");
	lua_pushboolean(L, player->hud_flags & HUD_FLAG_CROSSHAIR_VISIBLE);
	lua_setfield(L, -2, "crosshair");
	lua_pushboolean(L, player->hud_flags & HUD_FLAG_WIELDITEM_VISIBLE);
	lua_setfield(L, -2, "wielditem");
	lua_pushboolean(L, player->hud_flags & HUD_FLAG_BREATHBAR_VISIBLE);
	lua_setfield(L, -2, "breathbar");
	lua_pushboolean(L, player->hud_flags & HUD_FLAG_MINIMAP_VISIBLE);
	lua_setfield(L, -2, "minimap");

	return 1;
}

// hud_set_hotbar_itemcount(self, hotbar_itemcount)
int ObjectRef::l_hud_set_hotbar_itemcount(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	s32 hotbar_itemcount = lua_tonumber(L, 2);

	if (!getServer(L)->hudSetHotbarItemcount(player, hotbar_itemcount))
		return 0;

	lua_pushboolean(L, true);
	return 1;
}

// hud_get_hotbar_itemcount(self)
int ObjectRef::l_hud_get_hotbar_itemcount(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	s32 hotbar_itemcount = getServer(L)->hudGetHotbarItemcount(player);

	lua_pushnumber(L, hotbar_itemcount);
	return 1;
}

// hud_set_hotbar_image(self, name)
int ObjectRef::l_hud_set_hotbar_image(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	std::string name = lua_tostring(L, 2);

	getServer(L)->hudSetHotbarImage(player, name);
	return 1;
}

// hud_get_hotbar_image(self)
int ObjectRef::l_hud_get_hotbar_image(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	std::string name = getServer(L)->hudGetHotbarImage(player);
	lua_pushlstring(L, name.c_str(), name.size());
	return 1;
}

// hud_set_hotbar_selected_image(self, name)
int ObjectRef::l_hud_set_hotbar_selected_image(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	std::string name = lua_tostring(L, 2);

	getServer(L)->hudSetHotbarSelectedImage(player, name);
	return 1;
}

// hud_get_hotbar_selected_image(self)
int ObjectRef::l_hud_get_hotbar_selected_image(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	const std::string &name = getServer(L)->hudGetHotbarSelectedImage(player);
	lua_pushlstring(L, name.c_str(), name.size());
	return 1;
}

// set_sky(self, bgcolor, type, list)
int ObjectRef::l_set_sky(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	video::SColor bgcolor(255,255,255,255);
	read_color(L, 2, &bgcolor);

	std::string type = luaL_checkstring(L, 3);

	std::vector<std::string> params;
	if (lua_istable(L, 4)) {
		int table = lua_gettop(L);
		lua_pushnil(L);
		while (lua_next(L, table) != 0) {
			// key at index -2 and value at index -1
			if (lua_isstring(L, -1))
				params.push_back(lua_tostring(L, -1));
			else
				params.push_back("");
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	}

	if (type == "skybox" && params.size() != 6)
		throw LuaError("skybox expects 6 textures");

	if (!getServer(L)->setSky(player, bgcolor, type, params))
		return 0;

	lua_pushboolean(L, true);
	return 1;
}

// get_sky(self)
int ObjectRef::l_get_sky(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;
	video::SColor bgcolor(255, 255, 255, 255);
	std::string type;
	std::vector<std::string> params;

	player->getSky(&bgcolor, &type, &params);
	type = type == "" ? "regular" : type;

	push_ARGB8(L, bgcolor);
	lua_pushlstring(L, type.c_str(), type.size());
	lua_newtable(L);
	s16 i = 1;
	for (std::vector<std::string>::iterator it = params.begin();
			it != params.end(); ++it) {
		lua_pushlstring(L, it->c_str(), it->size());
		lua_rawseti(L, -2, i);
		i++;
	}
	return 3;
}

// set_clouds(self, {density=, color=, ambient=, height=, thickness=, speed=})
int ObjectRef::l_set_clouds(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (!player)
		return 0;
	if (!lua_istable(L, 2))
		return 0;

	CloudParams cloud_params = player->getCloudParams();

	cloud_params.density = getfloatfield_default(L, 2, "density", cloud_params.density);

	lua_getfield(L, 2, "color");
	if (!lua_isnil(L, -1))
		read_color(L, -1, &cloud_params.color_bright);
	lua_pop(L, 1);
	lua_getfield(L, 2, "ambient");
	if (!lua_isnil(L, -1))
		read_color(L, -1, &cloud_params.color_ambient);
	lua_pop(L, 1);

	cloud_params.height    = getfloatfield_default(L, 2, "height",    cloud_params.height   );
	cloud_params.thickness = getfloatfield_default(L, 2, "thickness", cloud_params.thickness);

	lua_getfield(L, 2, "speed");
	if (lua_istable(L, -1)) {
		v2f new_speed;
		new_speed.X = getfloatfield_default(L, -1, "x", 0);
		new_speed.Y = getfloatfield_default(L, -1, "y", 0);
		cloud_params.speed = new_speed;
	}
	lua_pop(L, 1);

	if (!getServer(L)->setClouds(player, cloud_params.density,
			cloud_params.color_bright, cloud_params.color_ambient,
			cloud_params.height, cloud_params.thickness,
			cloud_params.speed))
		return 0;

	player->setCloudParams(cloud_params);

	lua_pushboolean(L, true);
	return 1;
}

int ObjectRef::l_get_clouds(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (!player)
		return 0;
	const CloudParams &cloud_params = player->getCloudParams();

	lua_newtable(L);
	lua_pushnumber(L, cloud_params.density);
	lua_setfield(L, -2, "density");
	push_ARGB8(L, cloud_params.color_bright);
	lua_setfield(L, -2, "color");
	push_ARGB8(L, cloud_params.color_ambient);
	lua_setfield(L, -2, "ambient");
	lua_pushnumber(L, cloud_params.height);
	lua_setfield(L, -2, "height");
	lua_pushnumber(L, cloud_params.thickness);
	lua_setfield(L, -2, "thickness");
	lua_newtable(L);
	lua_pushnumber(L, cloud_params.speed.X);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, cloud_params.speed.Y);
	lua_setfield(L, -2, "y");
	lua_setfield(L, -2, "speed");

	return 1;
}


// override_day_night_ratio(self, brightness=0...1)
int ObjectRef::l_override_day_night_ratio(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	bool do_override = false;
	float ratio = 0.0f;
	if (!lua_isnil(L, 2)) {
		do_override = true;
		ratio = luaL_checknumber(L, 2);
	}

	if (!getServer(L)->overrideDayNightRatio(player, do_override, ratio))
		return 0;

	lua_pushboolean(L, true);
	return 1;
}

// get_day_night_ratio(self)
int ObjectRef::l_get_day_night_ratio(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == NULL)
		return 0;

	bool do_override;
	float ratio;
	player->getDayNightRatio(&do_override, &ratio);

	if (do_override)
		lua_pushnumber(L, ratio);
	else
		lua_pushnil(L);

	return 1;
}

ObjectRef::ObjectRef(ServerActiveObject *object):
	m_object(object)
{
	//infostream<<"ObjectRef created for id="<<m_object->getId()<<std::endl;
}

ObjectRef::~ObjectRef()
{
	/*if (m_object)
		infostream<<"ObjectRef destructing for id="
				<<m_object->getId()<<std::endl;
	else
		infostream<<"ObjectRef destructing for id=unknown"<<std::endl;*/
}

// Creates an ObjectRef and leaves it on top of stack
// Not callable from Lua; all references are created on the C side.
void ObjectRef::create(lua_State *L, ServerActiveObject *object)
{
	ObjectRef *o = new ObjectRef(object);
	//infostream<<"ObjectRef::create: o="<<o<<std::endl;
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void ObjectRef::set_null(lua_State *L)
{
	ObjectRef *o = checkobject(L, -1);
	o->m_object = NULL;
}

void ObjectRef::Register(lua_State *L)
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

const char ObjectRef::className[] = "ObjectRef";
const luaL_Reg ObjectRef::methods[] = {
	// ServerActiveObject
	luamethod(ObjectRef, remove),
	luamethod_aliased(ObjectRef, get_pos, getpos),
	luamethod_aliased(ObjectRef, set_pos, setpos),
	luamethod_aliased(ObjectRef, move_to, moveto),
	luamethod(ObjectRef, punch),
	luamethod(ObjectRef, right_click),
	luamethod(ObjectRef, set_hp),
	luamethod(ObjectRef, get_hp),
	luamethod(ObjectRef, get_inventory),
	luamethod(ObjectRef, get_wield_list),
	luamethod(ObjectRef, get_wield_index),
	luamethod(ObjectRef, get_wielded_item),
	luamethod(ObjectRef, set_wielded_item),
	luamethod(ObjectRef, set_armor_groups),
	luamethod(ObjectRef, get_armor_groups),
	luamethod(ObjectRef, set_animation),
	luamethod(ObjectRef, get_animation),
	luamethod(ObjectRef, set_bone_position),
	luamethod(ObjectRef, get_bone_position),
	luamethod(ObjectRef, set_attach),
	luamethod(ObjectRef, get_attach),
	luamethod(ObjectRef, set_detach),
	luamethod(ObjectRef, set_properties),
	luamethod(ObjectRef, get_properties),
	luamethod(ObjectRef, set_nametag_attributes),
	luamethod(ObjectRef, get_nametag_attributes),
	// LuaEntitySAO-only
	luamethod_aliased(ObjectRef, set_velocity, setvelocity),
	luamethod_aliased(ObjectRef, get_velocity, getvelocity),
	luamethod_aliased(ObjectRef, set_acceleration, setacceleration),
	luamethod_aliased(ObjectRef, get_acceleration, getacceleration),
	luamethod_aliased(ObjectRef, set_yaw, setyaw),
	luamethod_aliased(ObjectRef, get_yaw, getyaw),
	luamethod_aliased(ObjectRef, set_texture_mod, settexturemod),
	luamethod_aliased(ObjectRef, set_sprite, setsprite),
	luamethod(ObjectRef, get_entity_name),
	luamethod(ObjectRef, get_luaentity),
	// Player-only
	luamethod(ObjectRef, is_player),
	luamethod(ObjectRef, is_player_connected),
	luamethod(ObjectRef, get_player_name),
	luamethod(ObjectRef, get_player_velocity),
	luamethod(ObjectRef, get_look_dir),
	luamethod(ObjectRef, get_look_pitch),
	luamethod(ObjectRef, get_look_yaw),
	luamethod(ObjectRef, get_look_vertical),
	luamethod(ObjectRef, get_look_horizontal),
	luamethod(ObjectRef, set_look_horizontal),
	luamethod(ObjectRef, set_look_vertical),
	luamethod(ObjectRef, set_look_yaw),
	luamethod(ObjectRef, set_look_pitch),
	luamethod(ObjectRef, get_breath),
	luamethod(ObjectRef, set_breath),
	luamethod(ObjectRef, get_attribute),
	luamethod(ObjectRef, set_attribute),
	luamethod(ObjectRef, set_inventory_formspec),
	luamethod(ObjectRef, get_inventory_formspec),
	luamethod(ObjectRef, get_player_control),
	luamethod(ObjectRef, get_player_control_bits),
	luamethod(ObjectRef, set_physics_override),
	luamethod(ObjectRef, get_physics_override),
	luamethod(ObjectRef, hud_add),
	luamethod(ObjectRef, hud_remove),
	luamethod(ObjectRef, hud_change),
	luamethod(ObjectRef, hud_get),
	luamethod(ObjectRef, hud_set_flags),
	luamethod(ObjectRef, hud_get_flags),
	luamethod(ObjectRef, hud_set_hotbar_itemcount),
	luamethod(ObjectRef, hud_get_hotbar_itemcount),
	luamethod(ObjectRef, hud_set_hotbar_image),
	luamethod(ObjectRef, hud_get_hotbar_image),
	luamethod(ObjectRef, hud_set_hotbar_selected_image),
	luamethod(ObjectRef, hud_get_hotbar_selected_image),
	luamethod(ObjectRef, set_sky),
	luamethod(ObjectRef, get_sky),
	luamethod(ObjectRef, set_clouds),
	luamethod(ObjectRef, get_clouds),
	luamethod(ObjectRef, override_day_night_ratio),
	luamethod(ObjectRef, get_day_night_ratio),
	luamethod(ObjectRef, set_local_animation),
	luamethod(ObjectRef, get_local_animation),
	luamethod(ObjectRef, set_eye_offset),
	luamethod(ObjectRef, get_eye_offset),
	{0,0}
};

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
#include <cmath>
#include "lua_api/l_internal.h"
#include "lua_api/l_inventory.h"
#include "lua_api/l_item.h"
#include "lua_api/l_playermeta.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "log.h"
#include "tool.h"
#include "remoteplayer.h"
#include "server.h"
#include "hud.h"
#include "scripting_server.h"
#include "server/luaentity_sao.h"
#include "server/player_sao.h"
#include "server/serverinventorymgr.h"

/*
	ObjectRef
*/


ObjectRef* ObjectRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if (ud == nullptr)
		luaL_typerror(L, narg, className);
	return *(ObjectRef**)ud;  // unbox pointer
}

ServerActiveObject* ObjectRef::getobject(ObjectRef *ref)
{
	ServerActiveObject *sao = ref->m_object;
	if (sao && sao->isGone())
		return nullptr;
	return sao;
}

LuaEntitySAO* ObjectRef::getluaobject(ObjectRef *ref)
{
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return nullptr;
	if (sao->getType() != ACTIVEOBJECT_TYPE_LUAENTITY)
		return nullptr;
	return (LuaEntitySAO*)sao;
}

PlayerSAO* ObjectRef::getplayersao(ObjectRef *ref)
{
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return nullptr;
	if (sao->getType() != ACTIVEOBJECT_TYPE_PLAYER)
		return nullptr;
	return (PlayerSAO*)sao;
}

RemotePlayer *ObjectRef::getplayer(ObjectRef *ref)
{
	PlayerSAO *playersao = getplayersao(ref);
	if (playersao == nullptr)
		return nullptr;
	return playersao->getPlayer();
}

// Exported functions

// garbage collector
int ObjectRef::gc_object(lua_State *L) {
	ObjectRef *obj = *(ObjectRef **)(lua_touserdata(L, 1));
	delete obj;
	return 0;
}

// remove(self)
int ObjectRef::l_remove(lua_State *L)
{
	GET_ENV_PTR;

	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;
	if (sao->getType() == ACTIVEOBJECT_TYPE_PLAYER)
		return 0;

	sao->clearChildAttachments();
	sao->clearParentAttachment();

	verbosestream << "ObjectRef::l_remove(): id=" << sao->getId() << std::endl;
	sao->markForRemoval();
	return 0;
}

// get_pos(self)
int ObjectRef::l_get_pos(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	push_v3f(L, sao->getBasePosition() / BS);
	return 1;
}

// set_pos(self, pos)
int ObjectRef::l_set_pos(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	v3f pos = checkFloatPos(L, 2);

	sao->setPos(pos);
	return 0;
}

// move_to(self, pos, continuous)
int ObjectRef::l_move_to(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	v3f pos = checkFloatPos(L, 2);
	bool continuous = readParam<bool>(L, 3);

	sao->moveTo(pos, continuous);
	return 0;
}

// punch(self, puncher, time_from_last_punch, tool_capabilities, dir)
int ObjectRef::l_punch(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ObjectRef *puncher_ref = checkobject(L, 2);
	ServerActiveObject *sao = getobject(ref);
	ServerActiveObject *puncher = getobject(puncher_ref);
	if (sao == nullptr || puncher == nullptr)
		return 0;

	float time_from_last_punch = readParam<float>(L, 3, 1000000.0f);
	ToolCapabilities toolcap = read_tool_capabilities(L, 4);
	v3f dir = readParam<v3f>(L, 5, sao->getBasePosition() - puncher->getBasePosition());
	dir.normalize();

	u32 wear = sao->punch(dir, &toolcap, puncher, time_from_last_punch);
	lua_pushnumber(L, wear);

	return 1;
}

// right_click(self, clicker)
int ObjectRef::l_right_click(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ObjectRef *ref2 = checkobject(L, 2);
	ServerActiveObject *sao = getobject(ref);
	ServerActiveObject *sao2 = getobject(ref2);
	if (sao == nullptr || sao2 == nullptr)
		return 0;

	sao->rightClick(sao2);
	return 0;
}

// set_hp(self, hp, reason)
int ObjectRef::l_set_hp(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	int hp = readParam<float>(L, 2);
	PlayerHPChangeReason reason(PlayerHPChangeReason::SET_HP);

	reason.from_mod = true;
	if (lua_istable(L, 3)) {
		lua_pushvalue(L, 3);

		lua_getfield(L, -1, "type");
		if (lua_isstring(L, -1) &&
				!reason.setTypeFromString(readParam<std::string>(L, -1))) {
			errorstream << "Bad type given!" << std::endl;
		}
		lua_pop(L, 1);

		reason.lua_reference = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	sao->setHP(hp, reason);
	if (reason.hasLuaReference())
		luaL_unref(L, LUA_REGISTRYINDEX, reason.lua_reference);
	return 0;
}

// get_hp(self)
int ObjectRef::l_get_hp(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr) {
		// Default hp is 1
		lua_pushnumber(L, 1);
		return 1;
	}

	int hp = sao->getHP();

	lua_pushnumber(L, hp);
	return 1;
}

// get_inventory(self)
int ObjectRef::l_get_inventory(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	InventoryLocation loc = sao->getInventoryLocation();
	if (getServerInventoryMgr(L)->getInventory(loc) != nullptr)
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
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	lua_pushstring(L, sao->getWieldList().c_str());
	return 1;
}

// get_wield_index(self)
int ObjectRef::l_get_wield_index(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	lua_pushinteger(L, sao->getWieldIndex() + 1);
	return 1;
}

// get_wielded_item(self)
int ObjectRef::l_get_wielded_item(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr) {
		// Empty ItemStack
		LuaItemStack::create(L, ItemStack());
		return 1;
	}

	ItemStack selected_item;
	sao->getWieldedItem(&selected_item, nullptr);
	LuaItemStack::create(L, selected_item);
	return 1;
}

// set_wielded_item(self, item)
int ObjectRef::l_set_wielded_item(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	ItemStack item = read_item(L, 2, getServer(L)->idef());

	bool success = sao->setWieldedItem(item);
	if (success && sao->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
		getServer(L)->SendInventory((PlayerSAO *)sao, true);
	}
	lua_pushboolean(L, success);
	return 1;
}

// set_armor_groups(self, groups)
int ObjectRef::l_set_armor_groups(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	ItemGroupList groups;

	read_groups(L, 2, groups);
	if (sao->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
		if (!g_settings->getBool("enable_damage") && !itemgroup_get(groups, "immortal")) {
			warningstream << "Mod tried to enable damage for a player, but it's "
				"disabled globally. Ignoring." << std::endl;
			infostream << script_get_backtrace(L) << std::endl;
			groups["immortal"] = 1;
		}
	}

	sao->setArmorGroups(groups);
	return 0;
}

// get_armor_groups(self)
int ObjectRef::l_get_armor_groups(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	push_groups(L, sao->getArmorGroups());
	return 1;
}

// set_animation(self, frame_range, frame_speed, frame_blend, frame_loop)
int ObjectRef::l_set_animation(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	v2f frame_range   = readParam<v2f>(L,  2, v2f(1, 1));
	float frame_speed = readParam<float>(L, 3, 15.0f);
	float frame_blend = readParam<float>(L, 4, 0.0f);
	bool frame_loop   = readParam<bool>(L, 5, true);

	sao->setAnimation(frame_range, frame_speed, frame_blend, frame_loop);
	return 0;
}

// get_animation(self)
int ObjectRef::l_get_animation(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	v2f frames = v2f(1, 1);
	float frame_speed = 15;
	float frame_blend = 0;
	bool frame_loop = true;

	sao->getAnimation(&frames, &frame_speed, &frame_blend, &frame_loop);
	push_v2f(L, frames);
	lua_pushnumber(L, frame_speed);
	lua_pushnumber(L, frame_blend);
	lua_pushboolean(L, frame_loop);
	return 4;
}

// set_local_animation(self, idle, walk, dig, walk_while_dig, frame_speed)
int ObjectRef::l_set_local_animation(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	v2s32 frames[4];
	for (int i=0;i<4;i++) {
		if (!lua_isnil(L, 2+1))
			frames[i] = read_v2s32(L, 2+i);
	}
	float frame_speed = readParam<float>(L, 6, 30.0f);

	getServer(L)->setLocalPlayerAnimations(player, frames, frame_speed);
	lua_pushboolean(L, true);
	return 1;
}

// get_local_animation(self)
int ObjectRef::l_get_local_animation(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	v2s32 frames[4];
	float frame_speed;
	player->getLocalAnimations(frames, &frame_speed);

	for (const v2s32 &frame : frames) {
		push_v2s32(L, frame);
	}

	lua_pushnumber(L, frame_speed);
	return 5;
}

// set_eye_offset(self, firstperson, thirdperson)
int ObjectRef::l_set_eye_offset(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	v3f offset_first = readParam<v3f>(L, 2, v3f(0, 0, 0));
	v3f offset_third = readParam<v3f>(L, 3, v3f(0, 0, 0));

	// Prevent abuse of offset values (keep player always visible)
	offset_third.X = rangelim(offset_third.X,-10,10);
	offset_third.Z = rangelim(offset_third.Z,-5,5);
	/* TODO: if possible: improve the camera collision detection to allow Y <= -1.5) */
	offset_third.Y = rangelim(offset_third.Y,-10,15); //1.5*BS

	getServer(L)->setPlayerEyeOffset(player, offset_first, offset_third);
	lua_pushboolean(L, true);
	return 1;
}

// get_eye_offset(self)
int ObjectRef::l_get_eye_offset(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	push_v3f(L, player->eye_offset_first);
	push_v3f(L, player->eye_offset_third);
	return 2;
}

// send_mapblock(self, pos)
int ObjectRef::l_send_mapblock(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	v3s16 pos = read_v3s16(L, 2);

	session_t peer_id = player->getPeerId();
	bool r = getServer(L)->SendBlock(peer_id, pos);

	lua_pushboolean(L, r);
	return 1;
}

// set_animation_frame_speed(self, frame_speed)
int ObjectRef::l_set_animation_frame_speed(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	if (!lua_isnil(L, 2)) {
		float frame_speed = readParam<float>(L, 2);
		sao->setAnimationSpeed(frame_speed);
		lua_pushboolean(L, true);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}

// set_bone_position(self, bone, position, rotation)
int ObjectRef::l_set_bone_position(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	std::string bone = readParam<std::string>(L, 2, "");
	v3f position = readParam<v3f>(L, 3, v3f(0, 0, 0));
	v3f rotation = readParam<v3f>(L, 4, v3f(0, 0, 0));

	sao->setBonePosition(bone, position, rotation);
	return 0;
}

// get_bone_position(self, bone)
int ObjectRef::l_get_bone_position(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	std::string bone = readParam<std::string>(L, 2, "");

	v3f position = v3f(0, 0, 0);
	v3f rotation = v3f(0, 0, 0);
	sao->getBonePosition(bone, &position, &rotation);

	push_v3f(L, position);
	push_v3f(L, rotation);
	return 2;
}

// set_attach(self, parent, bone, position, rotation, force_visible)
int ObjectRef::l_set_attach(lua_State *L)
{
	GET_ENV_PTR;
	ObjectRef *ref = checkobject(L, 1);
	ObjectRef *parent_ref = checkobject(L, 2);
	ServerActiveObject *sao = getobject(ref);
	ServerActiveObject *parent = getobject(parent_ref);
	if (sao == nullptr || parent == nullptr)
		return 0;
	if (sao == parent)
		throw LuaError("ObjectRef::set_attach: attaching object to itself is not allowed.");

	int parent_id;
	std::string bone;
	v3f position;
	v3f rotation;
	bool force_visible;

	sao->getAttachment(&parent_id, &bone, &position, &rotation, &force_visible);
	if (parent_id) {
		ServerActiveObject *old_parent = env->getActiveObject(parent_id);
		old_parent->removeAttachmentChild(sao->getId());
	}

	bone          = readParam<std::string>(L, 3, "");
	position      = readParam<v3f>(L, 4, v3f(0, 0, 0));
	rotation      = readParam<v3f>(L, 5, v3f(0, 0, 0));
	force_visible = readParam<bool>(L, 6, false);

	sao->setAttachment(parent->getId(), bone, position, rotation, force_visible);
	parent->addAttachmentChild(sao->getId());
	return 0;
}

// get_attach(self)
int ObjectRef::l_get_attach(lua_State *L)
{
	GET_ENV_PTR;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	int parent_id;
	std::string bone;
	v3f position;
	v3f rotation;
	bool force_visible;

	sao->getAttachment(&parent_id, &bone, &position, &rotation, &force_visible);
	if (parent_id == 0)
		return 0;

	ServerActiveObject *parent = env->getActiveObject(parent_id);
	getScriptApiBase(L)->objectrefGetOrCreate(L, parent);
	lua_pushlstring(L, bone.c_str(), bone.size());
	push_v3f(L, position);
	push_v3f(L, rotation);
	lua_pushboolean(L, force_visible);
	return 5;
}

// get_children(self)
int ObjectRef::l_get_children(lua_State *L)
{
	GET_ENV_PTR;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	const std::unordered_set<int> child_ids = sao->getAttachmentChildIds();
	int i = 0;

	lua_createtable(L, child_ids.size(), 0);
	for (const int id : child_ids) {
		ServerActiveObject *child = env->getActiveObject(id);
		getScriptApiBase(L)->objectrefGetOrCreate(L, child);
		lua_rawseti(L, -2, ++i);
	}
	return 1;
}

// set_detach(self)
int ObjectRef::l_set_detach(lua_State *L)
{
	GET_ENV_PTR;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	sao->clearParentAttachment();
	return 0;
}

// set_properties(self, properties)
int ObjectRef::l_set_properties(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	ObjectProperties *prop = sao->accessObjectProperties();
	if (prop == nullptr)
		return 0;

	read_object_properties(L, 2, sao, prop, getServer(L)->idef());
	prop->validate();
	sao->notifyObjectPropertiesModified();
	return 0;
}

// get_properties(self)
int ObjectRef::l_get_properties(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	ObjectProperties *prop = sao->accessObjectProperties();
	if (prop == nullptr)
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
	lua_pushboolean(L, (player != nullptr));
	return 1;
}

// set_nametag_attributes(self, attributes)
int ObjectRef::l_set_nametag_attributes(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	ObjectProperties *prop = sao->accessObjectProperties();
	if (prop == nullptr)
		return 0;

	lua_getfield(L, 2, "color");
	if (!lua_isnil(L, -1)) {
		video::SColor color = prop->nametag_color;
		read_color(L, -1, &color);
		prop->nametag_color = color;
	}
	lua_pop(L, 1);

	lua_getfield(L, -1, "bgcolor");
	if (!lua_isnil(L, -1)) {
		if (lua_toboolean(L, -1)) {
			video::SColor color;
			if (read_color(L, -1, &color))
				prop->nametag_bgcolor = color;
		} else {
			prop->nametag_bgcolor = nullopt;
		}
	}
	lua_pop(L, 1);

	std::string nametag = getstringfield_default(L, 2, "text", "");
	prop->nametag = nametag;

	prop->validate();
	sao->notifyObjectPropertiesModified();
	lua_pushboolean(L, true);
	return 1;
}

// get_nametag_attributes(self)
int ObjectRef::l_get_nametag_attributes(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	ObjectProperties *prop = sao->accessObjectProperties();
	if (!prop)
		return 0;

	lua_newtable(L);

	push_ARGB8(L, prop->nametag_color);
	lua_setfield(L, -2, "color");

	if (prop->nametag_bgcolor) {
		push_ARGB8(L, prop->nametag_bgcolor.value());
		lua_setfield(L, -2, "bgcolor");
	} else {
		lua_pushboolean(L, false);
		lua_setfield(L, -2, "bgcolor");
	}

	lua_pushstring(L, prop->nametag.c_str());
	lua_setfield(L, -2, "text");



	return 1;
}

/* LuaEntitySAO-only */

// set_velocity(self, velocity)
int ObjectRef::l_set_velocity(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *sao = getluaobject(ref);
	if (sao == nullptr)
		return 0;

	v3f vel = checkFloatPos(L, 2);

	sao->setVelocity(vel);
	return 0;
}

// add_velocity(self, velocity)
int ObjectRef::l_add_velocity(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	v3f vel = checkFloatPos(L, 2);

	if (sao->getType() == ACTIVEOBJECT_TYPE_LUAENTITY) {
		LuaEntitySAO *entitysao = dynamic_cast<LuaEntitySAO*>(sao);
		entitysao->addVelocity(vel);
	} else if (sao->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
		PlayerSAO *playersao = dynamic_cast<PlayerSAO*>(sao);
		playersao->setMaxSpeedOverride(vel);
		getServer(L)->SendPlayerSpeed(playersao->getPeerID(), vel);
	}

	return 0;
}

// get_velocity(self)
int ObjectRef::l_get_velocity(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	ServerActiveObject *sao = getobject(ref);
	if (sao == nullptr)
		return 0;

	if (sao->getType() == ACTIVEOBJECT_TYPE_LUAENTITY) {
		LuaEntitySAO *entitysao = dynamic_cast<LuaEntitySAO*>(sao);
		v3f vel = entitysao->getVelocity();
		pushFloatPos(L, vel);
		return 1;
	} else if (sao->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
		RemotePlayer *player = dynamic_cast<PlayerSAO*>(sao)->getPlayer();
		push_v3f(L, player->getSpeed() / BS);
		return 1;
	}

	lua_pushnil(L);
	return 1;
}

// set_acceleration(self, acceleration)
int ObjectRef::l_set_acceleration(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *entitysao = getluaobject(ref);
	if (entitysao == nullptr)
		return 0;

	v3f acceleration = checkFloatPos(L, 2);

	entitysao->setAcceleration(acceleration);
	return 0;
}

// get_acceleration(self)
int ObjectRef::l_get_acceleration(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *entitysao = getluaobject(ref);
	if (entitysao == nullptr)
		return 0;

	v3f acceleration = entitysao->getAcceleration();
	pushFloatPos(L, acceleration);
	return 1;
}

// set_rotation(self, rotation)
int ObjectRef::l_set_rotation(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *entitysao = getluaobject(ref);
	if (entitysao == nullptr)
		return 0;

	v3f rotation = check_v3f(L, 2) * core::RADTODEG;

	entitysao->setRotation(rotation);
	return 0;
}

// get_rotation(self)
int ObjectRef::l_get_rotation(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *entitysao = getluaobject(ref);
	if (entitysao == nullptr)
		return 0;

	v3f rotation = entitysao->getRotation() * core::DEGTORAD;

	lua_newtable(L);
	push_v3f(L, rotation);
	return 1;
}

// set_yaw(self, yaw)
int ObjectRef::l_set_yaw(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *entitysao = getluaobject(ref);
	if (entitysao == nullptr)
		return 0;

	float yaw = readParam<float>(L, 2) * core::RADTODEG;

	entitysao->setRotation(v3f(0, yaw, 0));
	return 0;
}

// get_yaw(self)
int ObjectRef::l_get_yaw(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *entitysao = getluaobject(ref);
	if (entitysao == nullptr)
		return 0;

	float yaw = entitysao->getRotation().Y * core::DEGTORAD;

	lua_pushnumber(L, yaw);
	return 1;
}

// set_texture_mod(self, mod)
int ObjectRef::l_set_texture_mod(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *entitysao = getluaobject(ref);
	if (entitysao == nullptr)
		return 0;

	std::string mod = readParam<std::string>(L, 2);

	entitysao->setTextureMod(mod);
	return 0;
}

// get_texture_mod(self)
int ObjectRef::l_get_texture_mod(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *entitysao = getluaobject(ref);
	if (entitysao == nullptr)
		return 0;

	std::string mod = entitysao->getTextureMod();

	lua_pushstring(L, mod.c_str());
	return 1;
}

// set_sprite(self, start_frame, num_frames, framelength, select_x_by_camera)
int ObjectRef::l_set_sprite(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *entitysao = getluaobject(ref);
	if (entitysao == nullptr)
		return 0;

	v2s16 start_frame = readParam<v2s16>(L, 2, v2s16(0,0));
	int num_frames    = readParam<int>(L, 3, 1);
	float framelength = readParam<float>(L, 4, 0.2f);
	bool select_x_by_camera = readParam<bool>(L, 5, false);

	entitysao->setSprite(start_frame, num_frames, framelength, select_x_by_camera);
	return 0;
}

// DEPRECATED
// get_entity_name(self)
int ObjectRef::l_get_entity_name(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *entitysao = getluaobject(ref);
	log_deprecated(L,"Deprecated call to \"get_entity_name");
	if (entitysao == nullptr)
		return 0;

	std::string name = entitysao->getName();

	lua_pushstring(L, name.c_str());
	return 1;
}

// get_luaentity(self)
int ObjectRef::l_get_luaentity(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	LuaEntitySAO *entitysao = getluaobject(ref);
	if (entitysao == nullptr)
		return 0;

	luaentity_get(L, entitysao->getId());
	return 1;
}

/* Player-only */

// get_player_name(self)
int ObjectRef::l_get_player_name(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr) {
		lua_pushlstring(L, "", 0);
		return 1;
	}

	lua_pushstring(L, player->getName());
	return 1;
}

// get_look_dir(self)
int ObjectRef::l_get_look_dir(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	float pitch = playersao->getRadLookPitchDep();
	float yaw = playersao->getRadYawDep();
	v3f v(std::cos(pitch) * std::cos(yaw), std::sin(pitch), std::cos(pitch) *
		std::sin(yaw));

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
	PlayerSAO* playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	lua_pushnumber(L, playersao->getRadLookPitchDep());
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
	PlayerSAO* playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	lua_pushnumber(L, playersao->getRadYawDep());
	return 1;
}

// get_look_vertical(self)
int ObjectRef::l_get_look_vertical(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	lua_pushnumber(L, playersao->getRadLookPitch());
	return 1;
}

// get_look_horizontal(self)
int ObjectRef::l_get_look_horizontal(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	lua_pushnumber(L, playersao->getRadRotation().Y);
	return 1;
}

// set_look_vertical(self, radians)
int ObjectRef::l_set_look_vertical(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	float pitch = readParam<float>(L, 2) * core::RADTODEG;

	playersao->setLookPitchAndSend(pitch);
	return 1;
}

// set_look_horizontal(self, radians)
int ObjectRef::l_set_look_horizontal(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	float yaw = readParam<float>(L, 2) * core::RADTODEG;

	playersao->setPlayerYawAndSend(yaw);
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
	PlayerSAO* playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	float pitch = readParam<float>(L, 2) * core::RADTODEG;

	playersao->setLookPitchAndSend(pitch);
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
	PlayerSAO* playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	float yaw = readParam<float>(L, 2) * core::RADTODEG;

	playersao->setPlayerYawAndSend(yaw);
	return 1;
}

// set_fov(self, degrees, is_multiplier, transition_time)
int ObjectRef::l_set_fov(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	float degrees = static_cast<f32>(luaL_checknumber(L, 2));
	bool is_multiplier = readParam<bool>(L, 3, false);
	float transition_time = lua_isnumber(L, 4) ?
		static_cast<f32>(luaL_checknumber(L, 4)) : 0.0f;

	player->setFov({degrees, is_multiplier, transition_time});
	getServer(L)->SendPlayerFov(player->getPeerId());
	return 0;
}

// get_fov(self)
int ObjectRef::l_get_fov(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	PlayerFovSpec fov_spec = player->getFov();

	lua_pushnumber(L, fov_spec.fov);
	lua_pushboolean(L, fov_spec.is_multiplier);
	lua_pushnumber(L, fov_spec.transition_time);
	return 3;
}

// set_breath(self, breath)
int ObjectRef::l_set_breath(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	u16 breath = luaL_checknumber(L, 2);

	playersao->setBreath(breath);
	return 0;
}

// get_breath(self)
int ObjectRef::l_get_breath(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	u16 breath = playersao->getBreath();

	lua_pushinteger(L, breath);
	return 1;
}

// set_attribute(self, attribute, value)
int ObjectRef::l_set_attribute(lua_State *L)
{
	log_deprecated(L,
		"Deprecated call to set_attribute, use MetaDataRef methods instead.");

	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	std::string attr = luaL_checkstring(L, 2);
	if (lua_isnil(L, 3)) {
		playersao->getMeta().removeString(attr);
	} else {
		std::string value = luaL_checkstring(L, 3);
		playersao->getMeta().setString(attr, value);
	}
	return 1;
}

// get_attribute(self, attribute)
int ObjectRef::l_get_attribute(lua_State *L)
{
	log_deprecated(L,
		"Deprecated call to get_attribute, use MetaDataRef methods instead.");

	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO* playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	std::string attr = luaL_checkstring(L, 2);

	std::string value;
	if (playersao->getMeta().getStringToRef(attr, value)) {
		lua_pushstring(L, value.c_str());
		return 1;
	}

	return 0;
}


// get_meta(self, attribute)
int ObjectRef::l_get_meta(lua_State *L)
{
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO *playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	PlayerMetaRef::create(L, &playersao->getMeta());
	return 1;
}


// set_inventory_formspec(self, formspec)
int ObjectRef::l_set_inventory_formspec(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

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
	if (player == nullptr)
		return 0;

	std::string formspec = player->inventory_formspec;

	lua_pushlstring(L, formspec.c_str(), formspec.size());
	return 1;
}

// set_formspec_prepend(self, formspec)
int ObjectRef::l_set_formspec_prepend(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	std::string formspec = luaL_checkstring(L, 2);

	player->formspec_prepend = formspec;
	getServer(L)->reportFormspecPrependModified(player->getName());
	lua_pushboolean(L, true);
	return 1;
}

// get_formspec_prepend(self)
int ObjectRef::l_get_formspec_prepend(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		 return 0;

	std::string formspec = player->formspec_prepend;

	lua_pushlstring(L, formspec.c_str(), formspec.size());
	return 1;
}

// get_player_control(self)
int ObjectRef::l_get_player_control(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr) {
		lua_pushlstring(L, "", 0);
		return 1;
	}

	const PlayerControl &control = player->getPlayerControl();
	lua_newtable(L);
	lua_pushboolean(L, player->keyPressed & (1 << 0));
	lua_setfield(L, -2, "up");
	lua_pushboolean(L, player->keyPressed & (1 << 1));
	lua_setfield(L, -2, "down");
	lua_pushboolean(L, player->keyPressed & (1 << 2));
	lua_setfield(L, -2, "left");
	lua_pushboolean(L, player->keyPressed & (1 << 3));
	lua_setfield(L, -2, "right");
	lua_pushboolean(L, control.jump);
	lua_setfield(L, -2, "jump");
	lua_pushboolean(L, control.aux1);
	lua_setfield(L, -2, "aux1");
	lua_pushboolean(L, control.sneak);
	lua_setfield(L, -2, "sneak");
	lua_pushboolean(L, control.dig);
	lua_setfield(L, -2, "dig");
	lua_pushboolean(L, control.place);
	lua_setfield(L, -2, "place");
	// Legacy fields to ensure mod compatibility
	lua_pushboolean(L, control.dig);
	lua_setfield(L, -2, "LMB");
	lua_pushboolean(L, control.place);
	lua_setfield(L, -2, "RMB");
	lua_pushboolean(L, control.zoom);
	lua_setfield(L, -2, "zoom");
	return 1;
}

// get_player_control_bits(self)
int ObjectRef::l_get_player_control_bits(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr) {
		lua_pushlstring(L, "", 0);
		return 1;
	}

	lua_pushnumber(L, player->keyPressed);
	return 1;
}

// set_physics_override(self, override_table)
int ObjectRef::l_set_physics_override(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO *playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	if (lua_istable(L, 2)) {
		bool modified = false;
		modified |= getfloatfield(L, 2, "speed", playersao->m_physics_override_speed);
		modified |= getfloatfield(L, 2, "jump", playersao->m_physics_override_jump);
		modified |= getfloatfield(L, 2, "gravity", playersao->m_physics_override_gravity);
		modified |= getboolfield(L, 2, "sneak", playersao->m_physics_override_sneak);
		modified |= getboolfield(L, 2, "sneak_glitch", playersao->m_physics_override_sneak_glitch);
		modified |= getboolfield(L, 2, "new_move", playersao->m_physics_override_new_move);
		if (modified)
			playersao->m_physics_override_sent = false;
	} else {
		// old, non-table format
		// TODO: Remove this code after version 5.4.0
		log_deprecated(L, "Deprecated use of set_physics_override(num, num, num)");

		if (!lua_isnil(L, 2)) {
			playersao->m_physics_override_speed = lua_tonumber(L, 2);
			playersao->m_physics_override_sent = false;
		}
		if (!lua_isnil(L, 3)) {
			playersao->m_physics_override_jump = lua_tonumber(L, 3);
			playersao->m_physics_override_sent = false;
		}
		if (!lua_isnil(L, 4)) {
			playersao->m_physics_override_gravity = lua_tonumber(L, 4);
			playersao->m_physics_override_sent = false;
		}
	}
	return 0;
}

// get_physics_override(self)
int ObjectRef::l_get_physics_override(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	PlayerSAO *playersao = getplayersao(ref);
	if (playersao == nullptr)
		return 0;

	lua_newtable(L);
	lua_pushnumber(L, playersao->m_physics_override_speed);
	lua_setfield(L, -2, "speed");
	lua_pushnumber(L, playersao->m_physics_override_jump);
	lua_setfield(L, -2, "jump");
	lua_pushnumber(L, playersao->m_physics_override_gravity);
	lua_setfield(L, -2, "gravity");
	lua_pushboolean(L, playersao->m_physics_override_sneak);
	lua_setfield(L, -2, "sneak");
	lua_pushboolean(L, playersao->m_physics_override_sneak_glitch);
	lua_setfield(L, -2, "sneak_glitch");
	lua_pushboolean(L, playersao->m_physics_override_new_move);
	lua_setfield(L, -2, "new_move");
	return 1;
}

// hud_add(self, hud)
int ObjectRef::l_hud_add(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	HudElement *elem = new HudElement;
	read_hud_element(L, elem);

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
	if (player == nullptr)
		return 0;

	u32 id = luaL_checkint(L, 2);

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
	if (player == nullptr)
		return 0;

	u32 id = luaL_checkint(L, 2);

	HudElement *elem = player->getHud(id);
	if (elem == nullptr)
		return 0;

	HudElementStat stat;
	void *value = nullptr;
	bool ok = read_hud_change(L, stat, elem, &value);

	if (ok)
		getServer(L)->hudChange(player, id, stat, value);

	lua_pushboolean(L, ok);
	return 1;
}

// hud_get(self, id)
int ObjectRef::l_hud_get(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	u32 id = luaL_checkint(L, 2);

	HudElement *elem = player->getHud(id);
	if (elem == nullptr)
		return 0;

	push_hud_element(L, elem);
	return 1;
}

// hud_set_flags(self, flags)
int ObjectRef::l_hud_set_flags(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
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

// hud_get_flags(self)
int ObjectRef::l_hud_get_flags(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
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
	lua_pushboolean(L, player->hud_flags & HUD_FLAG_MINIMAP_RADAR_VISIBLE);
	lua_setfield(L, -2, "minimap_radar");
	return 1;
}

// hud_set_hotbar_itemcount(self, hotbar_itemcount)
int ObjectRef::l_hud_set_hotbar_itemcount(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	s32 hotbar_itemcount = luaL_checkint(L, 2);

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
	if (player == nullptr)
		return 0;

	lua_pushnumber(L, player->getHotbarItemcount());
	return 1;
}

// hud_set_hotbar_image(self, name)
int ObjectRef::l_hud_set_hotbar_image(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	std::string name = readParam<std::string>(L, 2);

	getServer(L)->hudSetHotbarImage(player, name);
	return 1;
}

// hud_get_hotbar_image(self)
int ObjectRef::l_hud_get_hotbar_image(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	const std::string &name = player->getHotbarImage();

	lua_pushlstring(L, name.c_str(), name.size());
	return 1;
}

// hud_set_hotbar_selected_image(self, name)
int ObjectRef::l_hud_set_hotbar_selected_image(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	std::string name = readParam<std::string>(L, 2);

	getServer(L)->hudSetHotbarSelectedImage(player, name);
	return 1;
}

// hud_get_hotbar_selected_image(self)
int ObjectRef::l_hud_get_hotbar_selected_image(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	const std::string &name = player->getHotbarSelectedImage();

	lua_pushlstring(L, name.c_str(), name.size());
	return 1;
}

// set_sky(self, sky_parameters)
int ObjectRef::l_set_sky(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	SkyboxParams sky_params = player->getSkyParams();
	bool is_colorspec = is_color_table(L, 2);

	if (lua_istable(L, 2) && !is_colorspec) {
		lua_getfield(L, 2, "base_color");
		if (!lua_isnil(L, -1))
			read_color(L, -1, &sky_params.bgcolor);
		lua_pop(L, 1);

		lua_getfield(L, 2, "type");
		if (!lua_isnil(L, -1))
			sky_params.type = luaL_checkstring(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 2, "textures");
		sky_params.textures.clear();
		if (lua_istable(L, -1) && sky_params.type == "skybox") {
			lua_pushnil(L);
			while (lua_next(L, -2) != 0) {
				// Key is at index -2 and value at index -1
				sky_params.textures.emplace_back(readParam<std::string>(L, -1));
				// Removes the value, but keeps the key for iteration
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 1);

		/*
		We want to avoid crashes, so we're checking even if we're not using them.
		However, we want to ensure that the skybox can be set to nil when
		using "regular" or "plain" skybox modes as textures aren't needed.
		*/

		if (sky_params.textures.size() != 6 && sky_params.textures.size() > 0)
			throw LuaError("Skybox expects 6 textures!");

		sky_params.clouds = getboolfield_default(L, 2,
			"clouds", sky_params.clouds);

		lua_getfield(L, 2, "sky_color");
		if (lua_istable(L, -1)) {
			lua_getfield(L, -1, "day_sky");
			read_color(L, -1, &sky_params.sky_color.day_sky);
			lua_pop(L, 1);

			lua_getfield(L, -1, "day_horizon");
			read_color(L, -1, &sky_params.sky_color.day_horizon);
			lua_pop(L, 1);

			lua_getfield(L, -1, "dawn_sky");
			read_color(L, -1, &sky_params.sky_color.dawn_sky);
			lua_pop(L, 1);

			lua_getfield(L, -1, "dawn_horizon");
			read_color(L, -1, &sky_params.sky_color.dawn_horizon);
			lua_pop(L, 1);

			lua_getfield(L, -1, "night_sky");
			read_color(L, -1, &sky_params.sky_color.night_sky);
			lua_pop(L, 1);

			lua_getfield(L, -1, "night_horizon");
			read_color(L, -1, &sky_params.sky_color.night_horizon);
			lua_pop(L, 1);

			lua_getfield(L, -1, "indoors");
			read_color(L, -1, &sky_params.sky_color.indoors);
			lua_pop(L, 1);

			// Prevent flickering clouds at dawn/dusk:
			sky_params.fog_sun_tint = video::SColor(255, 255, 255, 255);
			lua_getfield(L, -1, "fog_sun_tint");
			read_color(L, -1, &sky_params.fog_sun_tint);
			lua_pop(L, 1);

			sky_params.fog_moon_tint = video::SColor(255, 255, 255, 255);
			lua_getfield(L, -1, "fog_moon_tint");
			read_color(L, -1, &sky_params.fog_moon_tint);
			lua_pop(L, 1);

			lua_getfield(L, -1, "fog_tint_type");
			if (!lua_isnil(L, -1))
				sky_params.fog_tint_type = luaL_checkstring(L, -1);
			lua_pop(L, 1);

			// Because we need to leave the "sky_color" table.
			lua_pop(L, 1);
		}
	} else {
		// Handle old set_sky calls, and log deprecated:
		log_deprecated(L, "Deprecated call to set_sky, please check lua_api.txt");

		// Fix sun, moon and stars showing when classic textured skyboxes are used
		SunParams sun_params = player->getSunParams();
		MoonParams moon_params = player->getMoonParams();
		StarParams star_params = player->getStarParams();

		// Prevent erroneous background colors
		sky_params.bgcolor = video::SColor(255, 255, 255, 255);
		read_color(L, 2, &sky_params.bgcolor);

		sky_params.type = luaL_checkstring(L, 3);

		// Preserve old behaviour of the sun, moon and stars
		// when using the old set_sky call.
		if (sky_params.type == "regular") {
			sun_params.visible = true;
			sun_params.sunrise_visible = true;
			moon_params.visible = true;
			star_params.visible = true;
		} else {
			sun_params.visible = false;
			sun_params.sunrise_visible = false;
			moon_params.visible = false;
			star_params.visible = false;
		}

		sky_params.textures.clear();
		if (lua_istable(L, 4)) {
			lua_pushnil(L);
			while (lua_next(L, 4) != 0) {
			// Key at index -2, and value at index -1
				if (lua_isstring(L, -1))
					sky_params.textures.emplace_back(readParam<std::string>(L, -1));
				else
					sky_params.textures.emplace_back("");
				// Remove the value, keep the key for the next iteration
				lua_pop(L, 1);
			}
		}
		if (sky_params.type == "skybox" && sky_params.textures.size() != 6)
			throw LuaError("Skybox expects 6 textures.");

		sky_params.clouds = true;
		if (lua_isboolean(L, 5))
			sky_params.clouds = readParam<bool>(L, 5);

		getServer(L)->setSun(player, sun_params);
		getServer(L)->setMoon(player, moon_params);
		getServer(L)->setStars(player, star_params);
	}
	getServer(L)->setSky(player, sky_params);
	lua_pushboolean(L, true);
	return 1;
}

// get_sky(self)
int ObjectRef::l_get_sky(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	SkyboxParams skybox_params = player->getSkyParams();

	push_ARGB8(L, skybox_params.bgcolor);
	lua_pushlstring(L, skybox_params.type.c_str(), skybox_params.type.size());

	lua_newtable(L);
	s16 i = 1;
	for (const std::string &texture : skybox_params.textures) {
		lua_pushlstring(L, texture.c_str(), texture.size());
		lua_rawseti(L, -2, i++);
	}
	lua_pushboolean(L, skybox_params.clouds);
	return 4;
}

// get_sky_color(self)
int ObjectRef::l_get_sky_color(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	const SkyboxParams &skybox_params = player->getSkyParams();

	lua_newtable(L);
	if (skybox_params.type == "regular") {
		push_ARGB8(L, skybox_params.sky_color.day_sky);
		lua_setfield(L, -2, "day_sky");
		push_ARGB8(L, skybox_params.sky_color.day_horizon);
		lua_setfield(L, -2, "day_horizon");
		push_ARGB8(L, skybox_params.sky_color.dawn_sky);
		lua_setfield(L, -2, "dawn_sky");
		push_ARGB8(L, skybox_params.sky_color.dawn_horizon);
		lua_setfield(L, -2, "dawn_horizon");
		push_ARGB8(L, skybox_params.sky_color.night_sky);
		lua_setfield(L, -2, "night_sky");
		push_ARGB8(L, skybox_params.sky_color.night_horizon);
		lua_setfield(L, -2, "night_horizon");
		push_ARGB8(L, skybox_params.sky_color.indoors);
		lua_setfield(L, -2, "indoors");
	}
	push_ARGB8(L, skybox_params.fog_sun_tint);
	lua_setfield(L, -2, "fog_sun_tint");
	push_ARGB8(L, skybox_params.fog_moon_tint);
	lua_setfield(L, -2, "fog_moon_tint");
	lua_pushstring(L, skybox_params.fog_tint_type.c_str());
	lua_setfield(L, -2, "fog_tint_type");
	return 1;
}

// set_sun(self, sun_parameters)
int ObjectRef::l_set_sun(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	luaL_checktype(L, 2, LUA_TTABLE);
	SunParams sun_params = player->getSunParams();

	sun_params.visible = getboolfield_default(L, 2,
			"visible", sun_params.visible);
	sun_params.texture = getstringfield_default(L, 2,
			"texture", sun_params.texture);
	sun_params.tonemap = getstringfield_default(L, 2,
			"tonemap", sun_params.tonemap);
	sun_params.sunrise = getstringfield_default(L, 2,
			"sunrise", sun_params.sunrise);
	sun_params.sunrise_visible = getboolfield_default(L, 2,
			"sunrise_visible", sun_params.sunrise_visible);
	sun_params.scale = getfloatfield_default(L, 2,
			"scale", sun_params.scale);

	getServer(L)->setSun(player, sun_params);
	lua_pushboolean(L, true);
	return 1;
}

//get_sun(self)
int ObjectRef::l_get_sun(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	const SunParams &sun_params = player->getSunParams();

	lua_newtable(L);
	lua_pushboolean(L, sun_params.visible);
	lua_setfield(L, -2, "visible");
	lua_pushstring(L, sun_params.texture.c_str());
	lua_setfield(L, -2, "texture");
	lua_pushstring(L, sun_params.tonemap.c_str());
	lua_setfield(L, -2, "tonemap");
	lua_pushstring(L, sun_params.sunrise.c_str());
	lua_setfield(L, -2, "sunrise");
	lua_pushboolean(L, sun_params.sunrise_visible);
	lua_setfield(L, -2, "sunrise_visible");
	lua_pushnumber(L, sun_params.scale);
	lua_setfield(L, -2, "scale");
	return 1;
}

// set_moon(self, moon_parameters)
int ObjectRef::l_set_moon(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	luaL_checktype(L, 2, LUA_TTABLE);
	MoonParams moon_params = player->getMoonParams();

	moon_params.visible = getboolfield_default(L, 2,
		"visible", moon_params.visible);
	moon_params.texture = getstringfield_default(L, 2,
		"texture", moon_params.texture);
	moon_params.tonemap = getstringfield_default(L, 2,
		"tonemap", moon_params.tonemap);
	moon_params.scale = getfloatfield_default(L, 2,
		"scale", moon_params.scale);

	getServer(L)->setMoon(player, moon_params);
	lua_pushboolean(L, true);
	return 1;
}

// get_moon(self)
int ObjectRef::l_get_moon(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	const MoonParams &moon_params = player->getMoonParams();

	lua_newtable(L);
	lua_pushboolean(L, moon_params.visible);
	lua_setfield(L, -2, "visible");
	lua_pushstring(L, moon_params.texture.c_str());
	lua_setfield(L, -2, "texture");
	lua_pushstring(L, moon_params.tonemap.c_str());
	lua_setfield(L, -2, "tonemap");
	lua_pushnumber(L, moon_params.scale);
	lua_setfield(L, -2, "scale");
	return 1;
}

// set_stars(self, star_parameters)
int ObjectRef::l_set_stars(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	luaL_checktype(L, 2, LUA_TTABLE);
	StarParams star_params = player->getStarParams();

	star_params.visible = getboolfield_default(L, 2,
		"visible", star_params.visible);
	star_params.count = getintfield_default(L, 2,
		"count", star_params.count);

	lua_getfield(L, 2, "star_color");
	if (!lua_isnil(L, -1))
		read_color(L, -1, &star_params.starcolor);
	lua_pop(L, 1);

	star_params.scale = getfloatfield_default(L, 2,
		"scale", star_params.scale);

	getServer(L)->setStars(player, star_params);
	lua_pushboolean(L, true);
	return 1;
}

// get_stars(self)
int ObjectRef::l_get_stars(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	const StarParams &star_params = player->getStarParams();

	lua_newtable(L);
	lua_pushboolean(L, star_params.visible);
	lua_setfield(L, -2, "visible");
	lua_pushnumber(L, star_params.count);
	lua_setfield(L, -2, "count");
	push_ARGB8(L, star_params.starcolor);
	lua_setfield(L, -2, "star_color");
	lua_pushnumber(L, star_params.scale);
	lua_setfield(L, -2, "scale");
	return 1;
}

// set_clouds(self, cloud_parameters)
int ObjectRef::l_set_clouds(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	luaL_checktype(L, 2, LUA_TTABLE);
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
		new_speed.Y = getfloatfield_default(L, -1, "z", 0);
		cloud_params.speed = new_speed;
	}
	lua_pop(L, 1);

	getServer(L)->setClouds(player, cloud_params);
	lua_pushboolean(L, true);
	return 1;
}

int ObjectRef::l_get_clouds(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
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


// override_day_night_ratio(self, ratio)
int ObjectRef::l_override_day_night_ratio(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	bool do_override = false;
	float ratio = 0.0f;

	if (!lua_isnil(L, 2)) {
		do_override = true;
		ratio = readParam<float>(L, 2);
		luaL_argcheck(L, ratio >= 0.0f && ratio <= 1.0f, 1,
			"value must be between 0 and 1");
	}

	getServer(L)->overrideDayNightRatio(player, do_override, ratio);
	lua_pushboolean(L, true);
	return 1;
}

// get_day_night_ratio(self)
int ObjectRef::l_get_day_night_ratio(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
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

// set_minimap_modes(self, modes, selected_mode)
int ObjectRef::l_set_minimap_modes(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef *ref = checkobject(L, 1);
	RemotePlayer *player = getplayer(ref);
	if (player == nullptr)
		return 0;

	luaL_checktype(L, 2, LUA_TTABLE);
	std::vector<MinimapMode> modes;
	s16 selected_mode = readParam<s16>(L, 3);

	lua_pushnil(L);
	while (lua_next(L, 2) != 0) {
		/* key is at index -2, value is at index -1 */
		if (lua_istable(L, -1)) {
			bool ok = true;
			MinimapMode mode;
			std::string type = getstringfield_default(L, -1, "type", "");
			if (type == "off")
				mode.type = MINIMAP_TYPE_OFF;
			else if (type == "surface")
				mode.type = MINIMAP_TYPE_SURFACE;
			else if (type == "radar")
				mode.type = MINIMAP_TYPE_RADAR;
			else if (type == "texture") {
				mode.type = MINIMAP_TYPE_TEXTURE;
				mode.texture = getstringfield_default(L, -1, "texture", "");
				mode.scale = getintfield_default(L, -1, "scale", 1);
			} else {
				warningstream << "Minimap mode of unknown type \"" << type.c_str()
					<< "\" ignored.\n" << std::endl;
				ok = false;
			}

			if (ok) {
				mode.label = getstringfield_default(L, -1, "label", "");
				// Size is limited to 512. Performance gets poor if size too large, and
				// segfaults have been experienced.
				mode.size = rangelim(getintfield_default(L, -1, "size", 0), 1, 512);
				modes.push_back(mode);
			}
		}
		/* removes 'value'; keeps 'key' for next iteration */
		lua_pop(L, 1);
	}
	lua_pop(L, 1); // Remove key

	getServer(L)->SendMinimapModes(player->getPeerId(), modes, selected_mode);
	return 0;
}

ObjectRef::ObjectRef(ServerActiveObject *object):
	m_object(object)
{}

// Creates an ObjectRef and leaves it on top of stack
// Not callable from Lua; all references are created on the C side.
void ObjectRef::create(lua_State *L, ServerActiveObject *object)
{
	ObjectRef *obj = new ObjectRef(object);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = obj;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void ObjectRef::set_null(lua_State *L)
{
	ObjectRef *obj = checkobject(L, -1);
	obj->m_object = nullptr;
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

	luaL_register(L, nullptr, methods);  // fill methodtable
	lua_pop(L, 1);  // drop methodtable
}

const char ObjectRef::className[] = "ObjectRef";
luaL_Reg ObjectRef::methods[] = {
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
	luamethod(ObjectRef, set_animation_frame_speed),
	luamethod(ObjectRef, set_bone_position),
	luamethod(ObjectRef, get_bone_position),
	luamethod(ObjectRef, set_attach),
	luamethod(ObjectRef, get_attach),
	luamethod(ObjectRef, get_children),
	luamethod(ObjectRef, set_detach),
	luamethod(ObjectRef, set_properties),
	luamethod(ObjectRef, get_properties),
	luamethod(ObjectRef, set_nametag_attributes),
	luamethod(ObjectRef, get_nametag_attributes),

	luamethod_aliased(ObjectRef, set_velocity, setvelocity),
	luamethod_aliased(ObjectRef, add_velocity, add_player_velocity),
	luamethod_aliased(ObjectRef, get_velocity, getvelocity),
	luamethod_dep(ObjectRef, get_velocity, get_player_velocity),

	// LuaEntitySAO-only
	luamethod_aliased(ObjectRef, set_acceleration, setacceleration),
	luamethod_aliased(ObjectRef, get_acceleration, getacceleration),
	luamethod_aliased(ObjectRef, set_yaw, setyaw),
	luamethod_aliased(ObjectRef, get_yaw, getyaw),
	luamethod(ObjectRef, set_rotation),
	luamethod(ObjectRef, get_rotation),
	luamethod_aliased(ObjectRef, set_texture_mod, settexturemod),
	luamethod(ObjectRef, get_texture_mod),
	luamethod_aliased(ObjectRef, set_sprite, setsprite),
	luamethod(ObjectRef, get_entity_name),
	luamethod(ObjectRef, get_luaentity),

	// Player-only
	luamethod(ObjectRef, is_player),
	luamethod(ObjectRef, get_player_name),
	luamethod(ObjectRef, get_look_dir),
	luamethod(ObjectRef, get_look_pitch),
	luamethod(ObjectRef, get_look_yaw),
	luamethod(ObjectRef, get_look_vertical),
	luamethod(ObjectRef, get_look_horizontal),
	luamethod(ObjectRef, set_look_horizontal),
	luamethod(ObjectRef, set_look_vertical),
	luamethod(ObjectRef, set_look_yaw),
	luamethod(ObjectRef, set_look_pitch),
	luamethod(ObjectRef, get_fov),
	luamethod(ObjectRef, set_fov),
	luamethod(ObjectRef, get_breath),
	luamethod(ObjectRef, set_breath),
	luamethod(ObjectRef, get_attribute),
	luamethod(ObjectRef, set_attribute),
	luamethod(ObjectRef, get_meta),
	luamethod(ObjectRef, set_inventory_formspec),
	luamethod(ObjectRef, get_inventory_formspec),
	luamethod(ObjectRef, set_formspec_prepend),
	luamethod(ObjectRef, get_formspec_prepend),
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
	luamethod(ObjectRef, get_sky_color),
	luamethod(ObjectRef, set_sun),
	luamethod(ObjectRef, get_sun),
	luamethod(ObjectRef, set_moon),
	luamethod(ObjectRef, get_moon),
	luamethod(ObjectRef, set_stars),
	luamethod(ObjectRef, get_stars),
	luamethod(ObjectRef, set_clouds),
	luamethod(ObjectRef, get_clouds),
	luamethod(ObjectRef, override_day_night_ratio),
	luamethod(ObjectRef, get_day_night_ratio),
	luamethod(ObjectRef, set_local_animation),
	luamethod(ObjectRef, get_local_animation),
	luamethod(ObjectRef, set_eye_offset),
	luamethod(ObjectRef, get_eye_offset),
	luamethod(ObjectRef, send_mapblock),
	luamethod(ObjectRef, set_minimap_modes),
	{0,0}
};

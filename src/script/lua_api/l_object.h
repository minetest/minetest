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
#include "irrlichttypes.h"

class ServerActiveObject;
class LuaEntitySAO;
class PlayerSAO;
class RemotePlayer;

/*
	ObjectRef
*/

class ObjectRef : public ModApiBase {
public:
	ObjectRef(ServerActiveObject *object);

	~ObjectRef() = default;

	// Creates an ObjectRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	static void create(lua_State *L, ServerActiveObject *object);

	static void set_null(lua_State *L);

	static void Register(lua_State *L);

	static ObjectRef *checkobject(lua_State *L, int narg);

	static ServerActiveObject* getobject(ObjectRef *ref);
private:
	ServerActiveObject *m_object = nullptr;
	static const char className[];
	static luaL_Reg methods[];


	static LuaEntitySAO* getluaobject(ObjectRef *ref);

	static PlayerSAO* getplayersao(ObjectRef *ref);

	static RemotePlayer *getplayer(ObjectRef *ref);

	// Exported functions

	// garbage collector
	ENTRY_POINT_DECL(gc_object);

	// remove(self)
	ENTRY_POINT_DECL(l_remove);

	// get_pos(self)
	ENTRY_POINT_DECL(l_get_pos);

	// set_pos(self, pos)
	ENTRY_POINT_DECL(l_set_pos);

	// move_to(self, pos, continuous)
	ENTRY_POINT_DECL(l_move_to);

	// punch(self, puncher, time_from_last_punch, tool_capabilities, dir)
	ENTRY_POINT_DECL(l_punch);

	// right_click(self, clicker)
	ENTRY_POINT_DECL(l_right_click);

	// set_hp(self, hp, reason)
	ENTRY_POINT_DECL(l_set_hp);

	// get_hp(self)
	ENTRY_POINT_DECL(l_get_hp);

	// get_inventory(self)
	ENTRY_POINT_DECL(l_get_inventory);

	// get_wield_list(self)
	ENTRY_POINT_DECL(l_get_wield_list);

	// get_wield_index(self)
	ENTRY_POINT_DECL(l_get_wield_index);

	// get_wielded_item(self)
	ENTRY_POINT_DECL(l_get_wielded_item);

	// set_wielded_item(self, item)
	ENTRY_POINT_DECL(l_set_wielded_item);

	// set_armor_groups(self, groups)
	ENTRY_POINT_DECL(l_set_armor_groups);

	// get_armor_groups(self)
	ENTRY_POINT_DECL(l_get_armor_groups);

	// set_physics_override(self, override_table)
	ENTRY_POINT_DECL(l_set_physics_override);

	// get_physics_override(self)
	ENTRY_POINT_DECL(l_get_physics_override);

	// set_animation(self, frame_range, frame_speed, frame_blend, frame_loop)
	ENTRY_POINT_DECL(l_set_animation);

	// set_animation_frame_speed(self, frame_speed)
	ENTRY_POINT_DECL(l_set_animation_frame_speed);

	// get_animation(self)
	ENTRY_POINT_DECL(l_get_animation);

	// set_bone_position(self, bone, position, rotation)
	ENTRY_POINT_DECL(l_set_bone_position);

	// get_bone_position(self, bone)
	ENTRY_POINT_DECL(l_get_bone_position);

	// set_attach(self, parent, bone, position, rotation)
	ENTRY_POINT_DECL(l_set_attach);

	// get_attach(self)
	ENTRY_POINT_DECL(l_get_attach);

	// get_children(self)
	ENTRY_POINT_DECL(l_get_children);

	// set_detach(self)
	ENTRY_POINT_DECL(l_set_detach);

	// set_properties(self, properties)
	ENTRY_POINT_DECL(l_set_properties);

	// get_properties(self)
	ENTRY_POINT_DECL(l_get_properties);

	// is_player(self)
	ENTRY_POINT_DECL(l_is_player);

	/* LuaEntitySAO-only */

	// set_velocity(self, velocity)
	ENTRY_POINT_DECL(l_set_velocity);

	// add_velocity(self, velocity)
	ENTRY_POINT_DECL(l_add_velocity);

	// get_velocity(self)
	ENTRY_POINT_DECL(l_get_velocity);

	// set_acceleration(self, acceleration)
	ENTRY_POINT_DECL(l_set_acceleration);

	// get_acceleration(self)
	ENTRY_POINT_DECL(l_get_acceleration);

	// set_rotation(self, rotation)
	ENTRY_POINT_DECL(l_set_rotation);

	// get_rotation(self)
	ENTRY_POINT_DECL(l_get_rotation);

	// set_yaw(self, yaw)
	ENTRY_POINT_DECL(l_set_yaw);

	// get_yaw(self)
	ENTRY_POINT_DECL(l_get_yaw);

	// set_texture_mod(self, mod)
	ENTRY_POINT_DECL(l_set_texture_mod);

	// l_get_texture_mod(self)
	ENTRY_POINT_DECL(l_get_texture_mod);

	// set_sprite(self, start_frame, num_frames, framelength, select_x_by_camera)
	ENTRY_POINT_DECL(l_set_sprite);

	// DEPRECATED
	// get_entity_name(self)
	ENTRY_POINT_DECL(l_get_entity_name);

	// get_luaentity(self)
	ENTRY_POINT_DECL(l_get_luaentity);

	/* Player-only */

	// get_player_name(self)
	ENTRY_POINT_DECL(l_get_player_name);

	// get_fov(self)
	ENTRY_POINT_DECL(l_get_fov);

	// get_look_dir(self)
	ENTRY_POINT_DECL(l_get_look_dir);

	// DEPRECATED
	// get_look_pitch(self)
	ENTRY_POINT_DECL(l_get_look_pitch);

	// DEPRECATED
	// get_look_yaw(self)
	ENTRY_POINT_DECL(l_get_look_yaw);

	// get_look_pitch2(self)
	ENTRY_POINT_DECL(l_get_look_vertical);

	// get_look_yaw2(self)
	ENTRY_POINT_DECL(l_get_look_horizontal);

	// set_fov(self, degrees, is_multiplier, transition_time)
	ENTRY_POINT_DECL(l_set_fov);

	// set_look_vertical(self, radians)
	ENTRY_POINT_DECL(l_set_look_vertical);

	// set_look_horizontal(self, radians)
	ENTRY_POINT_DECL(l_set_look_horizontal);

	// DEPRECATED
	// set_look_pitch(self, radians)
	ENTRY_POINT_DECL(l_set_look_pitch);

	// DEPRECATED
	// set_look_yaw(self, radians)
	ENTRY_POINT_DECL(l_set_look_yaw);

	// set_breath(self, breath)
	ENTRY_POINT_DECL(l_set_breath);

	// get_breath(self, breath)
	ENTRY_POINT_DECL(l_get_breath);

	// DEPRECATED
	// set_attribute(self, attribute, value)
	ENTRY_POINT_DECL(l_set_attribute);

	// DEPRECATED
	// get_attribute(self, attribute)
	ENTRY_POINT_DECL(l_get_attribute);

	// get_meta(self)
	ENTRY_POINT_DECL(l_get_meta);

	// set_inventory_formspec(self, formspec)
	ENTRY_POINT_DECL(l_set_inventory_formspec);

	// get_inventory_formspec(self)
	ENTRY_POINT_DECL(l_get_inventory_formspec);

	// set_formspec_prepend(self, formspec)
	ENTRY_POINT_DECL(l_set_formspec_prepend);

	// get_formspec_prepend(self)
	ENTRY_POINT_DECL(l_get_formspec_prepend);

	// get_player_control(self)
	ENTRY_POINT_DECL(l_get_player_control);

	// get_player_control_bits(self)
	ENTRY_POINT_DECL(l_get_player_control_bits);

	// hud_add(self, id, form)
	ENTRY_POINT_DECL(l_hud_add);

	// hud_rm(self, id)
	ENTRY_POINT_DECL(l_hud_remove);

	// hud_change(self, id, stat, data)
	ENTRY_POINT_DECL(l_hud_change);

	// hud_get_next_id(self)
	static u32 hud_get_next_id(lua_State *L);

	// hud_get(self, id)
	ENTRY_POINT_DECL(l_hud_get);

	// hud_set_flags(self, flags)
	ENTRY_POINT_DECL(l_hud_set_flags);

	// hud_get_flags()
	ENTRY_POINT_DECL(l_hud_get_flags);

	// hud_set_hotbar_itemcount(self, hotbar_itemcount)
	ENTRY_POINT_DECL(l_hud_set_hotbar_itemcount);

	// hud_get_hotbar_itemcount(self)
	ENTRY_POINT_DECL(l_hud_get_hotbar_itemcount);

	// hud_set_hotbar_image(self, name)
	ENTRY_POINT_DECL(l_hud_set_hotbar_image);

	// hud_get_hotbar_image(self)
	ENTRY_POINT_DECL(l_hud_get_hotbar_image);

	// hud_set_hotbar_selected_image(self, name)
	ENTRY_POINT_DECL(l_hud_set_hotbar_selected_image);

	// hud_get_hotbar_selected_image(self)
	ENTRY_POINT_DECL(l_hud_get_hotbar_selected_image);

	// set_sky(self, sky_parameters)
	ENTRY_POINT_DECL(l_set_sky);

	// get_sky(self, as_table)
	ENTRY_POINT_DECL(l_get_sky);

	// DEPRECATED
	// get_sky_color(self)
	ENTRY_POINT_DECL(l_get_sky_color);

	// set_sun(self, sun_parameters)
	ENTRY_POINT_DECL(l_set_sun);

	// get_sun(self)
	ENTRY_POINT_DECL(l_get_sun);

	// set_moon(self, moon_parameters)
	ENTRY_POINT_DECL(l_set_moon);

	// get_moon(self)
	ENTRY_POINT_DECL(l_get_moon);

	// set_stars(self, star_parameters)
	ENTRY_POINT_DECL(l_set_stars);

	// get_stars(self)
	ENTRY_POINT_DECL(l_get_stars);

	// set_clouds(self, cloud_parameters)
	ENTRY_POINT_DECL(l_set_clouds);

	// get_clouds(self)
	ENTRY_POINT_DECL(l_get_clouds);

	// override_day_night_ratio(self, type)
	ENTRY_POINT_DECL(l_override_day_night_ratio);

	// get_day_night_ratio(self)
	ENTRY_POINT_DECL(l_get_day_night_ratio);

	// set_local_animation(self, idle, walk, dig, walk_while_dig, frame_speed)
	ENTRY_POINT_DECL(l_set_local_animation);

	// get_local_animation(self)
	ENTRY_POINT_DECL(l_get_local_animation);

	// set_eye_offset(self, firstperson, thirdperson)
	ENTRY_POINT_DECL(l_set_eye_offset);

	// get_eye_offset(self)
	ENTRY_POINT_DECL(l_get_eye_offset);

	// set_nametag_attributes(self, attributes)
	ENTRY_POINT_DECL(l_set_nametag_attributes);

	// get_nametag_attributes(self)
	ENTRY_POINT_DECL(l_get_nametag_attributes);

	// send_mapblock(pos)
	ENTRY_POINT_DECL(l_send_mapblock);

	// set_minimap_modes(self, modes, wanted_mode)
	ENTRY_POINT_DECL(l_set_minimap_modes);

	// set_lighting(self, lighting)
	ENTRY_POINT_DECL(l_set_lighting);
	
	// get_lighting(self)
	ENTRY_POINT_DECL(l_get_lighting);
};

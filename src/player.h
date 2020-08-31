/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "irrlichttypes_bloated.h"
#include "inventory.h"
#include "constants.h"
#include "network/networkprotocol.h"
#include "util/basic_macros.h"
#include <list>
#include <mutex>

#define PLAYERNAME_SIZE 20

#define PLAYERNAME_ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"
#define PLAYERNAME_ALLOWED_CHARS_USER_EXPL "'a' to 'z', 'A' to 'Z', '0' to '9', '-', '_'"

struct PlayerFovSpec
{
	f32 fov;

	// Whether to multiply the client's FOV or to override it
	bool is_multiplier;

	// The time to be take to trasition to the new FOV value.
	// Transition is instantaneous if omitted. Omitted by default.
	f32 transition_time;
};

struct PlayerControl
{
	PlayerControl() = default;

	PlayerControl(
		bool a_up,
		bool a_down,
		bool a_left,
		bool a_right,
		bool a_jump,
		bool a_aux1,
		bool a_sneak,
		bool a_zoom,
		bool a_dig,
		bool a_place,
		float a_pitch,
		float a_yaw,
		float a_sidew_move_joystick_axis,
		float a_forw_move_joystick_axis
	)
	{
		up = a_up;
		down = a_down;
		left = a_left;
		right = a_right;
		jump = a_jump;
		aux1 = a_aux1;
		sneak = a_sneak;
		zoom = a_zoom;
		dig = a_dig;
		place = a_place;
		pitch = a_pitch;
		yaw = a_yaw;
		sidew_move_joystick_axis = a_sidew_move_joystick_axis;
		forw_move_joystick_axis = a_forw_move_joystick_axis;
	}
	bool up = false;
	bool down = false;
	bool left = false;
	bool right = false;
	bool jump = false;
	bool aux1 = false;
	bool sneak = false;
	bool zoom = false;
	bool dig = false;
	bool place = false;
	float pitch = 0.0f;
	float yaw = 0.0f;
	float sidew_move_joystick_axis = 0.0f;
	float forw_move_joystick_axis = 0.0f;
};

struct PlayerSettings
{
	bool free_move = false;
	bool pitch_move = false;
	bool fast_move = false;
	bool continuous_forward = false;
	bool always_fly_fast = false;
	bool aux1_descends = false;
	bool noclip = false;
	bool autojump = false;

	const std::string setting_names[8] = {
		"free_move", "pitch_move", "fast_move", "continuous_forward", "always_fly_fast",
		"aux1_descends", "noclip", "autojump"
	};
	void readGlobalSettings();
};

class Map;
struct CollisionInfo;
struct HudElement;
class Environment;

class Player
{
public:

	Player(const char *name, IItemDefManager *idef);
	virtual ~Player() = 0;

	DISABLE_CLASS_COPY(Player);

	virtual void move(f32 dtime, Environment *env, f32 pos_max_d)
	{}
	virtual void move(f32 dtime, Environment *env, f32 pos_max_d,
			std::vector<CollisionInfo> *collision_info)
	{}

	const v3f &getSpeed() const
	{
		return m_speed;
	}

	void setSpeed(const v3f &speed)
	{
		m_speed = speed;
	}

	const char *getName() const { return m_name; }

	u32 getFreeHudID()
	{
		size_t size = hud.size();
		for (size_t i = 0; i != size; i++) {
			if (!hud[i])
				return i;
		}
		return size;
	}

	v3f eye_offset_first;
	v3f eye_offset_third;

	Inventory inventory;

	f32 movement_acceleration_default;
	f32 movement_acceleration_air;
	f32 movement_acceleration_fast;
	f32 movement_speed_walk;
	f32 movement_speed_crouch;
	f32 movement_speed_fast;
	f32 movement_speed_climb;
	f32 movement_speed_jump;
	f32 movement_liquid_fluidity;
	f32 movement_liquid_fluidity_smooth;
	f32 movement_liquid_sink;
	f32 movement_gravity;

	v2s32 local_animations[4];
	float local_animation_speed;

	std::string inventory_formspec;
	std::string formspec_prepend;

	PlayerControl control;
	const PlayerControl& getPlayerControl() { return control; }
	PlayerSettings &getPlayerSettings() { return m_player_settings; }
	static void settingsChangedCallback(const std::string &name, void *data);

	// Returns non-empty `selected` ItemStack. `hand` is a fallback, if specified
	ItemStack &getWieldedItem(ItemStack *selected, ItemStack *hand) const;
	void setWieldIndex(u16 index);
	u16 getWieldIndex() const { return m_wield_index; }

	void setFov(const PlayerFovSpec &spec)
	{
		m_fov_override_spec = spec;
	}

	const PlayerFovSpec &getFov() const
	{
		return m_fov_override_spec;
	}

	u32 keyPressed = 0;

	HudElement* getHud(u32 id);
	u32         addHud(HudElement* hud);
	HudElement* removeHud(u32 id);
	void        clearHud();

	u32 hud_flags;
	s32 hud_hotbar_itemcount;

protected:
	char m_name[PLAYERNAME_SIZE];
	v3f m_speed;
	u16 m_wield_index = 0;
	PlayerFovSpec m_fov_override_spec = { 0.0f, false, 0.0f };

	std::vector<HudElement *> hud;
private:
	// Protect some critical areas
	// hud for example can be modified by EmergeThread
	// and ServerThread
	std::mutex m_mutex;
	PlayerSettings m_player_settings;
};

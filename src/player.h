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

#ifndef PLAYER_HEADER
#define PLAYER_HEADER

#include "irrlichttypes_bloated.h"
#include "inventory.h"
#include <list>
#include <mutex>

#define PLAYERNAME_SIZE 20

#define PLAYERNAME_ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"
#define PLAYERNAME_ALLOWED_CHARS_USER_EXPL "'a' to 'z', 'A' to 'Z', '0' to '9', '-', '_'"

struct PlayerControl
{
	PlayerControl()
	{
		up = false;
		down = false;
		left = false;
		right = false;
		jump = false;
		aux1 = false;
		sneak = false;
		LMB = false;
		RMB = false;
		pitch = 0;
		yaw = 0;
		sidew_move_joystick_axis = .0f;
		forw_move_joystick_axis = .0f;
	}

	PlayerControl(
		bool a_up,
		bool a_down,
		bool a_left,
		bool a_right,
		bool a_jump,
		bool a_aux1,
		bool a_sneak,
		bool a_zoom,
		bool a_LMB,
		bool a_RMB,
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
		LMB = a_LMB;
		RMB = a_RMB;
		pitch = a_pitch;
		yaw = a_yaw;
		sidew_move_joystick_axis = a_sidew_move_joystick_axis;
		forw_move_joystick_axis = a_forw_move_joystick_axis;
	}
	bool up;
	bool down;
	bool left;
	bool right;
	bool jump;
	bool aux1;
	bool sneak;
	bool zoom;
	bool LMB;
	bool RMB;
	float pitch;
	float yaw;
	float sidew_move_joystick_axis;
	float forw_move_joystick_axis;
};

class Map;
struct CollisionInfo;
struct HudElement;
class Environment;

// IMPORTANT:
// Do *not* perform an assignment or copy operation on a Player or
// RemotePlayer object!  This will copy the lock held for HUD synchronization
class Player
{
public:

	Player(const char *name, IItemDefManager *idef);
	virtual ~Player() = 0;

	virtual void move(f32 dtime, Environment *env, f32 pos_max_d)
	{}
	virtual void move(f32 dtime, Environment *env, f32 pos_max_d,
			std::vector<CollisionInfo> *collision_info)
	{}

	v3f getSpeed()
	{
		return m_speed;
	}

	void setSpeed(v3f speed)
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

	u16 peer_id;

	std::string inventory_formspec;

	PlayerControl control;
	const PlayerControl& getPlayerControl() { return control; }

	u32 keyPressed;

	HudElement* getHud(u32 id);
	u32         addHud(HudElement* hud);
	HudElement* removeHud(u32 id);
	void        clearHud();

	u32 hud_flags;
	s32 hud_hotbar_itemcount;
protected:
	char m_name[PLAYERNAME_SIZE];
	v3f m_speed;

	std::vector<HudElement *> hud;
private:
	// Protect some critical areas
	// hud for example can be modified by EmergeThread
	// and ServerThread
	std::mutex m_mutex;
};

#endif


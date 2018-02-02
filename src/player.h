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
#include "environment.h"
#include "controllog.h"
#include <list>
#include <mutex>

#define PLAYERNAME_SIZE 20

#define PLAYERNAME_ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"
#define PLAYERNAME_ALLOWED_CHARS_USER_EXPL "'a' to 'z', 'A' to 'Z', '0' to '9', '-', '_'"

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
	bool up = false;
	bool down = false;
	bool left = false;
	bool right = false;
	bool jump = false;
	bool aux1 = false;
	bool sneak = false;
	bool zoom = false;
	bool LMB = false;
	bool RMB = false;
	float pitch = 0.0f;
	float yaw = 0.0f;
	float sidew_move_joystick_axis = 0.0f;
	float forw_move_joystick_axis = 0.0f;
};

class Map;
struct CollisionInfo;
struct HudElement;
class Environment;
class NodeDefManager;

class Player
{
public:

	Player(const char *name, IItemDefManager *idef);
	virtual ~Player() = 0;

	DISABLE_CLASS_COPY(Player);

	// SERVER SIDE MOVEMENT: should happen here
	virtual void move(f32 dtime, Environment *env, f32 pos_max_d);
	virtual void move(f32 dtime, Environment *env, f32 pos_max_d,
			std::vector<CollisionInfo> *collision_info);
	virtual void old_move(f32 dtime, Environment *env, f32 pos_max_d,
			std::vector<CollisionInfo> *collision_info);

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

	PlayerControl control;
	const PlayerControl& getPlayerControl() { return control; }

	u32 keyPressed = 0;

	HudElement* getHud(u32 id);
	u32         addHud(HudElement* hud);
	HudElement* removeHud(u32 id);
	void        clearHud();

	u32 hud_flags;
	s32 hud_hotbar_itemcount;

	bool touching_ground = false;
	// This oscillates so that the player jumps a bit above the surface
	bool in_liquid = false;
	// This is more stable and defines the maximum speed of the player
	bool in_liquid_stable = false;
	// Gets the viscosity of water to calculate friction
	u8 liquid_viscosity = 0;
	bool is_climbing = false;
	bool swimming_vertical = false;
	bool is_slipping = false;
	bool m_can_jump = false;
	f32 m_yaw = 0.0f;
	f32 m_pitch = 0.0f;

	float physics_override_speed = 1.0f;
	float physics_override_jump = 1.0f;
	float physics_override_gravity = 1.0f;
	bool physics_override_sneak = true;
	bool physics_override_sneak_glitch = false;
	// Temporary option for old move code
	bool physics_override_new_move = true;


	virtual bool isAttached() const = 0;

	void setYaw(f32 yaw) { m_yaw = yaw; }
	f32 getYaw() const { return m_yaw; }

	void setPitch(f32 pitch) { m_pitch = pitch; }
	f32 getPitch() const { return m_pitch; }

	inline void setPosition(const v3f &position)
	{
		m_position = position;
		m_sneak_node_exists = false;
	}

	v3f getPosition() const { return m_position; }

	v3s16 getStandingNodePos();
	void applyControlLogEntry(const ControlLogEntry &cle, Environment *env);

	void setCollisionbox(const aabb3f &box) { m_collisionbox = box; }

	void step(float dtime, Environment *env, std::vector<CollisionInfo> *player_collisions = NULL);

	virtual void debugVec(const std::string &title, const v3f &v, const std::string &prefix = "") const;
	virtual void debugStr(const std::string &str, bool newline = true, const std::string &prefix = "") const;
	virtual void debugFloat(const std::string &title, const float val, bool newline = true, const std::string &prefix = "") const;
protected:
	char m_name[PLAYERNAME_SIZE];
	v3f m_speed;

	std::vector<HudElement *> hud;

	virtual bool checkPrivilege(const std::string &priv) const = 0;
	virtual void triggerJumpEvent() = 0;
	void accelerateHorizontal(const v3f &target_speed, const f32 max_increase);
	void accelerateVertical(const v3f &target_speed, const f32 max_increase);
	float getSlipFactor(Environment *env, const v3f &speedH);
	v3f m_position;
	v3s16 m_standing_node;

	// Whether the player is allowed to sneak
	bool m_sneak_node_exists = false;
	v3s16 m_sneak_node = v3s16(32767, 32767, 32767);
	// Stores the top bounding box of m_sneak_node
	aabb3f m_sneak_node_bb_top = aabb3f(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	// Whether a "sneak ladder" structure is detected at the players pos
	// see detectSneakLadder() in the .cpp for more info (always false if disabled)
	bool m_sneak_ladder_detected = false;
	bool updateSneakNode(Map *map, const v3f &position, const v3f &sneak_max);

	// ***** Variables for temporary option of the old move code *****
	// Stores the max player uplift by m_sneak_node
	f32 m_sneak_node_bb_ymax = 0.0f;
	// Whether recalculation of m_sneak_node and its top bbox is needed
	bool m_need_to_get_new_sneak_node = true;
	// Node below player, used to determine whether it has been removed,
	// and its old type
	v3s16 m_old_node_below = v3s16(32767, 32767, 32767);
	std::string m_old_node_below_type = "air";
	// ***** End of variables for temporary option *****

	aabb3f m_collisionbox = aabb3f(-BS * 0.30f, 0.0f, -BS * 0.30f, BS * 0.30f,
			BS * 1.75f, BS * 0.30f);

	virtual const NodeDefManager *getNodeDefManager() const = 0;
	virtual void _handleAttachedMove() = 0;
	virtual float _getStepHeight() const = 0;
	//virtual IGameDef* getGameDef() const = 0;
	virtual void reportRegainGround() = 0;
	virtual void calculateCameraInCeiling(Map *map, const NodeDefManager *nodemgr) = 0;
private:
	// Protect some critical areas
	// hud for example can be modified by EmergeThread
	// and ServerThread
	std::mutex m_mutex;
};

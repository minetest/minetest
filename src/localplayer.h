/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef LOCALPLAYER_HEADER
#define LOCALPLAYER_HEADER

#include "player.h"

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
		pitch = 0;
		yaw = 0;
	}
	PlayerControl(
		bool a_up,
		bool a_down,
		bool a_left,
		bool a_right,
		bool a_jump,
		bool a_aux1,
		bool a_sneak,
		float a_pitch,
		float a_yaw
	)
	{
		up = a_up;
		down = a_down;
		left = a_left;
		right = a_right;
		jump = a_jump;
		aux1 = a_aux1;
		sneak = a_sneak;
		pitch = a_pitch;
		yaw = a_yaw;
	}
	bool up;
	bool down;
	bool left;
	bool right;
	bool jump;
	bool aux1;
	bool sneak;
	float pitch;
	float yaw;
};

class LocalPlayer : public Player
{
public:
	LocalPlayer(IGameDef *gamedef);
	virtual ~LocalPlayer();

	bool isLocal() const
	{
		return true;
	}
	
	void move(f32 dtime, Map &map, f32 pos_max_d,
			core::list<CollisionInfo> *collision_info);
	void move(f32 dtime, Map &map, f32 pos_max_d);

	void applyControl(float dtime);

	v3s16 getStandingNodePos();
	
	PlayerControl control;

private:
	// This is used for determining the sneaking range
	v3s16 m_sneak_node;
	// Whether the player is allowed to sneak
	bool m_sneak_node_exists;
	// Node below player, used to determine whether it has been removed,
	// and its old type
	v3s16 m_old_node_below;
	std::string m_old_node_below_type;
	// Whether recalculation of the sneak node is needed
	bool m_need_to_get_new_sneak_node;
	bool m_can_jump;
};

#endif


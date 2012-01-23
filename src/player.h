/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef PLAYER_HEADER
#define PLAYER_HEADER

#include "common_irrlicht.h"
#include "inventory.h"

#define PLAYERNAME_SIZE 20

#define PLAYERNAME_ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"


class Map;
class IGameDef;
struct CollisionInfo;

class Player
{
public:

	Player(IGameDef *gamedef);
	virtual ~Player();

	void resetInventory();

	//void move(f32 dtime, Map &map);
	virtual void move(f32 dtime, Map &map, f32 pos_max_d) = 0;

	v3f getSpeed()
	{
		return m_speed;
	}

	void setSpeed(v3f speed)
	{
		m_speed = speed;
	}
	
	// Y direction is ignored
	void accelerate(v3f target_speed, f32 max_increase);

	v3f getPosition()
	{
		return m_position;
	}

	v3s16 getLightPosition() const;

	v3f getEyeOffset()
	{
		// This is at the height of the eyes of the current figure
		// return v3f(0, BS+BS/2, 0);
		// This is more like in minecraft
		return v3f(0,BS+(5*BS)/8,0);
	}

	v3f getEyePosition()
	{
		return m_position + getEyeOffset();
	}

	virtual void setPosition(const v3f &position)
	{
		m_position = position;
	}

	void setPitch(f32 pitch)
	{
		m_pitch = pitch;
	}

	virtual void setYaw(f32 yaw)
	{
		m_yaw = yaw;
	}

	f32 getPitch()
	{
		return m_pitch;
	}

	f32 getYaw()
	{
		return m_yaw;
	}

	f32 getRadPitch()
	{
		return -1.0 * m_pitch * core::DEGTORAD;
	}

	f32 getRadYaw()
	{
		return (m_yaw + 90.) * core::DEGTORAD;
	}

	virtual void updateName(const char *name)
	{
		snprintf(m_name, PLAYERNAME_SIZE, "%s", name);
	}

	const char * getName() const
	{
		return m_name;
	}

	virtual bool isLocal() const = 0;

	virtual void updateLight(u8 light_at_pos)
	{
		light = light_at_pos;
	}
	
	// NOTE: Use peer_id == 0 for disconnected
	/*virtual bool isClientConnected() { return false; }
	virtual void setClientConnected(bool) {}*/
	
	/*
		serialize() writes a bunch of text that can contain
		any characters except a '\0', and such an ending that
		deSerialize stops reading exactly at the right point.
	*/
	void serialize(std::ostream &os);
	void deSerialize(std::istream &is);

	bool touching_ground;
	// This oscillates so that the player jumps a bit above the surface
	bool in_water;
	// This is more stable and defines the maximum speed of the player
	bool in_water_stable;
	bool is_climbing;
	bool swimming_up;
	
	u8 light;

	Inventory inventory;
	// Actual inventory is backed up here when creative mode is used
	Inventory *inventory_backup;

	u16 hp;

	u16 peer_id;

protected:
	IGameDef *m_gamedef;

	char m_name[PLAYERNAME_SIZE];
	f32 m_pitch;
	f32 m_yaw;
	v3f m_speed;
	v3f m_position;

public:

};

#ifndef SERVER
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
	
	PlayerControl control;

private:
	// This is used for determining the sneaking range
	v3s16 m_sneak_node;
	// Whether the player is allowed to sneak
	bool m_sneak_node_exists;
};
#endif // !SERVER

#endif


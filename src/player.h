/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "constants.h" // BS

#define PLAYERNAME_SIZE 20

#define PLAYERNAME_ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"

class Map;
class IGameDef;
struct CollisionInfo;
class PlayerSAO;

class Player
{
public:

	Player(IGameDef *gamedef);
	virtual ~Player() = 0;

	virtual void move(f32 dtime, Map &map, f32 pos_max_d)
	{}

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
		// return v3f(0, BS*1.5, 0);
		// This is more like in minecraft
		if(camera_barely_in_ceiling)
			return v3f(0,BS*1.5,0);
		else
			return v3f(0,BS*1.625,0);
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

	void updateName(const char *name)
	{
		snprintf(m_name, PLAYERNAME_SIZE, "%s", name);
	}

	const char * getName() const
	{
		return m_name;
	}

	virtual bool isLocal() const
	{ return false; }
	virtual PlayerSAO *getPlayerSAO()
	{ return NULL; }
	virtual void setPlayerSAO(PlayerSAO *sao)
	{ assert(0); }

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
	bool camera_barely_in_ceiling;
	
	u8 light;

	Inventory inventory;

	u16 hp;

	u16 peer_id;

	std::string inventory_formspec;

protected:
	IGameDef *m_gamedef;

	char m_name[PLAYERNAME_SIZE];
	f32 m_pitch;
	f32 m_yaw;
	v3f m_speed;
	v3f m_position;
};

/*
	Player on the server
*/
class RemotePlayer : public Player
{
public:
	RemotePlayer(IGameDef *gamedef): Player(gamedef), m_sao(0) {}
	virtual ~RemotePlayer() {}

	PlayerSAO *getPlayerSAO()
	{ return m_sao; }
	void setPlayerSAO(PlayerSAO *sao)
	{ m_sao = sao; }
	void setPosition(const v3f &position);

private:
	PlayerSAO *m_sao;
};

#endif


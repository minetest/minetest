/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#ifndef PLAYER_HEADER
#define PLAYER_HEADER

#include "common_irrlicht.h"
#include "inventory.h"

#define PLAYERNAME_SIZE 20

class Map;

class Player
{
public:
	Player();
	virtual ~Player();

	void move(f32 dtime, Map &map);

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

	virtual void setPosition(v3f position)
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

	virtual void updateName(const char *name)
	{
		snprintf(m_name, PLAYERNAME_SIZE, "%s", name);
	}

	const char * getName()
	{
		return m_name;
	}

	virtual bool isLocal() const = 0;

	bool touching_ground;
	bool in_water;
	
	Inventory inventory;

	u16 peer_id;

protected:
	char m_name[PLAYERNAME_SIZE];
	f32 m_pitch;
	f32 m_yaw;
	v3f m_speed;
	v3f m_position;
};

class RemotePlayer : public Player, public scene::ISceneNode
{
public:
	RemotePlayer(
		scene::ISceneNode* parent=NULL,
		IrrlichtDevice *device=NULL,
		s32 id=0);
	
	virtual ~RemotePlayer();

	/*
		ISceneNode methods
	*/

	virtual void OnRegisterSceneNode()
	{
		if (IsVisible)
			SceneManager->registerNodeForRendering(this);

		ISceneNode::OnRegisterSceneNode();
	}

	virtual void render()
	{
		// Do nothing
	}
	
	virtual const core::aabbox3d<f32>& getBoundingBox() const
	{
		return m_box;
	}

	void setPosition(v3f position)
	{
		Player::setPosition(position);
		ISceneNode::setPosition(position);
	}

	virtual void setYaw(f32 yaw)
	{
		Player::setYaw(yaw);
		ISceneNode::setRotation(v3f(0, -yaw, 0));
	}

	bool isLocal() const
	{
		return false;
	}

	void updateName(const char *name);

private:
	scene::ITextSceneNode* m_text;
	core::aabbox3d<f32> m_box;
};

struct PlayerControl
{
	PlayerControl()
	{
		up = false;
		down = false;
		left = false;
		right = false;
		jump = false;
		superspeed = false;
		pitch = 0;
		yaw = 0;
	}
	PlayerControl(
		bool a_up,
		bool a_down,
		bool a_left,
		bool a_right,
		bool a_jump,
		bool a_superspeed,
		float a_pitch,
		float a_yaw
	)
	{
		up = a_up;
		down = a_down;
		left = a_left;
		right = a_right;
		jump = a_jump;
		superspeed = a_superspeed;
		pitch = a_pitch;
		yaw = a_yaw;
	}
	bool up;
	bool down;
	bool left;
	bool right;
	bool jump;
	bool superspeed;
	float pitch;
	float yaw;
};

class LocalPlayer : public Player
{
public:
	LocalPlayer();
	virtual ~LocalPlayer();

	bool isLocal() const
	{
		return true;
	}

	void applyControl(float dtime);
	
	PlayerControl control;

private:
};

#endif


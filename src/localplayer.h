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

#include "player.h"
#include "constants.h"
#include "settings.h"
#include <list>

class Client;
class Environment;
class GenericCAO;
class ClientActiveObject;
class ClientEnvironment;
class IGameDef;
class NodeDefManager;

enum LocalPlayerAnimations
{
	NO_ANIM,
	WALK_ANIM,
	DIG_ANIM,
	WD_ANIM
}; // no local animation, walking, digging, both

class LocalPlayer : public Player
{
public:
	LocalPlayer(Client *client, const char *name);
	virtual ~LocalPlayer() = default;

	ClientActiveObject *parent = nullptr;

	// Initialize hp to 0, so that no hearts will be shown if server
	// doesn't support health points
	u16 hp = 0;
	bool is_attached = false;

	v3f overridePosition;

	void applyControl(float dtime, Environment *env);

	v3s16 getStandingNodePos();
	v3s16 getFootstepNodePos();

	// Used to check if anything changed and prevent sending packets if not
	v3f last_position;
	v3f last_speed;
	float last_pitch = 0.0f;
	float last_yaw = 0.0f;
	unsigned int last_keyPressed = 0;
	u8 last_camera_fov = 0;
	u8 last_wanted_range = 0;

	float camera_impact = 0.0f;

	bool makes_footstep_sound = true;

	int last_animation = NO_ANIM;
	float last_animation_speed;

	std::string hotbar_image = "";
	std::string hotbar_selected_image = "";

	video::SColor light_color = video::SColor(255, 255, 255, 255);

	float hurt_tilt_timer = 0.0f;
	float hurt_tilt_strength = 0.0f;

	GenericCAO *getCAO() const { return m_cao; }

	void setCAO(GenericCAO *toset)
	{
		assert(!m_cao); // Pre-condition
		m_cao = toset;
	}

	u32 maxHudId() const { return hud.size(); }

	u16 getBreath() const { return m_breath; }
	void setBreath(u16 breath) { m_breath = breath; }

	v3s16 getLightPosition() const;

	v3f getEyePosition() const { return m_position + getEyeOffset(); }
	v3f getEyeOffset() const;
	void setEyeHeight(float eye_height) { m_eye_height = eye_height; }

	void setCollisionbox(const aabb3f &box) { m_collisionbox = box; }

	float getZoomFOV() const { return m_zoom_fov; }
	void setZoomFOV(float zoom_fov) { m_zoom_fov = zoom_fov; }
	ControlLog &getControlLog() { return m_control_log; }
	virtual bool isAttached() const;
	virtual void setAttached(bool attached) { is_attached = attached; }

	virtual void debugVec(const std::string &title, const v3f &v,
			const std::string &unused = "") const;
	virtual void debugStr(const std::string &str, bool newline = true,
			const std::string &prefix = "") const;
	virtual void debugFloat(const std::string &title, const float val,
			bool newline = true, const std::string &prefix = "") const;
protected:
	virtual bool checkPrivilege(const std::string &priv) const;
	virtual void triggerJumpEvent();

	virtual const NodeDefManager *getNodeDefManager() const;
	virtual void _handleAttachedMove();
	virtual float _getStepHeight() const;
	virtual void reportRegainGround();
	virtual void calculateCameraInCeiling(Map *map, const NodeDefManager *nodemgr);

private:
	u16 m_breath = PLAYER_MAX_BREATH_DEFAULT;
	bool camera_barely_in_ceiling = false;
	aabb3f m_collisionbox = aabb3f(-BS * 0.30f, 0.0f, -BS * 0.30f, BS * 0.30f,
			BS * 1.75f, BS * 0.30f);
	float m_eye_height = 1.625f;
	float m_zoom_fov = 0.0f;

	GenericCAO *m_cao = nullptr;
	Client *m_client;

	ControlLog m_control_log;
};

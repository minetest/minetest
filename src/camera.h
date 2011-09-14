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

#ifndef CAMERA_HEADER
#define CAMERA_HEADER

#include "common_irrlicht.h"
#include "utility.h"

class LocalPlayer;
class MapDrawControl;

/*
	Client camera class, manages the player and camera scene nodes, the viewing distance
	and performs view bobbing etc.
*/
class Camera
{
public:
	Camera(scene::ISceneManager* smgr, MapDrawControl& draw_control);
	~Camera();

	// Get player scene node.
	// This node is positioned at the player's torso (without any view bobbing),
	// as given by Player::m_position. Yaw is applied but not pitch.
	inline scene::ISceneNode* getPlayerNode() const
	{
		return m_playernode;
	}

	// Get head scene node.
	// It has the eye transformation and pitch applied,
	// but no view bobbing.
	inline scene::ISceneNode* getHeadNode() const
	{
		return m_headnode;
	}

	// Get camera scene node.
	// It has the eye transformation, pitch and view bobbing applied.
	inline scene::ICameraSceneNode* getCameraNode() const
	{
		return m_cameranode;
	}

	// Get the camera position (in absolute scene coordinates).
	// This has view bobbing applied.
	inline v3f getPosition() const
	{
		return m_camera_position;
	}

	// Get the camera direction (in absolute camera coordinates).
	// This has view bobbing applied.
	inline v3f getDirection() const
	{
		return m_camera_direction;
	}

	// Horizontal field of view
	inline f32 getFovX() const
	{
		return m_fov_x;
	}

	// Vertical field of view
	inline f32 getFovY() const
	{
		return m_fov_y;
	}

	// Get maximum of getFovX() and getFovY()
	inline f32 getFovMax() const
	{
		return MYMAX(m_fov_x, m_fov_y);
	}

	// Checks if the constructor was able to create the scene nodes
	bool successfullyCreated(std::wstring& error_message);

	// Step the camera: updates the viewing range and view bobbing.
	void step(f32 dtime);

	// Update the camera from the local player's position.
	// frametime is used to adjust the viewing range.
	void update(LocalPlayer* player, f32 frametime, v2u32 screensize);

	// Render distance feedback loop
	void updateViewingRange(f32 frametime_in);

	// Update settings from g_settings
	void updateSettings();

private:
	// Scene manager and nodes
	scene::ISceneManager* m_smgr;
	scene::ISceneNode* m_playernode;
	scene::ISceneNode* m_headnode;
	scene::ICameraSceneNode* m_cameranode;

	// draw control
	MapDrawControl& m_draw_control;

	// viewing_range_min_nodes setting
	f32 m_viewing_range_min;
	// viewing_range_max_nodes setting
	f32 m_viewing_range_max;

	// Absolute camera position
	v3f m_camera_position;
	// Absolute camera direction
	v3f m_camera_direction;

	// Field of view and aspect ratio stuff
	f32 m_aspect;
	f32 m_fov_x;
	f32 m_fov_y;

	// Stuff for viewing range calculations
	f32 m_wanted_frametime;
	f32 m_added_frametime;
	s16 m_added_frames;
	f32 m_range_old;
	f32 m_frametime_old;
	f32 m_frametime_counter;
	f32 m_time_per_range;

	// View bobbing animation frame (0 <= m_view_bobbing < 0x1000000)
	s32 m_view_bobbing_anim;
	// If 0, view bobbing is off (e.g. player is standing).
	// If 1, view bobbing is on (player is walking).
	// If 2, view bobbing is getting switched off.
	s32 m_view_bobbing_state;
	// If true, view bobbing is slown down (player is swimming)
	bool m_view_bobbing_slow;
};

#endif


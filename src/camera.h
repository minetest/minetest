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

#ifndef CAMERA_HEADER
#define CAMERA_HEADER

#include "irrlichttypes_extrabloated.h"
#include "inventory.h"
#include "mesh.h"
#include "client/tile.h"
#include "util/numeric.h"
#include <ICameraSceneNode.h>
#include <ISceneNode.h>
#include <list>

#include "client.h"

class LocalPlayer;
struct MapDrawControl;
class Client;
class WieldMeshSceneNode;

struct Nametag {
	Nametag(scene::ISceneNode *a_parent_node,
			const std::string &a_nametag_text,
			const video::SColor &a_nametag_color):
		parent_node(a_parent_node),
		nametag_text(a_nametag_text),
		nametag_color(a_nametag_color)
	{
	}
	scene::ISceneNode *parent_node;
	std::string nametag_text;
	video::SColor nametag_color;
};

enum CameraMode {CAMERA_MODE_FIRST, CAMERA_MODE_THIRD, CAMERA_MODE_THIRD_FRONT};

/*
	Client camera class, manages the player and camera scene nodes, the viewing distance
	and performs view bobbing etc. It also displays the wielded tool in front of the
	first-person camera.
*/
class Camera
{
public:
	Camera(scene::ISceneManager* smgr, MapDrawControl& draw_control,
			Client *client);
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

	// Get the camera offset
	inline v3s16 getOffset() const
	{
		return m_camera_offset;
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
	bool successfullyCreated(std::string &error_message);

	// Step the camera: updates the viewing range and view bobbing.
	void step(f32 dtime);

	// Update the camera from the local player's position.
	// busytime is used to adjust the viewing range.
	void update(LocalPlayer* player, f32 frametime, f32 busytime,
			f32 tool_reload_ratio, ClientEnvironment &c_env);

	// Update render distance
	void updateViewingRange();

	// Start digging animation
	// Pass 0 for left click, 1 for right click
	void setDigging(s32 button);

	// Replace the wielded item mesh
	void wield(const ItemStack &item);

	// Draw the wielded tool.
	// This has to happen *after* the main scene is drawn.
	// Warning: This clears the Z buffer.
	void drawWieldedTool(irr::core::matrix4* translation=NULL);

	// Toggle the current camera mode
	void toggleCameraMode() {
		if (m_camera_mode == CAMERA_MODE_FIRST)
			m_camera_mode = CAMERA_MODE_THIRD;
		else if (m_camera_mode == CAMERA_MODE_THIRD)
			m_camera_mode = CAMERA_MODE_THIRD_FRONT;
		else
			m_camera_mode = CAMERA_MODE_FIRST;
	}

	//read the current camera mode
	inline CameraMode getCameraMode()
	{
		return m_camera_mode;
	}

	Nametag *addNametag(scene::ISceneNode *parent_node,
		std::string nametag_text, video::SColor nametag_color);

	void removeNametag(Nametag *nametag);

	const std::list<Nametag *> &getNametags() { return m_nametags; }

	void drawNametags();

private:
	// Nodes
	scene::ISceneNode* m_playernode;
	scene::ISceneNode* m_headnode;
	scene::ICameraSceneNode* m_cameranode;

	scene::ISceneManager* m_wieldmgr;
	WieldMeshSceneNode* m_wieldnode;

	// draw control
	MapDrawControl& m_draw_control;

	Client *m_client;
	video::IVideoDriver *m_driver;

	// Absolute camera position
	v3f m_camera_position;
	// Absolute camera direction
	v3f m_camera_direction;
	// Camera offset
	v3s16 m_camera_offset;

	// Field of view and aspect ratio stuff
	f32 m_aspect;
	f32 m_fov_x;
	f32 m_fov_y;

	// View bobbing animation frame (0 <= m_view_bobbing_anim < 1)
	f32 m_view_bobbing_anim;
	// If 0, view bobbing is off (e.g. player is standing).
	// If 1, view bobbing is on (player is walking).
	// If 2, view bobbing is getting switched off.
	s32 m_view_bobbing_state;
	// Speed of view bobbing animation
	f32 m_view_bobbing_speed;
	// Fall view bobbing
	f32 m_view_bobbing_fall;

	// Digging animation frame (0 <= m_digging_anim < 1)
	f32 m_digging_anim;
	// If -1, no digging animation
	// If 0, left-click digging animation
	// If 1, right-click digging animation
	s32 m_digging_button;

	// Animation when changing wielded item
	f32 m_wield_change_timer;
	ItemStack m_wield_item_next;

	CameraMode m_camera_mode;

	f32 m_cache_fall_bobbing_amount;
	f32 m_cache_view_bobbing_amount;
	f32 m_cache_fov;
	f32 m_cache_zoom_fov;

	std::list<Nametag *> m_nametags;
};

#endif

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

#include "camera.h"
#include "debug.h"
#include "main.h" // for g_settings
#include "map.h"
#include "player.h"
#include "utility.h"
#include <cmath>

Camera::Camera(scene::ISceneManager* smgr, MapDrawControl& draw_control):
	m_smgr(smgr),
	m_playernode(NULL),
	m_cameranode(NULL),
	m_draw_control(draw_control),
	m_viewing_range_min(5.0),
	m_viewing_range_max(5.0),

	m_camera_position(0,0,0),
	m_camera_direction(0,0,0),

	m_aspect(1.0),
	m_fov_x(1.0),
	m_fov_y(1.0),

	m_wanted_frametime(0.0),
	m_added_frametime(0),
	m_added_frames(0),
	m_range_old(0),
	m_frametime_old(0),
	m_frametime_counter(0),
	m_time_per_range(30. / 50), // a sane default of 30ms per 50 nodes of range

	m_view_bobbing_anim(0),
	m_view_bobbing_state(0)
{
	//dstream<<__FUNCTION_NAME<<std::endl;

	// note: making the camera node a child of the player node
	// would lead to unexpected behaviour, so we don't do that.
	m_playernode = smgr->addEmptySceneNode(smgr->getRootSceneNode());
	m_cameranode = smgr->addCameraSceneNode(smgr->getRootSceneNode());
	m_cameranode->bindTargetAndRotation(true);

	updateSettings();
}

Camera::~Camera()
{
}

void Camera::step(f32 dtime)
{
	if (m_view_bobbing_state != 0)
	{
		const f32 bobspeed = 0x1000000;
		s32 offset = MYMAX(dtime * bobspeed, 1);
		if (m_view_bobbing_state == 2)
		{
			// Animation is getting turned off
			s32 subanim = (m_view_bobbing_anim & 0x7fffff);
			if (subanim < 0x400000)
				offset = -1 *  MYMIN(offset, subanim);
			else
				offset = MYMIN(offset, 0x800000 - subanim);
		}
		m_view_bobbing_anim = (m_view_bobbing_anim + offset) & 0xffffff;
	}
}

void Camera::update(LocalPlayer* player, f32 frametime, v2u32 screensize)
{
	if (m_playernode == NULL || m_cameranode == NULL)
		return;

	// Set player node transformation
	m_playernode->setPosition(player->getPosition());
	//m_playernode->setRotation(v3f(player->getPitch(), -1 * player->getYaw(), 0));
	m_playernode->setRotation(v3f(0, -1 * player->getYaw(), 0));
	m_playernode->updateAbsolutePosition();

	// Compute relative camera position and target
	v3f relative_cam_pos = player->getEyePosition() - player->getPosition();
	v3f relative_cam_target = v3f(0,0,1);
	relative_cam_target.rotateYZBy(player->getPitch());
	relative_cam_target += relative_cam_pos;

	f32 bobangle = m_view_bobbing_anim * 2 * M_PI / 0x1000000;
	f32 bobangle_s = sin(bobangle);
	f32 bobangle_c = cos(bobangle);
	f32 bobwidth = 0.03 * cos(player->getPitch() * M_PI / 180)
		/ (bobangle_c * bobangle_c + 1);
	f32 bobheight = bobwidth;
	relative_cam_pos.X += bobwidth * bobangle_s;
	relative_cam_pos.Y += bobheight * bobangle_s * bobangle_c;

	// Compute absolute camera position and target
	m_playernode->getAbsoluteTransformation().transformVect(m_camera_position, relative_cam_pos);
	m_playernode->getAbsoluteTransformation().transformVect(m_camera_direction, relative_cam_target);
	m_camera_direction -= m_camera_position;

	// Set camera node transformation
	m_cameranode->setPosition(m_camera_position);
	// *100.0 helps in large map coordinates
	m_cameranode->setTarget(m_camera_position + 100.0 * m_camera_direction);

	// FOV and and aspect ratio
	m_aspect = (f32)screensize.X / (f32) screensize.Y;
	m_fov_x = 2 * atan(0.5 * m_aspect * tan(m_fov_y));
	m_cameranode->setAspectRatio(m_aspect);
	m_cameranode->setFOV(m_fov_y);
	// Just so big a value that everything rendered is visible
	// Some more allowance that m_viewing_range_max * BS because of active objects etc.
	m_cameranode->setFarValue(m_viewing_range_max * BS * 10);

	// Render distance feedback loop
	updateViewingRange(frametime);

	// If the player seems to be walking on solid ground,
	// view bobbing is enabled and free_move is off,
	// start (or continue) the view bobbing animation.
	v3f speed = player->getSpeed();
	//dstream<<"speed: ("<<speed.X<<","<<speed.Y<<","<<speed.Z<<")"<<std::endl;
	if ((hypot(speed.X, speed.Z) > BS) &&
		(fabs(speed.Y) < BS/10) &&
		(g_settings.getBool("view_bobbing") == true) &&
		(g_settings.getBool("free_move") == false))
	{
		// Start animation
		m_view_bobbing_state = 1;
	}
	else if (m_view_bobbing_state == 1)
	{
		// Stop animation
		m_view_bobbing_state = 2;
	}
}

void Camera::updateViewingRange(f32 frametime_in)
{
	if (m_draw_control.range_all)
		return;

	m_added_frametime += frametime_in;
	m_added_frames += 1;

	// Actually this counter kind of sucks because frametime is busytime
	m_frametime_counter -= frametime_in;
	if (m_frametime_counter > 0)
		return;
	m_frametime_counter = 0.2;

	/*dstream<<__FUNCTION_NAME
			<<": Collected "<<m_added_frames<<" frames, total of "
			<<m_added_frametime<<"s."<<std::endl;

	dstream<<"m_draw_control.blocks_drawn="
			<<m_draw_control.blocks_drawn
			<<", m_draw_control.blocks_would_have_drawn="
			<<m_draw_control.blocks_would_have_drawn
			<<std::endl;*/

	m_draw_control.wanted_min_range = m_viewing_range_min;
	m_draw_control.wanted_max_blocks = (1.5*m_draw_control.blocks_would_have_drawn)+1;
	if (m_draw_control.wanted_max_blocks < 10)
		m_draw_control.wanted_max_blocks = 10;

	f32 block_draw_ratio = 1.0;
	if (m_draw_control.blocks_would_have_drawn != 0)
	{
		block_draw_ratio = (f32)m_draw_control.blocks_drawn
			/ (f32)m_draw_control.blocks_would_have_drawn;
	}

	// Calculate the average frametime in the case that all wanted
	// blocks had been drawn
	f32 frametime = m_added_frametime / m_added_frames / block_draw_ratio;

	m_added_frametime = 0.0;
	m_added_frames = 0;

	f32 wanted_frametime_change = m_wanted_frametime - frametime;
	//dstream<<"wanted_frametime_change="<<wanted_frametime_change<<std::endl;

	// If needed frametime change is small, just return
	if (fabs(wanted_frametime_change) < m_wanted_frametime*0.4)
	{
		//dstream<<"ignoring small wanted_frametime_change"<<std::endl;
		return;
	}

	f32 range = m_draw_control.wanted_range;
	f32 new_range = range;

	f32 d_range = range - m_range_old;
	f32 d_frametime = frametime - m_frametime_old;
	if (d_range != 0)
	{
		m_time_per_range = d_frametime / d_range;
	}

	// The minimum allowed calculated frametime-range derivative:
	// Practically this sets the maximum speed of changing the range.
	// The lower this value, the higher the maximum changing speed.
	// A low value here results in wobbly range (0.001)
	// A high value here results in slow changing range (0.0025)
	// SUGG: This could be dynamically adjusted so that when
	//       the camera is turning, this is lower
	//f32 min_time_per_range = 0.0015;
	f32 min_time_per_range = 0.0010;
	//f32 min_time_per_range = 0.05 / range;
	if(m_time_per_range < min_time_per_range)
	{
		m_time_per_range = min_time_per_range;
		//dstream<<"m_time_per_range="<<m_time_per_range<<" (min)"<<std::endl;
	}
	else
	{
		//dstream<<"m_time_per_range="<<m_time_per_range<<std::endl;
	}

	f32 wanted_range_change = wanted_frametime_change / m_time_per_range;
	// Dampen the change a bit to kill oscillations
	//wanted_range_change *= 0.9;
	//wanted_range_change *= 0.75;
	wanted_range_change *= 0.5;
	//dstream<<"wanted_range_change="<<wanted_range_change<<std::endl;

	// If needed range change is very small, just return
	if(fabs(wanted_range_change) < 0.001)
	{
		//dstream<<"ignoring small wanted_range_change"<<std::endl;
		return;
	}

	new_range += wanted_range_change;
	
	//f32 new_range_unclamped = new_range;
	new_range = MYMAX(new_range, m_viewing_range_min);
	new_range = MYMIN(new_range, m_viewing_range_max);
	/*dstream<<"new_range="<<new_range_unclamped
			<<", clamped to "<<new_range<<std::endl;*/

	m_draw_control.wanted_range = new_range;

	m_range_old = new_range;
	m_frametime_old = frametime;
}

void Camera::updateSettings()
{
	m_viewing_range_min = g_settings.getS16("viewing_range_nodes_min");
	m_viewing_range_min = MYMAX(5.0, m_viewing_range_min);

	m_viewing_range_max = g_settings.getS16("viewing_range_nodes_max");
	m_viewing_range_max = MYMAX(m_viewing_range_min, m_viewing_range_max);

	f32 fov_degrees = g_settings.getFloat("fov");
	fov_degrees = MYMAX(fov_degrees, 10.0);
	fov_degrees = MYMIN(fov_degrees, 170.0);
	m_fov_y = fov_degrees * M_PI / 180.0;

	f32 wanted_fps = g_settings.getFloat("wanted_fps");
	wanted_fps = MYMAX(wanted_fps, 1.0);
	m_wanted_frametime = 1.0 / wanted_fps;
}


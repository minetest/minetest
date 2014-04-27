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

#include "camera.h"
#include "debug.h"
#include "client.h"
#include "main.h" // for g_settings
#include "map.h"
#include "clientmap.h" // MapDrawControl
#include "mesh.h"
#include "player.h"
#include "tile.h"
#include <cmath>
#include "settings.h"
#include "itemdef.h" // For wield visualization
#include "noise.h" // easeCurve
#include "gamedef.h"
#include "sound.h"
#include "event.h"
#include "profiler.h"
#include "util/numeric.h"
#include "util/mathconstants.h"
#include "constants.h"

#define CAMERA_OFFSET_STEP 200

#include "nodedef.h"

Camera::Camera(scene::ISceneManager* smgr, MapDrawControl& draw_control,
		IGameDef *gamedef):
	m_playernode(NULL),
	m_headnode(NULL),
	m_cameranode(NULL),

	m_wieldmgr(NULL),
	m_wieldnode(NULL),
	m_wieldlight(0),

	m_draw_control(draw_control),
	m_gamedef(gamedef),

	m_camera_position(0,0,0),
	m_camera_direction(0,0,0),
	m_camera_offset(0,0,0),

	m_aspect(1.0),
	m_fov_x(1.0),
	m_fov_y(1.0),

	m_added_busytime(0),
	m_added_frames(0),
	m_range_old(0),
	m_busytime_old(0),
	m_frametime_counter(0),
	m_time_per_range(30. / 50), // a sane default of 30ms per 50 nodes of range

	m_view_bobbing_anim(0),
	m_view_bobbing_state(0),
	m_view_bobbing_speed(0),
	m_view_bobbing_fall(0),

	m_digging_anim(0),
	m_digging_button(-1),
	m_dummymesh(createCubeMesh(v3f(1,1,1))),

	m_wield_change_timer(0.125),
	m_wield_mesh_next(NULL),
	m_previous_playeritem(-1),
	m_previous_itemname(""),

	m_camera_mode(CAMERA_MODE_FIRST)
{
	//dstream<<__FUNCTION_NAME<<std::endl;

	// note: making the camera node a child of the player node
	// would lead to unexpected behaviour, so we don't do that.
	m_playernode = smgr->addEmptySceneNode(smgr->getRootSceneNode());
	m_headnode = smgr->addEmptySceneNode(m_playernode);
	m_cameranode = smgr->addCameraSceneNode(smgr->getRootSceneNode());
	m_cameranode->bindTargetAndRotation(true);

	// This needs to be in its own scene manager. It is drawn after
	// all other 3D scene nodes and before the GUI.
	m_wieldmgr = smgr->createNewSceneManager();
	m_wieldmgr->addCameraSceneNode();
	m_wieldnode = m_wieldmgr->addMeshSceneNode(m_dummymesh, NULL);  // need a dummy mesh
}

Camera::~Camera()
{
	m_wieldmgr->drop();

	delete m_dummymesh;
}

bool Camera::successfullyCreated(std::wstring& error_message)
{
	if (m_playernode == NULL)
	{
		error_message = L"Failed to create the player scene node";
		return false;
	}
	if (m_headnode == NULL)
	{
		error_message = L"Failed to create the head scene node";
		return false;
	}
	if (m_cameranode == NULL)
	{
		error_message = L"Failed to create the camera scene node";
		return false;
	}
	if (m_wieldmgr == NULL)
	{
		error_message = L"Failed to create the wielded item scene manager";
		return false;
	}
	if (m_wieldnode == NULL)
	{
		error_message = L"Failed to create the wielded item scene node";
		return false;
	}
	return true;
}

// Returns the fractional part of x
inline f32 my_modf(f32 x)
{
	double dummy;
	return modf(x, &dummy);
}

void Camera::step(f32 dtime)
{
	if(m_view_bobbing_fall > 0)
	{
		m_view_bobbing_fall -= 3 * dtime;
		if(m_view_bobbing_fall <= 0)
			m_view_bobbing_fall = -1; // Mark the effect as finished
	}

	bool was_under_zero = m_wield_change_timer < 0;
	if(m_wield_change_timer < 0.125)
		m_wield_change_timer += dtime;
	if(m_wield_change_timer > 0.125)
		m_wield_change_timer = 0.125;

	if(m_wield_change_timer >= 0 && was_under_zero)
	{
		if(m_wield_mesh_next)
		{
			m_wieldnode->setMesh(m_wield_mesh_next);
			m_wieldnode->setVisible(true);
		} else {
			m_wieldnode->setVisible(false);
		}
		m_wield_mesh_next = NULL;
	}

	if (m_view_bobbing_state != 0)
	{
		//f32 offset = dtime * m_view_bobbing_speed * 0.035;
		f32 offset = dtime * m_view_bobbing_speed * 0.030;
		if (m_view_bobbing_state == 2)
		{
#if 0
			// Animation is getting turned off
			if (m_view_bobbing_anim < 0.5)
				m_view_bobbing_anim -= offset;
			else
				m_view_bobbing_anim += offset;
			if (m_view_bobbing_anim <= 0 || m_view_bobbing_anim >= 1)
			{
				m_view_bobbing_anim = 0;
				m_view_bobbing_state = 0;
			}
#endif
#if 1
			// Animation is getting turned off
			if(m_view_bobbing_anim < 0.25)
			{
				m_view_bobbing_anim -= offset;
			} else if(m_view_bobbing_anim > 0.75) {
				m_view_bobbing_anim += offset;
			}
			if(m_view_bobbing_anim < 0.5)
			{
				m_view_bobbing_anim += offset;
				if(m_view_bobbing_anim > 0.5)
					m_view_bobbing_anim = 0.5;
			} else {
				m_view_bobbing_anim -= offset;
				if(m_view_bobbing_anim < 0.5)
					m_view_bobbing_anim = 0.5;
			}
			if(m_view_bobbing_anim <= 0 || m_view_bobbing_anim >= 1 ||
					fabs(m_view_bobbing_anim - 0.5) < 0.01)
			{
				m_view_bobbing_anim = 0;
				m_view_bobbing_state = 0;
			}
#endif
		}
		else
		{
			float was = m_view_bobbing_anim;
			m_view_bobbing_anim = my_modf(m_view_bobbing_anim + offset);
			bool step = (was == 0 ||
					(was < 0.5f && m_view_bobbing_anim >= 0.5f) ||
					(was > 0.5f && m_view_bobbing_anim <= 0.5f));
			if(step)
			{
				MtEvent *e = new SimpleTriggerEvent("ViewBobbingStep");
				m_gamedef->event()->put(e);
			}
		}
	}

	if (m_digging_button != -1)
	{
		f32 offset = dtime * 3.5;
		float m_digging_anim_was = m_digging_anim;
		m_digging_anim += offset;
		if (m_digging_anim >= 1)
		{
			m_digging_anim = 0;
			m_digging_button = -1;
		}
		float lim = 0.15;
		if(m_digging_anim_was < lim && m_digging_anim >= lim)
		{
			if(m_digging_button == 0)
			{
				MtEvent *e = new SimpleTriggerEvent("CameraPunchLeft");
				m_gamedef->event()->put(e);
			} else if(m_digging_button == 1) {
				MtEvent *e = new SimpleTriggerEvent("CameraPunchRight");
				m_gamedef->event()->put(e);
			}
		}
	}
}

void Camera::update(LocalPlayer* player, f32 frametime, f32 busytime,
		f32 tool_reload_ratio, ClientEnvironment &c_env)
{
	// Get player position
	// Smooth the movement when walking up stairs
	v3f old_player_position = m_playernode->getPosition();
	v3f player_position = player->getPosition();
	if (player->isAttached && player->parent)
		player_position = player->parent->getPosition();
	//if(player->touching_ground && player_position.Y > old_player_position.Y)
	if(player->touching_ground &&
			player_position.Y > old_player_position.Y)
	{
		f32 oldy = old_player_position.Y;
		f32 newy = player_position.Y;
		f32 t = exp(-23*frametime);
		player_position.Y = oldy * t + newy * (1-t);
	}

	// Set player node transformation
	m_playernode->setPosition(player_position);
	m_playernode->setRotation(v3f(0, -1 * player->getYaw(), 0));
	m_playernode->updateAbsolutePosition();

	// Get camera tilt timer (hurt animation)
	float cameratilt = fabs(fabs(player->hurt_tilt_timer-0.75)-0.75);

	// Fall bobbing animation
	float fall_bobbing = 0;
	if(player->camera_impact >= 1 && m_camera_mode < CAMERA_MODE_THIRD)
	{
		if(m_view_bobbing_fall == -1) // Effect took place and has finished
			player->camera_impact = m_view_bobbing_fall = 0;
		else if(m_view_bobbing_fall == 0) // Initialize effect
			m_view_bobbing_fall = 1;

		// Convert 0 -> 1 to 0 -> 1 -> 0
		fall_bobbing = m_view_bobbing_fall < 0.5 ? m_view_bobbing_fall * 2 : -(m_view_bobbing_fall - 0.5) * 2 + 1;
		// Smoothen and invert the above
		fall_bobbing = sin(fall_bobbing * 0.5 * M_PI) * -1;
		// Amplify according to the intensity of the impact
		fall_bobbing *= (1 - rangelim(50 / player->camera_impact, 0, 1)) * 5;

		fall_bobbing *= g_settings->getFloat("fall_bobbing_amount");
	}

	// Calculate players eye offset for different camera modes
	v3f PlayerEyeOffset = player->getEyeOffset();
	if (m_camera_mode == CAMERA_MODE_FIRST)
		PlayerEyeOffset += player->eye_offset_first;
	else
		PlayerEyeOffset += player->eye_offset_third;
	
	// Set head node transformation
	m_headnode->setPosition(PlayerEyeOffset+v3f(0,cameratilt*-player->hurt_tilt_strength+fall_bobbing,0));
	m_headnode->setRotation(v3f(player->getPitch(), 0, cameratilt*player->hurt_tilt_strength));
	m_headnode->updateAbsolutePosition();

	// Compute relative camera position and target
	v3f rel_cam_pos = v3f(0,0,0);
	v3f rel_cam_target = v3f(0,0,1);
	v3f rel_cam_up = v3f(0,1,0);

	if (m_view_bobbing_anim != 0 && m_camera_mode < CAMERA_MODE_THIRD)
	{
		f32 bobfrac = my_modf(m_view_bobbing_anim * 2);
		f32 bobdir = (m_view_bobbing_anim < 0.5) ? 1.0 : -1.0;

		#if 1
		f32 bobknob = 1.2;
		f32 bobtmp = sin(pow(bobfrac, bobknob) * M_PI);
		//f32 bobtmp2 = cos(pow(bobfrac, bobknob) * M_PI);

		v3f bobvec = v3f(
			0.3 * bobdir * sin(bobfrac * M_PI),
			-0.28 * bobtmp * bobtmp,
			0.);

		//rel_cam_pos += 0.2 * bobvec;
		//rel_cam_target += 0.03 * bobvec;
		//rel_cam_up.rotateXYBy(0.02 * bobdir * bobtmp * M_PI);
		float f = 1.0;
		f *= g_settings->getFloat("view_bobbing_amount");
		rel_cam_pos += bobvec * f;
		//rel_cam_target += 0.995 * bobvec * f;
		rel_cam_target += bobvec * f;
		rel_cam_target.Z -= 0.005 * bobvec.Z * f;
		//rel_cam_target.X -= 0.005 * bobvec.X * f;
		//rel_cam_target.Y -= 0.005 * bobvec.Y * f;
		rel_cam_up.rotateXYBy(-0.03 * bobdir * bobtmp * M_PI * f);
		#else
		f32 angle_deg = 1 * bobdir * sin(bobfrac * M_PI);
		f32 angle_rad = angle_deg * M_PI / 180;
		f32 r = 0.05;
		v3f off = v3f(
			r * sin(angle_rad),
			r * (cos(angle_rad) - 1),
			0);
		rel_cam_pos += off;
		//rel_cam_target += off;
		rel_cam_up.rotateXYBy(angle_deg);
		#endif

	}

	// Compute absolute camera position and target
	m_headnode->getAbsoluteTransformation().transformVect(m_camera_position, rel_cam_pos);
	m_headnode->getAbsoluteTransformation().rotateVect(m_camera_direction, rel_cam_target - rel_cam_pos);

	v3f abs_cam_up;
	m_headnode->getAbsoluteTransformation().rotateVect(abs_cam_up, rel_cam_up);

	// Seperate camera position for calculation
	v3f my_cp = m_camera_position;
	
	// Reposition the camera for third person view
	if (m_camera_mode > CAMERA_MODE_FIRST)
	{
		if (m_camera_mode == CAMERA_MODE_THIRD_FRONT)
			m_camera_direction *= -1;

		my_cp.Y += 2;

		// Calculate new position
		bool abort = false;
		for (int i = BS; i <= BS*2; i++)
		{
			my_cp.X = m_camera_position.X + m_camera_direction.X*-i;
			my_cp.Z = m_camera_position.Z + m_camera_direction.Z*-i;
			if (i > 12)
				my_cp.Y = m_camera_position.Y + (m_camera_direction.Y*-i);

			// Prevent camera positioned inside nodes
			INodeDefManager *nodemgr = m_gamedef->ndef();
			MapNode n = c_env.getClientMap().getNodeNoEx(floatToInt(my_cp, BS));
			const ContentFeatures& features = nodemgr->get(n);
			if(features.walkable)
			{
				my_cp.X += m_camera_direction.X*-1*-BS/2;
				my_cp.Z += m_camera_direction.Z*-1*-BS/2;
				my_cp.Y += m_camera_direction.Y*-1*-BS/2;
				abort = true;
				break;
			}
		}

		// If node blocks camera position don't move y to heigh
		if (abort && my_cp.Y > player_position.Y+BS*2)
			my_cp.Y = player_position.Y+BS*2;
	}

	// Update offset if too far away from the center of the map
	m_camera_offset.X += CAMERA_OFFSET_STEP*
			(((s16)(my_cp.X/BS) - m_camera_offset.X)/CAMERA_OFFSET_STEP);
	m_camera_offset.Y += CAMERA_OFFSET_STEP*
			(((s16)(my_cp.Y/BS) - m_camera_offset.Y)/CAMERA_OFFSET_STEP);
	m_camera_offset.Z += CAMERA_OFFSET_STEP*
			(((s16)(my_cp.Z/BS) - m_camera_offset.Z)/CAMERA_OFFSET_STEP);
	
	// Set camera node transformation
	m_cameranode->setPosition(my_cp-intToFloat(m_camera_offset, BS));
	m_cameranode->setUpVector(abs_cam_up);
	// *100.0 helps in large map coordinates
	m_cameranode->setTarget(my_cp-intToFloat(m_camera_offset, BS) + 100 * m_camera_direction);

	// update the camera position in front-view mode to render blocks behind player
	if (m_camera_mode == CAMERA_MODE_THIRD_FRONT)
		m_camera_position = my_cp;

	// Get FOV setting
	f32 fov_degrees = g_settings->getFloat("fov");
	fov_degrees = MYMAX(fov_degrees, 10.0);
	fov_degrees = MYMIN(fov_degrees, 170.0);

	// FOV and aspect ratio
	m_aspect = (f32) porting::getWindowSize().X / (f32) porting::getWindowSize().Y;
	m_fov_y = fov_degrees * M_PI / 180.0;
	// Increase vertical FOV on lower aspect ratios (<16:10)
	m_fov_y *= MYMAX(1.0, MYMIN(1.4, sqrt(16./10. / m_aspect)));
	m_fov_x = 2 * atan(m_aspect * tan(0.5 * m_fov_y));
	m_cameranode->setAspectRatio(m_aspect);
	m_cameranode->setFOV(m_fov_y);

	// Position the wielded item
	//v3f wield_position = v3f(45, -35, 65);
	v3f wield_position = v3f(55, -35, 65);
	//v3f wield_rotation = v3f(-100, 120, -100);
	v3f wield_rotation = v3f(-100, 120, -100);
	if(m_wield_change_timer < 0)
		wield_position.Y -= 40 + m_wield_change_timer*320;
	else
		wield_position.Y -= 40 - m_wield_change_timer*320;
	if(m_digging_anim < 0.05 || m_digging_anim > 0.5)
	{
		f32 frac = 1.0;
		if(m_digging_anim > 0.5)
			frac = 2.0 * (m_digging_anim - 0.5);
		// This value starts from 1 and settles to 0
		f32 ratiothing = pow((1.0f - tool_reload_ratio), 0.5f);
		//f32 ratiothing2 = pow(ratiothing, 0.5f);
		f32 ratiothing2 = (easeCurve(ratiothing*0.5))*2.0;
		wield_position.Y -= frac * 25.0 * pow(ratiothing2, 1.7f);
		//wield_position.Z += frac * 5.0 * ratiothing2;
		wield_position.X -= frac * 35.0 * pow(ratiothing2, 1.1f);
		wield_rotation.Y += frac * 70.0 * pow(ratiothing2, 1.4f);
		//wield_rotation.X -= frac * 15.0 * pow(ratiothing2, 1.4f);
		//wield_rotation.Z += frac * 15.0 * pow(ratiothing2, 1.0f);
	}
	if (m_digging_button != -1)
	{
		f32 digfrac = m_digging_anim;
		wield_position.X -= 50 * sin(pow(digfrac, 0.8f) * M_PI);
		wield_position.Y += 24 * sin(digfrac * 1.8 * M_PI);
		wield_position.Z += 25 * 0.5;
	
		// Euler angles are PURE EVIL, so why not use quaternions?
		core::quaternion quat_begin(wield_rotation * core::DEGTORAD);
		core::quaternion quat_end(v3f(80, 30, 100) * core::DEGTORAD);
		core::quaternion quat_slerp;
		quat_slerp.slerp(quat_begin, quat_end, sin(digfrac * M_PI));
		quat_slerp.toEuler(wield_rotation);
		wield_rotation *= core::RADTODEG;
	} else {
		f32 bobfrac = my_modf(m_view_bobbing_anim);
		wield_position.X -= sin(bobfrac*M_PI*2.0) * 3.0;
		wield_position.Y += sin(my_modf(bobfrac*2.0)*M_PI) * 3.0;
	}
	m_wieldnode->setPosition(wield_position);
	m_wieldnode->setRotation(wield_rotation);
	m_wieldlight = player->light;

	// Render distance feedback loop
	updateViewingRange(frametime, busytime);

	// If the player seems to be walking on solid ground,
	// view bobbing is enabled and free_move is off,
	// start (or continue) the view bobbing animation.
	v3f speed = player->getSpeed();
	if ((hypot(speed.X, speed.Z) > BS) &&
		(player->touching_ground) &&
		(g_settings->getBool("view_bobbing") == true) &&
		(g_settings->getBool("free_move") == false ||
				!m_gamedef->checkLocalPrivilege("fly")))
	{
		// Start animation
		m_view_bobbing_state = 1;
		m_view_bobbing_speed = MYMIN(speed.getLength(), 40);
	}
	else if (m_view_bobbing_state == 1)
	{
		// Stop animation
		m_view_bobbing_state = 2;
		m_view_bobbing_speed = 60;
	}
}

void Camera::updateViewingRange(f32 frametime_in, f32 busytime_in)
{
	if (m_draw_control.range_all)
		return;

	m_added_busytime += busytime_in;
	m_added_frames += 1;

	m_frametime_counter -= frametime_in;
	if (m_frametime_counter > 0)
		return;
	m_frametime_counter = 0.2; // Same as ClientMap::updateDrawList interval

	/*dstream<<__FUNCTION_NAME
			<<": Collected "<<m_added_frames<<" frames, total of "
			<<m_added_busytime<<"s."<<std::endl;

	dstream<<"m_draw_control.blocks_drawn="
			<<m_draw_control.blocks_drawn
			<<", m_draw_control.blocks_would_have_drawn="
			<<m_draw_control.blocks_would_have_drawn
			<<std::endl;*/

	// Get current viewing range and FPS settings
	f32 viewing_range_min = g_settings->getS16("viewing_range_nodes_min");
	viewing_range_min = MYMAX(15.0, viewing_range_min);

	f32 viewing_range_max = g_settings->getS16("viewing_range_nodes_max");
	viewing_range_max = MYMAX(viewing_range_min, viewing_range_max);
	
	// Immediately apply hard limits
	if(m_draw_control.wanted_range < viewing_range_min)
		m_draw_control.wanted_range = viewing_range_min;
	if(m_draw_control.wanted_range > viewing_range_max)
		m_draw_control.wanted_range = viewing_range_max;

	// Just so big a value that everything rendered is visible
	// Some more allowance than viewing_range_max * BS because of clouds,
	// active objects, etc.
	if(viewing_range_max < 200*BS)
		m_cameranode->setFarValue(200 * BS * 10);
	else
		m_cameranode->setFarValue(viewing_range_max * BS * 10);

	f32 wanted_fps = g_settings->getFloat("wanted_fps");
	wanted_fps = MYMAX(wanted_fps, 1.0);
	f32 wanted_frametime = 1.0 / wanted_fps;

	m_draw_control.wanted_min_range = viewing_range_min;
	m_draw_control.wanted_max_blocks = (2.0*m_draw_control.blocks_would_have_drawn)+1;
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
	f32 frametime = m_added_busytime / m_added_frames / block_draw_ratio;

	m_added_busytime = 0.0;
	m_added_frames = 0;

	f32 wanted_frametime_change = wanted_frametime - frametime;
	//dstream<<"wanted_frametime_change="<<wanted_frametime_change<<std::endl;
	g_profiler->avg("wanted_frametime_change", wanted_frametime_change);

	// If needed frametime change is small, just return
	// This value was 0.4 for many months until 2011-10-18 by c55;
	if (fabs(wanted_frametime_change) < wanted_frametime*0.33)
	{
		//dstream<<"ignoring small wanted_frametime_change"<<std::endl;
		return;
	}

	f32 range = m_draw_control.wanted_range;
	f32 new_range = range;

	f32 d_range = range - m_range_old;
	f32 d_busytime = busytime_in - m_busytime_old;
	if (d_range != 0)
	{
		m_time_per_range = d_busytime / d_range;
	}
	//dstream<<"time_per_range="<<m_time_per_range<<std::endl;
	g_profiler->avg("time_per_range", m_time_per_range);

	// The minimum allowed calculated frametime-range derivative:
	// Practically this sets the maximum speed of changing the range.
	// The lower this value, the higher the maximum changing speed.
	// A low value here results in wobbly range (0.001)
	// A low value can cause oscillation in very nonlinear time/range curves.
	// A high value here results in slow changing range (0.0025)
	// SUGG: This could be dynamically adjusted so that when
	//       the camera is turning, this is lower
	//f32 min_time_per_range = 0.0010; // Up to 0.4.7
	f32 min_time_per_range = 0.0005;
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
	new_range = MYMAX(new_range, viewing_range_min);
	new_range = MYMIN(new_range, viewing_range_max);
	/*dstream<<"new_range="<<new_range_unclamped
			<<", clamped to "<<new_range<<std::endl;*/

	m_range_old = m_draw_control.wanted_range;
	m_busytime_old = busytime_in;

	m_draw_control.wanted_range = new_range;
}

void Camera::setDigging(s32 button)
{
	if (m_digging_button == -1)
		m_digging_button = button;
}

void Camera::wield(const ItemStack &item, u16 playeritem)
{
	IItemDefManager *idef = m_gamedef->idef();
	std::string itemname = item.getDefinition(idef).name;
	m_wield_mesh_next = idef->getWieldMesh(itemname, m_gamedef);
	if(playeritem != m_previous_playeritem &&
			!(m_previous_itemname == "" && itemname == ""))
	{
		m_previous_playeritem = playeritem;
		m_previous_itemname = itemname;
		if(m_wield_change_timer >= 0.125)
			m_wield_change_timer = -0.125;
		else if(m_wield_change_timer > 0)
		{
			m_wield_change_timer = -m_wield_change_timer;
		}
	} else {
		if(m_wield_mesh_next) {
			m_wieldnode->setMesh(m_wield_mesh_next);
			m_wieldnode->setVisible(true);
		} else {
			m_wieldnode->setVisible(false);
		}
		m_wield_mesh_next = NULL;
		if(m_previous_itemname != itemname)
		{
			m_previous_itemname = itemname;
			m_wield_change_timer = 0;
		}
		else
			m_wield_change_timer = 0.125;
	}
}

void Camera::drawWieldedTool(irr::core::matrix4* translation)
{
	// Set vertex colors of wield mesh according to light level
	u8 li = m_wieldlight;
	video::SColor color(255,li,li,li);
	setMeshColor(m_wieldnode->getMesh(), color);

	// Clear Z buffer
	m_wieldmgr->getVideoDriver()->clearZBuffer();

	// Draw the wielded node (in a separate scene manager)
	scene::ICameraSceneNode* cam = m_wieldmgr->getActiveCamera();
	cam->setAspectRatio(m_cameranode->getAspectRatio());
	cam->setFOV(72.0*M_PI/180.0);
	cam->setNearValue(0.1);
	cam->setFarValue(100);
	if (translation != NULL)
	{
		irr::core::matrix4 startMatrix = cam->getAbsoluteTransformation();
		irr::core::vector3df focusPoint = (cam->getTarget()
				- cam->getAbsolutePosition()).setLength(1)
				+ cam->getAbsolutePosition();

		irr::core::vector3df camera_pos =
				(startMatrix * *translation).getTranslation();
		cam->setPosition(camera_pos);
		cam->setTarget(focusPoint);
	}
	m_wieldmgr->drawAll();
}

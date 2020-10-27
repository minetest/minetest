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
#include "config.h"
#include "map.h"
#include "clientmap.h"     // MapDrawControl
#include "player.h"
#include <cmath>
#include "client/renderingengine.h"
#include "settings.h"
#include "wieldmesh.h"
#include "noise.h"         // easeCurve
#include "sound.h"
#include "mtevent.h"
#include "nodedef.h"
#include "util/numeric.h"
#include "constants.h"
#include "fontengine.h"
#include "script/scripting_client.h"

#define CAMERA_OFFSET_STEP 200
#define WIELDMESH_OFFSET_X 55.0f
#define WIELDMESH_OFFSET_Y -35.0f
#define WIELDMESH_AMPLITUDE_X 7.0f
#define WIELDMESH_AMPLITUDE_Y 10.0f

Camera::Camera(MapDrawControl &draw_control, Client *client):
	m_draw_control(draw_control),
	m_client(client)
{
	scene::ISceneManager *smgr = RenderingEngine::get_scene_manager();
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
	m_wieldnode = new WieldMeshSceneNode(m_wieldmgr, -1, false);
	m_wieldnode->setItem(ItemStack(), m_client);
	m_wieldnode->drop(); // m_wieldmgr grabbed it

	/* TODO: Add a callback function so these can be updated when a setting
	 *       changes.  At this point in time it doesn't matter (e.g. /set
	 *       is documented to change server settings only)
	 *
	 * TODO: Local caching of settings is not optimal and should at some stage
	 *       be updated to use a global settings object for getting thse values
	 *       (as opposed to the this local caching). This can be addressed in
	 *       a later release.
	 */
	m_cache_fall_bobbing_amount = g_settings->getFloat("fall_bobbing_amount");
	m_cache_view_bobbing_amount = g_settings->getFloat("view_bobbing_amount");
	// 45 degrees is the lowest FOV that doesn't cause the server to treat this
	// as a zoom FOV and load world beyond the set server limits.
	m_cache_fov                 = std::fmax(g_settings->getFloat("fov"), 45.0f);
	m_arm_inertia               = g_settings->getBool("arm_inertia");
	m_nametags.clear();
}

Camera::~Camera()
{
	m_wieldmgr->drop();
}

void Camera::notifyFovChange()
{
	LocalPlayer *player = m_client->getEnv().getLocalPlayer();
	assert(player);

	PlayerFovSpec spec = player->getFov();

	/*
	 * Update m_old_fov_degrees first - it serves as the starting point of the
	 * upcoming transition.
	 *
	 * If an FOV transition is already active, mark current FOV as the start of
	 * the new transition. If not, set it to the previous transition's target FOV.
	 */
	if (m_fov_transition_active)
		m_old_fov_degrees = m_curr_fov_degrees;
	else
		m_old_fov_degrees = m_server_sent_fov ? m_target_fov_degrees : m_cache_fov;

	/*
	 * Update m_server_sent_fov next - it corresponds to the target FOV of the
	 * upcoming transition.
	 *
	 * Set it to m_cache_fov, if server-sent FOV is 0. Otherwise check if
	 * server-sent FOV is a multiplier, and multiply it with m_cache_fov instead
	 * of overriding.
	 */
	if (spec.fov == 0.0f) {
		m_server_sent_fov = false;
		m_target_fov_degrees = m_cache_fov;
	} else {
		m_server_sent_fov = true;
		m_target_fov_degrees = spec.is_multiplier ? m_cache_fov * spec.fov : spec.fov;
	}

	if (spec.transition_time > 0.0f)
		m_fov_transition_active = true;

	// If FOV smooth transition is active, initialize required variables
	if (m_fov_transition_active) {
		m_transition_time = spec.transition_time;
		m_fov_diff = m_target_fov_degrees - m_old_fov_degrees;
	}
}

bool Camera::successfullyCreated(std::string &error_message)
{
	if (!m_playernode) {
		error_message = "Failed to create the player scene node";
	} else if (!m_headnode) {
		error_message = "Failed to create the head scene node";
	} else if (!m_cameranode) {
		error_message = "Failed to create the camera scene node";
	} else if (!m_wieldmgr) {
		error_message = "Failed to create the wielded item scene manager";
	} else if (!m_wieldnode) {
		error_message = "Failed to create the wielded item scene node";
	} else {
		error_message.clear();
	}

	if (m_client->modsLoaded())
		m_client->getScript()->on_camera_ready(this);

	return error_message.empty();
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
	m_wield_change_timer = MYMIN(m_wield_change_timer + dtime, 0.125);

	if (m_wield_change_timer >= 0 && was_under_zero)
		m_wieldnode->setItem(m_wield_item_next, m_client);

	if (m_view_bobbing_state != 0)
	{
		//f32 offset = dtime * m_view_bobbing_speed * 0.035;
		f32 offset = dtime * m_view_bobbing_speed * 0.030;
		if (m_view_bobbing_state == 2) {
			// Animation is getting turned off
			if (m_view_bobbing_anim < 0.25) {
				m_view_bobbing_anim -= offset;
			} else if (m_view_bobbing_anim > 0.75) {
				m_view_bobbing_anim += offset;
			}

			if (m_view_bobbing_anim < 0.5) {
				m_view_bobbing_anim += offset;
				if (m_view_bobbing_anim > 0.5)
					m_view_bobbing_anim = 0.5;
			} else {
				m_view_bobbing_anim -= offset;
				if (m_view_bobbing_anim < 0.5)
					m_view_bobbing_anim = 0.5;
			}

			if (m_view_bobbing_anim <= 0 || m_view_bobbing_anim >= 1 ||
					fabs(m_view_bobbing_anim - 0.5) < 0.01) {
				m_view_bobbing_anim = 0;
				m_view_bobbing_state = 0;
			}
		}
		else {
			float was = m_view_bobbing_anim;
			m_view_bobbing_anim = my_modf(m_view_bobbing_anim + offset);
			bool step = (was == 0 ||
					(was < 0.5f && m_view_bobbing_anim >= 0.5f) ||
					(was > 0.5f && m_view_bobbing_anim <= 0.5f));
			if(step) {
				m_client->getEventManager()->put(new SimpleTriggerEvent(MtEvent::VIEW_BOBBING_STEP));
			}
		}
	}

	if (m_digging_button != -1) {
		f32 offset = dtime * 3.5f;
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
			if (m_digging_button == 0) {
				m_client->getEventManager()->put(new SimpleTriggerEvent(MtEvent::CAMERA_PUNCH_LEFT));
			} else if(m_digging_button == 1) {
				m_client->getEventManager()->put(new SimpleTriggerEvent(MtEvent::CAMERA_PUNCH_RIGHT));
			}
		}
	}
}

static inline v2f dir(const v2f &pos_dist)
{
	f32 x = pos_dist.X - WIELDMESH_OFFSET_X;
	f32 y = pos_dist.Y - WIELDMESH_OFFSET_Y;

	f32 x_abs = std::fabs(x);
	f32 y_abs = std::fabs(y);

	if (x_abs >= y_abs) {
		y *= (1.0f / x_abs);
		x /= x_abs;
	}

	if (y_abs >= x_abs) {
		x *= (1.0f / y_abs);
		y /= y_abs;
	}

	return v2f(std::fabs(x), std::fabs(y));
}

void Camera::addArmInertia(f32 player_yaw)
{
	m_cam_vel.X = std::fabs(rangelim(m_last_cam_pos.X - player_yaw,
		-100.0f, 100.0f) / 0.016f) * 0.01f;
	m_cam_vel.Y = std::fabs((m_last_cam_pos.Y - m_camera_direction.Y) / 0.016f);
	f32 gap_X = std::fabs(WIELDMESH_OFFSET_X - m_wieldmesh_offset.X);
	f32 gap_Y = std::fabs(WIELDMESH_OFFSET_Y - m_wieldmesh_offset.Y);

	if (m_cam_vel.X > 1.0f || m_cam_vel.Y > 1.0f) {
		/*
		    The arm moves relative to the camera speed,
		    with an acceleration factor.
		*/

		if (m_cam_vel.X > 1.0f) {
			if (m_cam_vel.X > m_cam_vel_old.X)
				m_cam_vel_old.X = m_cam_vel.X;

			f32 acc_X = 0.12f * (m_cam_vel.X - (gap_X * 0.1f));
			m_wieldmesh_offset.X += m_last_cam_pos.X < player_yaw ? acc_X : -acc_X;

			if (m_last_cam_pos.X != player_yaw)
				m_last_cam_pos.X = player_yaw;

			m_wieldmesh_offset.X = rangelim(m_wieldmesh_offset.X,
				WIELDMESH_OFFSET_X - (WIELDMESH_AMPLITUDE_X * 0.5f),
				WIELDMESH_OFFSET_X + (WIELDMESH_AMPLITUDE_X * 0.5f));
		}

		if (m_cam_vel.Y > 1.0f) {
			if (m_cam_vel.Y > m_cam_vel_old.Y)
				m_cam_vel_old.Y = m_cam_vel.Y;

			f32 acc_Y = 0.12f * (m_cam_vel.Y - (gap_Y * 0.1f));
			m_wieldmesh_offset.Y +=
				m_last_cam_pos.Y > m_camera_direction.Y ? acc_Y : -acc_Y;

			if (m_last_cam_pos.Y != m_camera_direction.Y)
				m_last_cam_pos.Y = m_camera_direction.Y;

			m_wieldmesh_offset.Y = rangelim(m_wieldmesh_offset.Y,
				WIELDMESH_OFFSET_Y - (WIELDMESH_AMPLITUDE_Y * 0.5f),
				WIELDMESH_OFFSET_Y + (WIELDMESH_AMPLITUDE_Y * 0.5f));
		}

		m_arm_dir = dir(m_wieldmesh_offset);
	} else {
		/*
		    Now the arm gets back to its default position when the camera stops,
		    following a vector, with a smooth deceleration factor.
		*/

		f32 dec_X = 0.35f * (std::min(15.0f, m_cam_vel_old.X) * (1.0f +
			(1.0f - m_arm_dir.X))) * (gap_X / 20.0f);

		f32 dec_Y = 0.25f * (std::min(15.0f, m_cam_vel_old.Y) * (1.0f +
			(1.0f - m_arm_dir.Y))) * (gap_Y / 15.0f);

		if (gap_X < 0.1f)
			m_cam_vel_old.X = 0.0f;

		m_wieldmesh_offset.X -=
			m_wieldmesh_offset.X > WIELDMESH_OFFSET_X ? dec_X : -dec_X;

		if (gap_Y < 0.1f)
			m_cam_vel_old.Y = 0.0f;

		m_wieldmesh_offset.Y -=
			m_wieldmesh_offset.Y > WIELDMESH_OFFSET_Y ? dec_Y : -dec_Y;
	}
}

void Camera::update(LocalPlayer* player, f32 frametime, f32 busytime, f32 tool_reload_ratio)
{
	// Get player position
	// Smooth the movement when walking up stairs
	v3f old_player_position = m_playernode->getPosition();
	v3f player_position = player->getPosition();

	// This is worse than `LocalPlayer::getPosition()` but
	// mods expect the player head to be at the parent's position
	// plus eye height.
	if (player->getParent())
		player_position = player->getParent()->getPosition();

	// Smooth the camera movement when the player instantly moves upward due to stepheight.
	// To smooth the 'not touching_ground' stepheight, smoothing is necessary when jumping
	// or swimming (for when moving from liquid to land).
	// Disable smoothing if climbing or flying, to avoid upwards offset of player model
	// when seen in 3rd person view.
	bool flying = g_settings->getBool("free_move") && m_client->checkLocalPrivilege("fly");
	if (player_position.Y > old_player_position.Y && !player->is_climbing && !flying) {
		f32 oldy = old_player_position.Y;
		f32 newy = player_position.Y;
		f32 t = std::exp(-23 * frametime);
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

		fall_bobbing *= m_cache_fall_bobbing_amount;
	}

	// Calculate and translate the head SceneNode offsets
	{
		v3f eye_offset = player->getEyeOffset();
		if (m_camera_mode == CAMERA_MODE_FIRST)
			eye_offset += player->eye_offset_first;
		else
			eye_offset += player->eye_offset_third;

		// Set head node transformation
		eye_offset.Y += cameratilt * -player->hurt_tilt_strength + fall_bobbing;
		m_headnode->setPosition(eye_offset);
		m_headnode->setRotation(v3f(player->getPitch(), 0,
			cameratilt * player->hurt_tilt_strength));
		m_headnode->updateAbsolutePosition();
	}

	// Compute relative camera position and target
	v3f rel_cam_pos = v3f(0,0,0);
	v3f rel_cam_target = v3f(0,0,1);
	v3f rel_cam_up = v3f(0,1,0);

	if (m_cache_view_bobbing_amount != 0.0f && m_view_bobbing_anim != 0.0f &&
		m_camera_mode < CAMERA_MODE_THIRD) {
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
		f *= m_cache_view_bobbing_amount;
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
		for (int i = BS; i <= BS * 2.75; i++) {
			my_cp.X = m_camera_position.X + m_camera_direction.X * -i;
			my_cp.Z = m_camera_position.Z + m_camera_direction.Z * -i;
			if (i > 12)
				my_cp.Y = m_camera_position.Y + (m_camera_direction.Y * -i);

			// Prevent camera positioned inside nodes
			const NodeDefManager *nodemgr = m_client->ndef();
			MapNode n = m_client->getEnv().getClientMap()
				.getNode(floatToInt(my_cp, BS));

			const ContentFeatures& features = nodemgr->get(n);
			if (features.walkable) {
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

	// update the camera position in third-person mode to render blocks behind player
	// and correctly apply liquid post FX.
	if (m_camera_mode != CAMERA_MODE_FIRST)
		m_camera_position = my_cp;

	/*
	 * Apply server-sent FOV, instantaneous or smooth transition.
	 * If not, check for zoom and set to zoom FOV.
	 * Otherwise, default to m_cache_fov.
	 */
	if (m_fov_transition_active) {
		// Smooth FOV transition
		// Dynamically calculate FOV delta based on frametimes
		f32 delta = (frametime / m_transition_time) * m_fov_diff;
		m_curr_fov_degrees += delta;

		// Mark transition as complete if target FOV has been reached
		if ((m_fov_diff > 0.0f && m_curr_fov_degrees >= m_target_fov_degrees) ||
				(m_fov_diff < 0.0f && m_curr_fov_degrees <= m_target_fov_degrees)) {
			m_fov_transition_active = false;
			m_curr_fov_degrees = m_target_fov_degrees;
		}
	} else if (m_server_sent_fov) {
		// Instantaneous FOV change
		m_curr_fov_degrees = m_target_fov_degrees;
	} else if (player->getPlayerControl().zoom && player->getZoomFOV() > 0.001f) {
		// Player requests zoom, apply zoom FOV
		m_curr_fov_degrees = player->getZoomFOV();
	} else {
		// Set to client's selected FOV
		m_curr_fov_degrees = m_cache_fov;
	}
	m_curr_fov_degrees = rangelim(m_curr_fov_degrees, 1.0f, 160.0f);

	// FOV and aspect ratio
	const v2u32 &window_size = RenderingEngine::get_instance()->getWindowSize();
	m_aspect = (f32) window_size.X / (f32) window_size.Y;
	m_fov_y = m_curr_fov_degrees * M_PI / 180.0;
	// Increase vertical FOV on lower aspect ratios (<16:10)
	m_fov_y *= core::clamp(sqrt(16./10. / m_aspect), 1.0, 1.4);
	m_fov_x = 2 * atan(m_aspect * tan(0.5 * m_fov_y));
	m_cameranode->setAspectRatio(m_aspect);
	m_cameranode->setFOV(m_fov_y);

	if (m_arm_inertia)
		addArmInertia(player->getYaw());

	// Position the wielded item
	//v3f wield_position = v3f(45, -35, 65);
	v3f wield_position = v3f(m_wieldmesh_offset.X, m_wieldmesh_offset.Y, 65);
	//v3f wield_rotation = v3f(-100, 120, -100);
	v3f wield_rotation = v3f(-100, 120, -100);
	wield_position.Y += fabs(m_wield_change_timer)*320 - 40;
	if(m_digging_anim < 0.05 || m_digging_anim > 0.5)
	{
		f32 frac = 1.0;
		if(m_digging_anim > 0.5)
			frac = 2.0 * (m_digging_anim - 0.5);
		// This value starts from 1 and settles to 0
		f32 ratiothing = std::pow((1.0f - tool_reload_ratio), 0.5f);
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

	m_wieldnode->setNodeLightColor(player->light_color);

	// Set render distance
	updateViewingRange();

	// If the player is walking, swimming, or climbing,
	// view bobbing is enabled and free_move is off,
	// start (or continue) the view bobbing animation.
	const v3f &speed = player->getSpeed();
	const bool movement_XZ = hypot(speed.X, speed.Z) > BS;
	const bool movement_Y = fabs(speed.Y) > BS;

	const bool walking = movement_XZ && player->touching_ground;
	const bool swimming = (movement_XZ || player->swimming_vertical) && player->in_liquid;
	const bool climbing = movement_Y && player->is_climbing;
	if ((walking || swimming || climbing) && !flying) {
		// Start animation
		m_view_bobbing_state = 1;
		m_view_bobbing_speed = MYMIN(speed.getLength(), 70);
	} else if (m_view_bobbing_state == 1) {
		// Stop animation
		m_view_bobbing_state = 2;
		m_view_bobbing_speed = 60;
	}
}

void Camera::updateViewingRange()
{
	f32 viewing_range = g_settings->getFloat("viewing_range");

	// Ignore near_plane setting on all other platforms to prevent abuse
#if ENABLE_GLES
	m_cameranode->setNearValue(rangelim(
		g_settings->getFloat("near_plane"), 0.0f, 0.25f) * BS);
#else
	m_cameranode->setNearValue(0.1f * BS);
#endif

	m_draw_control.wanted_range = std::fmin(adjustDist(viewing_range, getFovMax()), 4000);
	if (m_draw_control.range_all) {
		m_cameranode->setFarValue(100000.0);
		return;
	}
	m_cameranode->setFarValue((viewing_range < 2000) ? 2000 * BS : viewing_range * BS);
}

void Camera::setDigging(s32 button)
{
	if (m_digging_button == -1)
		m_digging_button = button;
}

void Camera::wield(const ItemStack &item)
{
	if (item.name != m_wield_item_next.name ||
			item.metadata != m_wield_item_next.metadata) {
		m_wield_item_next = item;
		if (m_wield_change_timer > 0)
			m_wield_change_timer = -m_wield_change_timer;
		else if (m_wield_change_timer == 0)
			m_wield_change_timer = -0.001;
	}
}

void Camera::drawWieldedTool(irr::core::matrix4* translation)
{
	// Clear Z buffer so that the wielded tool stays in front of world geometry
	m_wieldmgr->getVideoDriver()->clearZBuffer();

	// Draw the wielded node (in a separate scene manager)
	scene::ICameraSceneNode* cam = m_wieldmgr->getActiveCamera();
	cam->setAspectRatio(m_cameranode->getAspectRatio());
	cam->setFOV(72.0*M_PI/180.0);
	cam->setNearValue(10);
	cam->setFarValue(1000);
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

void Camera::drawNametags()
{
	core::matrix4 trans = m_cameranode->getProjectionMatrix();
	trans *= m_cameranode->getViewMatrix();

	for (std::list<Nametag *>::const_iterator
			i = m_nametags.begin();
			i != m_nametags.end(); ++i) {
		Nametag *nametag = *i;
		if (nametag->nametag_color.getAlpha() == 0) {
			// Enforce hiding nametag,
			// because if freetype is enabled, a grey
			// shadow can remain.
			continue;
		}
		v3f pos = nametag->parent_node->getAbsolutePosition() + nametag->nametag_pos * BS;
		f32 transformed_pos[4] = { pos.X, pos.Y, pos.Z, 1.0f };
		trans.multiplyWith1x4Matrix(transformed_pos);
		if (transformed_pos[3] > 0) {
			std::wstring nametag_colorless =
				unescape_translate(utf8_to_wide(nametag->nametag_text));
			core::dimension2d<u32> textsize =
				g_fontengine->getFont()->getDimension(
				nametag_colorless.c_str());
			f32 zDiv = transformed_pos[3] == 0.0f ? 1.0f :
				core::reciprocal(transformed_pos[3]);
			v2u32 screensize = RenderingEngine::get_video_driver()->getScreenSize();
			v2s32 screen_pos;
			screen_pos.X = screensize.X *
				(0.5 * transformed_pos[0] * zDiv + 0.5) - textsize.Width / 2;
			screen_pos.Y = screensize.Y *
				(0.5 - transformed_pos[1] * zDiv * 0.5) - textsize.Height / 2;
			core::rect<s32> size(0, 0, textsize.Width, textsize.Height);
			g_fontengine->getFont()->draw(
				translate_string(utf8_to_wide(nametag->nametag_text)).c_str(),
				size + screen_pos, nametag->nametag_color);
		}
	}
}

Nametag *Camera::addNametag(scene::ISceneNode *parent_node,
		const std::string &nametag_text, video::SColor nametag_color,
		const v3f &pos)
{
	Nametag *nametag = new Nametag(parent_node, nametag_text, nametag_color, pos);
	m_nametags.push_back(nametag);
	return nametag;
}

void Camera::removeNametag(Nametag *nametag)
{
	m_nametags.remove(nametag);
	delete nametag;
}

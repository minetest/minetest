/*
Minetest
Copyright (C) 2022-2023 Jean-Patrick Guerrero <jeanpatrick.guerrero@gmail.com>
This program is free software;you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation;either version 2.1 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public License along
with this program;if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "client/universalcamera.h"

UniversalCamera::UniversalCamera(
		ClientEnvironment *env, video::IVideoDriver *driver, scene::ISceneNode *parent,
		scene::ISceneManager *smgr, s32 id)
	: scene::ICameraSceneNode(parent, smgr, id), m_env(env), m_driver(driver), m_smgr(smgr)
{
	buildProjectionMatrix();
	buildViewArea();

	auto rts = m_driver->getCurrentRenderTargetSize();
	m_aspect = (f32)rts.Width / rts.Height;
	m_screensize = m_driver->getScreenSize();
}

bool UniversalCamera::OnEvent(const SEvent &event)
{
	if (event.EventType != EET_MOUSE_INPUT_EVENT)
		return false;

	switch (event.MouseInput.Event) {
	case EMIE_MOUSE_MOVED:
		m_mouse_pressed = event.MouseInput.isLeftPressed();
		m_mouse_pos.set(event.MouseInput.X / (f32)(m_screensize.Width * m_undersampling),
				event.MouseInput.Y / (f32)(m_screensize.Height * m_undersampling));
		break;
	case EMIE_MOUSE_WHEEL: {
		f32 new_zoom = m_zoom - (event.MouseInput.Wheel * 10.f);
		if (new_zoom > m_zoom_min && new_zoom < m_zoom_max) {
			m_new_zoom = new_zoom;
			m_action.zooming = m_new_zoom < m_zoom ? Zoom::In : Zoom::Out;
		}
	}
		break;
	default:
		break;
	}

	return true;
}

void UniversalCamera::OnRegisterSceneNode()
{
	if (m_smgr->getActiveCamera() == this) {
		m_smgr->registerNodeForRendering(this, scene::ESNRP_CAMERA);

		const v3f pos = getAbsolutePosition();
		const v3f tgv = (m_target - pos).normalize();
		v3f up = m_up_vector.normalize();

		if (core::equals(std::fabs(tgv.dotProduct(up)), 1.f))
			up.X += 0.5f;

		m_view_area.getTransform(video::ETS_VIEW).buildCameraLookAtMatrixLH(pos, m_target, up);
		buildViewArea();
	}

	scene::ISceneNode::OnRegisterSceneNode();
}

void UniversalCamera::render()
{
	m_driver->clearBuffers(video::ECBF_DEPTH);
	m_screensize = m_driver->getScreenSize();

	const auto vp_size = getViewPort().getSize();
	m_aspect = (f32)vp_size.Width / vp_size.Height;
	setAspectRatio(m_aspect);

	core::matrix4 r;
	r.setRotationDegrees(v3f(0.f, 0.f, m_roll));

	m_driver->setTransform(video::ETS_PROJECTION, getProjectionMatrix() * r);
	m_driver->setTransform(video::ETS_VIEW, getViewMatrix());
}

void UniversalCamera::setPosition(const v3f &pos, bool interpolation, f32 speed)
{
	if (interpolation) {
		m_interp.translation = pos;
		m_interp.start_translation = m_pos;
		m_interp.speed.translate = std::max(0.f, speed);
	} else {
		m_pos = pos;
		updatePositionState();
		scene::ISceneNode::setPosition(m_pos);
		updateAbsolutePosition();
	}
}

void UniversalCamera::setRotation(const v3f &rot, bool interpolation, f32 speed)
{
	/*
	 *  Rotation X/Y is done by orbiting the target pos around a center (the camera pos).
	 *  For rolling around Z, we don't need to touch the target pos.
	 *  Rolling is done through the projection matrix which is much more convenient.
	 */

	v3f target(m_target);
	rotateTarget(target, rot);

	if (interpolation) {
		m_interp.rotation = rot;
		m_interp.target = target;
		m_interp.timer.rotate = 0.f;
		m_interp.rotation_counter = vecAbsolute(rot);
		m_interp.q_rotation = core::quaternion(rot);
		m_interp.speed.rotate = std::max(0.f, speed);
	} else {
		m_target = target;
		m_roll = rot.Z;
	}
}

void UniversalCamera::setFOV(f32 fov, bool interpolation, f32 speed)
{
	if (interpolation) {
		m_interp.fov = fov;
		m_interp.speed.fov = std::max(0.f, speed);
	} else {
		m_fov = fov;
		buildProjectionMatrix();
	}
}

void UniversalCamera::setZoom(f32 zoom, bool interpolation, f32 speed)
{
	if (interpolation) {
		m_interp.zoom = zoom;
		m_interp.speed.zoom = std::max(0.f, speed);
	} else
		m_zoom = zoom;
}

void UniversalCamera::updatePos(const v3f &rot)
{
	m_pos.set(m_zoom + m_target.X, m_target.Y, m_target.Z);
	m_pos.rotateXYBy( rot.Y, m_target);
	m_pos.rotateXZBy(-rot.X, m_target);

	m_up_vector.set(0.f, 1.f, 0.f);
	m_up_vector.rotateXYBy(-rot.Y);
	m_up_vector.rotateXZBy(-rot.X + 180.f);
}

void UniversalCamera::updateDeltaTime(u32 time)
{
	u32 new_time = time;
	m_dtime = 0;

	if (m_last_time != 0)
		m_dtime = new_time >= m_last_time ? new_time - m_last_time : m_last_time - new_time;

	m_last_time = new_time;
}

void UniversalCamera::doZoom()
{
	switch (m_action.zooming) {
	case Zoom::In:
		if (m_zoom >= m_new_zoom)
			m_zoom -= (f32)m_dtime * 0.1f;
		else
			m_action.zooming = Zoom::None;
		break;
	case Zoom::Out:
		if (m_zoom <= m_new_zoom)
			m_zoom += (f32)m_dtime * 0.1f;
		else
			m_action.zooming = Zoom::None;
		break;
	default:
		break;
	}
}

v3f UniversalCamera::doRotate(v3f &rotate)
{
	rotate = m_rot;

	if (m_action.translating | haveParent())
		return rotate;

	static const f32 rot_speed { 500.f };

	if (m_mouse_pressed) {
		if (m_action.rotating) {
			rotate.X += (m_rotate_start.X - m_mouse_pos.X) * rot_speed;
			rotate.Y += (m_rotate_start.Y - m_mouse_pos.Y) * rot_speed;
		} else {
			m_rotate_start.X = m_mouse_pos.X;
			m_rotate_start.Y = m_mouse_pos.Y;
			m_action.rotating = true;
		}
	} else if (m_action.rotating) {
		m_rot.X += (m_rotate_start.X - m_mouse_pos.X) * rot_speed;
		m_rot.Y += (m_rotate_start.Y - m_mouse_pos.Y) * rot_speed;

		rotate.X = m_rot.X;
		rotate.Y = m_rot.Y;

		m_action.rotating = false;
	}

	return rotate;
}

void UniversalCamera::doTranslate()
{
	if (m_action.rotating | haveParent())
		return;

	// Mouse coordinates go from 0 to 1 on both axes
	bool left  = m_mouse_pos.X > 0.f && m_mouse_pos.X < 0.05f;
	bool right = m_mouse_pos.X > 0.95f;
	bool up    = m_mouse_pos.Y > 0.f && m_mouse_pos.Y < 0.05f;
	bool down  = m_mouse_pos.Y > 0.95f;

	m_action.translating = left | right | up | down;

	v3f target = (getPosition() - m_target).normalize();
	v3f move;

	if (left | right) {
		move = target.crossProduct(m_up_vector).normalize();
		move = left ? -move : move;
	} else if (up | down) {
		target.Y = 0.f;
		move = up ? -target : target;
	}

	if (m_action.translating) {
		// Translation factor: move faster when camera's farther
		move *= (m_zoom * 0.7f + m_zoom_max * 0.3f) * 0.0045f;

		setPosition(getPosition() + move);
		setTarget(m_target + move);
	}
}

void UniversalCamera::manipulate(u32 time)
{
	v3f rotate;
	doZoom();
	doRotate(rotate);
	doTranslate();

	if ((m_action.zooming != Zoom::None) | m_action.rotating | m_action.translating)
		updatePos(rotate);
}

void UniversalCamera::interpolate()
{
	bool inter_fov = m_interp.speed.fov > 0.f;
	bool inter_zoom = m_interp.speed.zoom > 0.f;
	bool inter_rot = m_interp.speed.rotate > 0.f;
	bool inter_pos = m_interp.speed.translate > 0.f;

	m_action.interpolating = inter_zoom | inter_rot | inter_pos;

	if (inter_fov) {
		const f32 diff = m_fov - m_interp.fov;
		static const f32 step = 1e-3f;

		if (diff > 0.f)
			m_fov -= step * m_interp.speed.fov;
		else if (diff < 0.f)
			m_fov += step * m_interp.speed.fov;

		if (std::fabs(diff) < step) {
			m_fov = m_interp.fov;
			m_interp.speed.fov = 0.f;
		}
	}

	if (inter_zoom) {
		const f32 diff = m_zoom - m_interp.zoom;
		static const f32 step = 0.1f;

		if (diff > 0.f)
			m_zoom -= step * m_interp.speed.zoom;
		else if (diff < 0.f)
			m_zoom += step * m_interp.speed.zoom;

		if (std::fabs(diff) < step + 1e-3f) {
			m_zoom = m_interp.zoom;
			m_interp.speed.zoom = 0.f;
		}

		updatePos(m_rot);
	}

	if (inter_rot) {
		if (m_interp.rotation_counter != v3f()) {
			// TODO: X/Y interpolated rotation is not working correctly (needs reworking)
			// Z rolling is OK
			const f32 step = 1.f * m_interp.speed.rotate;

			m_interp.rotation_counter -= step;
			m_interp.rotation_counter = v3f(
				std::max(0.f, m_interp.rotation_counter.X),
				std::max(0.f, m_interp.rotation_counter.Y),
				std::max(0.f, m_interp.rotation_counter.Z)
			);

			m_interp.timer.rotate += step;
			core::quaternion rot;
			rot.slerp(rot, m_interp.q_rotation, m_interp.timer.rotate * 0.01f);

			v3f rot_euler;
			rot.toEuler(rot_euler);
			rot_euler *= core::RADTODEG;

			rotateTarget(m_target, rot_euler);

			if (m_interp.rotation_counter.Z > 0.f)
				m_roll += step;
		} else {
			m_target = m_interp.target;
			m_roll = m_interp.rotation.Z;
			m_interp.speed.rotate = 0.f;
		}
	}

	if (inter_pos) {
		if (m_interp.translation > m_interp.start_translation)
			m_pos = clamp(m_pos, m_interp.start_translation, m_interp.translation);
		else
			m_pos = clamp(m_pos, m_interp.translation, m_interp.start_translation);

		if (m_pos != m_interp.translation) {
			const v3f dir = (m_interp.translation - m_pos).normalize() *
					m_interp.speed.translate * 0.1f;
			m_pos += dir;
			m_target += dir;
		} else {
			m_pos = m_interp.translation;
			m_interp.speed.translate = 0.f;
		}

		updatePos(m_rot);
	}
}

void UniversalCamera::OnAnimate(u32 time)
{
	updateDeltaTime(time);

	if (haveParent())
		updatePos(m_rot);

	interpolate();

	/*
	 *  Camera's mouse manipulation is a special operation which require
	 *  some specific conditions to proceed:
	 *    1. There is no interpolation of any kind in progress.
	 *    2. ViewPort is set to full screen.
	 *    3. Game ain't paused, but there is a GUI active on the screen.
	 *    4. Camera is not attached to a parent. It's attached, thus direction is locked.
	 */

	if (!m_action.interpolating &&
			m_viewport == core::rectf(0.f, 0.f, 1.f, 1.f) && !m_env->isGamePaused())
		manipulate(time);

	scene::ISceneNode::setPosition(m_pos);
	updateAbsolutePosition();
}

void UniversalCamera::buildProjectionMatrix()
{
	m_view_area.getTransform(video::ETS_PROJECTION).buildProjectionMatrixPerspectiveFovLH(
			m_fov, m_aspect, m_znear, m_zfar);
}

void UniversalCamera::buildViewArea()
{
	m_view_area.cameraPosition = getAbsolutePosition();
	core::matrix4 mat = getProjectionMatrix() * getViewMatrix();
	m_view_area.setFrom(mat, true);
}

void UniversalCamera::updatePositionState()
{
	v3f pos(m_pos - m_target);

	v2f vec(pos.X, pos.Z);
	m_rot.X = (f32)vec.getAngle();

	pos.rotateXZBy(m_rot.X);
	vec.set(pos.X, pos.Y);
	m_rot.Y = -(f32)vec.getAngle();

	m_zoom = (f32)m_pos.getDistanceFrom(m_target);
}

void UniversalCamera::step()
{
	const v3f current_offset = m_camera_offset;
	setCameraOffset(m_env->getCameraOffset());

	// Camera not attached to a parent
	if (m_parent_id == -1)
		return;

	auto cao = m_env->getActiveObject(m_parent_id);
	if (!cao) {
		m_parent_id = -1;
		return;
	}

	const v3f pos = cao->getPosition() + m_target_offset - m_camera_offset;

	if (m_parent_follow)
		m_target = pos;
	else {
		if (current_offset != m_camera_offset)
			m_pos -= m_camera_offset;
		setTarget(pos);
	}
}

void UniversalCamera::attachTo(ClientActiveObject *object, bool follow)
{
	assert(object != nullptr);

	m_parent_id = object->getId();
	setTargetOffset(object->getPosition());
	m_parent_follow = follow;
}

/*
Minetest
Copyright (C) 2020 Jean-Patrick Guerrero <jeanpatrick.guerrero@gmail.com>

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

#include "guiScene.h"

#include <SViewFrustum.h>
#include <IAnimatedMeshSceneNode.h>
#include "porting.h"

GUIScene::GUIScene(gui::IGUIEnvironment *env, scene::ISceneManager *smgr,
		   gui::IGUIElement *parent, core::recti rect, s32 id)
	: IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rect)
{
	m_driver = env->getVideoDriver();
	m_smgr = smgr->createNewSceneManager(false);

	m_cam = m_smgr->addCameraSceneNode(0, v3f(0.f, 0.f, -100.f), v3f(0.f));
	m_cam->setFOV(30.f * core::DEGTORAD);

	m_smgr->getParameters()->setAttribute(scene::ALLOW_ZWRITE_ON_TRANSPARENT, true);
}

GUIScene::~GUIScene()
{
	setMesh(nullptr);

	m_smgr->drop();
}

scene::IAnimatedMeshSceneNode *GUIScene::setMesh(scene::IAnimatedMesh *mesh)
{
	if (m_mesh) {
		m_mesh->remove();
		m_mesh = nullptr;
	}

	if (!mesh)
		return nullptr;

	m_mesh = m_smgr->addAnimatedMeshSceneNode(mesh);
	m_mesh->setPosition(-m_mesh->getBoundingBox().getCenter());
	m_mesh->animateJoints();

	return m_mesh;
}

void GUIScene::setTexture(u32 idx, video::ITexture *texture)
{
	video::SMaterial &material = m_mesh->getMaterial(idx);
	material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	material.MaterialTypeParam = 0.5f;
	material.TextureLayer[0].Texture = texture;
	material.setFlag(video::EMF_LIGHTING, false);
	material.setFlag(video::EMF_FOG_ENABLE, true);
	material.setFlag(video::EMF_BILINEAR_FILTER, false);
	material.setFlag(video::EMF_BACK_FACE_CULLING, false);
	material.setFlag(video::EMF_ZWRITE_ENABLE, true);
}

void GUIScene::draw()
{
	m_driver->clearBuffers(video::ECBF_DEPTH);

	// Control rotation speed based on time
	u64 new_time = porting::getTimeMs();
	u64 dtime_ms = 0;
	if (m_last_time != 0)
		dtime_ms = porting::getDeltaMs(m_last_time, new_time);
	m_last_time = new_time;

	core::rect<s32> oldViewPort = m_driver->getViewPort();
	m_driver->setViewPort(getAbsoluteClippingRect());

	if (m_bgcolor != 0) {
		core::recti borderRect =
				Environment->getRootGUIElement()->getAbsoluteClippingRect();
		Environment->getSkin()->draw3DSunkenPane(
			this, m_bgcolor, false, true, borderRect, 0);
	}

	core::dimension2d<s32> size = getAbsoluteClippingRect().getSize();
	m_smgr->getActiveCamera()->setAspectRatio((f32)size.Width / (f32)size.Height);

	if (!m_target) {
		updateCamera(m_smgr->addEmptySceneNode());
		rotateCamera(v3f(0.f));
		m_cam->bindTargetAndRotation(true);
	}

	cameraLoop();

	// Continuous rotation
	if (m_inf_rot)
		rotateCamera(v3f(0.f, -0.03f * (float)dtime_ms, 0.f));

	m_smgr->drawAll();

	if (m_initial_rotation && m_mesh) {
		rotateCamera(v3f(m_custom_rot.X, m_custom_rot.Y, 0.f));
		calcOptimalDistance();

		m_initial_rotation = false;
	}

	m_driver->setViewPort(oldViewPort);
}

bool GUIScene::OnEvent(const SEvent &event)
{
	if (m_mouse_ctrl && event.EventType == EET_MOUSE_INPUT_EVENT) {
		if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
			m_last_pos = v2f((f32)event.MouseInput.X, (f32)event.MouseInput.Y);
			return true;
		} else if (event.MouseInput.Event == EMIE_MOUSE_MOVED) {
			if (event.MouseInput.isLeftPressed()) {
				m_curr_pos = v2f((f32)event.MouseInput.X, (f32)event.MouseInput.Y);

				rotateCamera(v3f(
					m_last_pos.Y - m_curr_pos.Y,
					m_curr_pos.X - m_last_pos.X, 0.f));

				m_last_pos = m_curr_pos;
				return true;
			}
		}
	}

	return gui::IGUIElement::OnEvent(event);
}

void GUIScene::setStyles(const std::array<StyleSpec, StyleSpec::NUM_STATES> &styles)
{
	StyleSpec::State state = StyleSpec::STATE_DEFAULT;
	StyleSpec style = StyleSpec::getStyleFromStatePropagation(styles, state);

	setNotClipped(style.getBool(StyleSpec::NOCLIP, false));
	setBackgroundColor(style.getColor(StyleSpec::BGCOLOR, m_bgcolor));
}

/**
 * Sets the frame loop range for the mesh
 */
void GUIScene::setFrameLoop(s32 begin, s32 end)
{
	if (m_mesh->getStartFrame() != begin || m_mesh->getEndFrame() != end)
		m_mesh->setFrameLoop(begin, end);
}

/**
 * Sets the animation speed (FPS) for the mesh
 */
void GUIScene::setAnimationSpeed(f32 speed)
{
	m_mesh->setAnimationSpeed(speed);
}

/* Camera control functions */

inline void GUIScene::calcOptimalDistance()
{
	core::aabbox3df box = m_mesh->getBoundingBox();
	f32 width  = box.MaxEdge.X - box.MinEdge.X;
	f32 height = box.MaxEdge.Y - box.MinEdge.Y;
	f32 depth  = box.MaxEdge.Z - box.MinEdge.Z;
	f32 max_width = width > depth ? width : depth;

	const scene::SViewFrustum *f = m_cam->getViewFrustum();
	f32 cam_far = m_cam->getFarValue();
	f32 far_width = core::line3df(f->getFarLeftUp(), f->getFarRightUp()).getLength();
	f32 far_height = core::line3df(f->getFarLeftUp(), f->getFarLeftDown()).getLength();

	core::recti rect = getAbsolutePosition();
	f32 zoomX = rect.getWidth() / max_width;
	f32 zoomY = rect.getHeight() / height;
	f32 dist;

	if (zoomX < zoomY)
		dist = (max_width / (far_width / cam_far)) + (0.5f * max_width);
	else
		dist = (height / (far_height / cam_far)) + (0.5f * max_width);

	m_cam_distance = dist;
	m_update_cam = true;
}

void GUIScene::updateCamera(scene::ISceneNode *target)
{
	m_target = target;
	updateTargetPos();

	m_last_target_pos = m_target_pos;
	updateCameraPos();

	m_update_cam = true;
}

void GUIScene::updateTargetPos()
{
	m_last_target_pos = m_target_pos;
	m_target->updateAbsolutePosition();
	m_target_pos = m_target->getAbsolutePosition();
}

void GUIScene::setCameraRotation(v3f rot)
{
	correctBounds(rot);

	core::matrix4 mat;
	mat.setRotationDegrees(rot);

	m_cam_pos = v3f(0.f, 0.f, m_cam_distance);
	mat.rotateVect(m_cam_pos);

	m_cam_pos += m_target_pos;
	m_cam->setPosition(m_cam_pos);
	m_update_cam = false;
}

bool GUIScene::correctBounds(v3f &rot)
{
	const float ROTATION_MAX_1 = 60.0f;
	const float ROTATION_MAX_2 = 300.0f;

	// Limit and correct the rotation when needed
	if (rot.X < 90.f) {
		if (rot.X > ROTATION_MAX_1) {
			rot.X = ROTATION_MAX_1;
			return true;
		}
	} else if (rot.X < ROTATION_MAX_2) {
		rot.X = ROTATION_MAX_2;
		return true;
	}

	// Not modified
	return false;
}

void GUIScene::cameraLoop()
{
	updateCameraPos();
	updateTargetPos();

	if (m_target_pos != m_last_target_pos)
		m_update_cam = true;

	if (m_update_cam) {
		m_cam_pos = m_target_pos + (m_cam_pos - m_target_pos).normalize() * m_cam_distance;

		v3f rot = getCameraRotation();
		if (correctBounds(rot))
			setCameraRotation(rot);

		m_cam->setPosition(m_cam_pos);
		m_cam->setTarget(m_target_pos);

		m_update_cam = false;
	}
}

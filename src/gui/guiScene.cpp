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
#include <ILightSceneNode.h>

static const v2f rotation_bound{60.f, 300.f};

GUIScene::GUIScene(gui::IGUIEnvironment *env, scene::ISceneManager *smgr,
		   gui::IGUIElement *parent, core::recti rect, s32 id)
	: IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rect)
{
	m_driver = env->getVideoDriver();
	m_smgr = smgr->createNewSceneManager(false);

	m_cam = m_smgr->addCameraSceneNode(0, v3f(0.f, 0.f, -100.f), v3f(0.f));
	m_cam->setFOV(30.f * core::DEGTORAD);

	scene::ILightSceneNode *light = m_smgr->addLightSceneNode(m_cam);
	light->setRadius(1000.f);

	m_smgr->getParameters()->setAttribute(scene::ALLOW_ZWRITE_ON_TRANSPARENT, true);
}

scene::IAnimatedMeshSceneNode *GUIScene::setMesh(scene::IAnimatedMesh *mesh)
{
	if (!mesh) {
		if (m_mesh) {
			m_mesh->remove();
			m_mesh = nullptr;
		}
		return m_mesh;
	} else if (m_mesh) {
		m_mesh->remove();
		m_mesh = nullptr;
	}

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
}

void GUIScene::draw()
{
	core::rect<s32> oldViewPort = m_driver->getViewPort();
	m_driver->setViewPort(getAbsoluteClippingRect());
	core::recti borderRect = Environment->getRootGUIElement()->getAbsoluteClippingRect();

	if (m_bgcolor != 0) {
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

	if (m_inf_rot || (!m_inf_rot && !has_rotated)) {
		rotateCamera(v3f(0.f, m_custom_rot.Y * -0.5f, 0.f));

		if (!m_inf_rot)
			has_rotated = true;
	}

	m_smgr->drawAll();

	if (m_mesh) {
		calcOptimalDistance();
		rotateCamera(v3f(m_custom_rot.X, 0.f, 0.f));
		m_mesh = nullptr;
	}

	m_driver->setViewPort(oldViewPort);
}

bool GUIScene::OnEvent(const SEvent &event)
{
	if (m_mouse_ctrl) {
		switch(event.EventType) {
		case EET_MOUSE_INPUT_EVENT:
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
			break;
		default: break;
		}
	}

	return gui::IGUIElement::OnEvent(event);
}

void GUIScene::setStyles(const std::array<StyleSpec, StyleSpec::NUM_STATES> &styles)
{
	m_styles = styles;
	StyleSpec::State state = StyleSpec::STATE_DEFAULT;
	StyleSpec style = StyleSpec::getStyleFromStatePropagation(m_styles, state);

	setNotClipped(style.getBool(StyleSpec::NOCLIP, false));
	setBackgroundColor(style.getColor(StyleSpec::BGCOLOR, m_bgcolor));
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
	if (rot.X < 90.f) {
		if (rot.X > rotation_bound.X)
			rot.X = rotation_bound.X;
	} else if (rot.X < rotation_bound.Y) {
		rot.X = rotation_bound.Y;
	}

	core::matrix4 mat;
	mat.setRotationDegrees(rot);

	m_cam_pos = v3f(0.f, 0.f, m_cam_distance);
	mat.rotateVect(m_cam_pos);

	m_cam_pos += m_target_pos;
	m_cam->setPosition(m_cam_pos);
	m_update_cam = false;
}

void GUIScene::checkBounds()
{
	v3f cam_angle = getCameraRotation();
	if (cam_angle.X < 90.f) {
	        if (cam_angle.X > rotation_bound.X) {
			cam_angle.X = rotation_bound.X;
			setCameraRotation(cam_angle);
		}
	} else if (cam_angle.X < rotation_bound.Y) {
		cam_angle.X = rotation_bound.Y;
		setCameraRotation(cam_angle);
	}
}

void GUIScene::cameraLoop()
{
	updateCameraPos();
	updateTargetPos();

	if (m_target_pos != m_last_target_pos)
		m_update_cam = true;

	if (m_update_cam) {
		m_cam_pos = m_target_pos + (m_cam_pos - m_target_pos).normalize() * m_cam_distance;
		checkBounds();

		m_cam->setPosition(m_cam_pos);
		m_cam->setTarget(m_target_pos);

		m_update_cam = false;
	}
}

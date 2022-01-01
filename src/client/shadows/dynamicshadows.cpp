/*
Minetest
Copyright (C) 2021 Liso <anlismon@gmail.com>

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

#include <cmath>

#include "client/shadows/dynamicshadows.h"
#include "client/client.h"
#include "client/clientenvironment.h"
#include "client/clientmap.h"
#include "client/camera.h"

using m4f = core::matrix4;

void DirectionalLight::createSplitMatrices(const Camera *cam)
{
	// Light-space perspective implementation for PSM
	// taken from https://www.cg.tuwien.ac.at/research/vr/lispsm/shadows_egsr2004_revised.pdf

	// camera properties
	scene::ICameraSceneNode *cam_node = cam->getCameraNode();
	v3f cam_pos = cam_node->getAbsolutePosition();
	v3f cam_dir = cam->getDirection().normalize();
	v3f cam_up = v3f(cam->getCameraNode()->getUpVector()).normalize();
	v3f cam_right = cam_up.crossProduct(cam_dir).normalize();
	float cam_near = cam_node->getNearValue();
	float cam_far = BS * MYMIN(farPlane, g_settings->getFloat("viewing_range"));

	v3f top_right_corner = cam_pos + 
				cam_near * cam_dir + 
				cam_near * tan(0.5 * cam_node->getAspectRatio() * cam_node->getFOV()) * cam_right + 
				cam_near * tan(0.5 * cam_node->getFOV()) * cam_up;
 
	// constructing light space
	// Y (up) axis points towards the light.
	// Z (dir) axis is in the same plane as Y and camera_dir and orthogonal to Y
	// X (right) axis complements the Y and Z

	// When looking towards or away from the light, use player's X axis as light space X
	float cos_light = abs(cam_dir.dotProduct(direction));
	float sin_light = sqrt(1 - sqr(cos_light));

	// calculate reference direction of the light
	v3f light_right = direction.crossProduct(v3f(0,1,0));
	if (light_right.getLengthSQ() < 1E-5)
		light_right = v3f(0,0,1);
	light_right = light_right.normalize();

	v3f lcam_up = -direction;
	v3f lcam_right = lcam_up.crossProduct(cam_dir).normalize();
	float lcam_right_max = MYMAX(MYMAX(abs(lcam_right.X), abs(lcam_right.Y)), abs(lcam_right.Z));
	v3f lcam_right_rounded = abs(lcam_right.X) == lcam_right_max ? v3f(lcam_right.X >= 0 ? 1.0 : -1.0, 0, 0) : 
			abs(lcam_right.Y) == lcam_right_max ? v3f(0, lcam_right.Y >= 0 ? 1.0 : -1.0, 0) : 
			v3f(0, 0, lcam_right.Z >= 0 ? 1.0 : -1.0);
	float blend = MYMIN(0.05, MYMAX(0, cos_light - 0.90)) / 0.05;
	lcam_right =  blend * lcam_right_rounded + (1 - blend) * lcam_right;
	v3f lcam_dir = lcam_right.crossProduct(lcam_up).normalize();

	// Define camera position and focus point in the view frustum
	float center_distance = cam_near + 0.5 * (cam_far - cam_near);
	v3f center = cam_pos + cam_dir * center_distance;
	float radius = (center - top_right_corner).getLength();

	// Perspective parameter
	float n = cam_near + sqrt(cam_near * (cam_near + 2 * radius));
	// scale n when light and camera vectors are aligned
	n /= MYMIN(0.15, MYMAX(0.001, pow(sin_light, 2.6))); // power of 2.6 aligns perspective compression with rotation

	v3f p = center - (n + radius + BS) * lcam_dir;
	m4f viewmatrix;
	viewmatrix.buildCameraLookAtMatrixLH(p, center, lcam_up);

	// Build light camera projection from point p to 
	// a sphere with center 'center' and radius 'radius'.
	float light_near = n + MYMIN(radius * (1.0 - sin_light), radius * (1.0 - cos_light));
	float light_far = n + 2 * MYMAX(radius * sin_light, radius * cos_light);
	// float fov_dy = (0.55 + 0.45 * sin_light) * radius / (n + radius);  // balance between aliasing and size of the shadowed area
	float fov_dy = MYMAX(radius * sin_light, radius * cos_light) / (n + radius);
	float fov_dx = fov_dy;
	float aspect = fov_dy / fov_dx;

	// Define projection matrix
	// s swaps Y and Z axes, so that we render from the direction of light
	m4f projmatrix;
	projmatrix.buildProjectionMatrixPerspectiveFovLH(atan(fov_dy) * 2.0f, aspect, light_near, light_far, false);
	m4f s(
		1.0f, 0.0f, 0.0f, 0.0f,
		// 0.0f, 0.0f, -1.0f / (10 * MAP_BLOCKSIZE * BS / 2 * radius), 0.0f,
		0.0f, 0.0f, -1.0f / (2 * radius), 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
	s *= projmatrix;
	projmatrix = s;

	v3f v = light_right.crossProduct(lcam_right);
	float c = lcam_right.dotProduct(light_right);

	float angle = acos(c) + M_PI;
	if (v.dotProduct(direction) < 0)
		angle = -angle;
	
	m4f f(core::matrix4::EM4CONST_IDENTITY);
	f.setRotationAxisRadians(angle, v3f(0,0,1));
	f *= projmatrix;
	projmatrix = f;

	// update the frustum settings
	future_frustum.position = cam->getPosition() - 1600.0f * direction;
	future_frustum.length = 1600.0f;
	future_frustum.ViewMat = viewmatrix;
	future_frustum.ProjOrthMat = projmatrix;
	future_frustum.camera_offset = cam->getOffset();
}

DirectionalLight::DirectionalLight(const u32 shadowMapResolution,
		const v3f &position, video::SColorf lightColor,
		f32 farValue) :
		diffuseColor(lightColor),
		farPlane(farValue), mapRes(shadowMapResolution), pos(position)
{}

void DirectionalLight::update_frustum(const Camera *cam, Client *client, bool force)
{
	if (dirty && !force)
		return;

	float zNear = cam->getCameraNode()->getNearValue();
	float zFar = getMaxFarValue();

	///////////////////////////////////
	// update splits near and fars
	future_frustum.zNear = zNear;
	future_frustum.zFar = zFar;

	// update shadow frustum
	createSplitMatrices(cam);
	// get the draw list for shadows
	client->getEnv().getClientMap().updateDrawListShadow(
			getPosition(), getDirection(), future_frustum.length);
	should_update_map_shadow = true;
	dirty = true;

	// when camera offset changes, adjust the current frustum view matrix to avoid flicker
	v3s16 cam_offset = cam->getOffset();
	if (cam_offset != shadow_frustum.camera_offset) {
		v3f rotated_offset;
		shadow_frustum.ViewMat.rotateVect(rotated_offset, intToFloat(cam_offset - shadow_frustum.camera_offset, BS));
		shadow_frustum.ViewMat.setTranslation(shadow_frustum.ViewMat.getTranslation() + rotated_offset);
		shadow_frustum.camera_offset = cam_offset;
	}
}

void DirectionalLight::commitFrustum()
{
	if (!dirty)
		return;

	shadow_frustum = future_frustum;
	dirty = false;
}

void DirectionalLight::setDirection(v3f dir)
{
	direction = -dir;
	direction.normalize();
}

v3f DirectionalLight::getPosition() const
{
	return shadow_frustum.position;
}

const m4f &DirectionalLight::getViewMatrix() const
{
	return shadow_frustum.ViewMat;
}

const m4f &DirectionalLight::getProjectionMatrix() const
{
	return shadow_frustum.ProjOrthMat;
}

const m4f &DirectionalLight::getFutureViewMatrix() const
{
	return future_frustum.ViewMat;
}

const m4f &DirectionalLight::getFutureProjectionMatrix() const
{
	return future_frustum.ProjOrthMat;
}

m4f DirectionalLight::getViewProjMatrix()
{
	return shadow_frustum.ProjOrthMat * shadow_frustum.ViewMat;
}

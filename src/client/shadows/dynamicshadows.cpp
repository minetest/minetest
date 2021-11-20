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

extern std::string current_debug_message;

void DirectionalLight::createSplitMatrices(const Camera *cam)
{
	// camera properties
	scene::ICameraSceneNode *cam_node = cam->getCameraNode();
	v3f cam_pos = cam_node->getAbsolutePosition();
	v3f cam_dir = cam->getDirection().normalize();
	v3f cam_up = v3f(cam->getCameraNode()->getUpVector()).normalize();
	v3f cam_right = cam_up.crossProduct(cam_dir).normalize();
	float cam_near = cam_node->getNearValue();
	float cam_far = MYMIN(farPlane * BS, cam_node->getFarValue());

	v3f top_right_corner = cam_pos + 
				cam_near * cam_dir + 
				cam_near * tan(0.5 * cam_node->getAspectRatio() * cam_node->getFOV()) * cam_right + 
				cam_near * tan(0.5 * cam_node->getFOV()) * cam_up;
 
	// constructing light space
	// Y (up) axis points towards the light.
	// Z (dir) axis is in the same plane as Y and camera_dir and orthogonal to Y
	// X (right) axis complements the Y and Z

	// When looking towards or away from the light, use player's X axis as light space X
	float r = abs(cam_dir.dotProduct(direction));

	v3f light_up = -direction;
	v3f light_right = (1 - r) * light_up.crossProduct(cam_dir).normalize() + r * cam_right;
	if (light_right.dotProduct(cam_right) < 0)
		light_right = -light_right;
	v3f light_dir = light_right.crossProduct(light_up).normalize();

	// Define camera position and focus point in the view frustum
	v3f center = cam_pos + cam_dir * (0.9 * cam_near + 0.1 * cam_far);
	float radius = (center - top_right_corner).getLength();

	float n = (cam_near + sqrt(cam_near * cam_far)) / MYMAX(0.0001, pow(1 - r, 3.0));
	v3f p = center - (n + radius) * light_dir;

	m4f viewmatrix;
	viewmatrix.buildCameraLookAtMatrixLH(p, center, light_up);

	// Build light camera projection from point p to 
	// a sphere with center 'center' and radius 'radius'.
	float light_near = n;
	float light_far = n + 2 * radius;
	float max_dx = radius / (n + radius);
	float max_dy = radius / (n + radius);

	float aspect = max_dy/max_dx;

	// Define projection matrix
	// s swaps Y and Z axes, so that we render from the direction of light
	m4f projmatrix;
	projmatrix.buildProjectionMatrixPerspectiveFovLH(asin(max_dy) * 2.0f, aspect, light_near, light_far, false);
	m4f s(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f / mapRes, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);
	s *= projmatrix;
	projmatrix = s;

	// update the frustum settings
	future_frustum.position = cam->getPosition() - 1600.0f * direction;
	future_frustum.length = 1600.0f;
	future_frustum.ViewMat = viewmatrix;
	future_frustum.ProjOrthMat = projmatrix;
	future_frustum.camera_offset = cam->getOffset();

	std::stringstream debug;
	debug << std::fixed
			// << "direction: " << direction.X << "," << direction.Y << "," << direction.Z << " | "
			// << "look: " << cam_dir.X << "," << cam_dir.Y << "," << cam_dir.Z << " | "
			// << "cos: " << direction.dotProduct(cam_dir) << " | "
			// << "right: " << cam_right.X << "," << cam_right.Y << "," << cam_right.Z << " | "
			// << "light_dir: " << light_dir.X << "," << light_dir.Y << "," << light_dir.Z << " | "
			// << "light_right: " << light_right.X << "," << light_right.Y << "," << light_right.Z << " | "
			<< "r: " << r << " | n: " << n << " | "
			<< "Fov X: " << max_dx << " | Fov Y: " << max_dy << " | Aspect: " << aspect << " | "
			<< "Near: " << light_near << " | Far: " << light_far << " | " << (light_far - light_near) << " | ";
			;
	current_debug_message = debug.str();
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

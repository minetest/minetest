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
#include "settings.h"

using m4f = core::matrix4;

static v3f quantizeDirection(v3f direction, float step)
{

	float yaw = std::atan2(direction.Z, direction.X);
	float pitch = std::asin(direction.Y); // assume look is normalized

	yaw = std::floor(yaw / step) * step;
	pitch = std::floor(pitch / step) * step;

	return v3f(std::cos(yaw)*std::cos(pitch), std::sin(pitch), std::sin(yaw)*std::cos(pitch));
}

ShadowFrustum ShadowCascade::createFrustum(v3f direction, const Camera *cam, f32 near_value, f32 far_value, f32 center_ratio)
{
	static const float COS_15_DEG = 0.965926f;
	v3f newCenter;
	v3f look = cam->getDirection().normalize();

	// if current look direction is < 15 degrees away from the captured
	// look direction then stick to the captured value, otherwise recapture.
	if (look.dotProduct(last_look) >= COS_15_DEG)
		look = last_look;
	else
		last_look = look;

	// adjusted frustum boundaries for the frustum's size
	float sfNear = near_value;
	float sfFar = far_value * scale;

	// adjusted camera positions
	v3f cam_pos_world = cam->getPosition();

	// if world position is less than 1 block away from the captured
	// world position then stick to the captured value, otherwise recapture.
	if (cam_pos_world.getDistanceFromSQ(last_cam_pos_world) < BS * BS)
		cam_pos_world = last_cam_pos_world;
	else
		last_cam_pos_world = cam_pos_world;

	v3f cam_pos_scene = v3f(cam_pos_world.X - cam->getOffset().X * BS,
			cam_pos_world.Y - cam->getOffset().Y * BS,
			cam_pos_world.Z - cam->getOffset().Z * BS);
	cam_pos_scene += look * sfNear;
	cam_pos_world += look * sfNear;

	// center point of light frustum
	v3f center_scene = cam_pos_scene + look * center_ratio * (sfFar - sfNear);
	v3f center_world = cam_pos_world + look * center_ratio * (sfFar - sfNear);

	float radius = sfFar * (1. - center_ratio); // assume radius is the distance from center to camera's far plane
	float length = far_value * 3.0f;
	v3f eye_displacement = quantizeDirection(direction, M_PI / 2880 /*15 seconds*/) * length;

	// we must compute the viewmat with the position - the camera offset
	// but the future_frustum position must be the actual world position
	ShadowFrustum f;
	f.center = center_scene;
	f.position = center_world - eye_displacement;
	f.length = length;
	f.radius = radius;
	f.zNear = near_value;
	f.zFar = far_value;
	f.ViewMat.buildCameraLookAtMatrixLH(center_scene, center_scene + eye_displacement, v3f(0.0f, 1.0f, 0.0f));
	f.ProjOrthMat.buildProjectionMatrixOrthoLH(2.0 * radius, 2.0 * radius,
			0.0f, length, false);
	f.camera_offset = cam->getOffset();
	return f;
}

bool ShadowCascade::update_frustum(v3f direction, const Camera *cam, Client *client, bool force)
{
	if (dirty && !force)
		return false;

	float zNear = cam->getCameraNode()->getNearValue();
	float zFar = getMaxFarValue();
	if (!client->getEnv().getClientMap().getControl().range_all)
		zFar = MYMIN(zFar, client->getEnv().getClientMap().getControl().wanted_range * BS);

	future_frustum = createFrustum(direction, cam, zNear, zFar, 0.3);
	current_frame = 0;

	dirty = true;

	// when camera offset changes, adjust the current frustum view matrix to avoid flicker
	v3s16 cam_offset = cam->getOffset();
	if (cam_offset != current_frustum.camera_offset) {
		v3f delta = intToFloat(cam_offset - current_frustum.camera_offset, BS);

		current_frustum.center -= delta;

		v3f rotated_delta;
		current_frustum.ViewMat.rotateVect(rotated_delta, delta);
		current_frustum.ViewMat.setTranslation(current_frustum.ViewMat.getTranslation() + rotated_delta);
		current_frustum.camera_offset = cam_offset;
	}
	return true;
}

void ShadowCascade::commitFrustum()
{
	if (!dirty)
		return;

	current_frustum = future_frustum;
	dirty = false;
}

v3f ShadowCascade::getPosition() const
{
	return current_frustum.position;
}

const m4f &ShadowCascade::getViewMatrix() const
{
	return current_frustum.ViewMat;
}

const m4f &ShadowCascade::getProjectionMatrix() const
{
	return current_frustum.ProjOrthMat;
}

const m4f &ShadowCascade::getFutureViewMatrix() const
{
	return future_frustum.ViewMat;
}

const m4f &ShadowCascade::getFutureProjectionMatrix() const
{
	return future_frustum.ProjOrthMat;
}

m4f ShadowCascade::getViewProjMatrix() const
{
	return current_frustum.ProjOrthMat * current_frustum.ViewMat;
}

DirectionalLight::DirectionalLight(const u32 shadowMapResolution,
		const v3f &position, video::SColorf lightColor,
		f32 farValue) :
		diffuseColor(lightColor),
		farPlane(farValue), mapRes(shadowMapResolution), pos(position)
{
	cascades[0].max_frames = 1;
	u8 shadow_update_frames = rangelim(g_settings->getS16("shadow_update_frames"), 0, 32);
	int n_cascades = getCascadesCount();
	if (n_cascades > 1)
		cascades[1].max_frames = rangelim(shadow_update_frames / 4, 1, 8);
	for (u8 i = 2; i < n_cascades; i++)
		getCascade(i).max_frames = shadow_update_frames;
}

void DirectionalLight::setDirection(v3f dir)
{
	direction = -dir;
	direction.normalize();
}

void DirectionalLight::update_frustum(const Camera *cam, Client *client, bool force, u8 max_cascades)
{
	client->getEnv().getClientMap().allocateDrawListShadowCascades(getCascadesCount());

	f32 scale = 20. * BS / getFarValue();
	f32 scale_factor = 1./pow(scale, 1./(cascades.size() - 1.));

	for (auto &cascade: cascades) {
		cascade.farPlane = farPlane;
		cascade.scale = scale;
		scale *= scale_factor;
	}

	for (u8 i = 0; i < MYMIN(max_cascades, getCascadesCount()); i++) {
		auto &cascade = getCascade(i);
		if (cascade.update_frustum(direction, cam, client, force)) {
			// get the draw list for shadows
			client->getEnv().getClientMap().updateDrawListShadowCascade(i,
					cascade.getPosition(), getDirection(), cascade.future_frustum.radius, cascade.future_frustum.length);
		}
	}
}
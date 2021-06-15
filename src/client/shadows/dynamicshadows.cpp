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
	float radius;
	v3f newCenter;

	float fovy = cam->getFovY();
	float adaptPOV = 0.05f;
	
	const float Rad2Grad = 180.f / 3.1415f;
	
	v3f look = cam->getDirection();
	look.normalize();
	//float angle = std::acosf(look.dotProduct(-direction));
	//angle *= Rad2Grad;
	// because / (look.getLength() * direction.getLength()) look and directior are normalized, so 1*1=1
	v3f camPos2 = cam->getPosition();

	if (fovy < .5) {
		//camPos2 += look * shadow_frustum.zFar * .15f;
		adaptPOV = 0.35f;
	}

	//camPos2.Y = 10.0f;
	v3f camPos = v3f(camPos2.X - cam->getOffset().X * BS,
			camPos2.Y - cam->getOffset().Y * BS,
			camPos2.Z - cam->getOffset().Z * BS);
	camPos += look * shadow_frustum.zNear;
	camPos2 += look * shadow_frustum.zNear;
	float end = shadow_frustum.zNear + shadow_frustum.zFar;

	
	newCenter = camPos + look * (end * adaptPOV);
	v3f world_center = camPos2 + look * (end * adaptPOV);
	// Create a vector to the frustum far corner
	// @Liso: move all vars we can outside the loop.
	
	float tanFovY = tanf(cam->getFovY() * 0.5f);
	float tanFovX = tanf(cam->getFovX() * 0.5f);

	const v3f &viewUp = cam->getCameraNode()->getUpVector();
	//viewUp.normalize();

	v3f viewRight = look.crossProduct(viewUp);
	//viewRight.normalize();

	v3f farCornerDirection = look + viewRight * tanFovX + viewUp * tanFovY;
	// Compute the frustumBoundingSphere radius
	v3f boundVec = (camPos + farCornerDirection * shadow_frustum.zFar) - newCenter;
	radius = boundVec.getLength();
	// boundVec.getLength();
	float diam = radius * 2.0f;

	float texelsPerUnit = getMapResolution() / diam;
	m4f mTexelScaling;
	mTexelScaling.setScale(texelsPerUnit);

	m4f mLookAt, mLookAtInv;

	mLookAt.buildCameraLookAtMatrixLH(
			v3f(0.0f, 0.0f, 0.0f), -direction, v3f(0.0f, 1.0f, 0.0f));

	mLookAt *= mTexelScaling;
	mLookAtInv = mLookAt;
	mLookAtInv.makeInverse();

	v3f frustumCenter = newCenter;
	mLookAt.transformVect(frustumCenter);
	frustumCenter.X = floorf(frustumCenter.X); // clamp to texel increment
	frustumCenter.Y = floorf(frustumCenter.Y); // clamp to texel increment
	frustumCenter.Z = floorf(frustumCenter.Z);
	mLookAtInv.transformVect(frustumCenter);
	// probar radius multipliacdor en funcion del I, a menor I mas multiplicador
	v3f eye_displacement = direction * radius;

	// we must compute the viewmat with the position - the camera offset
	// but the shadow_frustum position must be the actual world position
	v3f eye = frustumCenter - eye_displacement;
	shadow_frustum.position = world_center - eye_displacement;
	shadow_frustum.length = radius;
	shadow_frustum.ViewMat.buildCameraLookAtMatrixLH(
			eye, frustumCenter, v3f(0.0f, 1.0f, 0.0f));
	shadow_frustum.ProjOrthMat.buildProjectionMatrixOrthoLH(shadow_frustum.length,
			shadow_frustum.length, 0.0,
			shadow_frustum.length , true);
}

DirectionalLight::DirectionalLight(const u32 shadowMapResolution, const v3f &position,
		video::SColorf lightColor, f32 farValue) :
		diffuseColor(lightColor),
		farPlane(farValue), nearPlane(1.0), mapRes(shadowMapResolution),
		pos(position)
{
}

void DirectionalLight::update_frustum(const Camera *cam, Client *client)
{
	should_update_map_shadow = true;
	float zNear = cam->getCameraNode()->getNearValue();
	nearPlane = zNear;
	float zFar = getMaxFarValue();

	///////////////////////////////////
	// update splits near and fars
	shadow_frustum.zNear = zNear;
	shadow_frustum.zFar = zFar * BS;

	// update shadow frustum
	createSplitMatrices(cam);
	// get the draw list for shadows
	client->getEnv().getClientMap().updateDrawListShadow(
			getPosition(), getDirection(), shadow_frustum.length );
	should_update_map_shadow = true;
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

m4f DirectionalLight::getViewProjMatrix()
{
	return shadow_frustum.ProjOrthMat * shadow_frustum.ViewMat;
}

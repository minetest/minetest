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

#pragma once

#include <array>
#include "irrlichttypes_bloated.h"
#include <matrix4.h>
#include "util/basic_macros.h"
#include "constants.h"

class Camera;
class Client;

#define SHADOW_CASCADES 3

struct ShadowFrustum
{
	f32 zNear{0.0f};
	f32 zFar{0.0f};
	f32 length{0.0f};
	f32 radius{0.0f};
	core::matrix4 ProjOrthMat;
	core::matrix4 ViewMat;
	v3f position;
	v3f player;
	v3f center;
	v3s16 camera_offset;
};

class DirectionalLight;

struct ShadowCascade {
	ShadowFrustum current_frustum;
	ShadowFrustum future_frustum;

	f32 farPlane {1.0};
	v3f last_cam_pos_world {0,0,0};
	v3f last_look {0,1,0};
	bool dirty {false};
	f32 scale {1.0f};
	u8 max_frames {1};
	u8 current_frame {0};

	// Creates a frustum with parameters
	// z_near, z_far - distances of player's camera to take in to account for the frustum
	// center_ratio - interpolation ratio for center of shadow frustum between z_near and z_far
	ShadowFrustum createFrustum(v3f direction, const Camera *cam, f32 z_near, f32 z_far, f32 center_ratio);
	// returns true if the frustum was changed
	bool update_frustum(v3f direction, const Camera *cam, Client *client, bool force = false);

	/// Gets the light's maximum far value, i.e. the shadow boundary
	f32 getMaxFarValue() const
	{
		return farPlane * BS;
	}

	/// Gets the current far value of the light
	f32 getFarValue() const
	{
		return current_frustum.zFar;
	}

	v3f getPosition() const;

	const core::matrix4 &getViewMatrix() const;
	const core::matrix4 &getProjectionMatrix() const;
	const core::matrix4 &getFutureViewMatrix() const;
	const core::matrix4 &getFutureProjectionMatrix() const;
	core::matrix4 getViewProjMatrix() const;

	void commitFrustum();
};

class DirectionalLight
{
public:
	DirectionalLight(const u32 shadowMapResolution,
			const v3f &position,
			video::SColorf lightColor = video::SColor(0xffffffff),
			f32 farValue = 100.0f);
	~DirectionalLight() = default;

	//DISABLE_CLASS_COPY(DirectionalLight)

	void update_frustum(const Camera *cam, Client *client, bool force, u8 max_cascades);

	// when set direction is updated to negative normalized(direction)
	void setDirection(v3f dir);
	v3f getDirection() const{
		return direction;
	};

	/// Gets the light's maximum far value, i.e. the shadow boundary
	f32 getMaxFarValue() const
	{
		return farPlane * BS;
	}

	/// Gets the current far value of the light
	f32 getFarValue() const
	{
		return getCascade(getCascadesCount() - 1).getFarValue();
	}


	/// Gets the light's color.
	const video::SColorf &getLightColor() const
	{
		return diffuseColor;
	}

	/// Sets the light's color.
	void setLightColor(const video::SColorf &lightColor)
	{
		diffuseColor = lightColor;
	}

	/// Gets the shadow map resolution for this light.
	u32 getMapResolution() const
	{
		return mapRes;
	}

	bool should_update_map_shadow{true};

	u8 getCascadesCount() const { return cascades.size(); }
	const ShadowCascade &getCascade(u8 index) const { return cascades.at(index); }
	ShadowCascade &getCascade(u8 index) { return cascades.at(index); }

private:
	video::SColorf diffuseColor;

	f32 farPlane;
	u32 mapRes;

	v3f pos;
	v3f direction{0};

	std::array<ShadowCascade, SHADOW_CASCADES> cascades;
};

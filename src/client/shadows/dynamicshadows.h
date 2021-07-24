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

#include "irrlichttypes_bloated.h"
#include <matrix4.h>
#include "util/basic_macros.h"
#include <array>
#include "mapnode.h"

class Camera;
class Client;

typedef core::matrix4 m4f

struct shadowFrustum
{
	f32 zNear{0.0f};
	f32 zFar{0.0f};
	f32 length{0.0f};
	m4f ProjOrthMat;
	m4f ViewMat;
	v3f position;
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

	void update_frustum(const Camera *cam, Client *client);

	// when set direction is updated to negative normalized(direction)
	void setDirection(v3f dir);
	v3f getDirection() const{
		return direction;
	};
	v3f getPosition() const;

	/// Gets the light's matrices.
	const m4f &getViewMatrix() const;
	const m4f &getProjectionMatrix() const;
	m4f getViewProjMatrix();

	/// Gets the light's far value.
	f32 getMaxFarValue() const
	{
		return farPlane;
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

private:
	void createSplitMatrices(const Camera *cam);

	video::SColorf diffuseColor;

	f32 farPlane;
	u32 mapRes;

	v3f pos;
	v3f direction{0};
	shadowFrustum shadow_frustum;
};



// Shadow frustum for point light

struct pLShadowFrustum
{
	v3f direction{0.0f};

	f32 nearPlane{0.0f};
	f32 farPlane{0.0f};

	f32 fov{90.0f};

	m4f ProjPerspectiveMat;
	m4f ViewMat;
};

// Enum class of directions

enum class Direction : u16
{
	POS_X = 0,
	NEG_X,
	POS_Y,
	NEG_Y,
	POS_Z,
	NEG_Z,
	COUNT
};

// Point light class

class PointLight
{
public:
	// Constructor
	PointLight(u32 mapResolution, f32 farPlane, v3f position = v3f(0.0f), video::SColorf diffuseColor = video::SColorf(0xffffffff), MapNode* lightNode = nullptr);

	// Getters

	// Returns list of shadow frustums
	std::array<pLShadowFrustum, 6> getShadowFrustums() const
	{
		return m_shadow_frustums;
	}

	// Returns certain shadow frustum
	pLShadowFrustum getShadowFrustum(Direction dir) const
	{
		return m_shadow_frustums[dir];
	}

	// Return light color
	video::SColorf getLightColor() const
	{
		return m_diffuse_color;
	}

	// Returns frustum position in world coordinates
	v3f getFrustumPosition() const
	{
		return m_position;
	}

	// Returns depth map resolution
	u32 getMapResolution() const
	{
		return m_map_res;
	}

	f32 getMaxFarPlane() const
	{
		return m_far_plane;
	}

	// Returns bound light node
	MapNode* getBoundLightNode() const
	{
		return m_light_node;
	}

	// Sets light color
	void setLightColor(video::SColorf new_color)
	{
		m_diffuse_color = new_color;
	}

	//void updateShadowDrawList(Client *client) const;
private:
	// Color of light
	video::SColorf m_diffuse_color;

	// Shadow frustums for all 6 directions (+X, -X, +Y, -Y, +Z, -Z)
	std::array<pLShadowFrustum, 6> m_shadow_frustums;

	// Position of light source
	v3f m_position;

	// Map resolution
	u32 m_map_res;

	// Far plane
	f32 m_far_plane;

	// Point light should be bound to the node that is emitting it
	MapNode* m_light_node;
};

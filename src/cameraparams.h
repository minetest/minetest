/*
Minetest
Copyright (C) 2022 x2048, Dmitry Kostenko <codeforsmile@gmail.com>
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

// This file contains common data structures and constants for managing cameras across client and server

/// @brief Describes interpolation of camera parameters
struct CameraInterpolation
{
	bool enabled { false };
	float speed  { 0.f };
};

/// @brief Parameters for attaching a camera to an entity
struct CameraAttachment {
	bool enabled  { false };
	u16 object_id { 0 };
	bool follow   { false };
};

/// @brief Flags defining what has changed in the camera parameters since the last state
enum CameraChangeFlags
{
	CAM_CHANGE_POS        = 0x01,
	CAM_CHANGE_ROTATION   = 0x02,
	CAM_CHANGE_FOV        = 0x04,
	CAM_CHANGE_ZOOM       = 0x08,
	CAM_CHANGE_TARGET     = 0x10,
	CAM_CHANGE_VIEWPORT   = 0x20,
	CAM_CHANGE_ATTACHMENT = 0x40,
	CAM_CHANGE_TEXTURE    = 0x80,
	CAM_REMOVE            = 0x0100,
};

/// @brief Parameters of an API-controlled camera.
struct CameraParams
{
	s16 id;
	u16 change_flags { 0 };
	bool enabled { false };

	v3f pos;
	CameraInterpolation interpolate_pos;

	v3f rotation;
	CameraInterpolation interpolate_rotation;

	float fov { 72.f };
	CameraInterpolation interpolate_fov;

	float zoom { 0.f };
	CameraInterpolation interpolate_zoom;

	v3f target;
	f32 viewport[4] {0.f, 0.f, 0.f, 0.f};

	/// ID of the entity to which the camera should be attached, or 0
	CameraAttachment attachment;

	// Name and aspect ratio of the texture to render to
	std::string texture_name;
	f32 texture_aspect_ratio { 1.f };

	CameraParams(s16 _id) : id(_id) {}
	CameraParams() : id(0) {}
};

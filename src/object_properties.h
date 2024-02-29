/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <optional>
#include <tuple>
#include <string>
#include "irrlichttypes_bloated.h"
#include <iostream>
#include <map>
#include <vector>
#include "util/pointabilities.h"

struct ObjectProperties
{
	/* member variables ordered roughly by size */

	std::vector<std::string> textures;
	std::vector<video::SColor> colors;
	// Values are BS=1
	aabb3f collisionbox = aabb3f(-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f);
	// Values are BS=1
	aabb3f selectionbox = aabb3f(-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f);
	std::string visual = "sprite";
	std::string mesh;
	std::string damage_texture_modifier = "^[brighten";
	std::string nametag;
	std::string infotext;
	// For dropped items, this contains the serialized item.
	std::string wield_item;
	v3f visual_size = v3f(1, 1, 1);
	video::SColor nametag_color = video::SColor(255, 255, 255, 255);
	std::optional<video::SColor> nametag_bgcolor;
	v2s16 spritediv = v2s16(1, 1);
	v2s16 initial_sprite_basepos;
	f32 stepheight = 0.0f;
	float automatic_rotate = 0.0f;
	f32 automatic_face_movement_dir_offset = 0.0f;
	f32 automatic_face_movement_max_rotation_per_sec = -1.0f;
	float eye_height = 1.625f;
	float zoom_fov = 0.0f;
	u16 hp_max = 1;
	u16 breath_max = 0;
	s8 glow = 0;
	PointabilityType pointable = PointabilityType::POINTABLE;
	// In a future protocol these could be a flag field.
	bool physical = false;
	bool collideWithObjects = true;
	bool rotate_selectionbox = false;
	bool is_visible = true;
	bool makes_footstep_sound = false;
	bool automatic_face_movement_dir = false;
	bool backface_culling = true;
	bool static_save = true;
	bool use_texture_alpha = false;
	bool shaded = true;
	bool show_on_minimap = false;

	ObjectProperties();

	std::string dump() const;

private:
	auto tie() const {
		// Make sure to add new members to this list!
		return std::tie(
		textures, colors, collisionbox, selectionbox, visual, mesh, damage_texture_modifier,
		nametag, infotext, wield_item, visual_size, nametag_color, nametag_bgcolor,
		spritediv, initial_sprite_basepos, stepheight, automatic_rotate,
		automatic_face_movement_dir_offset, automatic_face_movement_max_rotation_per_sec,
		eye_height, zoom_fov, hp_max, breath_max, glow, pointable, physical,
		collideWithObjects, rotate_selectionbox, is_visible, makes_footstep_sound,
		automatic_face_movement_dir, backface_culling, static_save, use_texture_alpha,
		shaded, show_on_minimap
		);
	}

public:
	bool operator==(const ObjectProperties &other) const {
		return tie() == other.tie();
	};
	bool operator!=(const ObjectProperties &other) const {
		return tie() != other.tie();
	};

	/**
	 * Check limits of some important properties that'd cause exceptions later on.
	 * Errornous values are discarded after printing a warning.
	 * @return true if a problem was found
	*/
	bool validate();

	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);
};

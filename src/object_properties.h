// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#pragma once

#include <optional>
#include <string>
#include "irrlichttypes_bloated.h"
#include <iostream>
#include <vector>
#include "util/pointabilities.h"
#include <bitset>

struct ObjectProperties
{
	/* member variables ordered roughly by size */

	std::vector<std::string> textures;
	std::vector<video::SColor> colors; // Currently unused
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

	bool operator==(const ObjectProperties &other) const;
	bool operator!=(const ObjectProperties &other) const {
		return !(*this == other);
	}

	/**
	 * Check limits of some important properties that'd cause exceptions later on.
	 * Errornous values are discarded after printing a warning.
	 * @return true if a problem was found
	*/
	bool validate();

	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);

	using ChangedProperties = std::bitset<27>;
	ChangedProperties getChange(const ObjectProperties &other);
	static constexpr ChangedProperties nametag_change{0b1100010000000ul};
	static constexpr ChangedProperties full_change{~0ul};

	// Those only change values if the corresponding bit is set,
	// presumably a little slower to parse, but much more space efficient.
	// (Even if all bits are set, they are not compatible with serialize and deSerialize.)
	void serializeChanges(std::ostream &os, const ChangedProperties &fields) const;
	void deSerializeChanges(std::istream &is);
};

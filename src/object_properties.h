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

#ifndef OBJECT_PROPERTIES_HEADER
#define OBJECT_PROPERTIES_HEADER

#include <string>
#include "irrlichttypes_bloated.h"
#include <iostream>
#include <map>
#include <vector>

struct ObjectProperties
{
	// Values are BS=1
	s16 hp_max;
	bool physical;
	bool collideWithObjects;
	float weight;
	core::aabbox3d<f32> collisionbox;
	std::string visual;
	std::string mesh;
	v2f visual_size;
	std::vector<std::string> textures;
	std::vector<video::SColor> colors;
	v2s16 spritediv;
	v2s16 initial_sprite_basepos;
	bool is_visible;
	bool makes_footstep_sound;
	float automatic_rotate;
	f32 stepheight;
	bool automatic_face_movement_dir;
	f32 automatic_face_movement_dir_offset;


	ObjectProperties();
	std::string dump();
	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);
};

#endif


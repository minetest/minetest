/*
Minetest
Copyright (C) 2016 sfan5 <sfan5@live.de>

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

#include <iostream>
#include "irrlichttypes_bloated.h"

enum TileAnimationType
{
	TAT_NONE = 0,
	TAT_VERTICAL_FRAMES = 1,
	TAT_SHEET_2D = 2,
};

struct TileAnimationParams
{
	enum TileAnimationType type;
	union
	{
		// struct {
		// } none;
		struct
		{
			int aspect_w; // width for aspect ratio
			int aspect_h; // height for aspect ratio
			float length; // seconds
		} vertical_frames;
		struct
		{
			int frames_w;       // number of frames left-to-right
			int frames_h;       // number of frames top-to-bottom
			float frame_length; // seconds
		} sheet_2d;
	};

	void serialize(std::ostream &os, u16 protocol_ver) const;
	void deSerialize(std::istream &is, u16 protocol_ver);
	void determineParams(v2u32 texture_size, int *frame_count, int *frame_length_ms,
			v2u32 *frame_size) const;
	void getTextureModifer(std::ostream &os, v2u32 texture_size, int frame) const;
	v2f getTextureCoords(v2u32 texture_size, int frame) const;
};

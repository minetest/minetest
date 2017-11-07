/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include <vector>
#include "client/tile.h"
#include <S3DVertex.h>

struct MeshMakeData;

struct FastFace
{
	TileLayer layer;
	video::S3DVertex vertices[4]; // Precalculated vertices
	/*!
	 * The face is divided into two triangles. If this is true,
	 * vertices 0 and 2 are connected, othervise vertices 1 and 3
	 * are connected.
	 */
	bool vertex_0_2_connected;
	u8 layernum;
	bool world_aligned;
};

void updateAllFastFaceRows(MeshMakeData *data,
		std::vector<FastFace> &dest);

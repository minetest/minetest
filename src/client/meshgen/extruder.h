/*
Minetest
Copyright (C) 2017-2018 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

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
#include <vector>
#include "irrlichttypes_bloated.h"
#include <ITexture.h>
#include <S3DVertex.h>

struct ExtrudedMesh
{
	std::vector<video::S3DVertex> vertices;
	std::vector<u16> indices;
	std::vector<video::S3DVertex> overlay_vertices;
	std::vector<u16> overlay_indices;

	explicit operator bool() const noexcept { return !vertices.empty(); }
};

/*
 * Handles conversion of textures to extruded meshes.
 * Each instance may only be used once.
 */
class Extruder
{
	enum FaceDir
	{
		Up,
		Down,
		Left,
		Right,
	};

	static constexpr int threshold = 128;
	static constexpr float thickness = 0.1f;
	video::ITexture *texture;
	const video::SColor *data;
	u32 w, h;
	float dw, dh;
	ExtrudedMesh mesh;
	std::array<std::vector<char>, 4> faces;

	video::SColor pixel(u32 i, u32 j);
	bool isOpaque(u32 i, u32 j);
	void createEdgeFace(u32 k, FaceDir dir);
	void addLongFace(u32 i, u32 j, FaceDir dir);
	void addLargeFace(int id);

public:
	explicit Extruder(video::ITexture *_texture);
	Extruder(const Extruder &) = delete;
	Extruder(Extruder &&) = delete;
	~Extruder();
	Extruder &operator=(const Extruder &) = delete;
	Extruder &operator=(Extruder &&) = delete;
	void extrude();
	ExtrudedMesh takeMesh() noexcept;
};

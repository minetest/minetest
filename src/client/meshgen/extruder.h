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

/** Handles conversion of textures to extruded meshes.
 * @note Each instance may only be used once.
 *
 * Usage:
 *
 *     Extruder extruder(texture);
 *     extruder.extrude();
 *     extruded = extruder.takeMesh();
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

	/** Minimal pixel alpha for it to be considered opaque. */
	static constexpr int threshold = 128;

	/** Extruded mesh thickness. Mesh width is 1.0. */
	static constexpr float thickness = 0.1f;

	video::ITexture *texture;
	const video::SColor *data;
	u32 w, h;
	float dw, dh;
	ExtrudedMesh mesh;

	/** Flags controlling face creation.
	 *
	 * Array is indexed by #FaceDir. Vector is indexed by row (for horizontal faces)
	 * or column (for vertical ones) number, starting from the upper-left corner.
	 * Each entry is a *boolean* flag.
	 * @note @c char is used as @c vector<bool> is optimized for space
	 * and not for speed.
	 */
	std::array<std::vector<char>, 4> faces;

	video::SColor pixel(u32 i, u32 j);
	bool isOpaque(u32 i, u32 j);

	/** Marks the @p k -th face facing @p dir for creation.
	 * May be called many times for the same face.
	 */
	void createEdgeFace(u32 k, FaceDir dir);

	/** Actually creates a "long" face starting at pixel @p i, @p j, facing @p dir
	 * @p i should be zero for horizontal faces; analogously for @p j.
	 */
	void addLongFace(u32 i, u32 j, FaceDir dir);

	/** Creates the front (0) or back (1) quad.
	 * @note These are the only faces generated for the overlay too.
	 */
	void addLargeFace(int id);

public:
	explicit Extruder(video::ITexture *_texture);
	Extruder(const Extruder &) = delete;
	Extruder(Extruder &&) = delete;
	~Extruder();
	Extruder &operator=(const Extruder &) = delete;
	Extruder &operator=(Extruder &&) = delete;

	/** Does the actual work.
	 *
	 * @note This function treats each pixel as either transparent or opaque,
	 * according to the #threshold.
	 *
	 * This function creates the mesh, consisting of two "large" quads facing front
	 * and back, and many "long" narrow faces facing all of #FaceDir. Opaque pixels
	 * of long faces form the mesh edge. As a long face spans the whole image
	 * in one direction, it has only one coordinate.
	 *
	 * Algorithm:
	 *
	 * 1. Scan the texture top-to-bottom. For each row, scan it, left-to-right.
	 * If there is a transparent-to-opaque transition, mark the corresponding
	 * face (left-facing) for creation. If there is an opaque-to-transparent
	 * transition, mark the right-facing one for creation.
	 *
	 * 2. The same for vertical faces (left-to-right, then top-to-bottom).
	 *
	 * 3. Create front and back faces using #addLargeFace.
	 *
	 * 4. Now actually create all marked faces using #addLongFace.
	 */
	void extrude();

	/** Returns the generated mesh.
	 * @note After calling this, the Extruder is no more usable.
	 */
	ExtrudedMesh takeMesh() noexcept;
};

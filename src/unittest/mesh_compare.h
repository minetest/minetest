/*
Minetest
Copyright (C) 2023 Vitaliy Lobachevskiy

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
#include <irrlichttypes.h>
#include <S3DVertex.h>

/// Represents a triangle as three vertices.
/// “Smallest” (according to <) vertex is expected to be first, others should follow in the counter-clockwise order.
using Triangle = std::array<video::S3DVertex, 3>;

/// Represents a quad as four vertices.
/// Vertices should be in the counter-clockwise order.
using Quad = std::array<video::S3DVertex, 4>;

/// Compare two meshes for equality.
/// @param vertices Vertices of the first mesh. Order doesn’t matter.
/// @param indices Indices of the first mesh. Triangle order doesn’t matter. Vertex order in a triangle only matters for winding.
/// @param expected The second mesh, in an expanded form. Must be sorted.
/// @returns Whether the two meshes are equal.
[[nodiscard]] bool checkMeshEqual(const std::vector<video::S3DVertex> &vertices, const std::vector<u16> &indices, const std::vector<Triangle> &expected);

/// Compare two meshes for equality.
/// @param vertices Vertices of the first mesh. Order doesn’t matter.
/// @param indices Indices of the first mesh. Triangle order doesn’t matter. Vertex order in a triangle only matters for winding.
/// @param expected The second mesh, in a quad form.
/// @returns Whether the two meshes are equal.
/// @note There are two ways to split a quad into 2 triangles; either is allowed.
[[nodiscard]] bool checkMeshEqual(const std::vector<video::S3DVertex> &vertices, const std::vector<u16> &indices, const std::vector<Quad> &expected);

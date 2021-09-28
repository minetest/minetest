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

#include "irrlichttypes_extrabloated.h"
#include "nodedef.h"

/*!
 * Applies shading to a color based on the surface's
 * normal vector.
 */
void applyFacesShading(video::SColor &color, const v3f &normal);

/*
	Create a new cube mesh.
	Vertices are at (+-scale.X/2, +-scale.Y/2, +-scale.Z/2).

	The resulting mesh has 6 materials (up, down, right, left, back, front)
	which must be defined by the caller.
*/
scene::IAnimatedMesh* createCubeMesh(v3f scale);

/*
	Multiplies each vertex coordinate by the specified scaling factors
	(componentwise vector multiplication).
*/
void scaleMesh(scene::IMesh *mesh, v3f scale);

/*
	Translate each vertex coordinate by the specified vector.
*/
void translateMesh(scene::IMesh *mesh, v3f vec);

/*!
 * Sets a constant color for all vertices in the mesh buffer.
 */
void setMeshBufferColor(scene::IMeshBuffer *buf, const video::SColor &color);

/*
	Set a constant color for all vertices in the mesh
*/
void setMeshColor(scene::IMesh *mesh, const video::SColor &color);


/*
	Sets texture coords for vertices in the mesh buffer.
	`uv[]` must have `count` elements
*/
void setMeshBufferTextureCoords(scene::IMeshBuffer *buf, const v2f *uv, u32 count);

/*
	Set a constant color for an animated mesh
*/
void setAnimatedMeshColor(scene::IAnimatedMeshSceneNode *node, const video::SColor &color);

/*!
 * Overwrites the color of a mesh buffer.
 * The color is darkened based on the normal vector of the vertices.
 */
void colorizeMeshBuffer(scene::IMeshBuffer *buf, const video::SColor *buffercolor);

/*
	Set the color of all vertices in the mesh.
	For each vertex, determine the largest absolute entry in
	the normal vector, and choose one of colorX, colorY or
	colorZ accordingly.
*/
void setMeshColorByNormalXYZ(scene::IMesh *mesh,
		const video::SColor &colorX,
		const video::SColor &colorY,
		const video::SColor &colorZ);

void setMeshColorByNormal(scene::IMesh *mesh, const v3f &normal,
		const video::SColor &color);

/*
	Rotate the mesh by 6d facedir value.
	Method only for meshnodes, not suitable for entities.
*/
void rotateMeshBy6dFacedir(scene::IMesh *mesh, int facedir);

/*
	Rotate the mesh around the axis and given angle in degrees.
*/
void rotateMeshXYby (scene::IMesh *mesh, f64 degrees);
void rotateMeshXZby (scene::IMesh *mesh, f64 degrees);
void rotateMeshYZby (scene::IMesh *mesh, f64 degrees);

/*
 *  Clone the mesh buffer.
 *  The returned pointer should be dropped.
 */
scene::IMeshBuffer* cloneMeshBuffer(scene::IMeshBuffer *mesh_buffer);

/*
	Clone the mesh.
*/
scene::SMesh* cloneMesh(scene::IMesh *src_mesh);

/*
	Convert nodeboxes to mesh. Each tile goes into a different buffer.
	boxes - set of nodeboxes to be converted into cuboids
	uv_coords[24] - table of texture uv coords for each cuboid face
	expand - factor by which cuboids will be resized
*/
scene::IMesh* convertNodeboxesToMesh(const std::vector<aabb3f> &boxes,
		const f32 *uv_coords = NULL, float expand = 0);

/*
	Update bounding box for a mesh.
*/
void recalculateBoundingBox(scene::IMesh *src_mesh);

/*
	Check if mesh has valid normals and return true if it does.
	We assume normal to be valid when it's 0 < length < Inf. and not NaN
 */
bool checkMeshNormals(scene::IMesh *mesh);

/*
	Vertex cache optimization according to the Forsyth paper:
	http://home.comcast.net/~tom_forsyth/papers/fast_vert_cache_opt.html
	Ported from irrlicht 1.8
*/
scene::IMesh* createForsythOptimizedMesh(const scene::IMesh *mesh);

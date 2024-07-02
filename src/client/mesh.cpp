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

#include "mesh.h"
#include "debug.h"
#include "log.h"
#include <cmath>
#include <iostream>
#include <IAnimatedMesh.h>
#include <SAnimatedMesh.h>
#include <IAnimatedMeshSceneNode.h>

inline static void applyShadeFactor(video::SColor& color, float factor)
{
	color.setRed(core::clamp(core::round32(color.getRed()*factor), 0, 255));
	color.setGreen(core::clamp(core::round32(color.getGreen()*factor), 0, 255));
	color.setBlue(core::clamp(core::round32(color.getBlue()*factor), 0, 255));
}

void applyFacesShading(video::SColor &color, const v3f &normal)
{
	/*
		Some drawtypes have normals set to (0, 0, 0), this must result in
		maximum brightness: shade factor 1.0.
		Shade factors for aligned cube faces are:
		+Y 1.000000 sqrt(1.0)
		-Y 0.447213 sqrt(0.2)
		+-X 0.670820 sqrt(0.45)
		+-Z 0.836660 sqrt(0.7)
	*/
	float x2 = normal.X * normal.X;
	float y2 = normal.Y * normal.Y;
	float z2 = normal.Z * normal.Z;
	if (normal.Y < 0)
		applyShadeFactor(color, 0.670820f * x2 + 0.447213f * y2 + 0.836660f * z2);
	else if ((x2 > 1e-3) || (z2 > 1e-3))
		applyShadeFactor(color, 0.670820f * x2 + 1.000000f * y2 + 0.836660f * z2);
}

template<typename T>
std::array<T, 24> getCubeVertices(video::SColor c)
{

	return std::array<T, 24>{
		// Up
		T(-0.5,+0.5,-0.5, 0,1,0, c, 0,1),
		T(-0.5,+0.5,+0.5, 0,1,0, c, 0,0),
		T(+0.5,+0.5,+0.5, 0,1,0, c, 1,0),
		T(+0.5,+0.5,-0.5, 0,1,0, c, 1,1),
		// Down
		T(-0.5,-0.5,-0.5, 0,-1,0, c, 0,0),
		T(+0.5,-0.5,-0.5, 0,-1,0, c, 1,0),
		T(+0.5,-0.5,+0.5, 0,-1,0, c, 1,1),
		T(-0.5,-0.5,+0.5, 0,-1,0, c, 0,1),
		// Right
		T(+0.5,-0.5,-0.5, 1,0,0, c, 0,1),
		T(+0.5,+0.5,-0.5, 1,0,0, c, 0,0),
		T(+0.5,+0.5,+0.5, 1,0,0, c, 1,0),
		T(+0.5,-0.5,+0.5, 1,0,0, c, 1,1),
		// Left
		T(-0.5,-0.5,-0.5, -1,0,0, c, 1,1),
		T(-0.5,-0.5,+0.5, -1,0,0, c, 0,1),
		T(-0.5,+0.5,+0.5, -1,0,0, c, 0,0),
		T(-0.5,+0.5,-0.5, -1,0,0, c, 1,0),
		// Back
		T(-0.5,-0.5,+0.5, 0,0,1, c, 1,1),
		T(+0.5,-0.5,+0.5, 0,0,1, c, 0,1),
		T(+0.5,+0.5,+0.5, 0,0,1, c, 0,0),
		T(-0.5,+0.5,+0.5, 0,0,1, c, 1,0),
		// Front
		T(-0.5,-0.5,-0.5, 0,0,-1, c, 0,1),
		T(-0.5,+0.5,-0.5, 0,0,-1, c, 0,0),
		T(+0.5,+0.5,-0.5, 0,0,-1, c, 1,0),
		T(+0.5,-0.5,-0.5, 0,0,-1, c, 1,1),
	};
}

scene::IAnimatedMesh* createCubeMesh(v3f scale)
{
	video::SColor c(255,255,255,255);

	std::array<video::S3DVertex, 24> vertices = getCubeVertices<video::S3DVertex>(c);
	u16 indices[6] = {0,1,2,2,3,0};

	scene::SMesh *mesh = new scene::SMesh();
	for (u32 i=0; i<6; ++i)
	{
		scene::IMeshBuffer *buf = new scene::SMeshBuffer();
		buf->append(&vertices[0] + 4 * i, 4, indices, 6);
		// Set default material
		buf->getMaterial().Lighting = false;
		buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
		buf->getMaterial().forEachTexture([] (auto &tex) {
			tex.MinFilter = video::ETMINF_NEAREST_MIPMAP_NEAREST;
			tex.MagFilter = video::ETMAGF_NEAREST;
		});
		// Add mesh buffer to mesh
		mesh->addMeshBuffer(buf);
		buf->drop();
	}

	scene::SAnimatedMesh *anim_mesh = new scene::SAnimatedMesh(mesh);
	mesh->drop();
	scaleMesh(anim_mesh, scale);  // also recalculates bounding box
	return anim_mesh;
}

scene::IAnimatedMesh* createCubeMesh2Colors(v3f scale)
{
	video::SColor c(255,255,255,255);

	std::array<video::S3DVertex2Colors, 24> vertices = getCubeVertices<video::S3DVertex2Colors>(c);
	u16 indices[6] = {0,1,2,2,3,0};

	scene::SMesh *mesh = new scene::SMesh();
	for (u32 i=0; i<6; ++i)
	{
		scene::IMeshBuffer *buf = new scene::SMeshBuffer2Colors();
		buf->append(&vertices[0] + 4 * i, 4, indices, 6);
		// Set default material
		buf->getMaterial().Lighting = false;
		buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF_2COLORS;
		buf->getMaterial().forEachTexture([] (auto &tex) {
			tex.MinFilter = video::ETMINF_NEAREST_MIPMAP_NEAREST;
			tex.MagFilter = video::ETMAGF_NEAREST;
		});
		// Add mesh buffer to mesh
		mesh->addMeshBuffer(buf);
		buf->drop();
	}

	scene::SAnimatedMesh *anim_mesh = new scene::SAnimatedMesh(mesh);
	mesh->drop();
	scaleMesh(anim_mesh, scale);  // also recalculates bounding box
	return anim_mesh;
}

void scaleMesh(scene::IMesh *mesh, v3f scale)
{
	if (!mesh)
		return;

	aabb3f bbox;
	bbox.reset(0, 0, 0);

	for (u32 j = 0; j < mesh->getMeshBufferCount(); j++) {
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);

		if (buf->getVertexType() == video::EVT_2COLORS) {
			video::S3DVertex2Colors *vertices = static_cast<video::S3DVertex2Colors *>(buf->getVertices());

			for (u32 i = 0; i < buf->getVertexCount(); i++)
				vertices[i].Pos *= scale;
		}
		else {
			video::S3DVertex *vertices = static_cast<video::S3DVertex *>(buf->getVertices());

			for (u32 i = 0; i < buf->getVertexCount(); i++)
				vertices[i].Pos *= scale;
		}

		buf->recalculateBoundingBox();

		// calculate total bounding box
		if (j == 0)
			bbox = buf->getBoundingBox();
		else
			bbox.addInternalBox(buf->getBoundingBox());
	}
	mesh->setBoundingBox(bbox);
}

/*void scaleMesh2Colors(scene::IMesh *mesh, v3f scale)
{
	if (!mesh)
		return;

	aabb3f bbox;
	bbox.reset(0, 0, 0);

	for (u32 j = 0; j < mesh->getMeshBufferCount(); j++) {
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
		video::S3DVertex2Colors *vertices = static_cast<video::S3DVertex2Colors*>(buf->getVertices());
		for (u32 i = 0; i < buf->getVertexCount(); i++)
			vertices[i].Pos *= scale;

		buf->recalculateBoundingBox();

		// calculate total bounding box
		if (j == 0)
			bbox = buf->getBoundingBox();
		else
			bbox.addInternalBox(buf->getBoundingBox());
	}
	mesh->setBoundingBox(bbox);
}*/

void translateMesh(scene::IMesh *mesh, v3f vec)
{
	if (!mesh)
		return;

	aabb3f bbox;
	bbox.reset(0, 0, 0);

	for (u32 j = 0; j < mesh->getMeshBufferCount(); j++) {
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);

		if (buf->getVertexType() == video::EVT_2COLORS) {
			video::S3DVertex2Colors *vertices = static_cast<video::S3DVertex2Colors *>(buf->getVertices());

			for (u32 i = 0; i < buf->getVertexCount(); i++)
				vertices[i].Pos += vec;
		}
		else {
			video::S3DVertex *vertices = static_cast<video::S3DVertex *>(buf->getVertices());

			for (u32 i = 0; i < buf->getVertexCount(); i++)
				vertices[i].Pos += vec;
		}

		buf->recalculateBoundingBox();

		// calculate total bounding box
		if (j == 0)
			bbox = buf->getBoundingBox();
		else
			bbox.addInternalBox(buf->getBoundingBox());
	}
	mesh->setBoundingBox(bbox);
}

void setMeshBufferColor(scene::IMeshBuffer *buf, const video::SColor &color)
{
	video::S3DVertex *vertices = static_cast<video::S3DVertex *>(buf->getVertices());
	for (u32 i = 0; i < buf->getVertexCount(); i++)
		vertices[i].Color = color;
}

void setMeshBuffer2Colors(scene::IMeshBuffer *buf,
	const video::SColor &color, const core::vector3df &color2)
{
	video::S3DVertex2Colors *vertices = static_cast<video::S3DVertex2Colors *>(buf->getVertices());
	for (u32 i = 0; i < buf->getVertexCount(); i++) {
		vertices[i].Color = color;
		vertices[i].Color2 = color2;
	}
}

void setAnimatedMeshColor(scene::IAnimatedMeshSceneNode *node, const video::SColor &color)
{
	for (u32 i = 0; i < node->getMaterialCount(); ++i) {
		node->getMaterial(i).EmissiveColor = color;
	}
}

void setMeshColor(scene::IMesh *mesh, const video::SColor &color)
{
	if (!mesh)
		return;

	for (u32 j = 0; j < mesh->getMeshBufferCount(); j++)
		setMeshBufferColor(mesh->getMeshBuffer(j), color);
}

void setMeshBufferTextureCoords(scene::IMeshBuffer *buf, const v2f *uv, u32 count)
{
	assert(buf->getVertexCount() >= count);
	video::S3DVertex *vertices = static_cast<video::S3DVertex *>(buf->getVertices());
	for (u32 i = 0; i < count; i++)
		vertices[i].TCoords = uv[i];
}

template <typename T, typename F>
static void applyToMesh(scene::IMesh *mesh, const F &fn)
{
	for (u16 j = 0; j < mesh->getMeshBufferCount(); j++) {
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);

		T *vertices = static_cast<T *>(buf->getVertices());

		for (u32 i = 0; i < buf->getVertexCount(); i++)
			fn(vertices[i]);
	}
}

void colorizeMeshBuffer2Colors(scene::IMeshBuffer *buf,
	const video::SColor &color, const core::vector3df &color2)
{
	video::S3DVertex2Colors *vertices = static_cast<video::S3DVertex2Colors *>(buf->getVertices());
	for (u32 i = 0; i < buf->getVertexCount(); i++) {
		// Reset color
		vertices[i].Color = color;
		vertices[i].Color2 = color2;
		// Apply shading
		applyFacesShading(vertices[i].Color, vertices[i].Normal);
	}
}

void setMeshColorByNormal(scene::IMesh *mesh, const v3f &normal,
		const video::SColor &color)
{
	if (!mesh)
		return;
	auto colorizator = [normal, color] (video::S3DVertex &vertex) {
		if (vertex.Normal == normal)
			vertex.Color = color;
	};
	applyToMesh<video::S3DVertex>(mesh, colorizator);
}

template <typename T, float v3f::*U, float v3f::*V>
static void rotateMesh(scene::IMesh *mesh, float degrees)
{
	degrees *= M_PI / 180.0f;
	float c = std::cos(degrees);
	float s = std::sin(degrees);

	auto rotator = [c, s] (T &vertex) {
		float u = vertex.Pos.*U;
		float v = vertex.Pos.*V;
		vertex.Pos.*U = c * u - s * v;
		vertex.Pos.*V = s * u + c * v;
	};
	applyToMesh<T>(mesh, rotator);
}

void rotateMeshXYby(scene::IMesh *mesh, f64 degrees)
{
	if (mesh->getMeshBuffer(0)->getVertexType() == video::EVT_2COLORS)
		rotateMesh<video::S3DVertex2Colors, &v3f::X, &v3f::Y>(mesh, degrees);
	else
		rotateMesh<video::S3DVertex, &v3f::X, &v3f::Y>(mesh, degrees);
}

void rotateMeshXZby(scene::IMesh *mesh, f64 degrees)
{
	if (mesh->getMeshBuffer(0)->getVertexType() == video::EVT_2COLORS)
		rotateMesh<video::S3DVertex2Colors, &v3f::X, &v3f::Z>(mesh, degrees);
	else
		rotateMesh<video::S3DVertex, &v3f::X, &v3f::Z>(mesh, degrees);
}

void rotateMeshYZby(scene::IMesh *mesh, f64 degrees)
{
	if (mesh->getMeshBuffer(0)->getVertexType() == video::EVT_2COLORS)
		rotateMesh<video::S3DVertex2Colors, &v3f::Y, &v3f::Z>(mesh, degrees);
	else
		rotateMesh<video::S3DVertex, &v3f::Y, &v3f::Z>(mesh, degrees);
}

void rotateMeshBy6dFacedir(scene::IMesh *mesh, int facedir)
{
	int axisdir = facedir >> 2;
	facedir &= 0x03;
	switch (facedir) {
		case 1: rotateMeshXZby(mesh, -90); break;
		case 2: rotateMeshXZby(mesh, 180); break;
		case 3: rotateMeshXZby(mesh, 90); break;
	}
	switch (axisdir) {
		case 1: rotateMeshYZby(mesh, 90); break; // z+
		case 2: rotateMeshYZby(mesh, -90); break; // z-
		case 3: rotateMeshXYby(mesh, -90); break; // x+
		case 4: rotateMeshXYby(mesh, 90); break; // x-
		case 5: rotateMeshXYby(mesh, -180); break;
	}
}

void recalculateBoundingBox(scene::IMesh *src_mesh)
{
	aabb3f bbox;
	bbox.reset(0,0,0);
	for (u16 j = 0; j < src_mesh->getMeshBufferCount(); j++) {
		scene::IMeshBuffer *buf = src_mesh->getMeshBuffer(j);
		buf->recalculateBoundingBox();
		if (j == 0)
			bbox = buf->getBoundingBox();
		else
			bbox.addInternalBox(buf->getBoundingBox());
	}
	src_mesh->setBoundingBox(bbox);
}

bool checkMeshNormals(scene::IMesh *mesh)
{
	u32 buffer_count = mesh->getMeshBufferCount();

	for (u32 i = 0; i < buffer_count; i++) {
		scene::IMeshBuffer *buffer = mesh->getMeshBuffer(i);
		if (!buffer->getVertexCount())
			continue;

		// Here we intentionally check only first normal, assuming that if buffer
		// has it valid, then most likely all other ones are fine too. We can
		// check all of the normals to have length, but it seems like an overkill
		// hurting the performance and covering only really weird broken models.
		f32 length = buffer->getNormal(0).getLength();

		if (!std::isfinite(length) || length < 1e-10f)
			return false;
	}

	return true;
}

scene::SMesh *convertMeshTo2Colors(scene::IMesh *mesh)
{
	scene::SMesh *new_mesh = new scene::SMesh();

	for (u32 i = 0; i < mesh->getMeshBufferCount(); i++) {
		scene::IMeshBuffer *mb = mesh->getMeshBuffer(i);
		video::S3DVertex *v = static_cast<video::S3DVertex *>(mb->getVertices());

		scene::SMeshBuffer2Colors *new_mb = new scene::SMeshBuffer2Colors();
		core::array<video::S3DVertex2Colors> new_vertices;

		for (u32 j = 0; j < mb->getVertexCount(); j++)
			new_vertices.push_back(video::S3DVertex2Colors(v[j].Pos, v[j].Normal, v[j].Color, v[j].TCoords));

		new_mb->append(new_vertices.pointer(), new_vertices.size(), mb->getIndices(), mb->getIndexCount());
		new_mesh->addMeshBuffer(new_mb);
		new_mb->drop();
	}

	return new_mesh;
}

scene::IMeshBuffer* cloneMeshBuffer(scene::IMeshBuffer *mesh_buffer)
{
	switch (mesh_buffer->getVertexType()) {
	case video::EVT_STANDARD: {
		video::S3DVertex *v = static_cast<video::S3DVertex *>(mesh_buffer->getVertices());
		scene::SMeshBuffer *cloned_buffer = new scene::SMeshBuffer();
		cloned_buffer->append(v, mesh_buffer->getVertexCount(), mesh_buffer->getIndices(),
			mesh_buffer->getIndexCount());
		return cloned_buffer;
	}
	case video::EVT_2TCOORDS: {
		video::S3DVertex2TCoords *v =
			static_cast<video::S3DVertex2TCoords *>(mesh_buffer->getVertices());
		scene::SMeshBufferLightMap *cloned_buffer =
			new scene::SMeshBufferLightMap();
		cloned_buffer->append(v, mesh_buffer->getVertexCount(), mesh_buffer->getIndices(),
			mesh_buffer->getIndexCount());
		return cloned_buffer;
	}
	case video::EVT_TANGENTS: {
		video::S3DVertexTangents *v =
			static_cast<video::S3DVertexTangents *>(mesh_buffer->getVertices());
		scene::SMeshBufferTangents *cloned_buffer =
			new scene::SMeshBufferTangents();
		cloned_buffer->append(v, mesh_buffer->getVertexCount(), mesh_buffer->getIndices(),
			mesh_buffer->getIndexCount());
		return cloned_buffer;
	}
	case video::EVT_2COLORS: {
		video::S3DVertex2Colors *v =
			static_cast<video::S3DVertex2Colors *>(mesh_buffer->getVertices());
		scene::SMeshBuffer2Colors *cloned_buffer =
			new scene::SMeshBuffer2Colors();
		cloned_buffer->append(v, mesh_buffer->getVertexCount(), mesh_buffer->getIndices(),
			mesh_buffer->getIndexCount());
		return cloned_buffer;
	}
	}
	// This should not happen.
	sanity_check(false);
	return NULL;
}

scene::SMesh* cloneMesh(scene::IMesh *src_mesh)
{
	scene::SMesh* dst_mesh = new scene::SMesh();
	for (u16 j = 0; j < src_mesh->getMeshBufferCount(); j++) {
		scene::IMeshBuffer *temp_buf = cloneMeshBuffer(
			src_mesh->getMeshBuffer(j));
		dst_mesh->addMeshBuffer(temp_buf);
		temp_buf->drop();

	}
	return dst_mesh;
}

scene::IMesh* convertNodeboxesToMesh(const std::vector<aabb3f> &boxes,
		const f32 *uv_coords, float expand)
{
	scene::SMesh* dst_mesh = new scene::SMesh();

	for (u16 j = 0; j < 6; j++)
	{
		scene::IMeshBuffer *buf = new scene::SMeshBuffer();
		buf->getMaterial().Lighting = false;
		buf->getMaterial().forEachTexture([] (auto &tex) {
			tex.MinFilter = video::ETMINF_NEAREST_MIPMAP_NEAREST;
			tex.MagFilter = video::ETMAGF_NEAREST;
		});
		dst_mesh->addMeshBuffer(buf);
		buf->drop();
	}

	video::SColor c(255,255,255,255);

	for (aabb3f box : boxes) {
		box.repair();

		box.MinEdge.X -= expand;
		box.MinEdge.Y -= expand;
		box.MinEdge.Z -= expand;
		box.MaxEdge.X += expand;
		box.MaxEdge.Y += expand;
		box.MaxEdge.Z += expand;

		// Compute texture UV coords
		f32 tx1 = (box.MinEdge.X / BS) + 0.5;
		f32 ty1 = (box.MinEdge.Y / BS) + 0.5;
		f32 tz1 = (box.MinEdge.Z / BS) + 0.5;
		f32 tx2 = (box.MaxEdge.X / BS) + 0.5;
		f32 ty2 = (box.MaxEdge.Y / BS) + 0.5;
		f32 tz2 = (box.MaxEdge.Z / BS) + 0.5;

		f32 txc_default[24] = {
			// up
			tx1, 1 - tz2, tx2, 1 - tz1,
			// down
			tx1, tz1, tx2, tz2,
			// right
			tz1, 1 - ty2, tz2, 1 - ty1,
			// left
			1 - tz2, 1 - ty2, 1 - tz1, 1 - ty1,
			// back
			1 - tx2, 1 - ty2, 1 - tx1, 1 - ty1,
			// front
			tx1, 1 - ty2, tx2, 1 - ty1,
		};

		// use default texture UV mapping if not provided
		const f32 *txc = uv_coords ? uv_coords : txc_default;

		v3f min = box.MinEdge;
		v3f max = box.MaxEdge;

		video::S3DVertex vertices[24] =
		{
			// up
			video::S3DVertex(min.X,max.Y,max.Z, 0,1,0, c, txc[0],txc[1]),
			video::S3DVertex(max.X,max.Y,max.Z, 0,1,0, c, txc[2],txc[1]),
			video::S3DVertex(max.X,max.Y,min.Z, 0,1,0, c, txc[2],txc[3]),
			video::S3DVertex(min.X,max.Y,min.Z, 0,1,0, c, txc[0],txc[3]),
			// down
			video::S3DVertex(min.X,min.Y,min.Z, 0,-1,0, c, txc[4],txc[5]),
			video::S3DVertex(max.X,min.Y,min.Z, 0,-1,0, c, txc[6],txc[5]),
			video::S3DVertex(max.X,min.Y,max.Z, 0,-1,0, c, txc[6],txc[7]),
			video::S3DVertex(min.X,min.Y,max.Z, 0,-1,0, c, txc[4],txc[7]),
			// right
			video::S3DVertex(max.X,max.Y,min.Z, 1,0,0, c, txc[ 8],txc[9]),
			video::S3DVertex(max.X,max.Y,max.Z, 1,0,0, c, txc[10],txc[9]),
			video::S3DVertex(max.X,min.Y,max.Z, 1,0,0, c, txc[10],txc[11]),
			video::S3DVertex(max.X,min.Y,min.Z, 1,0,0, c, txc[ 8],txc[11]),
			// left
			video::S3DVertex(min.X,max.Y,max.Z, -1,0,0, c, txc[12],txc[13]),
			video::S3DVertex(min.X,max.Y,min.Z, -1,0,0, c, txc[14],txc[13]),
			video::S3DVertex(min.X,min.Y,min.Z, -1,0,0, c, txc[14],txc[15]),
			video::S3DVertex(min.X,min.Y,max.Z, -1,0,0, c, txc[12],txc[15]),
			// back
			video::S3DVertex(max.X,max.Y,max.Z, 0,0,1, c, txc[16],txc[17]),
			video::S3DVertex(min.X,max.Y,max.Z, 0,0,1, c, txc[18],txc[17]),
			video::S3DVertex(min.X,min.Y,max.Z, 0,0,1, c, txc[18],txc[19]),
			video::S3DVertex(max.X,min.Y,max.Z, 0,0,1, c, txc[16],txc[19]),
			// front
			video::S3DVertex(min.X,max.Y,min.Z, 0,0,-1, c, txc[20],txc[21]),
			video::S3DVertex(max.X,max.Y,min.Z, 0,0,-1, c, txc[22],txc[21]),
			video::S3DVertex(max.X,min.Y,min.Z, 0,0,-1, c, txc[22],txc[23]),
			video::S3DVertex(min.X,min.Y,min.Z, 0,0,-1, c, txc[20],txc[23]),
		};

		u16 indices[] = {0,1,2,2,3,0};

		for(u16 j = 0; j < 24; j += 4)
		{
			scene::IMeshBuffer *buf = dst_mesh->getMeshBuffer(j / 4);
			buf->append(vertices + j, 4, indices, 6);
		}
	}
	return dst_mesh;
}

void setMaterialFilters(video::SMaterialLayer &tex, bool bilinear, bool trilinear, bool anisotropic)
{
	if (trilinear)
		tex.MinFilter = video::ETMINF_LINEAR_MIPMAP_LINEAR;
	else if (bilinear)
		tex.MinFilter = video::ETMINF_LINEAR_MIPMAP_NEAREST;
	else
		tex.MinFilter = video::ETMINF_NEAREST_MIPMAP_NEAREST;

	tex.MagFilter = (trilinear || bilinear) ? video::ETMAGF_LINEAR : video::ETMAGF_NEAREST;

	tex.AnisotropicFilter = anisotropic ? 0xFF : 0;
}

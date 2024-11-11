// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CMeshManipulator.h"
#include "ISkinnedMesh.h"
#include "SMesh.h"
#include "CMeshBuffer.h"
#include "SAnimatedMesh.h"
#include "os.h"

namespace irr
{
namespace scene
{

static inline core::vector3df getAngleWeight(const core::vector3df &v1,
		const core::vector3df &v2,
		const core::vector3df &v3)
{
	// Calculate this triangle's weight for each of its three vertices
	// start by calculating the lengths of its sides
	const f32 a = v2.getDistanceFromSQ(v3);
	const f32 asqrt = sqrtf(a);
	const f32 b = v1.getDistanceFromSQ(v3);
	const f32 bsqrt = sqrtf(b);
	const f32 c = v1.getDistanceFromSQ(v2);
	const f32 csqrt = sqrtf(c);

	// use them to find the angle at each vertex
	return core::vector3df(
			acosf((b + c - a) / (2.f * bsqrt * csqrt)),
			acosf((-b + c + a) / (2.f * asqrt * csqrt)),
			acosf((b - c + a) / (2.f * bsqrt * asqrt)));
}

namespace
{
template <typename T>
void recalculateNormalsT(IMeshBuffer *buffer, bool smooth, bool angleWeighted)
{
	const u32 vtxcnt = buffer->getVertexCount();
	const u32 idxcnt = buffer->getIndexCount();
	const T *idx = reinterpret_cast<T *>(buffer->getIndices());

	if (!smooth) {
		for (u32 i = 0; i < idxcnt; i += 3) {
			const core::vector3df &v1 = buffer->getPosition(idx[i + 0]);
			const core::vector3df &v2 = buffer->getPosition(idx[i + 1]);
			const core::vector3df &v3 = buffer->getPosition(idx[i + 2]);
			const core::vector3df normal = core::plane3d<f32>(v1, v2, v3).Normal;
			buffer->getNormal(idx[i + 0]) = normal;
			buffer->getNormal(idx[i + 1]) = normal;
			buffer->getNormal(idx[i + 2]) = normal;
		}
	} else {
		u32 i;

		for (i = 0; i != vtxcnt; ++i)
			buffer->getNormal(i).set(0.f, 0.f, 0.f);

		for (i = 0; i < idxcnt; i += 3) {
			const core::vector3df &v1 = buffer->getPosition(idx[i + 0]);
			const core::vector3df &v2 = buffer->getPosition(idx[i + 1]);
			const core::vector3df &v3 = buffer->getPosition(idx[i + 2]);
			const core::vector3df normal = core::plane3d<f32>(v1, v2, v3).Normal;

			core::vector3df weight(1.f, 1.f, 1.f);
			if (angleWeighted)
				weight = getAngleWeight(v1, v2, v3);

			buffer->getNormal(idx[i + 0]) += weight.X * normal;
			buffer->getNormal(idx[i + 1]) += weight.Y * normal;
			buffer->getNormal(idx[i + 2]) += weight.Z * normal;
		}

		for (i = 0; i != vtxcnt; ++i)
			buffer->getNormal(i).normalize();
	}
}
}

namespace
{
	void calculateTangents(
		core::vector3df& normal,
		core::vector3df& tangent,
		core::vector3df& binormal,
		const core::vector3df& vt1, const core::vector3df& vt2, const core::vector3df& vt3, // vertices
		const core::vector2df& tc1, const core::vector2df& tc2, const core::vector2df& tc3) // texture coords
	{
		// choose one of them:
		//#define USE_NVIDIA_GLH_VERSION // use version used by nvidia in glh headers
#define USE_IRR_VERSION

#ifdef USE_IRR_VERSION

		core::vector3df v1 = vt1 - vt2;
		core::vector3df v2 = vt3 - vt1;
		normal = v2.crossProduct(v1);
		normal.normalize();

		// binormal

		f32 deltaX1 = tc1.X - tc2.X;
		f32 deltaX2 = tc3.X - tc1.X;
		binormal = (v1 * deltaX2) - (v2 * deltaX1);
		binormal.normalize();

		// tangent

		f32 deltaY1 = tc1.Y - tc2.Y;
		f32 deltaY2 = tc3.Y - tc1.Y;
		tangent = (v1 * deltaY2) - (v2 * deltaY1);
		tangent.normalize();

		// adjust

		core::vector3df txb = tangent.crossProduct(binormal);
		if (txb.dotProduct(normal) < 0.0f)
		{
			tangent *= -1.0f;
			binormal *= -1.0f;
		}

#endif // USE_IRR_VERSION

#ifdef USE_NVIDIA_GLH_VERSION

		tangent.set(0, 0, 0);
		binormal.set(0, 0, 0);

		core::vector3df v1(vt2.X - vt1.X, tc2.X - tc1.X, tc2.Y - tc1.Y);
		core::vector3df v2(vt3.X - vt1.X, tc3.X - tc1.X, tc3.Y - tc1.Y);

		core::vector3df txb = v1.crossProduct(v2);
		if (!core::iszero(txb.X))
		{
			tangent.X = -txb.Y / txb.X;
			binormal.X = -txb.Z / txb.X;
		}

		v1.X = vt2.Y - vt1.Y;
		v2.X = vt3.Y - vt1.Y;
		txb = v1.crossProduct(v2);

		if (!core::iszero(txb.X))
		{
			tangent.Y = -txb.Y / txb.X;
			binormal.Y = -txb.Z / txb.X;
		}

		v1.X = vt2.Z - vt1.Z;
		v2.X = vt3.Z - vt1.Z;
		txb = v1.crossProduct(v2);

		if (!core::iszero(txb.X))
		{
			tangent.Z = -txb.Y / txb.X;
			binormal.Z = -txb.Z / txb.X;
		}

		tangent.normalize();
		binormal.normalize();

		normal = tangent.crossProduct(binormal);
		normal.normalize();

		binormal = tangent.crossProduct(normal);
		binormal.normalize();

		core::plane3d<f32> pl(vt1, vt2, vt3);

		if (normal.dotProduct(pl.Normal) < 0.0f)
			normal *= -1.0f;

#endif // USE_NVIDIA_GLH_VERSION
	}


	//! Recalculates tangents for a tangent mesh buffer
	template <typename T>
	void recalculateTangentsT(IMeshBuffer* buffer, bool recalculateNormals, bool smooth, bool angleWeighted)
	{
		if (!buffer || (buffer->getVertexType() != video::EVT_TANGENTS))
			return;

		const u32 vtxCnt = buffer->getVertexCount();
		const u32 idxCnt = buffer->getIndexCount();

		T* idx = reinterpret_cast<T*>(buffer->getIndices());
		video::S3DVertexTangents* v =
			(video::S3DVertexTangents*)buffer->getVertices();

		if (smooth)
		{
			u32 i;

			for (i = 0; i != vtxCnt; ++i)
			{
				if (recalculateNormals)
					v[i].Normal.set(0.f, 0.f, 0.f);
				v[i].Tangent.set(0.f, 0.f, 0.f);
				v[i].Binormal.set(0.f, 0.f, 0.f);
			}

			//Each vertex gets the sum of the tangents and binormals from the faces around it
			for (i = 0; i < idxCnt; i += 3)
			{
				// if this triangle is degenerate, skip it!
				if (v[idx[i + 0]].Pos == v[idx[i + 1]].Pos ||
					v[idx[i + 0]].Pos == v[idx[i + 2]].Pos ||
					v[idx[i + 1]].Pos == v[idx[i + 2]].Pos
					/*||
					v[idx[i+0]].TCoords == v[idx[i+1]].TCoords ||
					v[idx[i+0]].TCoords == v[idx[i+2]].TCoords ||
					v[idx[i+1]].TCoords == v[idx[i+2]].TCoords */
					)
					continue;

				//Angle-weighted normals look better, but are slightly more CPU intensive to calculate
				core::vector3df weight(1.f, 1.f, 1.f);
				if (angleWeighted)
					weight = irr::scene::getAngleWeight(v[i + 0].Pos, v[i + 1].Pos, v[i + 2].Pos);	// writing irr::scene:: necessary for borland
				core::vector3df localNormal;
				core::vector3df localTangent;
				core::vector3df localBinormal;

				calculateTangents(
					localNormal,
					localTangent,
					localBinormal,
					v[idx[i + 0]].Pos,
					v[idx[i + 1]].Pos,
					v[idx[i + 2]].Pos,
					v[idx[i + 0]].TCoords,
					v[idx[i + 1]].TCoords,
					v[idx[i + 2]].TCoords);

				if (recalculateNormals)
					v[idx[i + 0]].Normal += localNormal * weight.X;
				v[idx[i + 0]].Tangent += localTangent * weight.X;
				v[idx[i + 0]].Binormal += localBinormal * weight.X;

				calculateTangents(
					localNormal,
					localTangent,
					localBinormal,
					v[idx[i + 1]].Pos,
					v[idx[i + 2]].Pos,
					v[idx[i + 0]].Pos,
					v[idx[i + 1]].TCoords,
					v[idx[i + 2]].TCoords,
					v[idx[i + 0]].TCoords);

				if (recalculateNormals)
					v[idx[i + 1]].Normal += localNormal * weight.Y;
				v[idx[i + 1]].Tangent += localTangent * weight.Y;
				v[idx[i + 1]].Binormal += localBinormal * weight.Y;

				calculateTangents(
					localNormal,
					localTangent,
					localBinormal,
					v[idx[i + 2]].Pos,
					v[idx[i + 0]].Pos,
					v[idx[i + 1]].Pos,
					v[idx[i + 2]].TCoords,
					v[idx[i + 0]].TCoords,
					v[idx[i + 1]].TCoords);

				if (recalculateNormals)
					v[idx[i + 2]].Normal += localNormal * weight.Z;
				v[idx[i + 2]].Tangent += localTangent * weight.Z;
				v[idx[i + 2]].Binormal += localBinormal * weight.Z;
			}

			// Normalize the tangents and binormals
			if (recalculateNormals)
			{
				for (i = 0; i != vtxCnt; ++i)
					v[i].Normal.normalize();
			}
			for (i = 0; i != vtxCnt; ++i)
			{
				v[i].Tangent.normalize();
				v[i].Binormal.normalize();
			}
		}
		else
		{
			core::vector3df localNormal;
			for (u32 i = 0; i < idxCnt; i += 3)
			{
				calculateTangents(
					localNormal,
					v[idx[i + 0]].Tangent,
					v[idx[i + 0]].Binormal,
					v[idx[i + 0]].Pos,
					v[idx[i + 1]].Pos,
					v[idx[i + 2]].Pos,
					v[idx[i + 0]].TCoords,
					v[idx[i + 1]].TCoords,
					v[idx[i + 2]].TCoords);
				if (recalculateNormals)
					v[idx[i + 0]].Normal = localNormal;

				calculateTangents(
					localNormal,
					v[idx[i + 1]].Tangent,
					v[idx[i + 1]].Binormal,
					v[idx[i + 1]].Pos,
					v[idx[i + 2]].Pos,
					v[idx[i + 0]].Pos,
					v[idx[i + 1]].TCoords,
					v[idx[i + 2]].TCoords,
					v[idx[i + 0]].TCoords);
				if (recalculateNormals)
					v[idx[i + 1]].Normal = localNormal;

				calculateTangents(
					localNormal,
					v[idx[i + 2]].Tangent,
					v[idx[i + 2]].Binormal,
					v[idx[i + 2]].Pos,
					v[idx[i + 0]].Pos,
					v[idx[i + 1]].Pos,
					v[idx[i + 2]].TCoords,
					v[idx[i + 0]].TCoords,
					v[idx[i + 1]].TCoords);
				if (recalculateNormals)
					v[idx[i + 2]].Normal = localNormal;
			}
		}
	}
}


//! Recalculates tangents for a tangent mesh buffer
void CMeshManipulator::recalculateTangents(IMeshBuffer* buffer, bool recalculateNormals, bool smooth, bool angleWeighted) const
{
	if (buffer && (buffer->getVertexType() == video::EVT_TANGENTS))
	{
		if (buffer->getIndexType() == video::EIT_16BIT)
			recalculateTangentsT<u16>(buffer, recalculateNormals, smooth, angleWeighted);
		else
			recalculateTangentsT<u32>(buffer, recalculateNormals, smooth, angleWeighted);
	}
}


//! Recalculates tangents for all tangent mesh buffers
void CMeshManipulator::recalculateTangents(IMesh* mesh, bool recalculateNormals, bool smooth, bool angleWeighted) const
{
	if (!mesh)
		return;

	const u32 meshBufferCount = mesh->getMeshBufferCount();
	for (u32 b = 0; b < meshBufferCount; ++b)
	{
		recalculateTangents(mesh->getMeshBuffer(b), recalculateNormals, smooth, angleWeighted);
	}
}


namespace
{
	//! Creates a planar texture mapping on the meshbuffer
	template<typename T>
	void makePlanarTextureMappingT(scene::IMeshBuffer* buffer, f32 resolution)
	{
		u32 idxcnt = buffer->getIndexCount();
		T* idx = reinterpret_cast<T*>(buffer->getIndices());

		for (u32 i = 0; i < idxcnt; i += 3)
		{
			core::plane3df p(buffer->getPosition(idx[i + 0]), buffer->getPosition(idx[i + 1]), buffer->getPosition(idx[i + 2]));
			p.Normal.X = fabsf(p.Normal.X);
			p.Normal.Y = fabsf(p.Normal.Y);
			p.Normal.Z = fabsf(p.Normal.Z);
			// calculate planar mapping worldspace coordinates

			if (p.Normal.X > p.Normal.Y && p.Normal.X > p.Normal.Z)
			{
				for (u32 o = 0; o != 3; ++o)
				{
					buffer->getTCoords(idx[i + o]).X = buffer->getPosition(idx[i + o]).Y * resolution;
					buffer->getTCoords(idx[i + o]).Y = buffer->getPosition(idx[i + o]).Z * resolution;
				}
			}
			else
				if (p.Normal.Y > p.Normal.X && p.Normal.Y > p.Normal.Z)
				{
					for (u32 o = 0; o != 3; ++o)
					{
						buffer->getTCoords(idx[i + o]).X = buffer->getPosition(idx[i + o]).X * resolution;
						buffer->getTCoords(idx[i + o]).Y = buffer->getPosition(idx[i + o]).Z * resolution;
					}
				}
				else
				{
					for (u32 o = 0; o != 3; ++o)
					{
						buffer->getTCoords(idx[i + o]).X = buffer->getPosition(idx[i + o]).X * resolution;
						buffer->getTCoords(idx[i + o]).Y = buffer->getPosition(idx[i + o]).Y * resolution;
					}
				}
		}
	}
}

//! Recalculates all normals of the mesh buffer.
/** \param buffer: Mesh buffer on which the operation is performed. */
void CMeshManipulator::recalculateNormals(IMeshBuffer *buffer, bool smooth, bool angleWeighted) const
{
	if (!buffer)
		return;

	if (buffer->getIndexType() == video::EIT_16BIT)
		recalculateNormalsT<u16>(buffer, smooth, angleWeighted);
	else
		recalculateNormalsT<u32>(buffer, smooth, angleWeighted);
}

//! Recalculates all normals of the mesh.
//! \param mesh: Mesh on which the operation is performed.
void CMeshManipulator::recalculateNormals(scene::IMesh *mesh, bool smooth, bool angleWeighted) const
{
	if (!mesh)
		return;

	if (mesh->getMeshType() == EAMT_SKINNED) {
		ISkinnedMesh *smesh = (ISkinnedMesh *)mesh;
		smesh->resetAnimation();
	}

	const u32 bcount = mesh->getMeshBufferCount();
	for (u32 b = 0; b < bcount; ++b)
		recalculateNormals(mesh->getMeshBuffer(b), smooth, angleWeighted);

	if (mesh->getMeshType() == EAMT_SKINNED) {
		ISkinnedMesh *smesh = (ISkinnedMesh *)mesh;
		smesh->refreshJointCache();
	}
}

template <typename T>
void copyVertices(const scene::IVertexBuffer *src, scene::CVertexBuffer<T> *dst)
{
	_IRR_DEBUG_BREAK_IF(T::getType() != src->getType());
	auto *data = static_cast<const T*>(src->getData());
	dst->Data.assign(data, data + src->getCount());
}

static void copyIndices(const scene::IIndexBuffer *src, scene::SIndexBuffer *dst)
{
	_IRR_DEBUG_BREAK_IF(src->getType() != video::EIT_16BIT);
	auto *data = static_cast<const u16*>(src->getData());
	dst->Data.assign(data, data + src->getCount());
}

//! Clones a static IMesh into a modifyable SMesh.
// not yet 32bit
SMesh *CMeshManipulator::createMeshCopy(scene::IMesh *mesh) const
{
	if (!mesh)
		return 0;

	SMesh *clone = new SMesh();

	const u32 meshBufferCount = mesh->getMeshBufferCount();

	for (u32 b = 0; b < meshBufferCount; ++b) {
		const IMeshBuffer *const mb = mesh->getMeshBuffer(b);
		switch (mb->getVertexType()) {
		case video::EVT_STANDARD: {
			SMeshBuffer *buffer = new SMeshBuffer();
			buffer->Material = mb->getMaterial();
			copyVertices(mb->getVertexBuffer(), buffer->Vertices);
			copyIndices(mb->getIndexBuffer(), buffer->Indices);
			clone->addMeshBuffer(buffer);
			buffer->drop();
		} break;
		case video::EVT_2TCOORDS: {
			SMeshBufferLightMap *buffer = new SMeshBufferLightMap();
			buffer->Material = mb->getMaterial();
			copyVertices(mb->getVertexBuffer(), buffer->Vertices);
			copyIndices(mb->getIndexBuffer(), buffer->Indices);
			clone->addMeshBuffer(buffer);
			buffer->drop();
		} break;
		case video::EVT_TANGENTS: {
			SMeshBufferTangents *buffer = new SMeshBufferTangents();
			buffer->Material = mb->getMaterial();
			copyVertices(mb->getVertexBuffer(), buffer->Vertices);
			copyIndices(mb->getIndexBuffer(), buffer->Indices);
			clone->addMeshBuffer(buffer);
			buffer->drop();
		} break;
		} // end switch

	} // end for all mesh buffers

	clone->BoundingBox = mesh->getBoundingBox();
	return clone;
}

//! Returns amount of polygons in mesh.
s32 CMeshManipulator::getPolyCount(scene::IMesh *mesh) const
{
	if (!mesh)
		return 0;

	s32 trianglecount = 0;

	for (u32 g = 0; g < mesh->getMeshBufferCount(); ++g)
		trianglecount += mesh->getMeshBuffer(g)->getIndexCount() / 3;

	return trianglecount;
}

//! Returns amount of polygons in mesh.
s32 CMeshManipulator::getPolyCount(scene::IAnimatedMesh *mesh) const
{
	if (mesh && mesh->getMaxFrameNumber() != 0)
		return getPolyCount(mesh->getMesh(0));

	return 0;
}

//! create a new AnimatedMesh and adds the mesh to it
IAnimatedMesh *CMeshManipulator::createAnimatedMesh(scene::IMesh *mesh, scene::E_ANIMATED_MESH_TYPE type) const
{
	return new SAnimatedMesh(mesh, type);
}

} // end namespace scene
} // end namespace irr

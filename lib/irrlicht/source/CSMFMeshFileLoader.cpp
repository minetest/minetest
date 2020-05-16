// Copyright (C) 2010-2012 Gaz Davidson / Joseph Ellis
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_SMF_LOADER_

#include "CSMFMeshFileLoader.h"
#include "SAnimatedMesh.h"
#include "SMeshBuffer.h"
#include "IReadFile.h"
#include "coreutil.h"
#include "os.h"
#include "IVideoDriver.h"

namespace irr
{
namespace scene
{

CSMFMeshFileLoader::CSMFMeshFileLoader(video::IVideoDriver* driver)
: Driver(driver)
{
}

//! Returns true if the file might be loaded by this class.
bool CSMFMeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension(filename, "smf");
}

//! Creates/loads an animated mesh from the file.
IAnimatedMesh* CSMFMeshFileLoader::createMesh(io::IReadFile* file)
{
	// create empty mesh
	SMesh *mesh = new SMesh();

	// load file
	u16 version;
	u8  flags;
	s32 limbCount;
	s32 i;

	io::BinaryFile::read(file, version);
	io::BinaryFile::read(file, flags);
	io::BinaryFile::read(file, limbCount);

	// load mesh data
	core::matrix4 identity;
	for (i=0; i < limbCount; ++i)
		loadLimb(file, mesh, identity);

	// recalculate buffer bounding boxes
	for (i=0; i < (s32)mesh->getMeshBufferCount(); ++i)
		mesh->getMeshBuffer(i)->recalculateBoundingBox();

	mesh->recalculateBoundingBox();
	SAnimatedMesh *am = new SAnimatedMesh();
	am->addMesh(mesh);
	mesh->drop();
	am->recalculateBoundingBox();

	return am;
}

void CSMFMeshFileLoader::loadLimb(io::IReadFile* file, SMesh* mesh, const core::matrix4 &parentTransformation)
{
	core::matrix4 transformation;

	// limb transformation
	core::vector3df translate, rotate, scale;
	io::BinaryFile::read(file, translate);
	io::BinaryFile::read(file, rotate);
	io::BinaryFile::read(file, scale);

	transformation.setTranslation(translate);
	transformation.setRotationDegrees(rotate);
	transformation.setScale(scale);

	transformation = parentTransformation * transformation;

	core::stringc textureName, textureGroupName;

	// texture information
	io::BinaryFile::read(file, textureGroupName);
	io::BinaryFile::read(file, textureName);

	// attempt to load texture using known formats
	video::ITexture* texture = 0;

	const c8* extensions[] = {".jpg", ".png", ".tga", ".bmp", 0};

	for (const c8 **ext = extensions; !texture && *ext; ++ext)
	{
		texture = Driver->getTexture(textureName + *ext);
		if (texture)
			textureName = textureName + *ext;
	}
	// find the correct mesh buffer
	u32 i;
	for (i=0; i<mesh->MeshBuffers.size(); ++i)
		if (mesh->MeshBuffers[i]->getMaterial().TextureLayer[0].Texture == texture)
			break;

	// create mesh buffer if none was found
	if (i == mesh->MeshBuffers.size())
	{
		CMeshBuffer<video::S3DVertex>* mb = new CMeshBuffer<video::S3DVertex>();
		mb->Material.TextureLayer[0].Texture = texture;

		// horribly hacky way to do this, maybe it's in the flags?
		if (core::hasFileExtension(textureName, "tga", "png"))
			mb->Material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		else
			mb->Material.MaterialType = video::EMT_SOLID;

		mesh->MeshBuffers.push_back(mb);
	}

	CMeshBuffer<video::S3DVertex>* mb = (CMeshBuffer<video::S3DVertex>*)mesh->MeshBuffers[i];

	u16 vertexCount, firstVertex = mb->getVertexCount();

	io::BinaryFile::read(file, vertexCount);
	mb->Vertices.reallocate(mb->Vertices.size() + vertexCount);

	// add vertices and set positions
	for (i=0; i<vertexCount; ++i)
	{
		core::vector3df pos;
		io::BinaryFile::read(file, pos);
		transformation.transformVect(pos);
		video::S3DVertex vert;
		vert.Color = 0xFFFFFFFF;
		vert.Pos = pos;
		mb->Vertices.push_back(vert);
	}

	// set vertex normals
	for (i=0; i < vertexCount; ++i)
	{
		core::vector3df normal;
		io::BinaryFile::read(file, normal);
		transformation.rotateVect(normal);
		mb->Vertices[firstVertex + i].Normal = normal;
	}
	// set texture coordinates

	for (i=0; i < vertexCount; ++i)
	{
		core::vector2df tcoords;
		io::BinaryFile::read(file, tcoords);
		mb->Vertices[firstVertex + i].TCoords = tcoords;
	}

	// triangles
	u32 triangleCount;
	// vertexCount used as temporary
	io::BinaryFile::read(file, vertexCount);
	triangleCount=3*vertexCount;
	mb->Indices.reallocate(mb->Indices.size() + triangleCount);

	for (i=0; i < triangleCount; ++i)
	{
		u16 index;
		io::BinaryFile::read(file, index);
		mb->Indices.push_back(firstVertex + index);
	}

	// read limbs
	s32 limbCount;
	io::BinaryFile::read(file, limbCount);

	for (s32 l=0; l < limbCount; ++l)
		loadLimb(file, mesh, transformation);
}

} // namespace scene

// todo: at some point in the future let's move these to a place where everyone can use them.
namespace io
{

#if _BIGENDIAN
#define _SYSTEM_BIG_ENDIAN_ (true)
#else
#define _SYSTEM_BIG_ENDIAN_ (false)
#endif

template <class T>
void BinaryFile::read(io::IReadFile* file, T &out, bool bigEndian)
{
	file->read((void*)&out, sizeof(out));
	if (bigEndian != (_SYSTEM_BIG_ENDIAN_))
		out = os::Byteswap::byteswap(out);
}

//! reads a 3d vector from the file, moving the file pointer along
void BinaryFile::read(io::IReadFile* file, core::vector3df &outVector2d, bool bigEndian)
{
	BinaryFile::read(file, outVector2d.X, bigEndian);
	BinaryFile::read(file, outVector2d.Y, bigEndian);
	BinaryFile::read(file, outVector2d.Z, bigEndian);
}

//! reads a 2d vector from the file, moving the file pointer along
void BinaryFile::read(io::IReadFile* file, core::vector2df &outVector2d, bool bigEndian)
{
	BinaryFile::read(file, outVector2d.X, bigEndian);
	BinaryFile::read(file, outVector2d.Y, bigEndian);
}

//! reads a null terminated string from the file, moving the file pointer along
void BinaryFile::read(io::IReadFile* file, core::stringc &outString, bool bigEndian)
{
	c8 c;
	file->read((void*)&c, 1);

	while (c)
	{
		outString += c;
		file->read((void*)&c, 1);
	}
}

} // namespace io

} // namespace irr

#endif // compile with SMF loader


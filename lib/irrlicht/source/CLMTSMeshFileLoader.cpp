// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
// This file was written by Jonas Petersen and modified by Nikolaus Gebhardt.
// See CLMTSMeshFileLoder.h for details.
/*

CLMTSMeshFileLoader.cpp

LMTSMeshFileLoader
Written by Jonas Petersen (a.k.a. jox)

Version 1.5 - 15 March 2005

Get the latest version here: http://development.mindfloaters.de/

This class allows loading meshes with lightmaps (*.lmts + *.tga files) that were created
using Pulsar LMTools by Lord Trancos (http://www.geocities.com/dxlab/index_en.html)

Notes:
- This version does not support user data in the *.lmts files, but still loads those files (by skipping the extra data).

License:
--------

It's free. You are encouraged to give me credit if you use it in your software.

Version History:
----------------

Version 1.5 - 15 March 2005
- Did a better cleanup. No memory leaks in case of an loading error.
- Added "#include <stdio.h>" for sprintf.

Version 1.4 - 12 March 2005
- Fixed bug in texture and subset loading code that would possibly cause crash.
- Fixed memory cleanup to avoid leak when loading more then one mesh
- Used the irrlicht Logger instead of cerr to output warnings and errors.
  For this I had to change the constructor
  from:
	CLMTSMeshFileLoader(io::IFileSystem* fs, video::IVideoDriver* driver)
  to:
	CLMTSMeshFileLoader(IrrlichtDevice* device)

Version 1.3 - 15 February 2005
- Fixed bug that prevented loading more than one different lmts files.
- Removed unnecessary "#include <os.h>".
- Added "std::" in front of "cerr". This was necessary for Visual Studio .NET,
  I hope it's not disturbing other compilers.
- Added warning message when a texture can not be loaded.
- Changed the documentation a bit (minor).

Version 1.2
- To avoid confusion I skipped version 1.2 because the website was offering
version 1.2 even though it was only version 1.1. Sorry about that.

Version 1.1 - 29 July 2004
- Added setTexturePath() function
- Minor improvements

Version 1.0 - 29 July 2004
- Initial release


*/
//////////////////////////////////////////////////////////////////////

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_LMTS_LOADER_

#include "SMeshBufferLightMap.h"
#include "SAnimatedMesh.h"
#include "SMeshBuffer.h"
#include "irrString.h"
#include "IReadFile.h"
#include "IAttributes.h"
#include "ISceneManager.h"
#include "CLMTSMeshFileLoader.h"
#include "os.h"

namespace irr
{
namespace scene
{

CLMTSMeshFileLoader::CLMTSMeshFileLoader(io::IFileSystem* fs,
		video::IVideoDriver* driver, io::IAttributes* parameters)
	: Textures(0), Subsets(0), Triangles(0),
	Parameters(parameters), Driver(driver), FileSystem(fs), FlipEndianess(false)
{
	#ifdef _DEBUG
	setDebugName("CLMTSMeshFileLoader");
	#endif

	if (Driver)
		Driver->grab();

	if (FileSystem)
		FileSystem->grab();
}


CLMTSMeshFileLoader::~CLMTSMeshFileLoader()
{
	cleanup();

	if (Driver)
		Driver->drop();

	if (FileSystem)
		FileSystem->drop();
}


void CLMTSMeshFileLoader::cleanup()
{
	delete [] Textures;
	Textures = 0;
	delete [] Subsets;
	Subsets = 0;
	delete [] Triangles;
	Triangles = 0;
}


bool CLMTSMeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "lmts" );
}


IAnimatedMesh* CLMTSMeshFileLoader::createMesh(io::IReadFile* file)
{
	u32 i;
	u32 id;

	// HEADER

	file->read(&Header, sizeof(SLMTSHeader));
	if (Header.MagicID == 0x4C4D5354)
	{
		FlipEndianess = true;
		Header.MagicID = os::Byteswap::byteswap(Header.MagicID);
		Header.Version = os::Byteswap::byteswap(Header.Version);
		Header.HeaderSize = os::Byteswap::byteswap(Header.HeaderSize);
		Header.TextureCount = os::Byteswap::byteswap(Header.TextureCount);
		Header.SubsetCount = os::Byteswap::byteswap(Header.SubsetCount);
		Header.TriangleCount = os::Byteswap::byteswap(Header.TriangleCount);
		Header.SubsetSize = os::Byteswap::byteswap(Header.SubsetSize);
		Header.VertexSize = os::Byteswap::byteswap(Header.VertexSize);
	}
	if (Header.MagicID != 0x53544D4C) { // "LMTS"
		os::Printer::log("LMTS ERROR: wrong header magic id!", ELL_ERROR);
		return 0;
	}

	//Skip any User Data (arbitrary app specific data)

	const s32 userSize = Header.HeaderSize - sizeof(SLMTSHeader);
	if (userSize>0)
		file->seek(userSize,true);

	// TEXTURES

	file->read(&id, sizeof(u32));
	if (FlipEndianess)
		id = os::Byteswap::byteswap(id);
	if (id != 0x54584554) { // "TEXT"
		os::Printer::log("LMTS ERROR: wrong texture magic id!", ELL_ERROR);
		return 0;
	}

	Textures = new SLMTSTextureInfoEntry[Header.TextureCount];

	file->read(Textures, sizeof(SLMTSTextureInfoEntry)*Header.TextureCount);
	if (FlipEndianess)
	{
		for (i=0; i<Header.TextureCount; ++i)
			Textures[i].Flags = os::Byteswap::byteswap(Textures[i].Flags);
	}

	// SUBSETS

	file->read(&id, sizeof(u32));
	if (FlipEndianess)
		id = os::Byteswap::byteswap(id);
	if (id != 0x53425553) // "SUBS"
	{
		os::Printer::log("LMTS ERROR: wrong subset magic id!", ELL_ERROR);
		cleanup();
		return 0;
	}

	Subsets = new SLMTSSubsetInfoEntry[Header.SubsetCount];
	const s32 subsetUserSize = Header.SubsetSize - sizeof(SLMTSSubsetInfoEntry);

	for (i=0; i<Header.SubsetCount; ++i)
	{
		file->read(&Subsets[i], sizeof(SLMTSSubsetInfoEntry));
		if (FlipEndianess)
		{
			Subsets[i].Offset = os::Byteswap::byteswap(Subsets[i].Offset);
			Subsets[i].Count = os::Byteswap::byteswap(Subsets[i].Count);
			Subsets[i].TextID1 = os::Byteswap::byteswap(Subsets[i].TextID1);
			Subsets[i].TextID2 = os::Byteswap::byteswap(Subsets[i].TextID2);
		}
		if (subsetUserSize>0)
			file->seek(subsetUserSize,true);
	}

	// TRIANGLES

	file->read(&id, sizeof(u32));
	if (FlipEndianess)
		id = os::Byteswap::byteswap(id);
	if (id != 0x53495254) // "TRIS"
	{
		os::Printer::log("LMTS ERROR: wrong triangle magic id!", ELL_ERROR);
		cleanup();
		return 0;
	}

	Triangles = new SLMTSTriangleDataEntry[(Header.TriangleCount*3)];
	const s32 triUserSize = Header.VertexSize - sizeof(SLMTSTriangleDataEntry);

	for (i=0; i<(Header.TriangleCount*3); ++i)
	{
		file->read(&Triangles[i], sizeof(SLMTSTriangleDataEntry));
		if (FlipEndianess)
		{
			Triangles[i].X = os::Byteswap::byteswap(Triangles[i].X);
			Triangles[i].Y = os::Byteswap::byteswap(Triangles[i].Y);
			Triangles[i].Z = os::Byteswap::byteswap(Triangles[i].Z);
			Triangles[i].U1 = os::Byteswap::byteswap(Triangles[i].U1);
			Triangles[i].V1 = os::Byteswap::byteswap(Triangles[i].U2);
			Triangles[i].U2 = os::Byteswap::byteswap(Triangles[i].V1);
			Triangles[i].V2 = os::Byteswap::byteswap(Triangles[i].V2);
		}
		if (triUserSize>0)
			file->seek(triUserSize,true);
	}

	/////////////////////////////////////////////////////////////////

	SMesh* mesh = new SMesh();

	constructMesh(mesh);

	loadTextures(mesh);

	cleanup();

	SAnimatedMesh* am = new SAnimatedMesh();
	am->Type = EAMT_LMTS; // not unknown to irrlicht anymore

	am->addMesh(mesh);
	am->recalculateBoundingBox();
	mesh->drop();
	return am;
}


void CLMTSMeshFileLoader::constructMesh(SMesh* mesh)
{
	for (s32 i=0; i<Header.SubsetCount; ++i)
	{
		scene::SMeshBufferLightMap* meshBuffer = new scene::SMeshBufferLightMap();

		// EMT_LIGHTMAP_M2/EMT_LIGHTMAP_M4 also possible
		meshBuffer->Material.MaterialType = video::EMT_LIGHTMAP;
		meshBuffer->Material.Wireframe = false;
		meshBuffer->Material.Lighting = false;

		mesh->addMeshBuffer(meshBuffer);

		const u32 offs = Subsets[i].Offset * 3;

		for (u32 sc=0; sc<Subsets[i].Count; sc++)
		{
			const u32 idx = meshBuffer->getVertexCount();

			for (u32 vu=0; vu<3; ++vu)
			{
				const SLMTSTriangleDataEntry& v = Triangles[offs+(3*sc)+vu];
				meshBuffer->Vertices.push_back(
						video::S3DVertex2TCoords(
							v.X, v.Y, v.Z,
							video::SColor(255,255,255,255),
							v.U1, v.V1, v.U2, v.V2));
			}
			const core::vector3df normal = core::plane3df(
				meshBuffer->Vertices[idx].Pos,
				meshBuffer->Vertices[idx+1].Pos,
				meshBuffer->Vertices[idx+2].Pos).Normal;

			meshBuffer->Vertices[idx].Normal = normal;
			meshBuffer->Vertices[idx+1].Normal = normal;
			meshBuffer->Vertices[idx+2].Normal = normal;

			meshBuffer->Indices.push_back(idx);
			meshBuffer->Indices.push_back(idx+1);
			meshBuffer->Indices.push_back(idx+2);
		}
		meshBuffer->drop();
	}

	for (u32 j=0; j<mesh->MeshBuffers.size(); ++j)
		mesh->MeshBuffers[j]->recalculateBoundingBox();

	mesh->recalculateBoundingBox();
}


void CLMTSMeshFileLoader::loadTextures(SMesh* mesh)
{
	if (!Driver || !FileSystem)
		return;

	// load textures

	// a little too much space, but won't matter here
	core::array<video::ITexture*> tex;
	tex.reallocate(Header.TextureCount);
	core::array<video::ITexture*> lig;
	lig.reallocate(Header.TextureCount);
	core::array<u32> id2id;
	id2id.reallocate(Header.TextureCount);

	const core::stringc Path = Parameters->getAttributeAsString(LMTS_TEXTURE_PATH);

	core::stringc s;
	for (u32 t=0; t<Header.TextureCount; ++t)
	{
		video::ITexture* tmptex = 0;
		s = Path;
		s.append(Textures[t].Filename);

		if (FileSystem->existFile(s))
			tmptex = Driver->getTexture(s);
		else
			os::Printer::log("LMTS WARNING: Texture does not exist", s.c_str(), ELL_WARNING);

		if (Textures[t].Flags & 0x01)
		{
			id2id.push_back(lig.size());
			lig.push_back(tmptex);
		}
		else
		{
			id2id.push_back(tex.size());
			tex.push_back(tmptex);
		}
	}

	// attach textures to materials.

	for (u32 i=0; i<Header.SubsetCount; ++i)
	{
		if (Subsets[i].TextID1 < Header.TextureCount && id2id[Subsets[i].TextID1] < tex.size())
			mesh->getMeshBuffer(i)->getMaterial().setTexture(0, tex[id2id[Subsets[i].TextID1]]);
		if (Subsets[i].TextID2 < Header.TextureCount && id2id[Subsets[i].TextID2] < lig.size())
			mesh->getMeshBuffer(i)->getMaterial().setTexture(1, lig[id2id[Subsets[i].TextID2]]);

		if (!mesh->getMeshBuffer(i)->getMaterial().getTexture(1))
			mesh->getMeshBuffer(i)->getMaterial().MaterialType = video::EMT_SOLID;
	}
}


} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_LMTS_LOADER_

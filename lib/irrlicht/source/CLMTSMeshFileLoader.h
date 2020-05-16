// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
//
// I (Nikolaus Gebhardt) did some few changes to Jonas Petersen's original loader:
// - removed setTexturePath() and replaced with the ISceneManager::getStringParameter()-stuff.
// - added EAMT_LMTS enumeration value
// Thanks a lot to Jonas Petersen for his work
// on this and that he gave me his permission to add it into Irrlicht.
/*

CLMTSMeshFileLoader.h

LMTSMeshFileLoader
Written by Jonas Petersen (a.k.a. jox)

Version 1.5 - 15 March 2005

*/

#if !defined(__C_LMTS_MESH_FILE_LOADER_H_INCLUDED__)
#define __C_LMTS_MESH_FILE_LOADER_H_INCLUDED__

#include "IMeshLoader.h"
#include "SMesh.h"
#include "IFileSystem.h"
#include "IVideoDriver.h"

namespace irr
{
namespace scene
{

class CLMTSMeshFileLoader : public IMeshLoader
{
public:

	CLMTSMeshFileLoader(io::IFileSystem* fs,
		video::IVideoDriver* driver, io::IAttributes* parameters);

	virtual ~CLMTSMeshFileLoader();

	virtual bool isALoadableFileExtension(const io::path& filename) const;

	virtual IAnimatedMesh* createMesh(io::IReadFile* file);

private:
	void constructMesh(SMesh* mesh);
	void loadTextures(SMesh* mesh);
	void cleanup();

// byte-align structures
#include "irrpack.h"

	struct SLMTSHeader
	{
		u32 MagicID;
		u32 Version;
		u32 HeaderSize;
		u16 TextureCount;
		u16 SubsetCount;
		u32 TriangleCount;
		u16 SubsetSize;
		u16 VertexSize;
	} PACK_STRUCT;

	struct SLMTSTextureInfoEntry
	{
		c8 Filename[256];
		u16 Flags;
	} PACK_STRUCT;

	struct SLMTSSubsetInfoEntry
	{
		u32 Offset;
		u32 Count;
		u16 TextID1;
		u16 TextID2;
	} PACK_STRUCT;

	struct SLMTSTriangleDataEntry
	{
		f32 X;
		f32 Y;
		f32 Z;
		f32 U1;
		f32 V1;
		f32 U2;
		f32 V2;
	} PACK_STRUCT;

// Default alignment
#include "irrunpack.h"

	SLMTSHeader Header;
	SLMTSTextureInfoEntry* Textures;
	SLMTSSubsetInfoEntry* Subsets;
	SLMTSTriangleDataEntry* Triangles;

	io::IAttributes* Parameters;
	video::IVideoDriver* Driver;
	io::IFileSystem* FileSystem;
	bool FlipEndianess;
};

} // end namespace scene
} // end namespace irr

#endif // !defined(__C_LMTS_MESH_FILE_LOADER_H_INCLUDED__)

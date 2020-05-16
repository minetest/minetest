// Copyright (C) 2006-2012 Luke Hoschke
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

// B3D Mesh loader
// File format designed by Mark Sibly for the Blitz3D engine and has been
// declared public domain

#include "IrrCompileConfig.h"

#ifndef __C_B3D_MESH_LOADER_H_INCLUDED__
#define __C_B3D_MESH_LOADER_H_INCLUDED__

#include "IMeshLoader.h"
#include "ISceneManager.h"
#include "CSkinnedMesh.h"
#include "IReadFile.h"

namespace irr
{

namespace scene
{

//! Meshloader for B3D format
class CB3DMeshFileLoader : public IMeshLoader
{
public:

	//! Constructor
	CB3DMeshFileLoader(scene::ISceneManager* smgr);

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".bsp")
	virtual bool isALoadableFileExtension(const io::path& filename) const;

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See IReferenceCounted::drop() for more information.
	virtual IAnimatedMesh* createMesh(io::IReadFile* file);

private:

	struct SB3dChunkHeader
	{
		c8 name[4];
		s32 size;
	};

	struct SB3dChunk
	{
		SB3dChunk(const SB3dChunkHeader& header, long sp)
			: length(header.size+8), startposition(sp)
		{
			name[0]=header.name[0];
			name[1]=header.name[1];
			name[2]=header.name[2];
			name[3]=header.name[3];
		}

		c8 name[4];
		s32 length;
		long startposition;
	};

	struct SB3dTexture
	{
		core::stringc TextureName;
		s32 Flags;
		s32 Blend;
		f32 Xpos;
		f32 Ypos;
		f32 Xscale;
		f32 Yscale;
		f32 Angle;
	};

	struct SB3dMaterial
	{
		SB3dMaterial() : red(1.0f), green(1.0f),
			blue(1.0f), alpha(1.0f), shininess(0.0f), blend(1),
			fx(0)
		{
			for (u32 i=0; i<video::MATERIAL_MAX_TEXTURES; ++i)
				Textures[i]=0;
		}
		video::SMaterial Material;
		f32 red, green, blue, alpha;
		f32 shininess;
		s32 blend,fx;
		SB3dTexture *Textures[video::MATERIAL_MAX_TEXTURES];
	};

	bool load();
	bool readChunkNODE(CSkinnedMesh::SJoint* InJoint);
	bool readChunkMESH(CSkinnedMesh::SJoint* InJoint);
	bool readChunkVRTS(CSkinnedMesh::SJoint* InJoint);
	bool readChunkTRIS(scene::SSkinMeshBuffer *MeshBuffer, u32 MeshBufferID, s32 Vertices_Start);
	bool readChunkBONE(CSkinnedMesh::SJoint* InJoint);
	bool readChunkKEYS(CSkinnedMesh::SJoint* InJoint);
	bool readChunkANIM();
	bool readChunkTEXS();
	bool readChunkBRUS();

	void loadTextures(SB3dMaterial& material) const;

	void readString(core::stringc& newstring);
	void readFloats(f32* vec, u32 count);

	core::array<SB3dChunk> B3dStack;

	core::array<SB3dMaterial> Materials;
	core::array<SB3dTexture> Textures;

	core::array<s32> AnimatedVertices_VertexID;

	core::array<s32> AnimatedVertices_BufferID;

	core::array<video::S3DVertex2TCoords> BaseVertices;

	ISceneManager*	SceneManager;
	CSkinnedMesh*	AnimatedMesh;
	io::IReadFile*	B3DFile;

	//B3Ds have Vertex ID's local within the mesh I don't want this
	// Variable needs to be class member due to recursion in calls
	u32 VerticesStart;

	bool NormalsInFile;
	bool HasVertexColors;
	bool ShowWarning;
};


} // end namespace scene
} // end namespace irr

#endif // __C_B3D_MESH_LOADER_H_INCLUDED__


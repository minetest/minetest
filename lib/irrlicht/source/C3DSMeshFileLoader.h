// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_3DS_MESH_FILE_LOADER_H_INCLUDED__
#define __C_3DS_MESH_FILE_LOADER_H_INCLUDED__

#include "IMeshLoader.h"
#include "IFileSystem.h"
#include "ISceneManager.h"
#include "irrString.h"
#include "SMesh.h"
#include "matrix4.h"

namespace irr
{
namespace scene
{

//! Meshloader capable of loading 3ds meshes.
class C3DSMeshFileLoader : public IMeshLoader
{
public:

	//! Constructor
	C3DSMeshFileLoader(ISceneManager* smgr, io::IFileSystem* fs);

	//! destructor
	virtual ~C3DSMeshFileLoader();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".cob")
	virtual bool isALoadableFileExtension(const io::path& filename) const;

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See IReferenceCounted::drop() for more information.
	virtual IAnimatedMesh* createMesh(io::IReadFile* file);

private:

// byte-align structures
#include "irrpack.h"

	struct ChunkHeader
	{
		u16 id;
		s32 length;
	} PACK_STRUCT;

// Default alignment
#include "irrunpack.h"

	struct ChunkData
	{
		ChunkData() : read(0) {}

		ChunkHeader header;
		s32 read;
	};

	struct SCurrentMaterial
	{
		void clear() {
			Material=video::SMaterial();
			Name="";
			Filename[0]="";
			Filename[1]="";
			Filename[2]="";
			Filename[3]="";
			Filename[4]="";
			Strength[0]=0.f;
			Strength[1]=0.f;
			Strength[2]=0.f;
			Strength[3]=0.f;
			Strength[4]=0.f;
		}

		video::SMaterial Material;
		core::stringc Name;
		core::stringc Filename[5];
		f32 Strength[5];
	};

	struct SMaterialGroup
	{
		SMaterialGroup() : faceCount(0), faces(0) {};

		SMaterialGroup(const SMaterialGroup& o)
		{
			*this = o;
		}

		~SMaterialGroup()
		{
			clear();
		}

		void clear()
		{
			delete [] faces;
			faces = 0;
			faceCount = 0;
		}

		void operator =(const SMaterialGroup& o)
		{
			MaterialName = o.MaterialName;
			faceCount = o.faceCount;
			faces = new u16[faceCount];
			for (u16 i=0; i<faceCount; ++i)
				faces[i] = o.faces[i];
		}

		core::stringc MaterialName;
		u16 faceCount;
		u16* faces;
	};

	bool readChunk(io::IReadFile* file, ChunkData* parent);
	bool readMaterialChunk(io::IReadFile* file, ChunkData* parent);
	bool readFrameChunk(io::IReadFile* file, ChunkData* parent);
	bool readTrackChunk(io::IReadFile* file, ChunkData& data,
				IMeshBuffer* mb, const core::vector3df& pivot);
	bool readObjectChunk(io::IReadFile* file, ChunkData* parent);
	bool readPercentageChunk(io::IReadFile* file, ChunkData* chunk, f32& percentage);
	bool readColorChunk(io::IReadFile* file, ChunkData* chunk, video::SColor& out);

	void readChunkData(io::IReadFile* file, ChunkData& data);
	void readString(io::IReadFile* file, ChunkData& data, core::stringc& out);
	void readVertices(io::IReadFile* file, ChunkData& data);
	void readIndices(io::IReadFile* file, ChunkData& data);
	void readMaterialGroup(io::IReadFile* file, ChunkData& data);
	void readTextureCoords(io::IReadFile* file, ChunkData& data);

	void composeObject(io::IReadFile* file, const core::stringc& name);
	void loadMaterials(io::IReadFile* file);
	void cleanUp();

	scene::ISceneManager* SceneManager;
	io::IFileSystem* FileSystem;

	f32* Vertices;
	u16* Indices;
	u32* SmoothingGroups;
	core::array<u16> TempIndices;
	f32* TCoords;
	u16 CountVertices;
	u16 CountFaces; // = CountIndices/4
	u16 CountTCoords;
	core::array<SMaterialGroup> MaterialGroups;

	SCurrentMaterial CurrentMaterial;
	core::array<SCurrentMaterial> Materials;
	core::array<core::stringc> MeshBufferNames;
	core::matrix4 TransformationMatrix;

	SMesh* Mesh;
};

} // end namespace scene
} // end namespace irr

#endif


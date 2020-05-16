// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
// orginally written by Christian Stehno, modified by Nikolaus Gebhardt

#ifndef __C_OGRE_MESH_FILE_LOADER_H_INCLUDED__
#define __C_OGRE_MESH_FILE_LOADER_H_INCLUDED__

#include "IMeshLoader.h"
#include "IFileSystem.h"
#include "IVideoDriver.h"
#include "irrString.h"
#include "SMesh.h"
#include "SMeshBuffer.h"
#include "SMeshBufferLightMap.h"
#include "IMeshManipulator.h"
#include "matrix4.h"
#include "quaternion.h"
#include "CSkinnedMesh.h"

namespace irr
{
namespace scene
{

//! Meshloader capable of loading ogre meshes.
class COgreMeshFileLoader : public IMeshLoader
{
public:

	//! Constructor
	COgreMeshFileLoader(io::IFileSystem* fs, video::IVideoDriver* driver);

	//! destructor
	virtual ~COgreMeshFileLoader();

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
		u32 length;
	} PACK_STRUCT;

// Default alignment
#include "irrunpack.h"


	struct ChunkData
	{
		ChunkData() : read(0) {}

		ChunkHeader header;
		u32 read;
	};

	struct OgreTexture
	{
		core::array<core::stringc> Filename;
		core::stringc Alias;
		core::stringc CoordsType;
		core::stringc MipMaps;
		core::stringc Alpha;
	};

	struct OgrePass
	{
		OgrePass() : AmbientTokenColor(false),
			DiffuseTokenColor(false), SpecularTokenColor(false),
			EmissiveTokenColor(false),
			MaxLights(8), PointSize(1.0f), PointSprites(false),
			PointSizeMin(0), PointSizeMax(0) {}

		video::SMaterial Material;
		OgreTexture Texture;
		bool AmbientTokenColor;
		bool DiffuseTokenColor;
		bool SpecularTokenColor;
		bool EmissiveTokenColor;
		u32 MaxLights;
		f32 PointSize;
		bool PointSprites;
		u32 PointSizeMin;
		u32 PointSizeMax;
	};

	struct OgreTechnique
	{
		OgreTechnique() : Name(""), LODIndex(0) {}

		core::stringc Name;
		core::stringc Scheme;
		u16 LODIndex;
		core::array<OgrePass> Passes;
	};

	struct OgreMaterial
	{
		OgreMaterial() : Name(""), ReceiveShadows(true),
			TransparencyCastsShadows(false) {}

		core::stringc Name;
		bool ReceiveShadows;
		bool TransparencyCastsShadows;
		core::array<f32> LODDistances;
		core::array<OgreTechnique> Techniques;
	};

	struct OgreVertexBuffer
	{
		OgreVertexBuffer() : BindIndex(0), VertexSize(0), Data(0) {}

		u16 BindIndex;
		u16 VertexSize;
		core::array<f32> Data;
	};

	struct OgreVertexElement
	{
		u16 Source,
		Type,
		Semantic,
		Offset,
		Index;
	};

	struct OgreGeometry
	{
		s32 NumVertex;
		core::array<OgreVertexElement> Elements;
		core::array<OgreVertexBuffer> Buffers;
		core::array<core::vector3df> Vertices;
		core::array<core::vector3df> Normals;
		core::array<s32> Colors;
		core::array<core::vector2df> TexCoords;
	};

	struct OgreTextureAlias
	{
		OgreTextureAlias() {};
		OgreTextureAlias(const core::stringc& a, const core::stringc& b) : Texture(a), Alias(b) {};
		core::stringc Texture;
		core::stringc Alias;
	};

	struct OgreBoneAssignment
	{
		s32 VertexID;
		u16 BoneID;
		f32 Weight;
	};

	struct OgreSubMesh
	{
		core::stringc Material;
		bool SharedVertices;
		core::array<s32> Indices;
		OgreGeometry Geometry;
		u16 Operation;
		core::array<OgreTextureAlias> TextureAliases;
		core::array<OgreBoneAssignment> BoneAssignments;
		bool Indices32Bit;
	};

	struct OgreMesh
	{
		bool SkeletalAnimation;
		OgreGeometry Geometry;
		core::array<OgreSubMesh> SubMeshes;
		core::array<OgreBoneAssignment> BoneAssignments;
		core::vector3df BBoxMinEdge;
		core::vector3df BBoxMaxEdge;
		f32 BBoxRadius;
	};

	struct OgreBone
	{
		core::stringc Name;
		core::vector3df Position;
		core::quaternion Orientation;
		core::vector3df Scale;
		u16 Handle;
		u16 Parent;
	};

	struct OgreKeyframe
	{
		u16 BoneID;
		f32 Time;
		core::vector3df Position;
		core::quaternion Orientation;
		core::vector3df Scale;
	};

	struct OgreAnimation
	{
		core::stringc Name;
		f32 Length;
		core::array<OgreKeyframe> Keyframes;
	};

	struct OgreSkeleton
	{
		core::array<OgreBone> Bones;
		core::array<OgreAnimation> Animations;
	};

	bool readChunk(io::IReadFile* file);
	bool readObjectChunk(io::IReadFile* file, ChunkData& parent, OgreMesh& mesh);
	bool readGeometry(io::IReadFile* file, ChunkData& parent, OgreGeometry& geometry);
	bool readVertexDeclaration(io::IReadFile* file, ChunkData& parent, OgreGeometry& geometry);
	bool readVertexBuffer(io::IReadFile* file, ChunkData& parent, OgreGeometry& geometry);
	bool readSubMesh(io::IReadFile* file, ChunkData& parent, OgreSubMesh& subMesh);

	void readChunkData(io::IReadFile* file, ChunkData& data);
	void readString(io::IReadFile* file, ChunkData& data, core::stringc& out);
	void readBool(io::IReadFile* file, ChunkData& data, bool& out);
	void readInt(io::IReadFile* file, ChunkData& data, s32* out, u32 num=1);
	void readShort(io::IReadFile* file, ChunkData& data, u16* out, u32 num=1);
	void readFloat(io::IReadFile* file, ChunkData& data, f32* out, u32 num=1);
	void readVector(io::IReadFile* file, ChunkData& data, core::vector3df& out);
	void readQuaternion(io::IReadFile* file, ChunkData& data, core::quaternion& out);

	void composeMeshBufferMaterial(scene::IMeshBuffer* mb, const core::stringc& materialName);
	scene::SMeshBuffer* composeMeshBuffer(const core::array<s32>& indices, const OgreGeometry& geom);
	scene::SMeshBufferLightMap* composeMeshBufferLightMap(const core::array<s32>& indices, const OgreGeometry& geom);
	scene::IMeshBuffer* composeMeshBufferSkinned(scene::CSkinnedMesh& mesh, const core::array<s32>& indices, const OgreGeometry& geom);
	void composeObject(void);
	bool readColor(io::IReadFile* meshFile, video::SColor& col);
	void getMaterialToken(io::IReadFile* file, core::stringc& token, bool noNewLine=false);
	void readTechnique(io::IReadFile* meshFile, OgreMaterial& mat);
	void readPass(io::IReadFile* file, OgreTechnique& technique);
	void loadMaterials(io::IReadFile* file);
	bool loadSkeleton(io::IReadFile* meshFile, const core::stringc& name);
	void clearMeshes();

	io::IFileSystem* FileSystem;
	video::IVideoDriver* Driver;

	core::stringc Version;
	bool SwapEndian;
	core::array<OgreMesh> Meshes;
	io::path CurrentlyLoadingFromPath;

	core::array<OgreMaterial> Materials;
	OgreSkeleton Skeleton;

	IMesh* Mesh;
	u32 NumUV;
};

} // end namespace scene
} // end namespace irr

#endif


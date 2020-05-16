// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_LWO_MESH_FILE_LOADER_H_INCLUDED__
#define __C_LWO_MESH_FILE_LOADER_H_INCLUDED__

#include "IMeshLoader.h"
#include "SMeshBuffer.h"
#include "irrString.h"

namespace irr
{
namespace io
{
	class IReadFile;
	class IFileSystem;
} // end namespace io
namespace scene
{

	struct SMesh;
	class ISceneManager;

//! Meshloader capable of loading Lightwave 3D meshes.
class CLWOMeshFileLoader : public IMeshLoader
{
public:

	//! Constructor
	CLWOMeshFileLoader(scene::ISceneManager* smgr, io::IFileSystem* fs);

	//! destructor
	virtual ~CLWOMeshFileLoader();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".bsp")
	virtual bool isALoadableFileExtension(const io::path& filename) const;

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See IUnknown::drop() for more information.
	virtual IAnimatedMesh* createMesh(io::IReadFile* file);

private:

	struct tLWOMaterial;

	bool readFileHeader();
	bool readChunks();
	void readObj1(u32 size);
	void readTagMapping(u32 size);
	void readVertexMapping(u32 size);
	void readDiscVertexMapping (u32 size);
	void readObj2(u32 size);
	void readMat(u32 size);
	u32 readString(core::stringc& name, u32 size=0);
	u32 readVec(core::vector3df& vec);
	u32 readVX(u32& num);
	u32 readColor(video::SColor& color);
	video::ITexture* loadTexture(const core::stringc& file);

	scene::ISceneManager* SceneManager;
	io::IFileSystem* FileSystem;
	io::IReadFile* File;
	SMesh* Mesh;

	core::array<core::vector3df> Points;
	core::array<core::array<u32> > Indices;
	core::array<core::stringc> UvName;
	core::array<core::array<u32> > UvIndex;
	core::array<core::stringc> DUvName;
	core::array<core::array<u32> > VmPolyPointsIndex;
	core::array<core::array<core::vector2df> > VmCoordsIndex;

	core::array<u16> MaterialMapping;
	core::array<core::array<core::vector2df> > TCoords;
	core::array<tLWOMaterial*> Materials;
	core::array<core::stringc> Images;
	u8 FormatVersion;
};

} // end namespace scene
} // end namespace irr

#endif

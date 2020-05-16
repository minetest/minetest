// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_IRR_MESH_FILE_LOADER_H_INCLUDED__
#define __C_IRR_MESH_FILE_LOADER_H_INCLUDED__

#include "IMeshLoader.h"
#include "IFileSystem.h"
#include "IVideoDriver.h"
#include "irrString.h"
#include "SMesh.h"
#include "SMeshBuffer.h"
#include "CDynamicMeshBuffer.h"
#include "ISceneManager.h"

namespace irr
{
namespace scene
{


//! Meshloader capable of loading .irrmesh meshes, the Irrlicht Engine mesh format for static meshes
class CIrrMeshFileLoader : public IMeshLoader
{
public:

	//! Constructor
	CIrrMeshFileLoader(scene::ISceneManager* smgr, io::IFileSystem* fs);

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".cob")
	virtual bool isALoadableFileExtension(const io::path& filename) const;

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See IReferenceCounted::drop() for more information.
	virtual IAnimatedMesh* createMesh(io::IReadFile* file);

private:

	//! reads a mesh sections and creates a mesh from it
	IAnimatedMesh* readMesh(io::IXMLReader* reader);

	//! reads a mesh sections and creates a mesh buffer from it
	IMeshBuffer* readMeshBuffer(io::IXMLReader* reader);

	//! skips an (unknown) section in the irrmesh file
	void skipSection(io::IXMLReader* reader, bool reportSkipping);

	//! reads a <material> element and stores it in the material section
	void readMaterial(io::IXMLReader* reader);

	//! parses a float from a char pointer and moves the pointer to
	//! the end of the parsed float
	inline f32 readFloat(const c8** p);

	//! parses an int from a char pointer and moves the pointer to
	//! the end of the parsed float
	inline s32 readInt(const c8** p);

	//! places pointer to next begin of a token
	void findNextNoneWhiteSpace(const c8** p);

	//! places pointer to next begin of a token
	void skipCurrentNoneWhiteSpace(const c8** p);

	//! reads floats from inside of xml element until end of xml element
	void readFloatsInsideElement(io::IXMLReader* reader, f32* floats, u32 count);

	//! read the mesh buffers
	void readMeshBuffer(io::IXMLReader* reader, int vertexCount, CDynamicMeshBuffer* sbuffer);

	//! read indices
	void readIndices(io::IXMLReader* reader, int indexCount, IIndexBuffer& indices);


	// member variables

	scene::ISceneManager* SceneManager;
	io::IFileSystem* FileSystem;
};


} // end namespace scene
} // end namespace irr

#endif


// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_BSP_LOADER_

#include "CBSPMeshFileLoader.h"
#include "CQ3LevelMesh.h"

namespace irr
{
namespace scene
{

//! Constructor
CBSPMeshFileLoader::CBSPMeshFileLoader(scene::ISceneManager* smgr,
		io::IFileSystem* fs)
: FileSystem(fs), SceneManager(smgr)
{

	#ifdef _DEBUG
	setDebugName("CBSPMeshFileLoader");
	#endif

	if (FileSystem)
		FileSystem->grab();
}


//! destructor
CBSPMeshFileLoader::~CBSPMeshFileLoader()
{
	if (FileSystem)
		FileSystem->drop();
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool CBSPMeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "bsp", "shader", "cfg" );
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* CBSPMeshFileLoader::createMesh(io::IReadFile* file)
{
	s32 type = core::isFileExtension ( file->getFileName(), "bsp", "shader", "cfg" );
	CQ3LevelMesh* q = 0;

	switch ( type )
	{
		case 1:
			q = new CQ3LevelMesh(FileSystem, SceneManager, LoadParam);

			// determine real shaders in LoadParam
			if ( 0 == LoadParam.loadAllShaders )
			{
				q->getShader("scripts/common.shader");
				q->getShader("scripts/sfx.shader");
				q->getShader("scripts/gfx.shader");
				q->getShader("scripts/liquid.shader");
				q->getShader("scripts/models.shader");
				q->getShader("scripts/walls.shader");
				//q->getShader("scripts/sky.shader");
			}

			if ( q->loadFile(file) )
				return q;

			q->drop();
			break;

		case 2:
			q = new CQ3LevelMesh(FileSystem, SceneManager,LoadParam);
			q->getShader( file );
			return q;
			break;

		case 3:
			// load quake 3 loading parameter
			if ( file->getFileName() == "levelparameter.cfg" )
			{
				file->read ( &LoadParam, sizeof ( LoadParam ) );
			}
			else
			{
				q = new CQ3LevelMesh(FileSystem, SceneManager,LoadParam);
				q->getConfiguration( file );
				return q;
			}
			break;
	}

	return 0;
}

} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_BSP_LOADER_


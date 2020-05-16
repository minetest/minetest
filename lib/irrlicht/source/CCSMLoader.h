// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
//
// This Loader has been originally written by Saurav Mohapatra. I (Nikolaus Gebhardt)
// modified some minor things and integrated it into Irrlicht 0.9. Thanks a lot
// to Saurav Mohapatra for his work on this and that he gave me his permission to
// add it into Irrlicht.
// I did some changes to Saurav Mohapatra's loader, so I'm writing this down here:
// - Replaced all dependencies to STL and stdio with irr:: methods/constructs.
// - Moved everything into namespace irr::scene
// - Replaced logging with Irrlicht's internal logger.
// - Removed dependency to IrrlichtDevice
// - Moved all internal structures into CCSMLoader.cpp
// - Made the texture root parameter dependent on a ISceneManager string parameter
// - removed exceptions
// - Implemented CCCSMLoader as IMeshLoader
// - Fixed some problems with memory leaks
// - Fixed bounding box calculation
//
// The original readme of this file looks like this:
//
// This component provides a loader for the Cartography shop 4.x .csm maps for Irrlicht Engine.
// This is a part of the M_TRIX Project.
// This is licensed under the ZLib/LibPNG license
// The IrrCSM library is written by Saurav Mohapatra.
//
// Features
//
// The IrrCSM library features the following capabilities
//
//  * Loads the .csm 4.0 and 4.1 files transparently
//  * Presents the loaded file as irr::scene::IAnimatedMesh for easy creation of IOctreeSceneNode
//  * Loads the textures given the correct texture root. hence map and textures can be in separate directories
//
// For more informations go to http://www.geocities.com/standard_template/irrcsm/downloads.html

#ifndef __CSM_LOADER_H_INCLUDED__
#define __CSM_LOADER_H_INCLUDED__

#include "irrArray.h"
#include "IMesh.h"
#include "irrString.h"
#include "IFileSystem.h"
#include "IMeshLoader.h"

namespace irr
{
namespace scene
{
	class CSMFile;
	class ISceneManager;

	class CCSMLoader : public scene::IMeshLoader
	{
	public:

		CCSMLoader(ISceneManager* manager, io::IFileSystem* fs);

		//! returns true if the file maybe is able to be loaded by this class
		//! based on the file extension (e.g. ".bsp")
		virtual bool isALoadableFileExtension(const io::path& filename) const;

		//! creates/loads an animated mesh from the file.
		virtual IAnimatedMesh* createMesh(io::IReadFile* file);

	private:

		scene::IMesh* createCSMMesh(io::IReadFile* file);

		scene::IMesh* createIrrlichtMesh(const CSMFile* csmFile,
			const core::stringc& textureRoot, const io::path& lmprefix);

		io::IFileSystem* FileSystem;
		scene::ISceneManager* SceneManager;
	};

} // end namespace
} // end namespace

#endif


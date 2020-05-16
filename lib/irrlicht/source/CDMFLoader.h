// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
//
// This file was originally written by Salvatore Russo.
// I (Nikolaus Gebhardt) did some minor modifications changes to it and integrated
// it into Irrlicht:
// - removed STL dependency
// - removed log file and replaced it with irrlicht logging
// - adapted code formatting a bit to Irrlicht style
// - removed memory leaks
// Thanks a lot to Salvatore for his work on this and that he gave me
// his permission to add it into Irrlicht.

/*
  CDMFLoader by Salvatore Russo
  Version 1.3

  This loader is used to load DMF files in Irrlicht.
  Look at the documentation for a sample application.

  Parts of this code are from Murphy McCauley COCTLoader just like
  GetFaceNormal() or indexes creation routines and a routine to add faces. So
  please refer to COCTLoader.h to know more about rights granted.

  You can use this software as you wish but you must not remove these notes about license nor
  credits to others for parts of this code.
*/

#ifndef __C_DMF_LOADER_H_INCLUDED__
#define __C_DMF_LOADER_H_INCLUDED__

#include "IMeshLoader.h"
#include "IReadFile.h"
#include "IFileSystem.h"
#include "SMesh.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "SAnimatedMesh.h"

namespace irr
{
namespace scene
{
	/** A class to load DeleD mesh files.*/
	class CDMFLoader : public IMeshLoader
	{
	public:

		/** constructor*/
		CDMFLoader(ISceneManager* smgr, io::IFileSystem* filesys);

		//! returns true if the file maybe is able to be loaded by this class
		//! based on the file extension (e.g. ".cob")
		virtual bool isALoadableFileExtension(const io::path& filename) const;

		/** creates/loads an animated mesh from the file.
		\return Pointer to the created mesh. Returns 0 if loading failed.
		If you no longer need the mesh, you should call IAnimatedMesh::drop().
		See IReferenceCounted::drop() for more information.*/
		virtual IAnimatedMesh* createMesh(io::IReadFile* file);

		/** loads dynamic lights present in this scene.
		Note that loaded lights from DeleD must have the suffix \b dynamic_ and must be \b pointlight.
		Irrlicht correctly loads specular color, diffuse color , position and distance of object affected by light.
		\return number of lights loaded or 0 if loading failed.*/
		int loadLights(const c8 * filename, ISceneManager* smgr,
			ISceneNode*  parent = 0, s32 base_id = 1000);

		/** loads water plains present in this scene.
		Note that loaded water plains from DeleD must have the suffix \b water_ and must be \b rectangle (with just 1 rectangular face).
		Irrlicht correctly loads position and rotation of water plain as well as texture layers.
		\return number of water plains loaded or 0 if loading failed.*/
		int loadWaterPlains(const c8 *filename,
				ISceneManager* smgr,
				ISceneNode * parent = 0,
				s32 base_id = 2000,
				bool mode = true);

	private:
		void findFile(bool use_mat_dirs, const core::stringc& path, const core::stringc& matPath, core::stringc& filename);

		ISceneManager* SceneMgr;
		io::IFileSystem* FileSystem;
	};

} // end namespace scene
} // end namespace irr

#endif


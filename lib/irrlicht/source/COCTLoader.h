// Copyright (C) 2002-2012 Nikolaus Gebhardt 
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
//
// Because I (Nikolaus Gebhardt) did some changes to Murphy McCauley's loader,
// I'm writing this down here:
// - Replaced all dependencies to STL and stdio with irr:: methods/constructs
// - Disabled logging define
// - Changed some minor things (Don't remember what exactly.)
// Thanks a lot to Murphy McCauley for writing this loader.

//
//  COCTLoader by Murphy McCauley (February 2005)
//  An Irrlicht loader for OCT files
//
//  OCT file format information comes from the sourcecode of the Fluid Studios
//  Radiosity Processor by Paul Nettle.  You can get that sourcecode from
//  http://www.fluidstudios.com .
//
//  Parts of this code are from Irrlicht's CQ3LevelMesh and C3DSMeshFileLoader,
//  and are Copyright (C) 2002-2004 Nikolaus Gebhardt.
//
//  Use of this code is subject to the following:
//
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//  4. You may not use this software to directly or indirectly cause harm to others.


#ifndef __C_OCT_LOADER_H_INCLUDED__
#define __C_OCT_LOADER_H_INCLUDED__

#include "IMeshLoader.h"
#include "IReadFile.h"
#include "SMesh.h"
#include "irrString.h"

namespace irr
{
namespace io
{
	class IFileSystem;
} // end namespace io
namespace scene
{
	class ISceneManager;
	class ISceneNode;

	class COCTLoader : public IMeshLoader
	{
	public:
		//! constructor
		COCTLoader(ISceneManager* smgr, io::IFileSystem* fs);

		//! destructor
		virtual ~COCTLoader();

		//! returns true if the file maybe is able to be loaded by this class
		//! based on the file extension (e.g. ".cob")
		virtual bool isALoadableFileExtension(const io::path& filename) const;

		//! creates/loads an animated mesh from the file.
		//! \return Pointer to the created mesh. Returns 0 if loading failed.
		//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
		//! See IReferenceCounted::drop() for more information.
		virtual IAnimatedMesh* createMesh(io::IReadFile* file);

		void OCTLoadLights(io::IReadFile* file,
				ISceneNode * parent = 0, f32 radius = 500.0f,
				f32 intensityScale = 0.0000001f*2.5,
				bool rewind = true);

	private:
		struct octHeader {
			u32 numVerts;
			u32 numFaces;
			u32 numTextures;
			u32 numLightmaps;
			u32 numLights;
		};

		struct octHeaderEx {
			u32 magic; // 'OCTX' - 0x4F435458L
			u32 numLightmaps;
			u32 lightmapWidth;
			u32 lightmapHeight;
			u32 containsVertexNormals;
		};

		struct octFace {
			u32 firstVert;
			u32 numVerts;
			u32 textureID;
			u32 lightmapID;
			f32 plane[4];
		};

		struct octVert {
			f32 tc[2];
			f32 lc[2];
			f32 pos[3];
		};

		struct octTexture {
			u32 id;
			char fileName[64];
		};

		struct octLightmap {
			u32 id;
			u8 data[128][128][3];
		};

		struct octLight {
			f32 pos[3];
			f32 color[3];
			u32 intensity;
		};

		ISceneManager* SceneManager;
		io::IFileSystem* FileSystem;
	};

} // end namespace scene
} // end namespace irr

#endif


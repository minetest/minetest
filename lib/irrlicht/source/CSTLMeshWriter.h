// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_STL_MESH_WRITER_H_INCLUDED__
#define __IRR_STL_MESH_WRITER_H_INCLUDED__

#include "IMeshWriter.h"
#include "S3DVertex.h"
#include "irrString.h"

namespace irr
{
namespace scene
{
	class IMeshBuffer;
	class ISceneManager;

	//! class to write meshes, implementing a STL writer
	class CSTLMeshWriter : public IMeshWriter
	{
	public:

		CSTLMeshWriter(scene::ISceneManager* smgr);
		virtual ~CSTLMeshWriter();

		//! Returns the type of the mesh writer
		virtual EMESH_WRITER_TYPE getType() const;

		//! writes a mesh 
		virtual bool writeMesh(io::IWriteFile* file, scene::IMesh* mesh, s32 flags=EMWF_NONE);

	protected:
		// write binary format
		bool writeMeshBinary(io::IWriteFile* file, scene::IMesh* mesh, s32 flags);

		// write text format
		bool writeMeshASCII(io::IWriteFile* file, scene::IMesh* mesh, s32 flags);

		// create vector output with line end into string
		void getVectorAsStringLine(const core::vector3df& v,
				core::stringc& s) const;

		// write face information to file
		void writeFace(io::IWriteFile* file, const core::vector3df& v1,
				const core::vector3df& v2, const core::vector3df& v3);

		scene::ISceneManager* SceneManager;
	};

} // end namespace
} // end namespace

#endif


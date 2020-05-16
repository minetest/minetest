// Copyright (C) 2009-2012 Gaz Davidson
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_PLY_MESH_WRITER_H_INCLUDED__
#define __IRR_PLY_MESH_WRITER_H_INCLUDED__

#include "IMeshWriter.h"

namespace irr
{

namespace scene
{
	class IMeshBuffer;

	//! class to write PLY mesh files
	class CPLYMeshWriter : public IMeshWriter
	{
	public:

		CPLYMeshWriter();

		//! Returns the type of the mesh writer
		virtual EMESH_WRITER_TYPE getType() const;

		//! writes a mesh 
		virtual bool writeMesh(io::IWriteFile* file, scene::IMesh* mesh, s32 flags=EMWF_NONE);

	};

} // end namespace
} // end namespace

#endif

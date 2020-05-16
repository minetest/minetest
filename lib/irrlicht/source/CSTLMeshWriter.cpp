// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_STL_WRITER_

#include "CSTLMeshWriter.h"
#include "os.h"
#include "IMesh.h"
#include "IMeshBuffer.h"
#include "IAttributes.h"
#include "ISceneManager.h"
#include "IMeshCache.h"
#include "IWriteFile.h"

namespace irr
{
namespace scene
{

CSTLMeshWriter::CSTLMeshWriter(scene::ISceneManager* smgr)
	: SceneManager(smgr)
{
	#ifdef _DEBUG
	setDebugName("CSTLMeshWriter");
	#endif

	if (SceneManager)
		SceneManager->grab();
}


CSTLMeshWriter::~CSTLMeshWriter()
{
	if (SceneManager)
		SceneManager->drop();
}


//! Returns the type of the mesh writer
EMESH_WRITER_TYPE CSTLMeshWriter::getType() const
{
	return EMWT_STL;
}


//! writes a mesh
bool CSTLMeshWriter::writeMesh(io::IWriteFile* file, scene::IMesh* mesh, s32 flags)
{
	if (!file)
		return false;

	os::Printer::log("Writing mesh", file->getFileName());

	if (flags & scene::EMWF_WRITE_COMPRESSED)
		return writeMeshBinary(file, mesh, flags);
	else
		return writeMeshASCII(file, mesh, flags);
}


bool CSTLMeshWriter::writeMeshBinary(io::IWriteFile* file, scene::IMesh* mesh, s32 flags)
{
	// write STL MESH header

	file->write("binary ",7);
	const core::stringc name(SceneManager->getMeshCache()->getMeshName(mesh));
	const s32 sizeleft = 73-name.size(); // 80 byte header
	if (sizeleft<0)
		file->write(name.c_str(),73);
	else
	{
		char* buf = new char[80];
		memset(buf, 0, 80);
		file->write(name.c_str(),name.size());
		file->write(buf,sizeleft);
		delete [] buf;
	}
	u32 facenum = 0;
	for (u32 j=0; j<mesh->getMeshBufferCount(); ++j)
		facenum += mesh->getMeshBuffer(j)->getIndexCount()/3;
	file->write(&facenum,4);

	// write mesh buffers

	for (u32 i=0; i<mesh->getMeshBufferCount(); ++i)
	{
		IMeshBuffer* buffer = mesh->getMeshBuffer(i);
		if (buffer)
		{
			const u32 indexCount = buffer->getIndexCount();
			const u16 attributes = 0;
			for (u32 j=0; j<indexCount; j+=3)
			{
				const core::vector3df& v1 = buffer->getPosition(buffer->getIndices()[j]);
				const core::vector3df& v2 = buffer->getPosition(buffer->getIndices()[j+1]);
				const core::vector3df& v3 = buffer->getPosition(buffer->getIndices()[j+2]);
				const core::plane3df tmpplane(v1,v2,v3);
				file->write(&tmpplane.Normal, 12);
				file->write(&v1, 12);
				file->write(&v2, 12);
				file->write(&v3, 12);
				file->write(&attributes, 2);
			}
		}
	}
	return true;
}


bool CSTLMeshWriter::writeMeshASCII(io::IWriteFile* file, scene::IMesh* mesh, s32 flags)
{
	// write STL MESH header

	file->write("solid ",6);
	const core::stringc name(SceneManager->getMeshCache()->getMeshName(mesh));
	file->write(name.c_str(),name.size());
	file->write("\n\n",2);

	// write mesh buffers

	for (u32 i=0; i<mesh->getMeshBufferCount(); ++i)
	{
		IMeshBuffer* buffer = mesh->getMeshBuffer(i);
		if (buffer)
		{
			const u32 indexCount = buffer->getIndexCount();
			for (u32 j=0; j<indexCount; j+=3)
			{
				writeFace(file,
					buffer->getPosition(buffer->getIndices()[j]),
					buffer->getPosition(buffer->getIndices()[j+1]),
					buffer->getPosition(buffer->getIndices()[j+2]));
			}
			file->write("\n",1);
		}
	}

	file->write("endsolid ",9);
	file->write(name.c_str(),name.size());

	return true;
}


void CSTLMeshWriter::getVectorAsStringLine(const core::vector3df& v, core::stringc& s) const
{
	s = core::stringc(v.X);
	s += " ";
	s += core::stringc(v.Y);
	s += " ";
	s += core::stringc(v.Z);
	s += "\n";
}


void CSTLMeshWriter::writeFace(io::IWriteFile* file,
		const core::vector3df& v1,
		const core::vector3df& v2,
		const core::vector3df& v3)
{
	core::stringc tmp;
	file->write("facet normal ",13);
	getVectorAsStringLine(core::plane3df(v1,v2,v3).Normal, tmp);
	file->write(tmp.c_str(),tmp.size());
	file->write("  outer loop\n",13);
	file->write("    vertex ",11);
	getVectorAsStringLine(v1, tmp);
	file->write(tmp.c_str(),tmp.size());
	file->write("    vertex ",11);
	getVectorAsStringLine(v2, tmp);
	file->write(tmp.c_str(),tmp.size());
	file->write("    vertex ",11);
	getVectorAsStringLine(v3, tmp);
	file->write(tmp.c_str(),tmp.size());
	file->write("  endloop\n",10);
	file->write("endfacet\n",9);
}


} // end namespace
} // end namespace

#endif


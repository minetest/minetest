// Copyright (C) 2008-2012 Christian Stehno
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_OBJ_WRITER_

#include "COBJMeshWriter.h"
#include "os.h"
#include "IMesh.h"
#include "IMeshBuffer.h"
#include "IAttributes.h"
#include "ISceneManager.h"
#include "IMeshCache.h"
#include "IWriteFile.h"
#include "IFileSystem.h"
#include "ITexture.h"

namespace irr
{
namespace scene
{

COBJMeshWriter::COBJMeshWriter(scene::ISceneManager* smgr, io::IFileSystem* fs)
	: SceneManager(smgr), FileSystem(fs)
{
	#ifdef _DEBUG
	setDebugName("COBJMeshWriter");
	#endif

	if (SceneManager)
		SceneManager->grab();

	if (FileSystem)
		FileSystem->grab();
}


COBJMeshWriter::~COBJMeshWriter()
{
	if (SceneManager)
		SceneManager->drop();

	if (FileSystem)
		FileSystem->drop();
}


//! Returns the type of the mesh writer
EMESH_WRITER_TYPE COBJMeshWriter::getType() const
{
	return EMWT_OBJ;
}


//! writes a mesh
bool COBJMeshWriter::writeMesh(io::IWriteFile* file, scene::IMesh* mesh, s32 flags)
{
	if (!file)
		return false;

	os::Printer::log("Writing mesh", file->getFileName());

	// write OBJ MESH header

	const core::stringc name(FileSystem->getFileBasename(SceneManager->getMeshCache()->getMeshName(mesh), false)+".mtl");
	file->write("# exported by Irrlicht\n",23);
	file->write("mtllib ",7);
	file->write(name.c_str(),name.size());
	file->write("\n\n",2);

	// write mesh buffers

	core::array<video::SMaterial*> mat;

	u32 allVertexCount=1; // count vertices over the whole file
	for (u32 i=0; i<mesh->getMeshBufferCount(); ++i)
	{
		core::stringc num(i+1);
		IMeshBuffer* buffer = mesh->getMeshBuffer(i);
		if (buffer && buffer->getVertexCount())
		{
			file->write("g grp", 5);
			file->write(num.c_str(), num.size());
			file->write("\n",1);

			u32 j;
			const u32 vertexCount = buffer->getVertexCount();
			for (j=0; j<vertexCount; ++j)
			{
				file->write("v ",2);
				getVectorAsStringLine(buffer->getPosition(j), num);
				file->write(num.c_str(), num.size());
			}

			for (j=0; j<vertexCount; ++j)
			{
				file->write("vt ",3);
				getVectorAsStringLine(buffer->getTCoords(j), num);
				file->write(num.c_str(), num.size());
			}

			for (j=0; j<vertexCount; ++j)
			{
				file->write("vn ",3);
				getVectorAsStringLine(buffer->getNormal(j), num);
				file->write(num.c_str(), num.size());
			}

			file->write("usemtl mat",10);
			num = "";
			for (j=0; j<mat.size(); ++j)
			{
				if (*mat[j]==buffer->getMaterial())
				{
					num = core::stringc(j);
					break;
				}
			}
			if (num == "")
			{
				num = core::stringc(mat.size());
				mat.push_back(&buffer->getMaterial());
			}
			file->write(num.c_str(), num.size());
			file->write("\n",1);

			const u32 indexCount = buffer->getIndexCount();
			for (j=0; j<indexCount; j+=3)
			{
				file->write("f ",2);
				num = core::stringc(buffer->getIndices()[j+2]+allVertexCount);
				file->write(num.c_str(), num.size());
				file->write("/",1);
				file->write(num.c_str(), num.size());
				file->write("/",1);
				file->write(num.c_str(), num.size());
				file->write(" ",1);

				num = core::stringc(buffer->getIndices()[j+1]+allVertexCount);
				file->write(num.c_str(), num.size());
				file->write("/",1);
				file->write(num.c_str(), num.size());
				file->write("/",1);
				file->write(num.c_str(), num.size());
				file->write(" ",1);

				num = core::stringc(buffer->getIndices()[j+0]+allVertexCount);
				file->write(num.c_str(), num.size());
				file->write("/",1);
				file->write(num.c_str(), num.size());
				file->write("/",1);
				file->write(num.c_str(), num.size());
				file->write(" ",1);

				file->write("\n",1);
			}
			file->write("\n",1);
			allVertexCount += vertexCount;
		}
	}

	if (mat.size() == 0)
		return true;

	file = FileSystem->createAndWriteFile( name );
	if (file)
	{
		os::Printer::log("Writing material", file->getFileName());

		file->write("# exported by Irrlicht\n\n",24);
		for (u32 i=0; i<mat.size(); ++i)
		{
			core::stringc num(i);
			file->write("newmtl mat",10);
			file->write(num.c_str(),num.size());
			file->write("\n",1);

			getColorAsStringLine(mat[i]->AmbientColor, "Ka", num);
			file->write(num.c_str(),num.size());
			getColorAsStringLine(mat[i]->DiffuseColor, "Kd", num);
			file->write(num.c_str(),num.size());
			getColorAsStringLine(mat[i]->SpecularColor, "Ks", num);
			file->write(num.c_str(),num.size());
			getColorAsStringLine(mat[i]->EmissiveColor, "Ke", num);
			file->write(num.c_str(),num.size());
			num = core::stringc((double)(mat[i]->Shininess/0.128f));
			file->write("Ns ", 3);
			file->write(num.c_str(),num.size());
			file->write("\n", 1);
			if (mat[i]->getTexture(0))
			{
				file->write("map_Kd ", 7);
				file->write(mat[i]->getTexture(0)->getName().getPath().c_str(), mat[i]->getTexture(0)->getName().getPath().size());
				file->write("\n",1);
			}
			file->write("\n",1);
		}
		file->drop();
	}
	return true;
}


void COBJMeshWriter::getVectorAsStringLine(const core::vector3df& v, core::stringc& s) const
{
	s = core::stringc(-v.X);
	s += " ";
	s += core::stringc(v.Y);
	s += " ";
	s += core::stringc(v.Z);
	s += "\n";
}


void COBJMeshWriter::getVectorAsStringLine(const core::vector2df& v, core::stringc& s) const
{
	s = core::stringc(v.X);
	s += " ";
	s += core::stringc(-v.Y);
	s += "\n";
}


void COBJMeshWriter::getColorAsStringLine(const video::SColor& color, const c8* const prefix, core::stringc& s) const
{
	s = prefix;
	s += " ";
	s += core::stringc((double)(color.getRed()/255.f));
	s += " ";
	s += core::stringc((double)(color.getGreen()/255.f));
	s += " ";
	s += core::stringc((double)(color.getBlue()/255.f));
	s += "\n";
}


} // end namespace
} // end namespace

#endif


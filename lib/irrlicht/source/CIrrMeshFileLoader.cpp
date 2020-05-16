// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_IRR_MESH_LOADER_

#include "CIrrMeshFileLoader.h"
#include "os.h"
#include "IXMLReader.h"
#include "SAnimatedMesh.h"
#include "fast_atof.h"
#include "IReadFile.h"
#include "IAttributes.h"
#include "IMeshSceneNode.h"
#include "CDynamicMeshBuffer.h"
#include "SMeshBufferLightMap.h"

namespace irr
{
namespace scene
{


//! Constructor
CIrrMeshFileLoader::CIrrMeshFileLoader(scene::ISceneManager* smgr,
		io::IFileSystem* fs)
	: SceneManager(smgr), FileSystem(fs)
{

	#ifdef _DEBUG
	setDebugName("CIrrMeshFileLoader");
	#endif

}


//! Returns true if the file maybe is able to be loaded by this class.
/** This decision should be based only on the file extension (e.g. ".cob") */
bool CIrrMeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "xml", "irrmesh" );
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* CIrrMeshFileLoader::createMesh(io::IReadFile* file)
{
	io::IXMLReader* reader = FileSystem->createXMLReader(file);
	if (!reader)
		return 0;

	// read until mesh section, skip other parts

	const core::stringc meshTagName = "mesh";
	IAnimatedMesh* mesh = 0;

	while(reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT)
		{
			if (meshTagName == reader->getNodeName())
			{
				mesh = readMesh(reader);
				break;
			}
			else
				skipSection(reader, true); // unknown section
		}
	}

	reader->drop();

	return mesh;
}


//! reads a mesh sections and creates a mesh from it
IAnimatedMesh* CIrrMeshFileLoader::readMesh(io::IXMLReader* reader)
{
	SAnimatedMesh* animatedmesh = new SAnimatedMesh();
	SMesh* mesh = new SMesh();

	animatedmesh->addMesh(mesh);
	mesh->drop();

	core::stringc bbSectionName = "boundingBox";
	core::stringc bufferSectionName = "buffer";
	core::stringc meshSectionName = "mesh";

	if (!reader->isEmptyElement())
	while(reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT)
		{
			const wchar_t* nodeName = reader->getNodeName();
			if (bbSectionName == nodeName)
			{
				// inside a bounding box, ignore it for now because
				// we are calculating this anyway ourselves later.
			}
			else
			if (bufferSectionName == nodeName)
			{
				// we've got a mesh buffer

				IMeshBuffer* buffer = readMeshBuffer(reader);
				if (buffer)
				{
					mesh->addMeshBuffer(buffer);
					buffer->drop();
				}
			}
			else
				skipSection(reader, true); // unknown section

		} // end if node type is element
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
		{
			if (meshSectionName == reader->getNodeName())
			{
				// end of mesh section reached, cancel out
				break;
			}
		}
	} // end while reader->read();

	mesh->recalculateBoundingBox();
	animatedmesh->recalculateBoundingBox();

	return animatedmesh;
}


//! reads a mesh sections and creates a mesh buffer from it
IMeshBuffer* CIrrMeshFileLoader::readMeshBuffer(io::IXMLReader* reader)
{
	CDynamicMeshBuffer* buffer = 0;

	core::stringc verticesSectionName = "vertices";
	core::stringc bbSectionName = "boundingBox";
	core::stringc materialSectionName = "material";
	core::stringc indicesSectionName = "indices";
	core::stringc bufferSectionName = "buffer";

	bool insideVertexSection = false;
	bool insideIndexSection = false;

	int vertexCount = 0;
	int indexCount = 0;

	video::SMaterial material;

	if (!reader->isEmptyElement())
	while(reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT)
		{
			const wchar_t* nodeName = reader->getNodeName();
			if (bbSectionName == nodeName)
			{
				// inside a bounding box, ignore it for now because
				// we are calculating this anyway ourselves later.
			}
			else
			if (materialSectionName == nodeName)
			{
				//we've got a material

				io::IAttributes* attributes = FileSystem->createEmptyAttributes(SceneManager->getVideoDriver());
				attributes->read(reader, true, L"material");

				SceneManager->getVideoDriver()->fillMaterialStructureFromAttributes(material, attributes);
				attributes->drop();
			}
			else
			if (verticesSectionName == nodeName)
			{
				// vertices section

				const core::stringc vertexTypeName1 = "standard";
				const core::stringc vertexTypeName2 = "2tcoords";
				const core::stringc vertexTypeName3 = "tangents";

				const wchar_t* vertexType = reader->getAttributeValue(L"type");
				vertexCount = reader->getAttributeValueAsInt(L"vertexCount");

				insideVertexSection = true;

				video::E_INDEX_TYPE itype = (vertexCount > 65536)?irr::video::EIT_32BIT:irr::video::EIT_16BIT;
				if (vertexTypeName1 == vertexType)
				{
					buffer = new CDynamicMeshBuffer(irr::video::EVT_STANDARD, itype);

				}
				else
				if (vertexTypeName2 == vertexType)
				{
					buffer = new CDynamicMeshBuffer(irr::video::EVT_2TCOORDS, itype);
				}
				else
				if (vertexTypeName3 == vertexType)
				{
					buffer = new CDynamicMeshBuffer(irr::video::EVT_TANGENTS, itype);
				}
				buffer->getVertexBuffer().reallocate(vertexCount);
				buffer->Material = material;
			}
			else
			if (indicesSectionName == nodeName)
			{
				// indices section

				indexCount = reader->getAttributeValueAsInt(L"indexCount");
				insideIndexSection = true;
			}

		} // end if node type is element
		else
		if (reader->getNodeType() == io::EXN_TEXT)
		{
			// read vertex data
			if (insideVertexSection)
			{
				readMeshBuffer(reader, vertexCount, buffer);
				insideVertexSection = false;

			} // end reading vertex array
			else
			if (insideIndexSection)
			{
				readIndices(reader, indexCount, buffer->getIndexBuffer());
				insideIndexSection = false;
			}

		} // end if node type is text
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
		{
			if (bufferSectionName == reader->getNodeName())
			{
				// end of buffer section reached, cancel out
				break;
			}
		}
	} // end while reader->read();

	if (buffer)
		buffer->recalculateBoundingBox();

	return buffer;
}


//! read indices
void CIrrMeshFileLoader::readIndices(io::IXMLReader* reader, int indexCount, IIndexBuffer& indices)
{
	indices.reallocate(indexCount);

	core::stringc data = reader->getNodeData();
	const c8* p = &data[0];

	for (int i=0; i<indexCount && *p; ++i)
	{
		findNextNoneWhiteSpace(&p);
		indices.push_back(readInt(&p));
	}
}


void CIrrMeshFileLoader::readMeshBuffer(io::IXMLReader* reader, int vertexCount, CDynamicMeshBuffer* sbuffer)
{
	core::stringc data = reader->getNodeData();
	const c8* p = &data[0];
	scene::IVertexBuffer& Vertices = sbuffer->getVertexBuffer();
	video::E_VERTEX_TYPE vType = Vertices.getType();

	if (sbuffer)
	{
		for (int i=0; i<vertexCount && *p; ++i)
		{
			switch(vType)
			{
			case video::EVT_STANDARD:
			{
				video::S3DVertex vtx;
				// position

				findNextNoneWhiteSpace(&p);
				vtx.Pos.X = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Pos.Y = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Pos.Z = readFloat(&p);

				// normal

				findNextNoneWhiteSpace(&p);
				vtx.Normal.X = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Normal.Y = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Normal.Z = readFloat(&p);

				// color

				u32 col;
				findNextNoneWhiteSpace(&p);
				sscanf(p, "%08x", &col);
				vtx.Color.set(col);
				skipCurrentNoneWhiteSpace(&p);

				// tcoord1

				findNextNoneWhiteSpace(&p);
				vtx.TCoords.X = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.TCoords.Y = readFloat(&p);

				Vertices.push_back(vtx);
			}
			break;
			case video::EVT_2TCOORDS:
			{
				video::S3DVertex2TCoords vtx;
				// position

				findNextNoneWhiteSpace(&p);
				vtx.Pos.X = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Pos.Y = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Pos.Z = readFloat(&p);

				// normal

				findNextNoneWhiteSpace(&p);
				vtx.Normal.X = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Normal.Y = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Normal.Z = readFloat(&p);

				// color

				u32 col;
				findNextNoneWhiteSpace(&p);
				sscanf(p, "%08x", &col);
				vtx.Color.set(col);
				skipCurrentNoneWhiteSpace(&p);

				// tcoord1

				findNextNoneWhiteSpace(&p);
				vtx.TCoords.X = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.TCoords.Y = readFloat(&p);

				// tcoord2

				findNextNoneWhiteSpace(&p);
				vtx.TCoords2.X = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.TCoords2.Y = readFloat(&p);

				Vertices.push_back(vtx);
			}
			break;

			case video::EVT_TANGENTS:
			{
				video::S3DVertexTangents vtx;
				// position

				findNextNoneWhiteSpace(&p);
				vtx.Pos.X = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Pos.Y = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Pos.Z = readFloat(&p);

				// normal

				findNextNoneWhiteSpace(&p);
				vtx.Normal.X = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Normal.Y = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Normal.Z = readFloat(&p);

				// color

				u32 col;
				findNextNoneWhiteSpace(&p);
				sscanf(p, "%08x", &col);
				vtx.Color.set(col);
				skipCurrentNoneWhiteSpace(&p);

				// tcoord1

				findNextNoneWhiteSpace(&p);
				vtx.TCoords.X = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.TCoords.Y = readFloat(&p);

				// tangent

				findNextNoneWhiteSpace(&p);
				vtx.Tangent.X = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Tangent.Y = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Tangent.Z = readFloat(&p);

				// binormal

				findNextNoneWhiteSpace(&p);
				vtx.Binormal.X = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Binormal.Y = readFloat(&p);
				findNextNoneWhiteSpace(&p);
				vtx.Binormal.Z = readFloat(&p);

				Vertices.push_back(vtx);
			}
			break;
			};

		}
	}
}


//! skips an (unknown) section in the irrmesh document
void CIrrMeshFileLoader::skipSection(io::IXMLReader* reader, bool reportSkipping)
{
#ifdef _DEBUG
	os::Printer::log("irrMesh skipping section", core::stringc(reader->getNodeName()).c_str());
#endif

	// skip if this element is empty anyway.
	if (reader->isEmptyElement())
		return;

	// read until we've reached the last element in this section
	u32 tagCounter = 1;

	while(tagCounter && reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT &&
			!reader->isEmptyElement())
		{
			#ifdef _DEBUG
			if (reportSkipping)
				os::Printer::log("irrMesh unknown element:", core::stringc(reader->getNodeName()).c_str());
			#endif

			++tagCounter;
		}
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
			--tagCounter;
	}
}


//! parses a float from a char pointer and moves the pointer
//! to the end of the parsed float
inline f32 CIrrMeshFileLoader::readFloat(const c8** p)
{
	f32 ftmp;
	*p = core::fast_atof_move(*p, ftmp);
	return ftmp;
}


//! parses an int from a char pointer and moves the pointer to
//! the end of the parsed float
inline s32 CIrrMeshFileLoader::readInt(const c8** p)
{
	return (s32)readFloat(p);
}


//! places pointer to next begin of a token
void CIrrMeshFileLoader::skipCurrentNoneWhiteSpace(const c8** start)
{
	const c8* p = *start;

	while(*p && !(*p==' ' || *p=='\n' || *p=='\r' || *p=='\t'))
		++p;

	// TODO: skip comments <!-- -->

	*start = p;
}

//! places pointer to next begin of a token
void CIrrMeshFileLoader::findNextNoneWhiteSpace(const c8** start)
{
	const c8* p = *start;

	while(*p && (*p==' ' || *p=='\n' || *p=='\r' || *p=='\t'))
		++p;

	// TODO: skip comments <!-- -->

	*start = p;
}


//! reads floats from inside of xml element until end of xml element
void CIrrMeshFileLoader::readFloatsInsideElement(io::IXMLReader* reader, f32* floats, u32 count)
{
	if (reader->isEmptyElement())
		return;

	while(reader->read())
	{
		// TODO: check for comments inside the element
		// and ignore them.

		if (reader->getNodeType() == io::EXN_TEXT)
		{
			// parse float data
			core::stringc data = reader->getNodeData();
			const c8* p = &data[0];

			for (u32 i=0; i<count; ++i)
			{
				findNextNoneWhiteSpace(&p);
				if (*p)
					floats[i] = readFloat(&p);
				else
					floats[i] = 0.0f;
			}
		}
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
			break; // end parsing text
	}
}




} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_IRR_MESH_LOADER_

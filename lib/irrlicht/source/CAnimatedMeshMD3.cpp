// Copyright (C) 2002-2012 Nikolaus Gebhardt / Fabio Concas / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_MD3_LOADER_

#include "CAnimatedMeshMD3.h"
#include "os.h"

namespace irr
{
namespace scene
{


// byte-align structures
#include "irrpack.h"

//! General properties of a single animation frame.
struct SMD3Frame
{
	f32  mins[3];		// bounding box per frame
	f32  maxs[3];
	f32  position[3];	// position of bounding box
	f32  radius;		// radius of bounding sphere
	c8   creator[16];	// name of frame
} PACK_STRUCT;


//! An attachment point for another MD3 model.
struct SMD3Tag
{
	c8 Name[64];		//name of 'tag' as it's usually called in the md3 files try to see it as a sub-mesh/seperate mesh-part.
	f32 position[3];	//relative position of tag
	f32 rotationMatrix[9];	//3x3 rotation direction of tag
} PACK_STRUCT;

//!Shader
struct SMD3Shader
{
	c8 name[64];		// name of shader
	s32 shaderIndex;
} PACK_STRUCT;

// Default alignment
#include "irrunpack.h"


//! Constructor
CAnimatedMeshMD3::CAnimatedMeshMD3()
:Mesh(0), IPolShift(0), LoopMode(0), Scaling(1.f)//, FramesPerSecond(25.f)
{
#ifdef _DEBUG
	setDebugName("CAnimatedMeshMD3");
#endif

	Mesh = new SMD3Mesh();
	MeshIPol = new SMesh();
	setInterpolationShift(0, 0);
}


//! Destructor
CAnimatedMeshMD3::~CAnimatedMeshMD3()
{
	if (Mesh)
		Mesh->drop();
	if (MeshIPol)
		MeshIPol->drop();
}


//! Returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.
u32 CAnimatedMeshMD3::getFrameCount() const
{
	return Mesh->MD3Header.numFrames << IPolShift;
}


//! Rendering Hint
void CAnimatedMeshMD3::setInterpolationShift(u32 shift, u32 loopMode)
{
	IPolShift = shift;
	LoopMode = loopMode;
}


//! returns amount of mesh buffers.
u32 CAnimatedMeshMD3::getMeshBufferCount() const
{
	return MeshIPol->getMeshBufferCount();
}


//! returns pointer to a mesh buffer
IMeshBuffer* CAnimatedMeshMD3::getMeshBuffer(u32 nr) const
{
	return MeshIPol->getMeshBuffer(nr);
}


//! Returns pointer to a mesh buffer which fits a material
IMeshBuffer* CAnimatedMeshMD3::getMeshBuffer(const video::SMaterial &material) const
{
	return MeshIPol->getMeshBuffer(material);
}


void CAnimatedMeshMD3::setMaterialFlag(video::E_MATERIAL_FLAG flag, bool newvalue)
{
	MeshIPol->setMaterialFlag(flag, newvalue);
}


//! set the hardware mapping hint, for driver
void CAnimatedMeshMD3::setHardwareMappingHint(E_HARDWARE_MAPPING newMappingHint,
		E_BUFFER_TYPE buffer)
{
	MeshIPol->setHardwareMappingHint(newMappingHint, buffer);
}


//! flags the meshbuffer as changed, reloads hardware buffers
void CAnimatedMeshMD3::setDirty(E_BUFFER_TYPE buffer)
{
	MeshIPol->setDirty(buffer);
}


//! set user axis aligned bounding box
void CAnimatedMeshMD3::setBoundingBox(const core::aabbox3df& box)
{
	MeshIPol->setBoundingBox(box);
}


//! Returns the animated tag list based on a detail level. 0 is the lowest, 255 the highest detail.
SMD3QuaternionTagList *CAnimatedMeshMD3::getTagList(s32 frame, s32 detailLevel, s32 startFrameLoop, s32 endFrameLoop)
{
	if (0 == Mesh)
		return 0;

	getMesh(frame, detailLevel, startFrameLoop, endFrameLoop);
	return &TagListIPol;
}


//! Returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail.
IMesh* CAnimatedMeshMD3::getMesh(s32 frame, s32 detailLevel, s32 startFrameLoop, s32 endFrameLoop)
{
	if (0 == Mesh)
		return 0;

	//! check if we have the mesh in our private cache
	SCacheInfo candidate(frame, startFrameLoop, endFrameLoop);
	if (candidate == Current)
		return MeshIPol;

	startFrameLoop = core::s32_max(0, startFrameLoop >> IPolShift);
	endFrameLoop = core::if_c_a_else_b(endFrameLoop < 0, Mesh->MD3Header.numFrames - 1, endFrameLoop >> IPolShift);

	const u32 mask = 1 << IPolShift;

	s32 frameA;
	s32 frameB;
	f32 iPol;

	if (LoopMode)
	{
		// correct frame to "pixel center"
		frame -= mask >> 1;

		// interpolation
		iPol = f32(frame & (mask - 1)) * core::reciprocal(f32(mask));

		// wrap anim
		frame >>= IPolShift;
		frameA = core::if_c_a_else_b(frame < startFrameLoop, endFrameLoop, frame);
		frameB = core::if_c_a_else_b(frameA + 1 > endFrameLoop, startFrameLoop, frameA + 1);
	}
	else
	{
		// correct frame to "pixel center"
		frame -= mask >> 1;

		iPol = f32(frame & (mask - 1)) * core::reciprocal(f32(mask));

		// clamp anim
		frame >>= IPolShift;
		frameA = core::s32_clamp(frame, startFrameLoop, endFrameLoop);
		frameB = core::s32_min(frameA + 1, endFrameLoop);
	}

	// build current vertex
	for (u32 i = 0; i!= Mesh->Buffer.size(); ++i)
	{
		buildVertexArray(frameA, frameB, iPol,
					Mesh->Buffer[i],
					(SMeshBufferLightMap*) MeshIPol->getMeshBuffer(i));
	}
	MeshIPol->recalculateBoundingBox();

	// build current tags
	buildTagArray(frameA, frameB, iPol);

	Current = candidate;
	return MeshIPol;
}


//! create a Irrlicht MeshBuffer for a MD3 MeshBuffer
IMeshBuffer * CAnimatedMeshMD3::createMeshBuffer(const SMD3MeshBuffer* source,
							 io::IFileSystem* fs, video::IVideoDriver * driver)
{
	SMeshBufferLightMap * dest = new SMeshBufferLightMap();
	dest->Vertices.set_used(source->MeshHeader.numVertices);
	dest->Indices.set_used(source->Indices.size());

	u32 i;

	// fill in static face info
	for (i = 0; i < source->Indices.size(); i += 3)
	{
		dest->Indices[i + 0] = (u16) source->Indices[i + 0];
		dest->Indices[i + 1] = (u16) source->Indices[i + 1];
		dest->Indices[i + 2] = (u16) source->Indices[i + 2];
	}

	// fill in static vertex info
	for (i = 0; i!= (u32)source->MeshHeader.numVertices; ++i)
	{
		video::S3DVertex2TCoords &v = dest->Vertices[i];
		v.Color = 0xFFFFFFFF;
		v.TCoords.X = source->Tex[i].u;
		v.TCoords.Y = source->Tex[i].v;
		v.TCoords2.X = 0.f;
		v.TCoords2.Y = 0.f;
	}

	// load static texture
	u32 pos = 0;
	quake3::tTexArray textureArray;
	quake3::getTextures(textureArray, source->Shader, pos, fs, driver);
	dest->Material.MaterialType = video::EMT_SOLID;
	dest->Material.setTexture(0, textureArray[0]);
	dest->Material.Lighting = false;

	return dest;
}


//! build final mesh's vertices from frames frameA and frameB with linear interpolation.
void CAnimatedMeshMD3::buildVertexArray(u32 frameA, u32 frameB, f32 interpolate,
					const SMD3MeshBuffer* source,
					SMeshBufferLightMap* dest)
{
	const u32 frameOffsetA = frameA * source->MeshHeader.numVertices;
	const u32 frameOffsetB = frameB * source->MeshHeader.numVertices;
	const f32 scale = (1.f/ 64.f);

	for (s32 i = 0; i != source->MeshHeader.numVertices; ++i)
	{
		video::S3DVertex2TCoords &v = dest->Vertices [ i ];

		const SMD3Vertex &vA = source->Vertices [ frameOffsetA + i ];
		const SMD3Vertex &vB = source->Vertices [ frameOffsetB + i ];

		// position
		v.Pos.X = scale * (vA.position[0] + interpolate * (vB.position[0] - vA.position[0]));
		v.Pos.Y = scale * (vA.position[2] + interpolate * (vB.position[2] - vA.position[2]));
		v.Pos.Z = scale * (vA.position[1] + interpolate * (vB.position[1] - vA.position[1]));

		// normal
		const core::vector3df nA(quake3::getMD3Normal(vA.normal[0], vA.normal[1]));
		const core::vector3df nB(quake3::getMD3Normal(vB.normal[0], vB.normal[1]));

		v.Normal.X = nA.X + interpolate * (nB.X - nA.X);
		v.Normal.Y = nA.Z + interpolate * (nB.Z - nA.Z);
		v.Normal.Z = nA.Y + interpolate * (nB.Y - nA.Y);
	}

	dest->recalculateBoundingBox();
}


//! build final mesh's tag from frames frameA and frameB with linear interpolation.
void CAnimatedMeshMD3::buildTagArray(u32 frameA, u32 frameB, f32 interpolate)
{
	const u32 frameOffsetA = frameA * Mesh->MD3Header.numTags;
	const u32 frameOffsetB = frameB * Mesh->MD3Header.numTags;

	for (s32 i = 0; i != Mesh->MD3Header.numTags; ++i)
	{
		SMD3QuaternionTag &d = TagListIPol [ i ];

		const SMD3QuaternionTag &qA = Mesh->TagList[ frameOffsetA + i];
		const SMD3QuaternionTag &qB = Mesh->TagList[ frameOffsetB + i];

		// rotation
		d.rotation.slerp(qA.rotation, qB.rotation, interpolate);

		// position
		d.position.X = qA.position.X + interpolate * (qB.position.X - qA.position.X);
		d.position.Y = qA.position.Y + interpolate * (qB.position.Y - qA.position.Y);
		d.position.Z = qA.position.Z + interpolate * (qB.position.Z - qA.position.Z);
	}
}


/*!
	loads a model
*/
bool CAnimatedMeshMD3::loadModelFile(u32 modelIndex, io::IReadFile* file,
			io::IFileSystem* fs, video::IVideoDriver* driver)
{
	if (!file)
		return false;

	//! Check MD3Header
	{
		file->read(&Mesh->MD3Header, sizeof(SMD3Header));

		if (strncmp("IDP3", Mesh->MD3Header.headerID, 4))
		{
			os::Printer::log("MD3 Loader: invalid header");
			return false;
		}
	}

	//! store model name
	Mesh->Name = file->getFileName();

	u32 i;

	//! Frame Data (ignore)
#if 0
	SMD3Frame frameImport;
	file->seek(Mesh->MD3Header.frameStart);
	for (i = 0; i != Mesh->MD3Header.numFrames; ++i)
	{
		file->read(&frameImport, sizeof(frameImport));
	}
#endif

	//! Tag Data
	const u32 totalTags = Mesh->MD3Header.numTags * Mesh->MD3Header.numFrames;

	SMD3Tag import;

	file->seek(Mesh->MD3Header.tagStart);
	Mesh->TagList.set_used(totalTags);
	for (i = 0; i != totalTags; ++i)
	{
		file->read(&import, sizeof(import));

		SMD3QuaternionTag &exp = Mesh->TagList[i];

		//! tag name
		exp.Name = import.Name;

		//! position
		exp.position.X = import.position[0];
		exp.position.Y = import.position[2];
		exp.position.Z = import.position[1];

		//! construct quaternion from a RH 3x3 Matrix
		exp.rotation.set(import.rotationMatrix[7],
					0.f,
					-import.rotationMatrix[6],
					1 + import.rotationMatrix[8]);
		exp.rotation.normalize();
	}

	//! Meshes
	u32 offset = Mesh->MD3Header.tagEnd;

	for (i = 0; i != (u32)Mesh->MD3Header.numMeshes; ++i)
	{
		//! construct a new mesh buffer
		SMD3MeshBuffer * buf = new SMD3MeshBuffer();

		// !read mesh header info
		SMD3MeshHeader &meshHeader = buf->MeshHeader;

		//! read mesh info
		file->seek(offset);
		file->read(&meshHeader, sizeof(SMD3MeshHeader));

		//! prepare memory
		buf->Vertices.set_used(meshHeader.numVertices * Mesh->MD3Header.numFrames);
		buf->Indices.set_used(meshHeader.numTriangles * 3);
		buf->Tex.set_used(meshHeader.numVertices);

		//! read skins (shaders). should be 1 per meshbuffer
		SMD3Shader skin;
		file->seek(offset + buf->MeshHeader.offset_shaders);
		for (s32 g = 0; g != buf->MeshHeader.numShader; ++g)
		{
			file->read(&skin, sizeof(skin));

			io::path name;
			cutFilenameExtension(name, skin.name);
			name.replace('\\', '/');
			buf->Shader = name;
		}

		//! read texture coordinates
		file->seek(offset + buf->MeshHeader.offset_st);
		file->read(buf->Tex.pointer(), buf->MeshHeader.numVertices * sizeof(SMD3TexCoord));

		//! read vertices
		file->seek(offset + meshHeader.vertexStart);
		file->read(buf->Vertices.pointer(), Mesh->MD3Header.numFrames * meshHeader.numVertices * sizeof(SMD3Vertex));

		//! read indices
		file->seek(offset + meshHeader.offset_triangles);
		file->read(buf->Indices.pointer(), meshHeader.numTriangles * sizeof(SMD3Face));

		//! store meshBuffer
		Mesh->Buffer.push_back(buf);

		offset += meshHeader.offset_end;
	}

	// Init Mesh Interpolation
	for (i = 0; i != Mesh->Buffer.size(); ++i)
	{
		IMeshBuffer * buffer = createMeshBuffer(Mesh->Buffer[i], fs, driver);
		MeshIPol->addMeshBuffer(buffer);
		buffer->drop();
	}
	MeshIPol->recalculateBoundingBox();

	// Init Tag Interpolation
	for (i = 0; i != (u32)Mesh->MD3Header.numTags; ++i)
	{
		TagListIPol.push_back(Mesh->TagList[i]);
	}

	return true;
}


SMD3Mesh * CAnimatedMeshMD3::getOriginalMesh()
{
	return Mesh;
}


//! Returns an axis aligned bounding box
const core::aabbox3d<f32>& CAnimatedMeshMD3::getBoundingBox() const
{
	return MeshIPol->BoundingBox;
}


//! Returns the type of the animated mesh.
E_ANIMATED_MESH_TYPE CAnimatedMeshMD3::getMeshType() const
{
	return EAMT_MD3;
}


} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_MD3_LOADER_

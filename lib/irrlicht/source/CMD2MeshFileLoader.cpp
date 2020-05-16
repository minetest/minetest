// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_MD2_LOADER_

#include "CMD2MeshFileLoader.h"
#include "CAnimatedMeshMD2.h"
#include "os.h"

namespace irr
{
namespace scene
{


	// structs needed to load the md2-format

	const s32 MD2_MAGIC_NUMBER  = 844121161;
	const s32 MD2_VERSION       = 8;
	const s32 MD2_MAX_VERTS     = 2048;

// byte-align structures
#include "irrpack.h"

	struct SMD2Header
	{
		s32 magic;           // four character code "IDP2"
		s32 version;         // must be 8
		s32 skinWidth;	     // width of the texture
		s32 skinHeight;      // height of the texture
		s32 frameSize;       // size in bytes of an animation frame
		s32 numSkins;        // number of textures
		s32 numVertices;     // total number of vertices
		s32 numTexcoords;    // number of vertices with texture coords
		s32 numTriangles;    // number of triangles
		s32 numGlCommands;   // number of opengl commands (triangle strip or triangle fan)
		s32 numFrames;       // animation keyframe count
		s32 offsetSkins;     // offset in bytes to 64 character skin names
		s32 offsetTexcoords; // offset in bytes to texture coordinate list
		s32 offsetTriangles; // offset in bytes to triangle list
		s32 offsetFrames;    // offset in bytes to frame list
		s32 offsetGlCommands;// offset in bytes to opengl commands
		s32 offsetEnd;       // offset in bytes to end of file
	} PACK_STRUCT;

	struct SMD2Vertex
	{
		u8 vertex[3];        // [0] = X, [1] = Z, [2] = Y
		u8 lightNormalIndex; // index in the normal table
	} PACK_STRUCT;

	struct SMD2Frame
	{
		f32	scale[3];           // first scale the vertex position
		f32	translate[3];       // then translate the position
		c8	name[16];           // the name of the animation that this key belongs to
		SMD2Vertex vertices[1]; // vertex 1 of SMD2Header.numVertices
	} PACK_STRUCT;

	struct SMD2Triangle
	{
		u16 vertexIndices[3];
		u16 textureIndices[3];
	} PACK_STRUCT;

	struct SMD2TextureCoordinate
	{
		s16 s;
		s16 t;
	} PACK_STRUCT;

	struct SMD2GLCommand
	{
		f32 s, t;
		s32 vertexIndex;
	} PACK_STRUCT;

// Default alignment
#include "irrunpack.h"

//! Constructor
CMD2MeshFileLoader::CMD2MeshFileLoader()
{
	#ifdef _DEBUG
	setDebugName("CMD2MeshFileLoader");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool CMD2MeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "md2" );
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* CMD2MeshFileLoader::createMesh(io::IReadFile* file)
{
	IAnimatedMesh* msh = new CAnimatedMeshMD2();
	if (msh)
	{
		if (loadFile(file, (CAnimatedMeshMD2*)msh) )
			return msh;

		msh->drop();
	}

	return 0;
}

//! loads an md2 file
bool CMD2MeshFileLoader::loadFile(io::IReadFile* file, CAnimatedMeshMD2* mesh)
{
	if (!file)
		return false;

	SMD2Header header;

	file->read(&header, sizeof(SMD2Header));

#ifdef __BIG_ENDIAN__
	header.magic = os::Byteswap::byteswap(header.magic);
	header.version = os::Byteswap::byteswap(header.version);
	header.skinWidth = os::Byteswap::byteswap(header.skinWidth);
	header.skinHeight = os::Byteswap::byteswap(header.skinHeight);
	header.frameSize = os::Byteswap::byteswap(header.frameSize);
	header.numSkins = os::Byteswap::byteswap(header.numSkins);
	header.numVertices = os::Byteswap::byteswap(header.numVertices);
	header.numTexcoords = os::Byteswap::byteswap(header.numTexcoords);
	header.numTriangles = os::Byteswap::byteswap(header.numTriangles);
	header.numGlCommands = os::Byteswap::byteswap(header.numGlCommands);
	header.numFrames = os::Byteswap::byteswap(header.numFrames);
	header.offsetSkins = os::Byteswap::byteswap(header.offsetSkins);
	header.offsetTexcoords = os::Byteswap::byteswap(header.offsetTexcoords);
	header.offsetTriangles = os::Byteswap::byteswap(header.offsetTriangles);
	header.offsetFrames = os::Byteswap::byteswap(header.offsetFrames);
	header.offsetGlCommands = os::Byteswap::byteswap(header.offsetGlCommands);
	header.offsetEnd = os::Byteswap::byteswap(header.offsetEnd);
#endif

	if (header.magic != MD2_MAGIC_NUMBER || header.version != MD2_VERSION)
	{
		os::Printer::log("MD2 Loader: Wrong file header", file->getFileName(), ELL_WARNING);
		return false;
	}

	//
	// prepare mesh and allocate memory
	//

	mesh->FrameCount = header.numFrames;

	// create keyframes
	mesh->FrameTransforms.set_used(header.numFrames);

	// create vertex arrays for each keyframe
	if (mesh->FrameList)
		delete [] mesh->FrameList;
	mesh->FrameList = new core::array<CAnimatedMeshMD2::SMD2Vert>[header.numFrames];

	// allocate space in vertex arrays
	s32 i;
	for (i=0; i<header.numFrames; ++i)
		mesh->FrameList[i].reallocate(header.numVertices);

	// allocate interpolation buffer vertices
	mesh->InterpolationBuffer->Vertices.set_used(header.numTriangles*3);

	// populate triangles
	mesh->InterpolationBuffer->Indices.reallocate(header.numTriangles*3);
	const s32 count = header.numTriangles*3;
	for (i=0; i<count; i+=3)
	{
		mesh->InterpolationBuffer->Indices.push_back(i);
		mesh->InterpolationBuffer->Indices.push_back(i+1);
		mesh->InterpolationBuffer->Indices.push_back(i+2);
	}

	//
	// read texture coordinates
	//

	file->seek(header.offsetTexcoords);
	SMD2TextureCoordinate* textureCoords = new SMD2TextureCoordinate[header.numTexcoords];

	if (!file->read(textureCoords, sizeof(SMD2TextureCoordinate)*header.numTexcoords))
	{
		delete[] textureCoords;
		os::Printer::log("MD2 Loader: Error reading TextureCoords.", file->getFileName(), ELL_ERROR);
		return false;
	}

#ifdef __BIG_ENDIAN__
	for (i=0; i<header.numTexcoords; ++i)
	{
		textureCoords[i].s = os::Byteswap::byteswap(textureCoords[i].s);
		textureCoords[i].t = os::Byteswap::byteswap(textureCoords[i].t);
	}
#endif

	// read Triangles

	file->seek(header.offsetTriangles);

	SMD2Triangle *triangles = new SMD2Triangle[header.numTriangles];
	if (!file->read(triangles, header.numTriangles *sizeof(SMD2Triangle)))
	{
		delete[] triangles;
		delete[] textureCoords;

		os::Printer::log("MD2 Loader: Error reading triangles.", file->getFileName(), ELL_ERROR);
		return false;
	}

#ifdef __BIG_ENDIAN__
	for (i=0; i<header.numTriangles; ++i)
	{
		triangles[i].vertexIndices[0] = os::Byteswap::byteswap(triangles[i].vertexIndices[0]);
		triangles[i].vertexIndices[1] = os::Byteswap::byteswap(triangles[i].vertexIndices[1]);
		triangles[i].vertexIndices[2] = os::Byteswap::byteswap(triangles[i].vertexIndices[2]);
		triangles[i].textureIndices[0] = os::Byteswap::byteswap(triangles[i].textureIndices[0]);
		triangles[i].textureIndices[1] = os::Byteswap::byteswap(triangles[i].textureIndices[1]);
		triangles[i].textureIndices[2] = os::Byteswap::byteswap(triangles[i].textureIndices[2]);
	}
#endif

	// read Vertices

	u8 buffer[MD2_MAX_VERTS*4+128];
	SMD2Frame* frame = (SMD2Frame*)buffer;

	file->seek(header.offsetFrames);

	for (i = 0; i<header.numFrames; ++i)
	{
		// read vertices

		file->read(frame, header.frameSize);

#ifdef __BIG_ENDIAN__
		frame->scale[0] = os::Byteswap::byteswap(frame->scale[0]);
		frame->scale[1] = os::Byteswap::byteswap(frame->scale[1]);
		frame->scale[2] = os::Byteswap::byteswap(frame->scale[2]);
		frame->translate[0] = os::Byteswap::byteswap(frame->translate[0]);
		frame->translate[1] = os::Byteswap::byteswap(frame->translate[1]);
		frame->translate[2] = os::Byteswap::byteswap(frame->translate[2]);
#endif
		//
		// store frame data
		//

		CAnimatedMeshMD2::SAnimationData adata;
		adata.begin = i;
		adata.end = i;
		adata.fps = 7;

		// Add new named animation if necessary
		if (frame->name[0])
		{
			// get animation name
			for (s32 s = 0; s < 16 && frame->name[s]!=0 && (frame->name[s] < '0' || frame->name[s] > '9'); ++s)
			{
				adata.name += frame->name[s];
			}

			// Does this keyframe have the same animation name as the current animation?
			if (!mesh->AnimationData.empty() && mesh->AnimationData[mesh->AnimationData.size()-1].name == adata.name)
			{
				// Increase the length of the animation
				++mesh->AnimationData[mesh->AnimationData.size() - 1].end;
			}
			else
			{
				// Add the new animation
				mesh->AnimationData.push_back(adata);
			}
		}

		// save keyframe scale and translation

		mesh->FrameTransforms[i].scale.X = frame->scale[0];
		mesh->FrameTransforms[i].scale.Z = frame->scale[1];
		mesh->FrameTransforms[i].scale.Y = frame->scale[2];
		mesh->FrameTransforms[i].translate.X = frame->translate[0];
		mesh->FrameTransforms[i].translate.Z = frame->translate[1];
		mesh->FrameTransforms[i].translate.Y = frame->translate[2];

		// add vertices
		for (s32 j=0; j<header.numTriangles; ++j)
		{
			for (u32 ti=0; ti<3; ++ti)
			{
				CAnimatedMeshMD2::SMD2Vert v;
				u32 num = triangles[j].vertexIndices[ti];
				v.Pos.X = frame->vertices[num].vertex[0];
				v.Pos.Z = frame->vertices[num].vertex[1];
				v.Pos.Y = frame->vertices[num].vertex[2];
				v.NormalIdx = frame->vertices[num].lightNormalIndex;

				mesh->FrameList[i].push_back(v);
			}
		}

		// calculate bounding boxes
		if (header.numVertices)
		{
			core::aabbox3d<f32> box;
			core::vector3df pos;
			pos.X = f32(mesh->FrameList[i] [0].Pos.X) * mesh->FrameTransforms[i].scale.X + mesh->FrameTransforms[i].translate.X;
			pos.Y = f32(mesh->FrameList[i] [0].Pos.Y) * mesh->FrameTransforms[i].scale.Y + mesh->FrameTransforms[i].translate.Y;
			pos.Z = f32(mesh->FrameList[i] [0].Pos.Z) * mesh->FrameTransforms[i].scale.Z + mesh->FrameTransforms[i].translate.Z;

			box.reset(pos);

			for (s32 j=1; j<header.numTriangles*3; ++j)
			{
				pos.X = f32(mesh->FrameList[i] [j].Pos.X) * mesh->FrameTransforms[i].scale.X + mesh->FrameTransforms[i].translate.X;
				pos.Y = f32(mesh->FrameList[i] [j].Pos.Y) * mesh->FrameTransforms[i].scale.Y + mesh->FrameTransforms[i].translate.Y;
				pos.Z = f32(mesh->FrameList[i] [j].Pos.Z) * mesh->FrameTransforms[i].scale.Z + mesh->FrameTransforms[i].translate.Z;

				box.addInternalPoint(pos);
			}
			mesh->BoxList.push_back(box);
		}
	}

	// populate interpolation buffer with texture coordinates and colors
	if (header.numFrames)
	{
		f32 dmaxs = 1.0f/(header.skinWidth);
		f32 dmaxt = 1.0f/(header.skinHeight);

		for (s32 t=0; t<header.numTriangles; ++t)
		{
			for (s32 n=0; n<3; ++n)
			{
				mesh->InterpolationBuffer->Vertices[t*3 + n].TCoords.X = (textureCoords[triangles[t].textureIndices[n]].s + 0.5f) * dmaxs;
				mesh->InterpolationBuffer->Vertices[t*3 + n].TCoords.Y = (textureCoords[triangles[t].textureIndices[n]].t + 0.5f) * dmaxt;
				mesh->InterpolationBuffer->Vertices[t*3 + n].Color = video::SColor(255,255,255,255);
			}
		}
	}

	// clean up
	delete [] triangles;
	delete [] textureCoords;

	// init buffer with start frame.
	mesh->getMesh(0);
	return true;
}

} // end namespace scene
} // end namespace irr


#endif // _IRR_COMPILE_WITH_MD2_LOADER_

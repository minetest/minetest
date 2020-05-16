// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
//
// originally written by Murphy McCauley, see COCTLoader.h for details.
//
// COCTLoader by Murphy McCauley (February 2005)
// An Irrlicht loader for OCT files
//
// See the header file for additional information including use and distribution rights.

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_OCT_LOADER_

#include "COCTLoader.h"
#include "IVideoDriver.h"
#include "IFileSystem.h"
#include "os.h"
#include "SAnimatedMesh.h"
#include "SMeshBufferLightMap.h"
#include "irrString.h"
#include "ISceneManager.h"

namespace irr
{
namespace scene
{

//! constructor
COCTLoader::COCTLoader(ISceneManager* smgr, io::IFileSystem* fs)
	: SceneManager(smgr), FileSystem(fs)
{
	#ifdef _DEBUG
	setDebugName("COCTLoader");
	#endif
	if (FileSystem)
		FileSystem->grab();
}


//! destructor
COCTLoader::~COCTLoader()
{
	if (FileSystem)
		FileSystem->drop();
}


// Doesn't really belong here, but it's jammed in for now.
void COCTLoader::OCTLoadLights(io::IReadFile* file, scene::ISceneNode * parent, f32 radius, f32 intensityScale, bool rewind)
{
	if (rewind)
		file->seek(0);

	octHeader header;
	file->read(&header, sizeof(octHeader));

	file->seek(sizeof(octVert)*header.numVerts, true);
	file->seek(sizeof(octFace)*header.numFaces, true);
	file->seek(sizeof(octTexture)*header.numTextures, true);
	file->seek(sizeof(octLightmap)*header.numLightmaps, true);

	octLight * lights = new octLight[header.numLights];
	file->read(lights, header.numLights * sizeof(octLight));

	//TODO: Skip past my extended data just for good form

	for (u32 i = 0; i < header.numLights; i++)
	{
		const f32 intensity = lights[i].intensity * intensityScale;

		SceneManager->addLightSceneNode(parent, core::vector3df(lights[i].pos[0], lights[i].pos[2], lights[i].pos[1]),
			video::SColorf(lights[i].color[0] * intensity, lights[i].color[1] * intensity, lights[i].color[2] * intensity, 1.0f),
			radius);
	}
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* COCTLoader::createMesh(io::IReadFile* file)
{
	if (!file)
		return 0;

	octHeader header;
	file->read(&header, sizeof(octHeader));

	octVert * verts = new octVert[header.numVerts];
	octFace * faces = new octFace[header.numFaces];
	octTexture * textures = new octTexture[header.numTextures];
	octLightmap * lightmaps = new octLightmap[header.numLightmaps];
	octLight * lights = new octLight[header.numLights];

	file->read(verts, sizeof(octVert) * header.numVerts);
	file->read(faces, sizeof(octFace) * header.numFaces);
	//TODO: Make sure id is in the legal range for Textures and Lightmaps

	u32 i;
	for (i = 0; i < header.numTextures; i++) {
		octTexture t;
		file->read(&t, sizeof(octTexture));
		textures[t.id] = t;
	}
	for (i = 0; i < header.numLightmaps; i++) {
		octLightmap t;
		file->read(&t, sizeof(octLightmap));
		lightmaps[t.id] = t;
	}
	file->read(lights, sizeof(octLight) * header.numLights);

	//TODO: Now read in my extended OCT header (flexible lightmaps and vertex normals)


	// This is the method Nikolaus Gebhardt used in the Q3 loader -- create a
	// meshbuffer for every possible combination of lightmap and texture including
	// a "null" texture and "null" lightmap.  Ones that end up with nothing in them
	// will be removed later.

	SMesh * Mesh = new SMesh();
	for (i=0; i<(header.numTextures+1) * (header.numLightmaps+1); ++i)
	{
		scene::SMeshBufferLightMap* buffer = new scene::SMeshBufferLightMap();

		buffer->Material.MaterialType = video::EMT_LIGHTMAP;
		buffer->Material.Lighting = false;
		Mesh->addMeshBuffer(buffer);
		buffer->drop();
	}


	// Build the mesh buffers
	for (i = 0; i < header.numFaces; i++)
	{
		if (faces[i].numVerts < 3)
			continue;

		const f32* const a = verts[faces[i].firstVert].pos;
		const f32* const b = verts[faces[i].firstVert+1].pos;
		const f32* const c = verts[faces[i].firstVert+2].pos;
		const core::vector3df normal =
			core::plane3df(core::vector3df(a[0],a[1],a[2]), core::vector3df(b[0],c[1],c[2]), core::vector3df(c[0],c[1],c[2])).Normal;

		const u32 textureID = core::min_(s32(faces[i].textureID), s32(header.numTextures - 1)) + 1;
		const u32 lightmapID = core::min_(s32(faces[i].lightmapID),s32(header.numLightmaps - 1)) + 1;
		SMeshBufferLightMap * meshBuffer = (SMeshBufferLightMap*)Mesh->getMeshBuffer(lightmapID * (header.numTextures + 1) + textureID);
		const u32 base = meshBuffer->Vertices.size();

		// Add this face's verts
		u32 v;
		for (v = 0; v < faces[i].numVerts; ++v)
		{
			octVert * vv = &verts[faces[i].firstVert + v];
			video::S3DVertex2TCoords vert;
			vert.Pos.set(vv->pos[0], vv->pos[1], vv->pos[2]);
			vert.Color = video::SColor(0,255,255,255);
			vert.Normal.set(normal);

			if (textureID == 0)
			{
				// No texture -- just a lightmap.  Thus, use lightmap coords for texture 1.
				// (the actual texture will be swapped later)
				vert.TCoords.set(vv->lc[0], vv->lc[1]);
			}
			else
			{
				vert.TCoords.set(vv->tc[0], vv->tc[1]);
				vert.TCoords2.set(vv->lc[0], vv->lc[1]);
			}

			meshBuffer->Vertices.push_back(vert);
		}

		// Now add the indices
		// This weird loop turns convex polygons into triangle strips.
		// I do it this way instead of a simple fan because it usually looks a lot better in wireframe, for example.
		// High, Low
		u32 h = faces[i].numVerts - 1;
		u32 l = 0;
		for (v = 0; v < faces[i].numVerts - 2; ++v)
		{
			const u32 center = (v & 1)? h - 1: l + 1;

			meshBuffer->Indices.push_back(base + h);
			meshBuffer->Indices.push_back(base + l);
			meshBuffer->Indices.push_back(base + center);

			if (v & 1)
				--h;
			else
				++l;
		}
	}

	// load textures
	core::array<video::ITexture*> tex;
	tex.reallocate(header.numTextures + 1);
	tex.push_back(0);

	const core::stringc relpath = FileSystem->getFileDir(file->getFileName())+"/";
	for (i = 1; i < (header.numTextures + 1); i++)
	{
		core::stringc path(textures[i-1].fileName);
		path.replace('\\','/');
		if (FileSystem->existFile(path))
			tex.push_back(SceneManager->getVideoDriver()->getTexture(path));
		else
			// try to read in the relative path of the OCT file
			tex.push_back(SceneManager->getVideoDriver()->getTexture( (relpath + path) ));
	}

	// prepare lightmaps
	core::array<video::ITexture*> lig;
	lig.set_used(header.numLightmaps + 1);
	lig[0] = 0;

	const u32 lightmapWidth = 128;
	const u32 lightmapHeight = 128;
	const core::dimension2d<u32> lmapsize(lightmapWidth, lightmapHeight);

	bool oldMipMapState = SceneManager->getVideoDriver()->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);
	SceneManager->getVideoDriver()->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);

	video::IImage* tmpImage = SceneManager->getVideoDriver()->createImage(video::ECF_R8G8B8, lmapsize);
	for (i = 1; i < (header.numLightmaps + 1); ++i)
	{
		core::stringc lightmapname = file->getFileName();
		lightmapname += ".lightmap.";
		lightmapname += (int)i;

		const octLightmap* lm = &lightmaps[i-1];

		for (u32 x=0; x<lightmapWidth; ++x)
		{
			for (u32 y=0; y<lightmapHeight; ++y)
			{
				tmpImage->setPixel(x, y,
						video::SColor(255,
						lm->data[x][y][2],
						lm->data[x][y][1],
						lm->data[x][y][0]));
			}
		}

		lig[i] = SceneManager->getVideoDriver()->addTexture(lightmapname.c_str(), tmpImage);
	}
	tmpImage->drop();
	SceneManager->getVideoDriver()->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, oldMipMapState);

	// Free stuff
	delete [] verts;
	delete [] faces;
	delete [] textures;
	delete [] lightmaps;
	delete [] lights;

	// attach materials
	for (i = 0; i < header.numLightmaps + 1; i++)
	{
		for (u32 j = 0; j < header.numTextures + 1; j++)
		{
			u32 mb = i * (header.numTextures + 1) + j;
			SMeshBufferLightMap * meshBuffer = (SMeshBufferLightMap*)Mesh->getMeshBuffer(mb);
			meshBuffer->Material.setTexture(0, tex[j]);
			meshBuffer->Material.setTexture(1, lig[i]);

			if (meshBuffer->Material.getTexture(0) == 0)
			{
				// This material has no texture, so we'll just show the lightmap if there is one.
				// We swapped the texture coordinates earlier.
				meshBuffer->Material.setTexture(0, meshBuffer->Material.getTexture(1));
				meshBuffer->Material.setTexture(1, 0);
			}
			if (meshBuffer->Material.getTexture(1) == 0)
			{
				// If there is only one texture, it should be solid and lit.
				// Among other things, this way you can preview OCT lights.
				meshBuffer->Material.MaterialType = video::EMT_SOLID;
				meshBuffer->Material.Lighting = true;
			}
		}
	}

	// delete all buffers without geometry in it.
	i = 0;
	while(i < Mesh->MeshBuffers.size())
	{
		if (Mesh->MeshBuffers[i]->getVertexCount() == 0 ||
			Mesh->MeshBuffers[i]->getIndexCount() == 0 ||
			Mesh->MeshBuffers[i]->getMaterial().getTexture(0) == 0)
		{
			// Meshbuffer is empty -- drop it
			Mesh->MeshBuffers[i]->drop();
			Mesh->MeshBuffers.erase(i);
		}
		else
		{
			++i;
		}
	}


	// create bounding box
	for (i = 0; i < Mesh->MeshBuffers.size(); ++i)
	{
		Mesh->MeshBuffers[i]->recalculateBoundingBox();
	}
	Mesh->recalculateBoundingBox();


	// Set up an animated mesh to hold the mesh
	SAnimatedMesh* AMesh = new SAnimatedMesh();
	AMesh->Type = EAMT_OCT;
	AMesh->addMesh(Mesh);
	AMesh->recalculateBoundingBox();
	Mesh->drop();

	return AMesh;
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool COCTLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "oct" );
}


} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_OCT_LOADER_


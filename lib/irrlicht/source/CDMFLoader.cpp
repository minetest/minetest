// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
//
// This file was originally written by Salvatore Russo.
// I (Nikolaus Gebhardt) did some minor modifications and changes to it and
// integrated it into Irrlicht.
// Thanks a lot to Salvatore for his work on this and that he gave me
// his permission to add it into Irrlicht using the zlib license.
/*
  CDMFLoader by Salvatore Russo (September 2005)

  See the header file for additional information including use and distribution rights.
*/

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DMF_LOADER_

#ifdef _DEBUG
#define _IRR_DMF_DEBUG_
#include "os.h"
#endif

#include "CDMFLoader.h"
#include "ISceneManager.h"
#include "IAttributes.h"
#include "SAnimatedMesh.h"
#include "SSkinMeshBuffer.h"
#include "irrString.h"
#include "irrMath.h"
#include "dmfsupport.h"

namespace irr
{
namespace scene
{

/** Constructor*/
CDMFLoader::CDMFLoader(ISceneManager* smgr, io::IFileSystem* filesys)
: SceneMgr(smgr), FileSystem(filesys)
{
	#ifdef _DEBUG
	IReferenceCounted::setDebugName("CDMFLoader");
	#endif
}


void CDMFLoader::findFile(bool use_mat_dirs, const core::stringc& path, const core::stringc& matPath, core::stringc& filename)
{
	// path + texpath + full name
	if (use_mat_dirs && FileSystem->existFile(path+matPath+filename))
		filename = path+matPath+filename;
	// path + full name
	else if (FileSystem->existFile(path+filename))
		filename = path+filename;
	// path + texpath + base name
	else if (use_mat_dirs && FileSystem->existFile(path+matPath+FileSystem->getFileBasename(filename)))
		filename = path+matPath+FileSystem->getFileBasename(filename);
	// path + base name
	else if (FileSystem->existFile(path+FileSystem->getFileBasename(filename)))
		filename = path+FileSystem->getFileBasename(filename);
	// texpath + full name
	else if (use_mat_dirs && FileSystem->existFile(matPath+filename))
		filename = matPath+filename;
	// texpath + base name
	else if (use_mat_dirs && FileSystem->existFile(matPath+FileSystem->getFileBasename(filename)))
		filename = matPath+FileSystem->getFileBasename(filename);
	// base name
	else if (FileSystem->existFile(FileSystem->getFileBasename(filename)))
		filename = FileSystem->getFileBasename(filename);
}


/**Creates/loads an animated mesh from the file.
 \return Pointer to the created mesh. Returns 0 if loading failed.
 If you no longer need the mesh, you should call IAnimatedMesh::drop().
 See IReferenceCounted::drop() for more information.*/
IAnimatedMesh* CDMFLoader::createMesh(io::IReadFile* file)
{
	if (!file)
		return 0;
	video::IVideoDriver* driver = SceneMgr->getVideoDriver();

	//Load stringlist
	StringList dmfRawFile;
	LoadFromFile(file, dmfRawFile);

	if (dmfRawFile.size()==0)
		return 0;

	SMesh * mesh = new SMesh();

	u32 i;

	dmfHeader header;

	//load header
	core::array<dmfMaterial> materiali;
	if (GetDMFHeader(dmfRawFile, header))
	{
		//let's set ambient light
		SceneMgr->setAmbientLight(header.dmfAmbient);

		//let's create the correct number of materials, vertices and faces
		dmfVert *verts=new dmfVert[header.numVertices];
		dmfFace *faces=new dmfFace[header.numFaces];

		//let's get the materials
#ifdef _IRR_DMF_DEBUG_
		os::Printer::log("Loading materials", core::stringc(header.numMaterials).c_str());
#endif
		GetDMFMaterials(dmfRawFile, materiali, header.numMaterials);

		//let's get vertices and faces
#ifdef _IRR_DMF_DEBUG_
		os::Printer::log("Loading geometry");
#endif
		GetDMFVerticesFaces(dmfRawFile, verts, faces);

		//create a meshbuffer for each material, then we'll remove empty ones
#ifdef _IRR_DMF_DEBUG_
		os::Printer::log("Creating meshbuffers.");
#endif
		for (i=0; i<header.numMaterials; i++)
		{
			//create a new SMeshBufferLightMap for each material
			SSkinMeshBuffer* buffer = new SSkinMeshBuffer();
			buffer->Material.MaterialType = video::EMT_LIGHTMAP_LIGHTING;
			buffer->Material.Wireframe = false;
			buffer->Material.Lighting = true;
			mesh->addMeshBuffer(buffer);
			buffer->drop();
		}

		// Build the mesh buffers
#ifdef _IRR_DMF_DEBUG_
		os::Printer::log("Adding geometry to mesh.");
#endif
		for (i = 0; i < header.numFaces; i++)
		{
#ifdef _IRR_DMF_DEBUG_
		os::Printer::log("Polygon with #vertices", core::stringc(faces[i].numVerts).c_str());
#endif
			if (faces[i].numVerts < 3)
				continue;

			const core::vector3df normal =
				core::triangle3df(verts[faces[i].firstVert].pos,
						verts[faces[i].firstVert+1].pos,
						verts[faces[i].firstVert+2].pos).getNormal().normalize();

			SSkinMeshBuffer* meshBuffer = (SSkinMeshBuffer*)mesh->getMeshBuffer(
					faces[i].materialID);

			const bool use2TCoords = meshBuffer->Vertices_2TCoords.size() ||
				materiali[faces[i].materialID].lightmapName.size();
			if (use2TCoords && meshBuffer->Vertices_Standard.size())
				meshBuffer->convertTo2TCoords();
			const u32 base = meshBuffer->Vertices_2TCoords.size()?meshBuffer->Vertices_2TCoords.size():meshBuffer->Vertices_Standard.size();

			// Add this face's verts
			if (use2TCoords)
			{
				// make sure we have the proper type set
				meshBuffer->VertexType=video::EVT_2TCOORDS;
				for (u32 v = 0; v < faces[i].numVerts; v++)
				{
					const dmfVert& vv = verts[faces[i].firstVert + v];
					video::S3DVertex2TCoords vert(vv.pos,
						normal, video::SColor(255,255,255,255), vv.tc, vv.lc);
					if (materiali[faces[i].materialID].textureBlend==4 &&
							SceneMgr->getParameters()->getAttributeAsBool(DMF_FLIP_ALPHA_TEXTURES))
					{
						vert.TCoords.set(vv.tc.X,-vv.tc.Y);
					}
					meshBuffer->Vertices_2TCoords.push_back(vert);
				}
			}
			else
			{
				for (u32 v = 0; v < faces[i].numVerts; v++)
				{
					const dmfVert& vv = verts[faces[i].firstVert + v];
					video::S3DVertex vert(vv.pos,
						normal, video::SColor(255,255,255,255), vv.tc);
					if (materiali[faces[i].materialID].textureBlend==4 &&
							SceneMgr->getParameters()->getAttributeAsBool(DMF_FLIP_ALPHA_TEXTURES))
					{
						vert.TCoords.set(vv.tc.X,-vv.tc.Y);
					}
					meshBuffer->Vertices_Standard.push_back(vert);
				}
			}

			// Now add the indices
			// This weird loop turns convex polygons into triangle strips.
			// I do it this way instead of a simple fan because it usually
			// looks a lot better in wireframe, for example.
			u32 h = faces[i].numVerts - 1, l = 0, c; // High, Low, Center
			for (u32 v = 0; v < faces[i].numVerts - 2; v++)
			{
				if (v & 1) // odd
					c = h - 1;
				else // even
					c = l + 1;

				meshBuffer->Indices.push_back(base + h);
				meshBuffer->Indices.push_back(base + l);
				meshBuffer->Indices.push_back(base + c);

				if (v & 1) // odd
					h--;
				else // even
					l++;
			}
		}

		delete [] verts;
		delete [] faces;
	}

	// delete all buffers without geometry in it.
#ifdef _IRR_DMF_DEBUG_
	os::Printer::log("Cleaning meshbuffers.");
#endif
	i = 0;
	while(i < mesh->MeshBuffers.size())
	{
		if (mesh->MeshBuffers[i]->getVertexCount() == 0 ||
			mesh->MeshBuffers[i]->getIndexCount() == 0)
		{
			// Meshbuffer is empty -- drop it
			mesh->MeshBuffers[i]->drop();
			mesh->MeshBuffers.erase(i);
			materiali.erase(i);
		}
		else
		{
			i++;
		}
	}


	{
		//load textures and lightmaps in materials.
		//don't worry if you receive a could not load texture, cause if you don't need
		//a particular material in your scene it will be loaded and then destroyed.
#ifdef _IRR_DMF_DEBUG_
		os::Printer::log("Loading textures.");
#endif
		const bool use_mat_dirs=!SceneMgr->getParameters()->getAttributeAsBool(DMF_IGNORE_MATERIALS_DIRS);

		core::stringc path;
		if ( SceneMgr->getParameters()->existsAttribute(DMF_TEXTURE_PATH) )
			path = SceneMgr->getParameters()->getAttributeAsString(DMF_TEXTURE_PATH);
		else
			path = FileSystem->getFileDir(file->getFileName());
		path += ('/');

		for (i=0; i<mesh->getMeshBufferCount(); i++)
		{
			//texture and lightmap
			video::ITexture *tex = 0;
			video::ITexture *lig = 0;

			//current buffer to apply material
			video::SMaterial& mat = mesh->getMeshBuffer(i)->getMaterial();

			//Primary texture is normal
			if (materiali[i].textureFlag==0)
			{
				if (materiali[i].textureBlend==4)
					driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT,true);
				findFile(use_mat_dirs, path, materiali[i].pathName, materiali[i].textureName);
				tex = driver->getTexture(materiali[i].textureName);
			}
			//Primary texture is just a color
			else if(materiali[i].textureFlag==1)
			{
				video::SColor color(axtoi(materiali[i].textureName.c_str()));

				//just for compatibility with older Irrlicht versions
				//to support transparent materials
				if (color.getAlpha()!=255 && materiali[i].textureBlend==4)
					driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT,true);

				video::IImage *immagine= driver->createImage(video::ECF_A8R8G8B8,
					core::dimension2d<u32>(8,8));
				immagine->fill(color);
				tex = driver->addTexture("", immagine);
				immagine->drop();

				//to support transparent materials
				if (color.getAlpha()!=255 && materiali[i].textureBlend==4)
				{
					mat.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
					mat.MaterialTypeParam =(((f32) (color.getAlpha()-1))/255.0f);
				}
			}

			//Lightmap is present
			if (materiali[i].lightmapFlag == 0)
			{
				findFile(use_mat_dirs, path, materiali[i].pathName, materiali[i].lightmapName);
				lig = driver->getTexture(materiali[i].lightmapName);
			}
			else //no lightmap
			{
				mat.MaterialType = video::EMT_SOLID;
				const f32 mult = 100.0f - header.dmfShadow;
				mat.AmbientColor=header.dmfAmbient.getInterpolated(video::SColor(255,0,0,0),mult/100.f);
			}

			if (materiali[i].textureBlend==4)
			{
				mat.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
				mat.MaterialTypeParam =
					SceneMgr->getParameters()->getAttributeAsFloat(DMF_ALPHA_CHANNEL_REF);
			}

			//if texture is present mirror vertically owing to DeleD representation
			if (tex && header.dmfVersion<1.1)
			{
				const core::dimension2d<u32> texsize = tex->getSize();
				void* pp = tex->lock();
				if (pp)
				{
					const video::ECOLOR_FORMAT format = tex->getColorFormat();
					if (format == video::ECF_A1R5G5B5)
					{
						s16* p = (s16*)pp;
						s16 tmp=0;
						for (u32 x=0; x<texsize.Width; x++)
							for (u32 y=0; y<texsize.Height/2; y++)
							{
								tmp=p[y*texsize.Width + x];
								p[y*texsize.Width + x] = p[(texsize.Height-y-1)*texsize.Width + x];
								p[(texsize.Height-y-1)*texsize.Width + x]=tmp;
							}
					}
					else
					if (format == video::ECF_A8R8G8B8)
					{
						s32* p = (s32*)pp;
						s32 tmp=0;
						for (u32 x=0; x<texsize.Width; x++)
							for (u32 y=0; y<texsize.Height/2; y++)
							{
								tmp=p[y*texsize.Width + x];
								p[y*texsize.Width + x] = p[(texsize.Height-y-1)*texsize.Width + x];
								p[(texsize.Height-y-1)*texsize.Width + x]=tmp;
							}
					}
				}
				tex->unlock();
				tex->regenerateMipMapLevels();
			}

			//if lightmap is present mirror vertically owing to DeleD rapresentation
			if (lig && header.dmfVersion<1.1)
			{
				const core::dimension2d<u32> ligsize=lig->getSize();
				void* pp = lig->lock();
				if (pp)
				{
					video::ECOLOR_FORMAT format = lig->getColorFormat();
					if (format == video::ECF_A1R5G5B5)
					{
						s16* p = (s16*)pp;
						s16 tmp=0;
						for (u32 x=0; x<ligsize.Width; x++)
						{
							for (u32 y=0; y<ligsize.Height/2; y++)
							{
								tmp=p[y*ligsize.Width + x];
								p[y*ligsize.Width + x] = p[(ligsize.Height-y-1)*ligsize.Width + x];
								p[(ligsize.Height-y-1)*ligsize.Width + x]=tmp;
							}
						}
					}
					else if (format == video::ECF_A8R8G8B8)
					{
						s32* p = (s32*)pp;
						s32 tmp=0;
						for (u32 x=0; x<ligsize.Width; x++)
						{
							for (u32 y=0; y<ligsize.Height/2; y++)
							{
								tmp=p[y*ligsize.Width + x];
								p[y*ligsize.Width + x] = p[(ligsize.Height-y-1)*ligsize.Width + x];
								p[(ligsize.Height-y-1)*ligsize.Width + x]=tmp;
							}
						}
					}
				}
				lig->unlock();
				lig->regenerateMipMapLevels();
			}

			mat.setTexture(0, tex);
			mat.setTexture(1, lig);
		}
	}

	// create bounding box
	for (i = 0; i < mesh->MeshBuffers.size(); ++i)
	{
		mesh->MeshBuffers[i]->recalculateBoundingBox();
	}
	mesh->recalculateBoundingBox();

	// Set up an animated mesh to hold the mesh
	SAnimatedMesh* AMesh = new SAnimatedMesh();
	AMesh->Type = EAMT_UNKNOWN;
	AMesh->addMesh(mesh);
	AMesh->recalculateBoundingBox();
	mesh->drop();

	return AMesh;
}


/** \brief Tell us if this file is able to be loaded by this class
 based on the file extension (e.g. ".bsp")
 \return true if file is loadable.*/
bool CDMFLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "dmf" );
}


} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_DMF_LOADER_


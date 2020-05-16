// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
//
// This file was originally written by ZDimitor.

//-----------------------------------------------------------------------------
// This tool created by ZDimitor everyone can use it as wants
//-----------------------------------------------------------------------------

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_MY3D_LOADER_

#include "CMY3DMeshFileLoader.h"

#include "SAnimatedMesh.h"
#include "SMeshBuffer.h"
#include "IReadFile.h"
#include "IAttributes.h"

#include "CMY3DHelper.h"
#include "os.h"

// v3.15 - May 16, 2005

namespace irr
{
namespace scene
{

static const u32 MY3D_ID = 0x4d593344;
static const u16 MY3D_VER = 0x0003;
static const u16 MY3D_SCENE_HEADER_ID = 0x1000;
static const u16 MY3D_MAT_LIST_ID = 0x2000;
static const u16 MY3D_MAT_HEADER_ID = 0x2100;
static const u16 MY3D_TEX_FNAME_ID = 0x2101;
static const u16 MY3D_TEXDATA_HEADER_ID = 0x2501;
static const u16 MY3D_TEXDATA_RLE_HEADER_ID = 0x2502;
static const u16 MY3D_MESH_LIST_ID =  0x3000;
static const u16 MY3D_MESH_HEADER_ID = 0x3100;
static const u16 MY3D_VERTS_ID = 0x3101;
static const u16 MY3D_FACES_ID = 0x3102;
static const u16 MY3D_TVERTS_ID = 0x3103;
static const u16 MY3D_TFACES_ID = 0x3104;
static const u16 MY3D_FILE_END_ID = 0xFFFF;

static const unsigned long MY3D_TEXDATA_COMPR_NONE_ID = 0x4e4f4e45;
static const unsigned long MY3D_TEXDATA_COMPR_SIMPLE_ID = 0x53494d50;
static const unsigned long MY3D_TEXDATA_COMPR_RLE_ID = 0x20524c45;

static const unsigned long MY3D_PIXEL_FORMAT_24 = 0x5f32345f;
static const unsigned long MY3D_PIXEL_FORMAT_16 = 0x5f31365f;

CMY3DMeshFileLoader::CMY3DMeshFileLoader(ISceneManager* scmgr, io::IFileSystem* fs)
	: SceneManager(scmgr), FileSystem(fs)
{
	#ifdef _DEBUG
	setDebugName("CMY3DMeshFileLoader");
	#endif

	if (FileSystem)
		FileSystem->grab();
}


CMY3DMeshFileLoader::~CMY3DMeshFileLoader()
{
	if (FileSystem)
		FileSystem->drop();
}


bool CMY3DMeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "my3d" );
}


IAnimatedMesh* CMY3DMeshFileLoader::createMesh(io::IReadFile* file)
{
	MaterialEntry.clear();
	MeshBufferEntry.clear();
	ChildNodes.clear();

	// working directory (from which we load the scene)
	core::stringc filepath = FileSystem->getFileDir(file->getFileName());
	if (filepath==".")
		filepath="";
	else
		filepath.append("/");

	// read file into memory
	SMyFileHeader fileHeader;
	file->read(&fileHeader, sizeof(SMyFileHeader));
#ifdef __BIG_ENDIAN__
	fileHeader.MyId = os::Byteswap::byteswap(fileHeader.MyId);
	fileHeader.Ver = os::Byteswap::byteswap(fileHeader.Ver);
#endif

	if (fileHeader.MyId!=MY3D_ID || fileHeader.Ver!=MY3D_VER)
	{
		os::Printer::log("Bad MY3D file header, loading failed!", ELL_ERROR);
		return 0;
	}

	u16 id;

	file->read(&id, sizeof(id));
#ifdef __BIG_ENDIAN__
	id = os::Byteswap::byteswap(id);
#endif

	if (id!=MY3D_SCENE_HEADER_ID)
	{
		os::Printer::log("Cannot find MY3D_SCENE_HEADER_ID, loading failed!", ELL_ERROR);
		return 0;
	}

	SMySceneHeader sceneHeader;
	file->read(&sceneHeader, sizeof(SMySceneHeader));
#ifdef __BIG_ENDIAN__
	sceneHeader.MaterialCount = os::Byteswap::byteswap(sceneHeader.MaterialCount);
	sceneHeader.MeshCount = os::Byteswap::byteswap(sceneHeader.MeshCount);
#endif

	file->read(&id, sizeof(id));
#ifdef __BIG_ENDIAN__
	id = os::Byteswap::byteswap(id);
#endif

	if (id!=MY3D_MAT_LIST_ID)
	{
		os::Printer::log("Can not find MY3D_MAT_LIST_ID, loading failed!", ELL_ERROR);
		return 0;
	}

	core::stringc texturePath =
		SceneManager->getParameters()->getAttributeAsString(MY3D_TEXTURE_PATH);

	file->read(&id, sizeof(id));
#ifdef __BIG_ENDIAN__
	id = os::Byteswap::byteswap(id);
#endif

	c8 namebuf[256];
	for (s32 m=0; m<sceneHeader.MaterialCount; ++m)
	{
		if (id != MY3D_MAT_HEADER_ID)
		{
			os::Printer::log("Cannot find MY3D_MAT_HEADER_ID, loading failed!", ELL_ERROR);
			return 0;
		}

		// read material header
		MaterialEntry.push_back(SMyMaterialEntry());
		SMyMaterialEntry& me=MaterialEntry.getLast();
		file->read(&(me.Header), sizeof(SMyMaterialHeader));

		// read next identificator
		file->read(&id, sizeof(id));
#ifdef __BIG_ENDIAN__
		id = os::Byteswap::byteswap(id);
#endif

		bool gotLightMap=false, gotMainMap=false;

		for (u32 t=0; t<me.Header.TextureCount; ++t)
		{
			if (id==MY3D_TEX_FNAME_ID)
				file->read(namebuf, 256);
			else
			{
				me.Texture2 = readEmbeddedLightmap(file, namebuf);
				if (!me.Texture2)
					return 0;
				gotLightMap = true;
			}

			const core::stringc name(namebuf);
			const s32 pos = name.findLast('.');
			const core::stringc LightingMapStr = "LightingMap";
			const s32 ls = LightingMapStr.size();
			const bool isSubString = (LightingMapStr == name.subString(core::max_(0, (pos - ls)), ls));
			if ((isSubString || (name[pos-1]=='m' &&
				name[pos-2]=='l' && name[pos-3]=='_')) &&
				!gotLightMap)
			{
				const bool oldMipMapState = SceneManager->getVideoDriver()->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);
				SceneManager->getVideoDriver()->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);

				me.Texture2FileName = texturePath.size() ? texturePath : filepath;
				me.Texture2FileName.append("Lightmaps/");
				me.Texture2FileName.append(name);

				if (name.size())
					me.Texture2 = SceneManager->getVideoDriver()->getTexture(me.Texture2FileName);

				me.MaterialType = video::EMT_LIGHTMAP_M2;
				gotLightMap = true;

				SceneManager->getVideoDriver()->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, oldMipMapState);
			}
			else
			if (!gotLightMap && gotMainMap)
			{
				me.Texture2FileName = texturePath.size() ? texturePath : filepath;
				me.Texture2FileName.append(name);

				if (name.size())
					me.Texture2 = SceneManager->getVideoDriver()->getTexture(me.Texture2FileName);

				me.MaterialType = video::EMT_REFLECTION_2_LAYER;
			}
			else
			if (!gotMainMap && !gotLightMap)
			{
				me.Texture1FileName = filepath;
				me.Texture1FileName.append(name);
				if (name.size())
					me.Texture1 = SceneManager->getVideoDriver()->getTexture(me.Texture1FileName);

				gotMainMap = true;
				me.MaterialType = video::EMT_SOLID;
			}
			else
			if (gotLightMap)
			{
				me.MaterialType = video::EMT_LIGHTMAP_M2;
			}

			file->read(&id, sizeof(id));
#ifdef __BIG_ENDIAN__
			id = os::Byteswap::byteswap(id);
#endif
		}

		// override material types based on their names
		if (!strncmp(me.Header.Name, "AlphaChannel-", 13))
			me.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		else
		if (!strncmp(me.Header.Name, "SphereMap-", 10))
			me.MaterialType = video::EMT_SPHERE_MAP;
	}

	// loading meshes

	if (id!=MY3D_MESH_LIST_ID)
	{
		os::Printer::log("Can not find MY3D_MESH_LIST_ID, loading failed!", ELL_ERROR);
		return 0;
	}

	file->read(&id, sizeof(id));
#ifdef __BIG_ENDIAN__
	id = os::Byteswap::byteswap(id);
#endif

	for (s32 mesh_id=0; mesh_id<sceneHeader.MeshCount; mesh_id++)
	{
		// Warning!!! In some cases MY3D exporter uncorrectly calculates
		// MeshCount (it's a problem, has to be solved) thats why
		// i added this code line
		if (id!=MY3D_MESH_HEADER_ID)
			break;

		if (id!=MY3D_MESH_HEADER_ID)
		{
			os::Printer::log("Can not find MY3D_MESH_HEADER_ID, loading failed!", ELL_ERROR);
			return 0;
		}

		SMyMeshHeader meshHeader;
		file->read(&meshHeader, sizeof(SMyMeshHeader));

		core::array <SMyVertex> Vertex;
		core::array <SMyFace> Face;
		core::array <SMyTVertex> TVertex1, TVertex2;
		core::array <SMyFace> TFace1, TFace2;

		s32 vertsNum=0;
		s32 facesNum=0;

		// vertices
		file->read(&id, sizeof(id));
#ifdef __BIG_ENDIAN__
	id = os::Byteswap::byteswap(id);
#endif
		if (id!=MY3D_VERTS_ID)
		{
			os::Printer::log("Can not find MY3D_VERTS_ID, loading failed!", ELL_ERROR);
			return 0;
		}

		file->read(&vertsNum, sizeof(vertsNum));
		Vertex.set_used(vertsNum);
		file->read(Vertex.pointer(), sizeof(SMyVertex)*vertsNum);

		// faces
		file->read(&id, sizeof(id));
#ifdef __BIG_ENDIAN__
		id = os::Byteswap::byteswap(id);
#endif
		if (id!=MY3D_FACES_ID)
		{
			os::Printer::log("Can not find MY3D_FACES_ID, loading failed!", ELL_ERROR);
			return 0;
		}

		file->read(&facesNum, sizeof(facesNum));
		Face.set_used(facesNum);
		file->read(Face.pointer(), sizeof(SMyFace)*facesNum);

		// reading texture channels
		for (s32 tex=0; tex<(s32)meshHeader.TChannelCnt; tex++)
		{
			// Max 2 texture channels allowed (but in format .my3d can be more)
			s32 tVertsNum=0, tFacesNum=0;

			// reading texture coords
			file->read(&id, sizeof(id));
#ifdef __BIG_ENDIAN__
			id = os::Byteswap::byteswap(id);
#endif

			if (id!=MY3D_TVERTS_ID)
			{
				core::stringc msg="Can not find MY3D_TVERTS_ID (";
				msg.append(core::stringc(tex));
				msg.append("texture channel), loading failed!");
				os::Printer::log(msg.c_str(), ELL_ERROR);
				return 0;
			}

			file->read(&tVertsNum, sizeof(tVertsNum));

			if (tex==0)
			{
				// 1st texture channel
				TVertex1.set_used(tVertsNum);
				file->read(TVertex1.pointer(), sizeof(SMyTVertex)*tVertsNum);
			}
			else
			if (tex==1)
			{
				// 2nd texture channel
				TVertex2.set_used(tVertsNum);
				file->read(TVertex2.pointer(), sizeof(SMyTVertex)*tVertsNum);
			}
			else
			{
				// skip other texture channels
				file->seek(file->getPos()+sizeof(SMyTVertex)*tVertsNum);
			}

			// reading texture faces
			file->read(&id, sizeof(id));
#ifdef __BIG_ENDIAN__
			id = os::Byteswap::byteswap(id);
#endif

			if (id!=MY3D_TFACES_ID)
			{
				core::stringc msg="Can not find MY3D_TFACES_ID (";
				msg.append(core::stringc(tex));
				msg.append("texture channel), loading failed!");
				os::Printer::log(msg.c_str(), ELL_ERROR);
				return 0;
			}

			file->read(&tFacesNum, sizeof(tFacesNum));

			if (tex==0)
			{
				// 1st texture channel
				TFace1.set_used(tFacesNum);
				file->read(TFace1.pointer(), sizeof(SMyFace)*tFacesNum);
			}
			else if (tex==1)
			{
				// 2nd texture channel
				TFace2.set_used(tFacesNum);
				file->read(TFace2.pointer(), sizeof(SMyFace)*tFacesNum);
			}
			else
			{
				// skip other texture channels
				file->seek(file->getPos()+sizeof(SMyFace)*tFacesNum);
			}
		}

		// trying to find material

		SMyMaterialEntry* matEnt = getMaterialEntryByIndex(meshHeader.MatIndex);

		// creating geometry for the mesh

		// trying to find mesh buffer for this material
		SMeshBufferLightMap* buffer = getMeshBufferByMaterialIndex(meshHeader.MatIndex);

		if (!buffer ||
			(buffer->Vertices.size()+vertsNum) > SceneManager->getVideoDriver()->getMaximalPrimitiveCount())
		{
			// creating new mesh buffer for this material
			buffer = new scene::SMeshBufferLightMap();

			buffer->Material.MaterialType = video::EMT_LIGHTMAP_M2; // EMT_LIGHTMAP_M4 also possible
			buffer->Material.Wireframe = false;
			buffer->Material.Lighting = false;

			if (matEnt)
			{
				buffer->Material.MaterialType = matEnt->MaterialType;

				if (buffer->Material.MaterialType == video::EMT_REFLECTION_2_LAYER)
				{
					buffer->Material.Lighting = true;
					buffer->Material.setTexture(1, matEnt->Texture1);
					buffer->Material.setTexture(0, matEnt->Texture2);
				}
				else
				{
					buffer->Material.setTexture(0, matEnt->Texture1);
					buffer->Material.setTexture(1, matEnt->Texture2);
				}

				if (buffer->Material.MaterialType == video::EMT_TRANSPARENT_ALPHA_CHANNEL)
				{
					buffer->Material.BackfaceCulling = true;
					buffer->Material.Lighting  = true;
				}
				else
				if (buffer->Material.MaterialType == video::EMT_SPHERE_MAP)
				{
					buffer->Material.Lighting  = true;
				}

				buffer->Material.AmbientColor = video::SColor(
					matEnt->Header.AmbientColor.A, matEnt->Header.AmbientColor.R,
					matEnt->Header.AmbientColor.G, matEnt->Header.AmbientColor.B
					);
				buffer->Material.DiffuseColor =	video::SColor(
					matEnt->Header.DiffuseColor.A, matEnt->Header.DiffuseColor.R,
					matEnt->Header.DiffuseColor.G, matEnt->Header.DiffuseColor.B
					);
				buffer->Material.EmissiveColor = video::SColor(
					matEnt->Header.EmissiveColor.A, matEnt->Header.EmissiveColor.R,
					matEnt->Header.EmissiveColor.G, matEnt->Header.EmissiveColor.B
					);
				buffer->Material.SpecularColor = video::SColor(
					matEnt->Header.SpecularColor.A, matEnt->Header.SpecularColor.R,
					matEnt->Header.SpecularColor.G, matEnt->Header.SpecularColor.B
					);
			}
			else
			{
				buffer->Material.setTexture(0, 0);
				buffer->Material.setTexture(1, 0);

				buffer->Material.AmbientColor = video::SColor(255, 255, 255, 255);
				buffer->Material.DiffuseColor =	video::SColor(255, 255, 255, 255);
				buffer->Material.EmissiveColor = video::SColor(0, 0, 0, 0);
				buffer->Material.SpecularColor = video::SColor(0, 0, 0, 0);
			}

			if (matEnt && matEnt->Header.Transparency!=0)
			{
				if (buffer->Material.MaterialType == video::EMT_REFLECTION_2_LAYER )
				{
					buffer->Material.MaterialType = video::EMT_TRANSPARENT_REFLECTION_2_LAYER;
					buffer->Material.Lighting  = true;
					buffer->Material.BackfaceCulling = true;
				}
				else
				{
					buffer->Material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
					buffer->Material.Lighting = false;
					buffer->Material.BackfaceCulling = false;
				}
			}
			else if (
				!buffer->Material.getTexture(1) &&
				buffer->Material.MaterialType != video::EMT_TRANSPARENT_ALPHA_CHANNEL &&
				buffer->Material.MaterialType != video::EMT_SPHERE_MAP)
			{
				buffer->Material.MaterialType = video::EMT_SOLID;
				buffer->Material.Lighting  = true;
			}

			MeshBufferEntry.push_back(
			SMyMeshBufferEntry(meshHeader.MatIndex, buffer));
		}

		video::S3DVertex2TCoords VertexA, VertexB, VertexC;

		// vertices (A, B, C) color
		video::SColor vert_color;
		if (matEnt &&
			(buffer->Material.MaterialType == video::EMT_TRANSPARENT_VERTEX_ALPHA ||
			buffer->Material.MaterialType == video::EMT_TRANSPARENT_REFLECTION_2_LAYER))
		{
			video::SColor color(
			matEnt->Header.DiffuseColor.A, matEnt->Header.DiffuseColor.R,
			matEnt->Header.DiffuseColor.G, matEnt->Header.DiffuseColor.B);

			vert_color = color.getInterpolated(video::SColor(0,0,0,0),
				1-matEnt->Header.Transparency);
		}
		else
		{
			vert_color = buffer->Material.DiffuseColor;
		}

		VertexA.Color = VertexB.Color = VertexC.Color = vert_color;

		if (buffer->Material.MaterialType == video::EMT_TRANSPARENT_ALPHA_CHANNEL)
		{
			buffer->Indices.reallocate(buffer->Indices.size()+6*facesNum);
			buffer->Vertices.reallocate(buffer->Vertices.size()+6*facesNum);
		}
		else
		{
			buffer->Indices.reallocate(buffer->Indices.size()+3*facesNum);
			buffer->Vertices.reallocate(buffer->Vertices.size()+3*facesNum);
		}
		for (int f=0; f<facesNum; f++)
		{
			// vertex A

			VertexA.Pos.X = Vertex[Face[f].C].Coord.X;
			VertexA.Pos.Y = Vertex[Face[f].C].Coord.Y;
			VertexA.Pos.Z = Vertex[Face[f].C].Coord.Z;

			VertexA.Normal.X = Vertex[Face[f].C].Normal.X;
			VertexA.Normal.Y = Vertex[Face[f].C].Normal.Y;
			VertexA.Normal.Z = Vertex[Face[f].C].Normal.Z;

			if (meshHeader.TChannelCnt>0)
			{
				VertexA.TCoords.X  = TVertex1[TFace1[f].C].TCoord.X;
				VertexA.TCoords.Y  = TVertex1[TFace1[f].C].TCoord.Y;
			}

			if (meshHeader.TChannelCnt>1)
			{
				VertexA.TCoords2.X = TVertex2[TFace2[f].C].TCoord.X;
				VertexA.TCoords2.Y = TVertex2[TFace2[f].C].TCoord.Y;
			}

			// vertex B

			VertexB.Pos.X = Vertex[Face[f].B].Coord.X;
			VertexB.Pos.Y = Vertex[Face[f].B].Coord.Y;
			VertexB.Pos.Z = Vertex[Face[f].B].Coord.Z;

			VertexB.Normal.X = Vertex[Face[f].B].Normal.X;
			VertexB.Normal.Y = Vertex[Face[f].B].Normal.Y;
			VertexB.Normal.Z = Vertex[Face[f].B].Normal.Z;

			if (meshHeader.TChannelCnt>0)
			{
				VertexB.TCoords.X  = TVertex1[TFace1[f].B].TCoord.X;
				VertexB.TCoords.Y  = TVertex1[TFace1[f].B].TCoord.Y;
			}

			if (meshHeader.TChannelCnt>1)
			{
				VertexB.TCoords2.X = TVertex2[TFace2[f].B].TCoord.X;
				VertexB.TCoords2.Y = TVertex2[TFace2[f].B].TCoord.Y;
			}

			// vertex C

			VertexC.Pos.X = Vertex[Face[f].A].Coord.X;
			VertexC.Pos.Y = Vertex[Face[f].A].Coord.Y;
			VertexC.Pos.Z = Vertex[Face[f].A].Coord.Z;

			VertexC.Normal.X = Vertex[Face[f].A].Normal.X;
			VertexC.Normal.Y = Vertex[Face[f].A].Normal.Y;
			VertexC.Normal.Z = Vertex[Face[f].A].Normal.Z;

			if (meshHeader.TChannelCnt>0)
			{
				VertexC.TCoords.X  = TVertex1[TFace1[f].A].TCoord.X;
				VertexC.TCoords.Y  = TVertex1[TFace1[f].A].TCoord.Y;
			}
			if (meshHeader.TChannelCnt>1)
			{
				VertexC.TCoords2.X = TVertex2[TFace2[f].A].TCoord.X;
				VertexC.TCoords2.Y = TVertex2[TFace2[f].A].TCoord.Y;
			}

			// store 3d data in mesh buffer

			buffer->Indices.push_back(buffer->Vertices.size());
			buffer->Vertices.push_back(VertexA);

			buffer->Indices.push_back(buffer->Vertices.size());
			buffer->Vertices.push_back(VertexB);

			buffer->Indices.push_back(buffer->Vertices.size());
			buffer->Vertices.push_back(VertexC);

			//*****************************************************************
			//          !!!!!! W A R N I N G !!!!!!!
			//*****************************************************************
			// For materials with alpha channel we duplicate all faces.
			// This has be done for proper lighting calculation of the back faces.
			// So you must remember this while you creating your models !!!!!
			//*****************************************************************
			//          !!!!!! W A R N I N G !!!!!!!
			//*****************************************************************

			if (buffer->Material.MaterialType == video::EMT_TRANSPARENT_ALPHA_CHANNEL)
			{
				VertexA.Normal = core::vector3df(-VertexA.Normal.X, -VertexA.Normal.Y, -VertexA.Normal.Z);
				VertexB.Normal = core::vector3df(-VertexB.Normal.X, -VertexB.Normal.Y, -VertexB.Normal.Z);
				VertexC.Normal = core::vector3df(-VertexC.Normal.X, -VertexC.Normal.Y, -VertexC.Normal.Z);

				buffer->Indices.push_back(buffer->Vertices.size());
				buffer->Vertices.push_back(VertexC);

				buffer->Indices.push_back(buffer->Vertices.size());
				buffer->Vertices.push_back(VertexB);

				buffer->Indices.push_back(buffer->Vertices.size());
				buffer->Vertices.push_back(VertexA);
			}
		}
		file->read(&id, sizeof(id));
#ifdef __BIG_ENDIAN__
		id = os::Byteswap::byteswap(id);
#endif
	}

	// creating mesh
	SMesh* mesh = new SMesh();

	for (u32 num=0; num<MeshBufferEntry.size(); ++num)
	{
		SMeshBufferLightMap* buffer = MeshBufferEntry[num].MeshBuffer;

		if (!buffer)
			continue;

		mesh->addMeshBuffer(buffer);

		buffer->recalculateBoundingBox();
		buffer->drop();
	}

	mesh->recalculateBoundingBox();

	if (id != MY3D_FILE_END_ID)
		os::Printer::log("Loading finished, but can not find MY3D_FILE_END_ID token.", ELL_WARNING);

	SAnimatedMesh* am = new SAnimatedMesh();

	am->addMesh(mesh);
	mesh->drop();
	am->recalculateBoundingBox();

	return am;
}


video::ITexture* CMY3DMeshFileLoader::readEmbeddedLightmap(io::IReadFile* file, char* namebuf)
{
	static int LightMapIndex=0;
	u16 id;
	file->read(&id, sizeof(id));
#ifdef __BIG_ENDIAN__
	id = os::Byteswap::byteswap(id);
#endif
	if (id!=MY3D_TEXDATA_HEADER_ID)
	{
		os::Printer::log("Can not find MY3D_TEXDATA_HEADER_ID, loading failed!", ELL_ERROR);
		return 0;
	}

	SMyTexDataHeader texDataHeader;

	file->read(&texDataHeader, sizeof(SMyTexDataHeader));

	strcpy(texDataHeader.Name, namebuf);

	char LightMapName[255];
	sprintf(LightMapName,"My3D.Lightmap.%d",++LightMapIndex);

	core::stringc pixFormatStr;
	if (texDataHeader.PixelFormat == MY3D_PIXEL_FORMAT_24)
		pixFormatStr = "24bit,";
	else
	if (texDataHeader.PixelFormat == MY3D_PIXEL_FORMAT_16)
		pixFormatStr = "16bit,";
	else
	{
		core::stringc msg="Unknown format of image data (";
		msg.append(LightMapName);
		msg.append("), loading failed!");
		os::Printer::log(msg.c_str(), ELL_ERROR);
		return 0;
	}

	if (texDataHeader.ComprMode != MY3D_TEXDATA_COMPR_NONE_ID &&
			texDataHeader.ComprMode != MY3D_TEXDATA_COMPR_RLE_ID &&
			texDataHeader.ComprMode != MY3D_TEXDATA_COMPR_SIMPLE_ID )
	{
		os::Printer::log("Unknown method of compression image data, loading failed!", ELL_ERROR);
		return 0;
	}

	const u32 num_pixels = texDataHeader.Width*texDataHeader.Height;

	void* data = 0;

	if (texDataHeader.ComprMode==MY3D_TEXDATA_COMPR_NONE_ID)
	{
		// none compressed image data
		if (texDataHeader.PixelFormat == MY3D_PIXEL_FORMAT_24)
		{
			data = (void*) new SMyPixelColor24[num_pixels];
			file->read(data, sizeof(SMyPixelColor24)*num_pixels);
		}
		else
		{
			data = (void*) new SMyPixelColor16[num_pixels];
			file->read(data, sizeof(SMyPixelColor16)*num_pixels);
		}
	}
	else
	if (texDataHeader.ComprMode==MY3D_TEXDATA_COMPR_RLE_ID)
	{
		// read RLE header identificator
		file->read(&id, sizeof(id));
#ifdef __BIG_ENDIAN__
		id = os::Byteswap::byteswap(id);
#endif
		if (id!=MY3D_TEXDATA_RLE_HEADER_ID)
		{
			os::Printer::log("Can not find MY3D_TEXDATA_RLE_HEADER_ID, loading failed!", ELL_ERROR);
			return 0;
		}

		// read RLE header
		SMyRLEHeader rleHeader;
		file->read(&rleHeader, sizeof(SMyRLEHeader));

		//allocate memory for input and output buffers
		void *input_buffer  = (void*) new unsigned char[rleHeader.nEncodedBytes];
		void *output_buffer = (void*) new unsigned char[rleHeader.nDecodedBytes];

		// read encoded data
		file->read(input_buffer, rleHeader.nEncodedBytes);

		// decode data
		data = 0;//(void*) new unsigned char[rleHeader.nDecodedBytes];
		s32 decodedBytes = core::rle_decode(
			(unsigned char*)input_buffer,  rleHeader.nEncodedBytes,
			(unsigned char*)output_buffer, rleHeader.nDecodedBytes);

		if (decodedBytes!=(s32)rleHeader.nDecodedBytes)
		{
			os::Printer::log("Error extracting data from RLE compression, loading failed!", ELL_ERROR);
			return 0;
		}

		// free input buffer
		delete [] (unsigned char*)input_buffer;

		// here decoded data
		data = output_buffer;
	}
	else if (texDataHeader.ComprMode==MY3D_TEXDATA_COMPR_SIMPLE_ID)
	{
		// simple compressed image data
		if (texDataHeader.PixelFormat == MY3D_PIXEL_FORMAT_24)
			data = (void*) new SMyPixelColor24[num_pixels];
		else
			data = (void*) new SMyPixelColor16[num_pixels];

		u32 nReadedPixels=0, nToRead=0;
		while (true)
		{
			file->read(&nToRead, sizeof(nToRead));

			if ((nReadedPixels+nToRead) > num_pixels)
				break;

			if (texDataHeader.PixelFormat == MY3D_PIXEL_FORMAT_24)
			{
				SMyPixelColor24 col24;
				file->read(&col24, sizeof(SMyPixelColor24));
				for (u32 p=0; p<nToRead; p++)
				{
					((SMyPixelColor24*)data)[nReadedPixels+p] =
						SMyPixelColor24(col24.r, col24.g, col24.b);
				}
			}
			else
			{
				SMyPixelColor16 col16;
				file->read(&col16, sizeof(SMyPixelColor16));
				for (u32 p=0; p<nToRead; p++)
					((SMyPixelColor16*)data)[nReadedPixels+p].argb = col16.argb;
			}

			nReadedPixels+=nToRead;

			if (nReadedPixels >= num_pixels)
				break;
		}

		if (nReadedPixels != num_pixels)
		{
			os::Printer::log("Image data seems to be corrupted, loading failed!", ELL_ERROR);
			return 0;
		}
	}

	//! Creates a software image from a byte array.
	video::IImage* light_img = 0;

	if (texDataHeader.PixelFormat == MY3D_PIXEL_FORMAT_24)
	{
		// 24 bit lightmap format
		light_img = SceneManager->getVideoDriver()->createImageFromData(
		video::ECF_R8G8B8,
		core::dimension2d<u32>(texDataHeader.Width, texDataHeader.Height),
			data, true);
	}
	else
	{
		// 16 bit lightmap format
		light_img = SceneManager->getVideoDriver()->createImageFromData(
			video::ECF_A1R5G5B5,
			core::dimension2d<u32>(texDataHeader.Width, texDataHeader.Height),
			data, true);
	}

	const bool oldMipMapState = SceneManager->getVideoDriver()->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);
	SceneManager->getVideoDriver()->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);
	video::ITexture* lmtex = SceneManager->getVideoDriver()->addTexture(LightMapName, light_img);
	SceneManager->getVideoDriver()->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, oldMipMapState);

	light_img->drop();
	return lmtex;
}


CMY3DMeshFileLoader::SMyMaterialEntry* CMY3DMeshFileLoader::getMaterialEntryByIndex(u32 matInd)
{
	for (u32 m=0; m<MaterialEntry.size(); ++m)
		if (MaterialEntry[m].Header.Index == matInd)
			return &MaterialEntry[m];

	return 0;
}



SMeshBufferLightMap* CMY3DMeshFileLoader::getMeshBufferByMaterialIndex(u32 matInd)
{
	for (u32 m=0; m<MeshBufferEntry.size(); ++m)
	{
		if (MeshBufferEntry[m].MaterialIndex == (s32)matInd)
			return MeshBufferEntry[m].MeshBuffer;
	}
	return 0;
}


const core::array<ISceneNode*>& CMY3DMeshFileLoader::getChildNodes() const
{
	return ChildNodes;
}


} // end namespace scnene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_MY3D_LOADER_


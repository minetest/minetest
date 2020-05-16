// Copyright (C) 2007-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CLWOMeshFileLoader.h"
#include "os.h"
#include "SAnimatedMesh.h"
#include "SMesh.h"
#include "IReadFile.h"
#include "ISceneManager.h"
#include "IFileSystem.h"
#include "IVideoDriver.h"
#include "IMeshManipulator.h"

namespace irr
{
namespace scene
{

#ifdef _DEBUG
#define LWO_READER_DEBUG
#endif

#define charsToUIntD(a, b, c, d) ((a << 24) | (b << 16) | (c << 8) | d)
inline unsigned int charsToUInt(const char *str)
{
	return (str[0] << 24) | (str[1] << 16) | (str[2] << 8) | str[3];
}


struct tLWOTextureInfo
{
	tLWOTextureInfo() : UVTag(0), DUVTag(0), Flags(0), WidthWrap(2),
			HeightWrap(2), OpacType(0), Color(0xffffffff),
			Value(0.0f), AntiAliasing(1.0f), Opacity(1.0f),
			Axis(255), Projection(0), Active(false) {}
	core::stringc Type;
	core::stringc Map;
	core::stringc AlphaMap;
	core::stringc UVname;
	u16 UVTag;
	u16 DUVTag;
	u16 Flags;
	u16 WidthWrap;
	u16 HeightWrap;
	u16 OpacType;
	u16 IParam[3];
	core::vector3df Size;
	core::vector3df Center;
	core::vector3df Falloff;
	core::vector3df Velocity;
	video::SColor Color;
	f32 Value;
	f32 AntiAliasing;
	f32 Opacity;
	f32 FParam[3];
	u8 Axis;
	u8 Projection;
	bool Active;
};

struct CLWOMeshFileLoader::tLWOMaterial
{
	tLWOMaterial() : Meshbuffer(0), TagType(0), Flags(0), ReflMode(3), TranspMode(3),
		Glow(0), AlphaMode(2), Luminance(0.0f), Diffuse(1.0f), Specular(0.0f),
		Reflection(0.0f), Transparency(0.0f), Translucency(0.0f),
		Sharpness(0.0f), ReflSeamAngle(0.0f), ReflBlur(0.0f),
		RefrIndex(1.0f), TranspBlur(0.0f), SmoothingAngle(0.0f),
		EdgeTransparency(0.0f), HighlightColor(0.0f), ColorFilter(0.0f),
		AdditiveTransparency(0.0f), GlowIntensity(0.0f), GlowSize(0.0f),
		AlphaValue(0.0f), VertexColorIntensity(0.0f), VertexColor() {}

	core::stringc Name;
	scene::SMeshBuffer *Meshbuffer;
	core::stringc ReflMap;
	u16 TagType;
	u16 Flags;
	u16 ReflMode;
	u16 TranspMode;
	u16 Glow;
	u16 AlphaMode;
	f32 Luminance;
	f32 Diffuse;
	f32 Specular;
	f32 Reflection;
	f32 Transparency;
	f32 Translucency;
	f32 Sharpness;
	f32 ReflSeamAngle;
	f32 ReflBlur;
	f32 RefrIndex;
	f32 TranspBlur;
	f32 SmoothingAngle;
	f32 EdgeTransparency;
	f32 HighlightColor;
	f32 ColorFilter;
	f32 AdditiveTransparency;
	f32 GlowIntensity;
	f32 GlowSize;
	f32 AlphaValue;
	f32 VertexColorIntensity;
	video::SColorf VertexColor;
	u32 Envelope[23];
	tLWOTextureInfo Texture[7];
};

struct tLWOLayerInfo
{
	u16 Number;
	u16 Parent;
	u16 Flags;
	bool Active;
	core::stringc Name;
	core::vector3df Pivot;
};


//! Constructor
CLWOMeshFileLoader::CLWOMeshFileLoader(scene::ISceneManager* smgr,
		io::IFileSystem* fs)
: SceneManager(smgr), FileSystem(fs), File(0), Mesh(0)
{
	#ifdef _DEBUG
	setDebugName("CLWOMeshFileLoader");
	#endif
}


//! destructor
CLWOMeshFileLoader::~CLWOMeshFileLoader()
{
	if (Mesh)
		Mesh->drop();
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool CLWOMeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension(filename, "lwo");
}


//! creates/loads an animated mesh from the file.
IAnimatedMesh* CLWOMeshFileLoader::createMesh(io::IReadFile* file)
{
	File = file;

	if (Mesh)
		Mesh->drop();

	Mesh = new SMesh();

	if (!readFileHeader())
		return 0;

	if (!readChunks())
		return 0;

#ifdef LWO_READER_DEBUG
	os::Printer::log("LWO loader: Creating geometry.");
	os::Printer::log("LWO loader: Assigning UV maps.");
#endif
	u32 i;
	for (i=0; i<Materials.size(); ++i)
	{
		u16 uvTag;
		for (u32 j=0; j<2; ++j) // max 2 texture coords
		{
			if (Materials[i]->Texture[j].UVname.size())
			{
				for (uvTag=0; uvTag<UvName.size(); ++uvTag)
				{
					if (Materials[i]->Texture[j].UVname == UvName[uvTag])
					{
						Materials[i]->Texture[j].UVTag=uvTag;
						break;
					}
				}
				for (uvTag=0; uvTag<DUvName.size(); ++uvTag)
				{
					if (Materials[i]->Texture[j].UVname == DUvName[uvTag])
					{
						Materials[i]->Texture[j].DUVTag=uvTag;
						break;
					}
				}
			}
		}
	}
#ifdef LWO_READER_DEBUG
	os::Printer::log("LWO loader: Creating polys.");
#endif
	// create actual geometry for lwo2
	if (FormatVersion==2)
	{
		core::array<u32> vertexCount;
		vertexCount.reallocate(Materials.size());
		for (i=0; i<Materials.size(); ++i)
			vertexCount.push_back(0);
		for (u32 polyIndex=0; polyIndex<Indices.size(); ++polyIndex)
			vertexCount[MaterialMapping[polyIndex]] += Indices[polyIndex].size();
		for (i=0; i<Materials.size(); ++i)
		{
			Materials[i]->Meshbuffer->Vertices.reallocate(vertexCount[i]);
			Materials[i]->Meshbuffer->Indices.reallocate(vertexCount[i]);
		}
	}
	// create actual geometry for lwo2
	for (u32 polyIndex=0; polyIndex<Indices.size(); ++polyIndex)
	{
		const u16 tag = MaterialMapping[polyIndex];
		scene::SMeshBuffer *mb=Materials[tag]->Meshbuffer;
		const core::array<u32>& poly = Indices[polyIndex];
		const u32 polySize=poly.size();
		const u16 uvTag = Materials[tag]->Texture[0].UVTag;
		const u16 duvTag = Materials[tag]->Texture[0].DUVTag;
		video::S3DVertex vertex;
		vertex.Color=0xffffffff;
		const u32 vertCount=mb->Vertices.size();
		for (u32 i=0; i<polySize; ++i)
		{
			const u32 j=poly[i];
			vertex.Pos=Points[j];
			if (uvTag<UvIndex.size())
			{
				for (u32 uvsearch=0; uvsearch < UvIndex[uvTag].size(); ++uvsearch)
				{
					if(j==UvIndex[uvTag][uvsearch])
					{
						vertex.TCoords=TCoords[uvTag][uvsearch];
						break;
					}
				}
				if (duvTag<DUvName.size())
				{
					for (u32 polysearch = 0; polysearch < VmPolyPointsIndex[duvTag].size(); polysearch += 2)
					{
						if (polyIndex==VmPolyPointsIndex[duvTag][polysearch] && j==VmPolyPointsIndex[duvTag][polysearch+1])
						{
							vertex.TCoords=VmCoordsIndex[duvTag][polysearch/2];
							break;
						}
					}
				}
			}
			mb->Vertices.push_back(vertex);
		}
		// triangulate as trifan
		if (polySize>2)
		{
			for (u32 i=1; i<polySize-1; ++i)
			{
				mb->Indices.push_back(vertCount);
				mb->Indices.push_back(vertCount+i);
				mb->Indices.push_back(vertCount+i+1);
			}
		}
	}
#ifdef LWO_READER_DEBUG
	os::Printer::log("LWO loader: Fixing meshbuffers.");
#endif
	for (u32 i=0; i<Materials.size(); ++i)
	{
#ifdef LWO_READER_DEBUG
		os::Printer::log("LWO loader: Material name", Materials[i]->Name);
		os::Printer::log("LWO loader: Vertex count", core::stringc(Materials[i]->Meshbuffer->Vertices.size()));
#endif
		if (!Materials[i]->Meshbuffer->Vertices.size())
		{
			Materials[i]->Meshbuffer->drop();
			delete Materials[i];
			continue;
		}
		for (u32 j=0; j<Materials[i]->Meshbuffer->Vertices.size(); ++j)
			Materials[i]->Meshbuffer->Vertices[j].Color=Materials[i]->Meshbuffer->Material.DiffuseColor;
		Materials[i]->Meshbuffer->recalculateBoundingBox();

		// load textures
		video::SMaterial& irrMat=Materials[i]->Meshbuffer->Material;
		if (Materials[i]->Texture[0].Map != "") // diffuse
			irrMat.setTexture(0,loadTexture(Materials[i]->Texture[0].Map));
		if (Materials[i]->Texture[3].Map != "") // reflection
		{
#ifdef LWO_READER_DEBUG
			os::Printer::log("LWO loader: loading reflection texture.");
#endif
			video::ITexture* reflTexture = loadTexture(Materials[i]->Texture[3].Map);
			if (reflTexture && irrMat.getTexture(0))
				irrMat.setTexture(1, irrMat.getTexture(0));
			irrMat.setTexture(0, reflTexture);
			irrMat.MaterialType=video::EMT_REFLECTION_2_LAYER;
		}
		if (Materials[i]->Texture[4].Map != "") // transparency
		{
#ifdef LWO_READER_DEBUG
			os::Printer::log("LWO loader: loading transparency texture.");
#endif
			video::ITexture* transTexture = loadTexture(Materials[i]->Texture[4].Map);
			if (transTexture && irrMat.getTexture(0))
				irrMat.setTexture(1, irrMat.getTexture(0));
			irrMat.setTexture(0, transTexture);
			irrMat.MaterialType=video::EMT_TRANSPARENT_ADD_COLOR;
		}
		if (Materials[i]->Texture[6].Map != "") // bump
		{
#ifdef LWO_READER_DEBUG
			os::Printer::log("LWO loader: loading bump texture.");
#endif
			const u8 pos = irrMat.getTexture(0)?1:0;
			irrMat.setTexture(pos, loadTexture(Materials[i]->Texture[6].Map));
			if (irrMat.getTexture(pos))
			{
	//			SceneManager->getVideoDriver()->makeNormalMapTexture(irrMat.getTexture(1));
	//			irrMat.MaterialType=video::EMT_NORMAL_MAP_SOLID;
			}
		}

		// cope with planar mapping texture coords
		if (Materials[i]->Texture[0].Projection != 5)
		{
			if (FormatVersion!=2)
			{
				if (Materials[i]->Texture[0].Flags&1)
					Materials[i]->Texture[0].Axis=0;
				else if (Materials[i]->Texture[0].Flags&2)
					Materials[i]->Texture[0].Axis=1;
				else if (Materials[i]->Texture[0].Flags&4)
					Materials[i]->Texture[0].Axis=2;
			}
			// if no axis given choose the dominant one
			else if (Materials[i]->Texture[0].Axis>2)
			{
				if (Materials[i]->Meshbuffer->getBoundingBox().getExtent().Y<Materials[i]->Meshbuffer->getBoundingBox().getExtent().X)
				{
					if (Materials[i]->Meshbuffer->getBoundingBox().getExtent().Y<Materials[i]->Meshbuffer->getBoundingBox().getExtent().Z)
						Materials[i]->Texture[0].Axis=1;
					else
						Materials[i]->Texture[0].Axis=2;
				}
				else
				{
					if (Materials[i]->Meshbuffer->getBoundingBox().getExtent().X<Materials[i]->Meshbuffer->getBoundingBox().getExtent().Z)
						Materials[i]->Texture[0].Axis=0;
					else
						Materials[i]->Texture[0].Axis=2;
				}
			}
			// get the resolution for this axis
			f32 resolutionS = core::reciprocal(Materials[i]->Texture[0].Size.Z);
			f32 resolutionT = core::reciprocal(Materials[i]->Texture[0].Size.Y);
			if (Materials[i]->Texture[0].Axis==1)
			{
				resolutionS = core::reciprocal(Materials[i]->Texture[0].Size.X);
				resolutionT = core::reciprocal(Materials[i]->Texture[0].Size.Z);
			}
			else if (Materials[i]->Texture[0].Axis==2)
			{
				resolutionS = core::reciprocal(Materials[i]->Texture[0].Size.X);
				resolutionT = core::reciprocal(Materials[i]->Texture[0].Size.Y);
			}
			// use the two-way planar mapping
			SceneManager->getMeshManipulator()->makePlanarTextureMapping(Materials[i]->Meshbuffer, resolutionS, resolutionT, Materials[i]->Texture[0].Axis, Materials[i]->Texture[0].Center);
		}

		// add bump maps
		if (Materials[i]->Meshbuffer->Material.MaterialType==video::EMT_NORMAL_MAP_SOLID)
		{
			SMesh* tmpmesh = new SMesh();
			tmpmesh->addMeshBuffer(Materials[i]->Meshbuffer);
			SceneManager->getMeshManipulator()->createMeshWithTangents(tmpmesh, true, true);
			Mesh->addMeshBuffer(tmpmesh->getMeshBuffer(0));
			tmpmesh->getMeshBuffer(0)->drop();
			tmpmesh->drop();
		}
		else
		{
			SceneManager->getMeshManipulator()->recalculateNormals(Materials[i]->Meshbuffer);
			Mesh->addMeshBuffer(Materials[i]->Meshbuffer);
		}
		Materials[i]->Meshbuffer->drop();
		// clear the material array elements
		delete Materials[i];
	}
	Mesh->recalculateBoundingBox();

	SAnimatedMesh* am = new SAnimatedMesh();
	am->Type = EAMT_3DS;
	am->addMesh(Mesh);
	am->recalculateBoundingBox();
	Mesh->drop();
	Mesh = 0;

	Points.clear();
	Indices.clear();
	MaterialMapping.clear();
	TCoords.clear();
	Materials.clear();
	Images.clear();
	VmPolyPointsIndex.clear();
	VmCoordsIndex.clear();
	UvIndex.clear();
	UvName.clear();

	return am;
}


bool CLWOMeshFileLoader::readChunks()
{
	s32 lastPos;
	u32 size;
	unsigned int uiType;
	char type[5];
	type[4]=0;
	tLWOLayerInfo layer;

	while(File->getPos()<File->getSize())
	{
		File->read(&type, 4);
		//Convert 4-char string to 4-byte integer
		//Makes it possible to do a switch statement
		uiType = charsToUInt(type);
		File->read(&size, 4);
#ifndef __BIG_ENDIAN__
		size=os::Byteswap::byteswap(size);
#endif
		lastPos=File->getPos();

		switch(uiType)
		{
			case charsToUIntD('L','A','Y','R'):
				{
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader: loading layer.");
#endif
					u16 tmp16;
					File->read(&tmp16, 2); // number
					File->read(&tmp16, 2); // flags
					size -= 4;
#ifndef __BIG_ENDIAN__
					tmp16=os::Byteswap::byteswap(tmp16);
#endif
					if (((FormatVersion==1)&&(tmp16!=1)) ||
						((FormatVersion==2)&&(tmp16&1)))
						layer.Active=false;
					else
						layer.Active=true;
					if (FormatVersion==2)
						size -= readVec(layer.Pivot);
					size -= readString(layer.Name);
					if (size)
					{
						File->read(&tmp16, 2);
#ifndef __BIG_ENDIAN__
						tmp16=os::Byteswap::byteswap(tmp16);
#endif
						layer.Parent = tmp16;
					}
				}
				break;
			case charsToUIntD('P','N','T','S'):
				{
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader: loading points.");
#endif
					core::vector3df vec;
					Points.clear();
					const u32 tmpsize = size/12;
					Points.reallocate(tmpsize);
					for (u32 i=0; i<tmpsize; ++i)
					{
						readVec(vec);
						Points.push_back(vec);
					}
				}
				break;
			case charsToUIntD('V','M','A','P'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading Vertex mapping.");
#endif
				readVertexMapping(size);
				break;
			case charsToUIntD('P','O','L','S'):
			case charsToUIntD('P','T','C','H'): // TODO: should be a subdivison mesh
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading polygons.");
#endif
				if (FormatVersion!=2)
					readObj1(size);
				else
					readObj2(size);
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: Done loading polygons.");
#endif
				break;
			case charsToUIntD('T','A','G','S'):
			case charsToUIntD('S','R','F','S'):
				{
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader: loading surface names.");
#endif
					while (size!=0)
					{
						tLWOMaterial *mat=new tLWOMaterial();
						mat->Name="";
						mat->Meshbuffer=new scene::SMeshBuffer();
						size -= readString(mat->Name);
						if (FormatVersion!=2)
							mat->TagType = 1; // format 2 has more types
						Materials.push_back(mat);
					}
				}
				break;
			case charsToUIntD('P','T','A','G'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading tag mapping.");
#endif
				readTagMapping(size);
				break;
			case charsToUIntD('V','M','A','D'): // discontinuous vertex mapping, i.e. additional texcoords
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading Vertex mapping VMAD.");
#endif
				readDiscVertexMapping(size);
//			case charsToUIntD('V','M','P','A'):
//			case charsToUIntD('E','N','V','L'):
				break;
			case charsToUIntD('C','L','I','P'):
				{
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader: loading clips.");
#endif
					u32 index;
					u16 subsize;
					File->read(&index, 4);
#ifndef __BIG_ENDIAN__
					index=os::Byteswap::byteswap(index);
#endif
					size -= 4;
					while (size != 0)
					{
						File->read(&type, 4);
						File->read(&subsize, 2);
#ifndef __BIG_ENDIAN__
						subsize=os::Byteswap::byteswap(subsize);
#endif
						size -= 6;
						if (strncmp(type, "STIL", 4))
						{
							File->seek(subsize, true);
							size -= subsize;
							continue;
						}
						core::stringc path;
						size -= readString(path, subsize);
	#ifdef LWO_READER_DEBUG
						os::Printer::log("LWO loader: loaded clip", path.c_str());
	#endif
						Images.push_back(path);
					}
				}
				break;
			case charsToUIntD('S','U','R','F'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading material.");
#endif
				readMat(size);
				break;
			case charsToUIntD('B','B','O','X'):
				{
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader: loading bbox.");
#endif
					// not stored
					core::vector3df vec;
					for (u32 i=0; i<2; ++i)
						readVec(vec);
					size -= 24;
				}
				break;
			case charsToUIntD('D','E','S','C'):
			case charsToUIntD('T','E','X','T'):
				{
					core::stringc text;
					size -= readString(text, size);
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader text", text);
#endif
				}
				break;
			// not needed
			case charsToUIntD('I','C','O','N'):
			// not yet supported
			case charsToUIntD('P','C','H','S'):
			case charsToUIntD('C','R','V','S'):
			default:
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: skipping ", type);
#endif
				//Go to next chunk
				File->seek(lastPos + size, false);
				break;
		}
	}
	return true;
}


void CLWOMeshFileLoader::readObj1(u32 size)
{
	u32 pos;
	u16 numVerts, vertIndex;
	s16 material;
	video::S3DVertex vertex;
	vertex.Color=0xffffffff;

	while (size!=0)
	{
		File->read(&numVerts, 2);
#ifndef __BIG_ENDIAN__
		numVerts=os::Byteswap::byteswap(numVerts);
#endif
		pos=File->getPos();
		// skip forward to material number
		File->seek(2*numVerts, true);
		File->read(&material, 2);
#ifndef __BIG_ENDIAN__
		material=os::Byteswap::byteswap(material);
#endif
		size -=2*numVerts+4;
		// detail meshes ?
		scene::SMeshBuffer *mb;
		if (material<0)
			mb=Materials[-material-1]->Meshbuffer;
		else
			mb=Materials[material-1]->Meshbuffer;
		// back to vertex list start
		File->seek(pos, false);

		const u16 vertCount=mb->Vertices.size();
		for (u16 i=0; i<numVerts; ++i)
		{
			File->read(&vertIndex, 2);
#ifndef __BIG_ENDIAN__
			vertIndex=os::Byteswap::byteswap(vertIndex);
#endif
			vertex.Pos=Points[vertIndex];
			mb->Vertices.push_back(vertex);
		}
		for (u16 i=1; i<numVerts-1; ++i)
		{
			mb->Indices.push_back(vertCount);
			mb->Indices.push_back(vertCount+i);
			mb->Indices.push_back(vertCount+i+1);
		}
		// skip material number and detail surface count
		// detail surface can be read just as a normal one now
		if (material<0)
			File->read(&material, 2);
		File->read(&material, 2);
	}
}


void CLWOMeshFileLoader::readVertexMapping(u32 size)
{
	char type[5]={0};
	u16 dimension;
	core::stringc name;
	File->read(&type, 4);
#ifdef LWO_READER_DEBUG
	os::Printer::log("LWO loader: Vertex map type", type);
#endif
	File->read(&dimension,2);
#ifndef __BIG_ENDIAN__
	dimension=os::Byteswap::byteswap(dimension);
#endif
	size -= 6;
	size -= readString(name);
#ifdef LWO_READER_DEBUG
	os::Printer::log("LWO loader: Vertex map", name.c_str());
#endif
	if (strncmp(type, "TXUV", 4)) // also support RGB, RGBA, WGHT, ...
	{
		File->seek(size, true);
		return;
	}
	UvName.push_back(name);

	TCoords.push_back(core::array<core::vector2df>());
	core::array<core::vector2df>& UvCoords=TCoords.getLast();
	UvCoords.reallocate(Points.size());
	UvIndex.push_back(core::array<u32>());
	core::array<u32>& UvPointsArray=UvIndex.getLast();
	UvPointsArray.reallocate(Points.size());

	u32 index;
	core::vector2df tcoord;
	while (size)
	{
		size -= readVX(index);
		File->read(&tcoord.X, 4);
		File->read(&tcoord.Y, 4);
		size -= 8;
#ifndef __BIG_ENDIAN__
		index=os::Byteswap::byteswap(index);
		tcoord.X=os::Byteswap::byteswap(tcoord.X);
		tcoord.Y=os::Byteswap::byteswap(tcoord.Y);
#endif
		UvCoords.push_back(tcoord);
		UvPointsArray.push_back(index);
	}
#ifdef LWO_READER_DEBUG
	os::Printer::log("LWO loader: UvCoords", core::stringc(UvCoords.size()));
#endif
}


void CLWOMeshFileLoader::readDiscVertexMapping(u32 size)
{
	char type[5]={0};
	u16 dimension;
	core::stringc name;
	File->read(&type, 4);
#ifdef LWO_READER_DEBUG
	os::Printer::log("LWO loader: Discontinuous vertex map type", type);
#endif
	File->read(&dimension,2);
#ifndef __BIG_ENDIAN__
	dimension=os::Byteswap::byteswap(dimension);
#endif
	size -= 6;
	size -= readString(name);
#ifdef LWO_READER_DEBUG
	os::Printer::log("LWO loader: Discontinuous vertex map", name.c_str());
#endif
	if (strncmp(type, "TXUV", 4))
	{
		File->seek(size, true);
		return;
	}
	DUvName.push_back(name);
	VmPolyPointsIndex.push_back(core::array<u32>());
	core::array<u32>& VmPolyPoints=VmPolyPointsIndex.getLast();

	VmCoordsIndex.push_back(core::array<core::vector2df>());
	core::array<core::vector2df>& VmCoords=VmCoordsIndex.getLast();

	u32 vmpolys;
	u32 vmpoints;
	core::vector2df vmcoords;
	while (size)
	{
		size-=readVX(vmpoints);
		size-=readVX(vmpolys);
		File->read(&vmcoords.X, 4);
		File->read(&vmcoords.Y, 4);
		size -= 8;
#ifndef __BIG_ENDIAN__
		vmpoints=os::Byteswap::byteswap(vmpoints);
		vmpolys=os::Byteswap::byteswap(vmpolys);
		vmcoords.X=os::Byteswap::byteswap(vmcoords.X);
		vmcoords.Y=os::Byteswap::byteswap(vmcoords.Y);
#endif
		VmCoords.push_back(vmcoords);
		VmPolyPoints.push_back(vmpolys);
		VmPolyPoints.push_back(vmpoints);
	}
#ifdef LWO_READER_DEBUG
	os::Printer::log("LWO loader: VmCoords", core::stringc(VmCoords.size()));
#endif
}


void CLWOMeshFileLoader::readTagMapping(u32 size)
{
	char type[5];
	type[4]=0;
	File->read(&type, 4);
	size -= 4;
	if ((strncmp(type, "SURF", 4))||(Indices.size()==0))
	{
		File->seek(size, true);
		return;
	}

	while (size!=0)
	{
		u16 tag;
		u32 polyIndex;
		size-=readVX(polyIndex);
		File->read(&tag, 2);
#ifndef __BIG_ENDIAN__
		tag=os::Byteswap::byteswap(tag);
#endif
		size -= 2;
		MaterialMapping[polyIndex]=tag;
		Materials[tag]->TagType=1;
	}
}


void CLWOMeshFileLoader::readObj2(u32 size)
{
	char type[5];
	type[4]=0;
	File->read(&type, 4);
	size -= 4;
	Indices.clear();
	if (strncmp(type, "FACE", 4)) // also possible are splines, subdivision patches, metaballs, and bones
	{
		File->seek(size, true);
		return;
	}
	u16 numVerts=0;
	while (size!=0)
	{
		File->read(&numVerts, 2);
#ifndef __BIG_ENDIAN__
		numVerts=os::Byteswap::byteswap(numVerts);
#endif
		// mask out flags
		numVerts &= 0x03FF;

		size -= 2;
		Indices.push_back(core::array<u32>());
		u32 vertIndex;
		core::array<u32>& polyArray = Indices.getLast();
		polyArray.reallocate(numVerts);
		for (u16 i=0; i<numVerts; ++i)
		{
			size -= readVX(vertIndex);
			polyArray.push_back(vertIndex);
		}
	}
	MaterialMapping.reallocate(Indices.size());
	for (u32 j=0; j<Indices.size(); ++j)
		MaterialMapping.push_back(0);
}


void CLWOMeshFileLoader::readMat(u32 size)
{
	core::stringc name;

	tLWOMaterial* mat=0;
	size -= readString(name);
#ifdef LWO_READER_DEBUG
	os::Printer::log("LWO loader: material name", name.c_str());
#endif
	for (u32 i=0; i<Materials.size(); ++i)
	{
		if ((Materials[i]->TagType==1) && (Materials[i]->Name==name))
		{
			mat=Materials[i];
			break;
		}
	}
	if (!mat)
	{
		File->seek(size, true);
		return;
	}
	if (FormatVersion==2)
		size -= readString(name);

	video::SMaterial& irrMat=mat->Meshbuffer->Material;

	u8 currTexture=0;
	while (size!=0)
	{
		char type[5];
		type[4]=0;
		u32 uiType;
		u32 tmp32;
		u16 subsize, tmp16;
		f32 tmpf32;
		File->read(&type, 4);
		//Convert 4-char string to 4-byte integer
		//Makes it possible to do a switch statement
		uiType = charsToUInt(type);
		File->read(&subsize, 2);
#ifndef __BIG_ENDIAN__
		subsize=os::Byteswap::byteswap(subsize);
#endif
		size -= 6;
		switch (uiType)
		{
			case charsToUIntD('C','O','L','R'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading Ambient color.");
#endif
				{
					s32 colSize = readColor(irrMat.DiffuseColor);
					irrMat.AmbientColor=irrMat.DiffuseColor;
					size -= colSize;
					subsize -= colSize;
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[0]);
				}
				break;
			case charsToUIntD('D','I','F','F'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading Diffuse color.");
#endif
				{
					if (FormatVersion==2)
					{
						File->read(&mat->Diffuse, 4);
#ifndef __BIG_ENDIAN__
						mat->Diffuse=os::Byteswap::byteswap(mat->Diffuse);
#endif
						size -= 4;
						subsize -= 4;
						size -= readVX(mat->Envelope[1]);
					}
					else
					{
						File->read(&tmp16, 2);
#ifndef __BIG_ENDIAN__
						tmp16=os::Byteswap::byteswap(tmp16);
#endif
						mat->Diffuse=tmp16/256.0f;
						size -= 2;
						subsize -= 2;
					}
				}
				break;
			case charsToUIntD('V','D','I','F'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading Diffuse color.");
#endif
				{
					File->read(&mat->Diffuse, 4);
#ifndef __BIG_ENDIAN__
					mat->Diffuse=os::Byteswap::byteswap(mat->Diffuse);
#endif
					size -= 4;
				}
				break;
			case charsToUIntD('L','U','M','I'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading luminance.");
#endif
				{
					if (FormatVersion==2)
					{
						File->read(&mat->Luminance, 4);
#ifndef __BIG_ENDIAN__
						mat->Luminance=os::Byteswap::byteswap(mat->Luminance);
#endif
						size -= 4;
						subsize -= 4;
						size -= readVX(mat->Envelope[2]);
					}
					else
					{
						File->read(&tmp16, 2);
#ifndef __BIG_ENDIAN__
						tmp16=os::Byteswap::byteswap(tmp16);
#endif
						mat->Luminance=tmp16/256.0f;
						size -= 2;
						subsize -= 2;
					}				}
				break;
			case charsToUIntD('V','L','U','M'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading luminance.");
#endif
				{
					File->read(&mat->Luminance, 4);
#ifndef __BIG_ENDIAN__
					mat->Luminance=os::Byteswap::byteswap(mat->Luminance);
#endif
					size -= 4;
				}
				break;
			case charsToUIntD('S','P','E','C'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading specular.");
#endif
				{
					if (FormatVersion==2)
					{
						File->read(&mat->Specular, 4);
#ifndef __BIG_ENDIAN__
						mat->Specular=os::Byteswap::byteswap(mat->Specular);
#endif
						size -= 4;
						subsize -= 4;
						size -= readVX(mat->Envelope[3]);
					}
					else
					{
						File->read(&tmp16, 2);
#ifndef __BIG_ENDIAN__
						tmp16=os::Byteswap::byteswap(tmp16);
#endif
						mat->Specular=tmp16/256.0f;;
						size -= 2;
						subsize -= 2;
					}
				}
				break;
			case charsToUIntD('V','S','P','C'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading specular.");
#endif
				{
					File->read(&mat->Specular, 4);
#ifndef __BIG_ENDIAN__
					mat->Specular=os::Byteswap::byteswap(mat->Specular);
#endif
					size -= 4;
				}
				break;
			case charsToUIntD('R','E','F','L'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading reflection.");
#endif
				{
					if (FormatVersion==2)
					{
						File->read(&mat->Reflection, 4);
#ifndef __BIG_ENDIAN__
						mat->Reflection=os::Byteswap::byteswap(mat->Reflection);
#endif
						size -= 4;
						subsize -= 4;
						size -= readVX(mat->Envelope[4]);
					}
					else
					{
						File->read(&tmp16, 2);
#ifndef __BIG_ENDIAN__
						tmp16=os::Byteswap::byteswap(tmp16);
#endif
						mat->Reflection=tmp16/256.0f;
						size -= 2;
						subsize -= 2;
					}
				}
				break;
			case charsToUIntD('V','R','F','L'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading reflection.");
#endif
				{
					File->read(&mat->Reflection, 4);
#ifndef __BIG_ENDIAN__
					mat->Reflection=os::Byteswap::byteswap(mat->Reflection);
#endif
					size -= 4;
				}
				break;
			case charsToUIntD('T','R','A','N'):
				{
					if (FormatVersion==2)
					{
						File->read(&mat->Transparency, 4);
#ifndef __BIG_ENDIAN__
						mat->Transparency=os::Byteswap::byteswap(mat->Transparency);
#endif
						size -= 4;
						subsize -= 4;
						size -= readVX(mat->Envelope[5]);
					}
					else
					{
						File->read(&tmp16, 2);
#ifndef __BIG_ENDIAN__
						tmp16=os::Byteswap::byteswap(tmp16);
#endif
						mat->Transparency=tmp16/256.0f;
						size -= 2;
						subsize -= 2;
					}
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading transparency", core::stringc(mat->Transparency).c_str());
#endif
				}
				break;
			case charsToUIntD('V','T','R','N'):
				{
					File->read(&mat->Transparency, 4);
#ifndef __BIG_ENDIAN__
					mat->Transparency=os::Byteswap::byteswap(mat->Transparency);
#endif
					size -= 4;
				}
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading transparency", core::stringc(mat->Transparency).c_str());
#endif
				break;
			case charsToUIntD('T','R','N','L'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading translucency.");
#endif
				{
					File->read(&mat->Translucency, 4);
#ifndef __BIG_ENDIAN__
					mat->Translucency=os::Byteswap::byteswap(mat->Translucency);
#endif
					size -= 4;
					subsize -= 4;
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[6]);
				}
				break;
			case charsToUIntD('G','L','O','S'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading glossy.");
#endif
				{
					if (FormatVersion == 2)
					{
						File->read(&irrMat.Shininess, 4);
#ifndef __BIG_ENDIAN__
						irrMat.Shininess=os::Byteswap::byteswap(irrMat.Shininess);
#endif
						size -= 4;
						subsize -= 4;
						size -= readVX(mat->Envelope[7]);
					}
					else
					{
						File->read(&tmp16, 2);
#ifndef __BIG_ENDIAN__
						tmp16=os::Byteswap::byteswap(tmp16);
#endif
						irrMat.Shininess=tmp16/16.f;
						size -= 2;
						subsize -= 2;
					}
				}
				break;
			case charsToUIntD('S','H','R','P'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading sharpness.");
#endif
				{
					File->read(&mat->Sharpness, 4);
#ifndef __BIG_ENDIAN__
					mat->Sharpness=os::Byteswap::byteswap(mat->Sharpness);
#endif
					size -= 4;
					subsize -= 4;
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[8]);
				}
				break;
			case charsToUIntD('B','U','M','P'):
			case charsToUIntD('T','A','M','P'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading bumpiness.");
#endif
				{
					File->read(&tmpf32, 4);
#ifndef __BIG_ENDIAN__
						tmpf32=os::Byteswap::byteswap(tmpf32);
#endif
					if (currTexture==6)
						irrMat.MaterialTypeParam=tmpf32;
					size -= 4;
					subsize -= 4;
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[9]);
				}
				break;
			case charsToUIntD('S','I','D','E'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading backface culled.");
#endif
				{
					File->read(&tmp16, 2);
#ifndef __BIG_ENDIAN__
					tmp16=os::Byteswap::byteswap(tmp16);
#endif
					if (tmp16==1)
						irrMat.BackfaceCulling=true;
					else if (tmp16==3)
						irrMat.BackfaceCulling=false;
					size -= 2;
				}
				break;
			case charsToUIntD('S','M','A','N'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading smoothing angle.");
#endif
				{
					File->read(&mat->SmoothingAngle, 4);
#ifndef __BIG_ENDIAN__
					mat->SmoothingAngle=os::Byteswap::byteswap(mat->SmoothingAngle);
#endif
					size -= 4;
				}
				break;
			case charsToUIntD('R','F','O','P'):
			case charsToUIntD('R','F','L','T'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading reflection mode.");
#endif
				{
					File->read(&mat->ReflMode, 2);
#ifndef __BIG_ENDIAN__
					mat->ReflMode=os::Byteswap::byteswap(mat->ReflMode);
#endif
					size -= 2;
				}
				break;
			case charsToUIntD('R','I','M','G'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading reflection map.");
#endif
				{
					if (FormatVersion==2)
					{
						size -= readVX(tmp32);
						if (tmp32)
							mat->ReflMap=Images[tmp32-1];
					}
					else
						size -= readString(mat->ReflMap, size);
				}
				break;
			case charsToUIntD('R','S','A','N'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading reflection seam angle.");
#endif
				{
					File->read(&mat->ReflSeamAngle, 4);
#ifndef __BIG_ENDIAN__
					mat->ReflSeamAngle=os::Byteswap::byteswap(mat->ReflSeamAngle);
#endif
					size -= 4;
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[10]);
				}
				break;
			case charsToUIntD('R','B','L','R'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading reflection blur.");
#endif
				{
					File->read(&mat->ReflBlur, 4);
#ifndef __BIG_ENDIAN__
					mat->ReflBlur=os::Byteswap::byteswap(mat->ReflBlur);
#endif
					size -= 4;
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[11]);
				}
				break;
			case charsToUIntD('R','I','N','D'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading refraction index.");
#endif
				{
					File->read(&mat->RefrIndex, 4);
#ifndef __BIG_ENDIAN__
					mat->RefrIndex=os::Byteswap::byteswap(mat->RefrIndex);
#endif
					size -= 4;
					subsize -= 4;
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[12]);
				}
				break;
			case charsToUIntD('T','R','O','P'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading refraction options.");
#endif
				{
					File->read(&mat->TranspMode, 2);
#ifndef __BIG_ENDIAN__
					mat->TranspMode=os::Byteswap::byteswap(mat->TranspMode);
#endif
					size -= 2;
				}
				break;
			case charsToUIntD('T','I','M','G'):
				{
					if (FormatVersion==2)
					{
#ifdef LWO_READER_DEBUG
						os::Printer::log("LWO loader: loading refraction map.");
#endif
						size -= readVX(tmp32);
#ifndef __BIG_ENDIAN__
						tmp32=os::Byteswap::byteswap(tmp32);
#endif
						if (tmp32)
							mat->Texture[currTexture].Map=Images[tmp32-1];
					}
					else
					{
						size -= readString(mat->Texture[currTexture].Map, size);
#ifdef LWO_READER_DEBUG
						os::Printer::log("LWO loader: loading image", mat->Texture[currTexture].Map.c_str());
#endif
					}
				}
				break;
			case charsToUIntD('T','B','L','R'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading transparency blur.");
#endif
				{
					File->read(&mat->TranspBlur, 4);
#ifndef __BIG_ENDIAN__
					mat->TranspBlur=os::Byteswap::byteswap(mat->TranspBlur);
#endif
					size -= 4;
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[13]);
				}
				break;
			case charsToUIntD('C','L','R','H'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading highlight color.");
#endif
				{
					File->read(&mat->HighlightColor, 4);
#ifndef __BIG_ENDIAN__
					mat->HighlightColor=os::Byteswap::byteswap(mat->HighlightColor);
#endif
					size -= 4;
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[14]);
				}
				break;
			case charsToUIntD('C','L','R','F'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading color filter.");
#endif
				{
					File->read(&mat->ColorFilter, 4);
#ifndef __BIG_ENDIAN__
					mat->ColorFilter=os::Byteswap::byteswap(mat->ColorFilter);
#endif
					size -= 4;
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[15]);
				}
				break;
			case charsToUIntD('A','D','T','R'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading additive transparency.");
#endif
				{
					File->read(&mat->AdditiveTransparency, 4);
#ifndef __BIG_ENDIAN__
					mat->AdditiveTransparency=os::Byteswap::byteswap(mat->AdditiveTransparency);
#endif
					size -= 4;
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[16]);
				}
				break;
			case charsToUIntD('G','L','O','W'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading glow.");
#endif
				{
					if (FormatVersion==0)
					{
						File->read(&mat->GlowIntensity, 4);
#ifndef __BIG_ENDIAN__
						mat->GlowIntensity=os::Byteswap::byteswap(mat->GlowIntensity);
#endif
						size -= 4;
					}
					else
					{
						File->read(&mat->Glow, 2);
#ifndef __BIG_ENDIAN__
						mat->Glow=os::Byteswap::byteswap(mat->Glow);
#endif
						size -= 2;
						File->read(&mat->GlowIntensity, 4);
#ifndef __BIG_ENDIAN__
						mat->GlowIntensity=os::Byteswap::byteswap(mat->GlowIntensity);
#endif
						size -= 4;
						if (FormatVersion==2)
							size -= readVX(mat->Envelope[17]);
						File->read(&mat->GlowSize, 4);
#ifndef __BIG_ENDIAN__
						mat->GlowSize=os::Byteswap::byteswap(mat->GlowSize);
#endif
						size -= 4;
						if (FormatVersion==2)
							size -= readVX(mat->Envelope[18]);
					}
				}
				break;
			case charsToUIntD('G','V','A','L'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading glow intensity.");
#endif
				{
					File->read(&mat->GlowIntensity, 4);
#ifndef __BIG_ENDIAN__
					mat->GlowIntensity=os::Byteswap::byteswap(mat->GlowIntensity);
#endif
					size -= 4;
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[17]);
				}
				break;
			case charsToUIntD('L','I','N','E'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading isWireframe.");
#endif
				{
					File->read(&tmp16, 2);
#ifndef __BIG_ENDIAN__
					tmp16=os::Byteswap::byteswap(tmp16);
#endif
					if (tmp16&1)
						irrMat.Wireframe=true;
					size -= 2;
					if (size!=0)
					{
						File->read(&irrMat.Thickness, 4);
#ifndef __BIG_ENDIAN__
						irrMat.Thickness=os::Byteswap::byteswap(irrMat.Thickness);
#endif
						size -= 4;
						if (FormatVersion==2)
							size -= readVX(mat->Envelope[19]);
					}
					if (size!=0)
					{
						video::SColor lineColor;
						size -= readColor(lineColor);
						if (FormatVersion==2)
							size -= readVX(mat->Envelope[20]);
					}
				}
				break;
			case charsToUIntD('A','L','P','H'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading alpha mode.");
#endif
				{
					File->read(&mat->AlphaMode, 2);
#ifndef __BIG_ENDIAN__
					mat->AlphaMode=os::Byteswap::byteswap(mat->AlphaMode);
#endif
					size -= 2;
					File->read(&mat->AlphaValue, 4);
#ifndef __BIG_ENDIAN__
					mat->AlphaValue=os::Byteswap::byteswap(mat->AlphaValue);
#endif
					size -= 4;
				}
				break;
			case charsToUIntD('V','C','O','L'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading vertex color.");
#endif
				{
					File->read(&mat->VertexColorIntensity, 4);
#ifndef __BIG_ENDIAN__
					mat->VertexColorIntensity=os::Byteswap::byteswap(mat->VertexColorIntensity);
#endif
					size -= 4;
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[21]);
					File->read(&tmp32, 4); // skip type
					size -= 4;
					core::stringc tmpname;
					size -= readString(tmpname, size);
//					mat->VertexColor = getColorVMAP(tmpname);
				}
				break;
			case charsToUIntD('F','L','A','G'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading flag.");
#endif
				{
					File->read(&mat->Flags, 2);
#ifndef __BIG_ENDIAN__
					mat->Flags=os::Byteswap::byteswap(mat->Flags);
#endif
					if (mat->Flags&1)
						mat->Luminance=1.0f;
					if (mat->Flags&256)
						irrMat.BackfaceCulling=false;
					size -= 2;
				}
				break;
			case charsToUIntD('E','D','G','E'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading edge.");
#endif
				{
					File->read(&mat->EdgeTransparency, 4);
#ifndef __BIG_ENDIAN__
					mat->EdgeTransparency=os::Byteswap::byteswap(mat->EdgeTransparency);
#endif
					size -= 4;
				}
				break;
			case charsToUIntD('C','T','E','X'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading ctex.");
#endif
				currTexture=0;
				size -= readString(mat->Texture[currTexture].Type, size);
				break;
			case charsToUIntD('D','T','E','X'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading dtex.");
#endif
				currTexture=1;
				size -= readString(mat->Texture[currTexture].Type, size);
				break;
			case charsToUIntD('S','T','E','X'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading stex.");
#endif
				currTexture=2;
				size -= readString(mat->Texture[currTexture].Type, size);
				break;
			case charsToUIntD('R','T','E','X'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading rtex.");
#endif
				currTexture=3;
				size -= readString(mat->Texture[currTexture].Type, size);
				break;
			case charsToUIntD('T','T','E','X'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading ttex.");
#endif
				currTexture=4;
				size -= readString(mat->Texture[currTexture].Type, size);
				break;
			case charsToUIntD('L','T','E','X'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading ltex.");
#endif
				currTexture=5;
				size -= readString(mat->Texture[currTexture].Type, size);
				break;
			case charsToUIntD('B','T','E','X'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading btex.");
#endif
				currTexture=6;
				size -= readString(mat->Texture[currTexture].Type, size);
				break;
			case charsToUIntD('T','A','L','P'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading alpha map.");
#endif
				size -= readString(mat->Texture[currTexture].AlphaMap, size);
				break;
			case charsToUIntD('T','F','L','G'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading texture flag.");
#endif
				{
					File->read(&mat->Texture[currTexture].Flags, 2);
#ifndef __BIG_ENDIAN__
					mat->Texture[currTexture].Flags=os::Byteswap::byteswap(mat->Texture[currTexture].Flags);
#endif
					size -= 2;
				}
				break;
			case charsToUIntD('E','N','A','B'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading isEnabled.");
#endif
				{
					File->read(&tmp16, 2);
#ifndef __BIG_ENDIAN__
					tmp16=os::Byteswap::byteswap(tmp16);
#endif
					mat->Texture[currTexture].Active=(tmp16!=0);
					size -= 2;
				}
				break;
			case charsToUIntD('W','R','A','P'):
			case charsToUIntD('T','W','R','P'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading texture wrap.");
#endif
				{
					File->read(&mat->Texture[currTexture].WidthWrap, 2);
#ifndef __BIG_ENDIAN__
					mat->Texture[currTexture].WidthWrap=os::Byteswap::byteswap(mat->Texture[currTexture].WidthWrap);
#endif
					File->read(&mat->Texture[currTexture].HeightWrap, 2);
#ifndef __BIG_ENDIAN__
					mat->Texture[currTexture].HeightWrap=os::Byteswap::byteswap(mat->Texture[currTexture].HeightWrap);
#endif
					size -= 4;
				}
				break;
			case charsToUIntD('T','V','E','L'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading texture velocity.");
#endif
				size -= readVec(mat->Texture[currTexture].Velocity);
				break;
			case charsToUIntD('T','C','L','R'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading texture color.");
#endif
				size -= readColor(mat->Texture[currTexture].Color);
				break;
			case charsToUIntD('A','A','S','T'):
			case charsToUIntD('T','A','A','S'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading texture antialias.");
#endif
				{
					tmp16=0;
					if (FormatVersion==2)
					{
						File->read(&tmp16, 2);
#ifndef __BIG_ENDIAN__
						tmp16=os::Byteswap::byteswap(tmp16);
#endif
						size -= 2;
					}
					File->read(&mat->Texture[currTexture].AntiAliasing, 4);
#ifndef __BIG_ENDIAN__
					mat->Texture[currTexture].AntiAliasing=os::Byteswap::byteswap(mat->Texture[currTexture].AntiAliasing);
#endif
					if (tmp16 & ~0x01)
						mat->Texture[currTexture].AntiAliasing=0.0f; // disabled
					size -= 4;
				}
				break;
			case charsToUIntD('T','O','P','C'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading texture opacity.");
#endif
				{
					File->read(&mat->Texture[currTexture].Opacity, 4);
#ifndef __BIG_ENDIAN__
					mat->Texture[currTexture].Opacity=os::Byteswap::byteswap(mat->Texture[currTexture].Opacity);
#endif
					size -= 4;
				}
				break;
			case charsToUIntD('O','P','A','C'):
				{
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader: loading texture opacity and type.");
#endif
					File->read(&mat->Texture[currTexture].OpacType, 2);
#ifndef __BIG_ENDIAN__
					mat->Texture[currTexture].OpacType=os::Byteswap::byteswap(mat->Texture[currTexture].OpacType);
#endif
					File->read(&mat->Texture[currTexture].Opacity, 4);
#ifndef __BIG_ENDIAN__
					mat->Texture[currTexture].Opacity=os::Byteswap::byteswap(mat->Texture[currTexture].Opacity);
#endif
					size -= 6;
					subsize -= 6;
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[22]);
				}
				break;
			case charsToUIntD('A','X','I','S'):
				{
					File->read(&tmp16, 2);
#ifndef __BIG_ENDIAN__
					tmp16=os::Byteswap::byteswap(tmp16);
#endif
					mat->Texture[currTexture].Axis=(u8)tmp16;
					size -= 2;
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader: loading axis value", core::stringc(tmp16).c_str());
#endif
				}
				break;
			case charsToUIntD('T','M','A','P'): // empty separation chunk
				break;
			case charsToUIntD('T','C','T','R'):
			case charsToUIntD('C','N','T','R'):
				{
					core::vector3df& center=mat->Texture[currTexture].Center;
					size -= readVec(center);
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[22]);
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader: loading texture center", (core::stringc(center.X)+" "+core::stringc(center.Y)+" "+core::stringc(center.Z)).c_str());
#endif
				}
				break;
			case charsToUIntD('T','S','I','Z'):
			case charsToUIntD('S','I','Z','E'):
				{
					core::vector3df& tsize=mat->Texture[currTexture].Size;
					size -= readVec(tsize);
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[22]);
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader: loading texture size", (core::stringc(tsize.X)+" "+core::stringc(tsize.Y)+" "+core::stringc(tsize.Z)).c_str());
#endif
				}
				break;
			case charsToUIntD('R','O','T','A'):
				{
					core::vector3df rotation;
					size -= readVec(rotation);
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[22]);
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader: loading texture rotation", (core::stringc(rotation.X)+" "+core::stringc(rotation.Y)+" "+core::stringc(rotation.Z)).c_str());
#endif
				}
				break;
			case charsToUIntD('O','R','E','F'):
				{
					core::stringc tmpname;
					size -= readString(tmpname);
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: texture reference object", tmpname.c_str());
#endif
				}
				break;
			case charsToUIntD('T','F','A','L'):
			case charsToUIntD('F','A','L','L'):
				{
					if (FormatVersion==2)
					{
						u16 tmp16;
						File->read(&tmp16, 2);
						size -= 2;
#ifndef __BIG_ENDIAN__
						tmp16=os::Byteswap::byteswap(tmp16);
#endif
					}

					core::vector3df& falloff=mat->Texture[currTexture].Falloff;
					size -= readVec(falloff);
					if (FormatVersion==2)
						size -= readVX(mat->Envelope[22]);
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader: loading texture falloff");
#endif
				}
				break;
			case charsToUIntD('C','S','Y','S'):
				{
					u16 tmp16;
					File->read(&tmp16, 2);
					size -= 2;
#ifndef __BIG_ENDIAN__
					tmp16=os::Byteswap::byteswap(tmp16);
#endif
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader: texture coordinate system", tmp16==0?"object coords":"world coords");
#endif
				}
				break;
			case charsToUIntD('T','V','A','L'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading texture value.");
#endif
				{
					File->read(&tmp16, 2);
#ifndef __BIG_ENDIAN__
					tmp16=os::Byteswap::byteswap(tmp16);
#endif
					mat->Texture[currTexture].Value=tmp16/256.0f;
					size -= 2;
				}
				break;
			case charsToUIntD('T','F','P','0'):
			case charsToUIntD('T','S','P','0'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading texture param 0.");
#endif
				{
					File->read(&mat->Texture[currTexture].FParam[0], 4);
#ifndef __BIG_ENDIAN__
					mat->Texture[currTexture].FParam[0]=os::Byteswap::byteswap(mat->Texture[currTexture].FParam[0]);
#endif
					size -= 4;
				}
				break;
			case charsToUIntD('T','F','P','1'):
			case charsToUIntD('T','S','P','1'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading texture param 1.");
#endif
				{
					File->read(&mat->Texture[currTexture].FParam[1], 4);
#ifndef __BIG_ENDIAN__
					mat->Texture[currTexture].FParam[1]=os::Byteswap::byteswap(mat->Texture[currTexture].FParam[1]);
#endif
					size -= 4;
				}
				break;
			case charsToUIntD('T','F','P','2'):
			case charsToUIntD('T','S','P','2'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading texture param 2.");
#endif
				{
					File->read(&mat->Texture[currTexture].FParam[2], 4);
#ifndef __BIG_ENDIAN__
					mat->Texture[currTexture].FParam[2]=os::Byteswap::byteswap(mat->Texture[currTexture].FParam[2]);
#endif
					size -= 4;
				}
				break;
			case charsToUIntD('T','F','R','Q'):
			case charsToUIntD('T','I','P','0'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading texture iparam 0.");
#endif
				{
					File->read(&mat->Texture[currTexture].IParam[0], 2);
#ifndef __BIG_ENDIAN__
					mat->Texture[currTexture].IParam[0]=os::Byteswap::byteswap(mat->Texture[currTexture].IParam[0]);
#endif
					size -= 2;
				}
				break;
			case charsToUIntD('T','I','P','1'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading texture param 1.");
#endif
				{
					File->read(&mat->Texture[currTexture].IParam[1], 2);
#ifndef __BIG_ENDIAN__
					mat->Texture[currTexture].IParam[1]=os::Byteswap::byteswap(mat->Texture[currTexture].IParam[1]);
#endif
					size -= 2;
				}
				break;
			case charsToUIntD('T','I','P','2'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading texture param 2.");
#endif
				{
					File->read(&mat->Texture[currTexture].IParam[2], 2);
#ifndef __BIG_ENDIAN__
					mat->Texture[currTexture].IParam[2]=os::Byteswap::byteswap(mat->Texture[currTexture].IParam[2]);
#endif
					size -= 2;
				}
				break;
			case charsToUIntD('V','M','A','P'):
				{
					size -= readString(mat->Texture[currTexture].UVname);
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader: loading material vmap binding",mat->Texture[currTexture].UVname.c_str());
#endif
				}
				break;
			case charsToUIntD('B','L','O','K'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading blok.");
#endif
				{
					core::stringc ordinal;
					File->read(&type, 4);
					File->read(&subsize, 2);
#ifndef __BIG_ENDIAN__
					subsize=os::Byteswap::byteswap(subsize);
#endif
					size -= 6;
					size -= readString(ordinal, size);
				}
				break;
			case charsToUIntD('C','H','A','N'):
				{
					File->read(&type, 4);
					size -= 4;
					if (!strncmp(type, "COLR", 4))
						currTexture=0;
					else if (!strncmp(type, "DIFF", 4))
						currTexture=1;
					else if (!strncmp(type, "LUMI", 4))
						currTexture=5;
					else if (!strncmp(type, "SPEC", 4))
						currTexture=2;
					else if (!strncmp(type, "REFL", 4))
						currTexture=3;
					else if (!strncmp(type, "TRAN", 4))
						currTexture=4;
					else if (!strncmp(type, "BUMP", 4))
						currTexture=6;
				}
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading channel ", type);
#endif
				break;
			case charsToUIntD('I','M','A','G'):
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading channel map.");
#endif
				{
					u16 index;
					File->read(&index, 2);
#ifndef __BIG_ENDIAN__
					index=os::Byteswap::byteswap(index);
#endif
					size -= 2;
					if (index)
						mat->Texture[currTexture].Map=Images[index-1];
				}
				break;
			case charsToUIntD('P','R','O','J'): // define the projection type
#ifdef LWO_READER_DEBUG
				os::Printer::log("LWO loader: loading channel projection type.");
#endif
				{
					u16 index;
					File->read(&index, 2);
#ifndef __BIG_ENDIAN__
					index=os::Byteswap::byteswap(index);
#endif
					size -= 2;
#ifdef LWO_READER_DEBUG
					if (index != 5)
						os::Printer::log("LWO loader: wrong channel projection type", core::stringc(index).c_str());
#endif
					mat->Texture[currTexture].Projection=(u8)index;
				}
				break;
			case charsToUIntD('W','R','P','W'): // for cylindrical and spherical projections
			case charsToUIntD('W','R','P','H'): // for cylindrical and spherical projections
			default:
				{
#ifdef LWO_READER_DEBUG
					os::Printer::log("LWO loader: skipping ", core::stringc((char*)&uiType, 4));
#endif
					File->seek(subsize, true);
					size -= subsize;
				}
		}
	}

	if (mat->Transparency != 0.f)
	{
		irrMat.MaterialType=video::EMT_TRANSPARENT_ADD_COLOR;
	}
}


u32 CLWOMeshFileLoader::readColor(video::SColor& color)
{
	if (FormatVersion!=2)
	{
		u8 colorComponent;
		File->read(&colorComponent, 1);
		color.setRed(colorComponent);
		File->read(&colorComponent, 1);
		color.setGreen(colorComponent);
		File->read(&colorComponent, 1);
		color.setBlue(colorComponent);
		// unknown value
		File->read(&colorComponent, 1);
		return 4;
	}
	else
	{
		video::SColorf col;
		File->read(&col.r, 4);
#ifndef __BIG_ENDIAN__
		col.r=os::Byteswap::byteswap(col.r);
#endif
		File->read(&col.g, 4);
#ifndef __BIG_ENDIAN__
		col.g=os::Byteswap::byteswap(col.g);
#endif
		File->read(&col.b, 4);
#ifndef __BIG_ENDIAN__
		col.b=os::Byteswap::byteswap(col.b);
#endif
		color=col.toSColor();
		return 12;
	}
}

u32 CLWOMeshFileLoader::readString(core::stringc& name, u32 size)
{
	c8 c;

	name="";
	if (size)
		name.reserve(size);
	File->read(&c, 1);
	while (c)
	{
		name.append(c);
		File->read(&c, 1);
	}
	// read extra 0 upon odd file position
	if (File->getPos() & 0x1)
	{
		File->read(&c, 1);
		return (name.size()+2);
	}
	return (name.size()+1);
}


u32 CLWOMeshFileLoader::readVec(core::vector3df& vec)
{
	File->read(&vec.X, 4);
#ifndef __BIG_ENDIAN__
	vec.X=os::Byteswap::byteswap(vec.X);
#endif
	File->read(&vec.Y, 4);
#ifndef __BIG_ENDIAN__
	vec.Y=os::Byteswap::byteswap(vec.Y);
#endif
	File->read(&vec.Z, 4);
#ifndef __BIG_ENDIAN__
	vec.Z=os::Byteswap::byteswap(vec.Z);
#endif
	return 12;
}


u32 CLWOMeshFileLoader::readVX(u32& num)
{
	u16 tmpIndex;

	File->read(&tmpIndex, 2);
#ifndef __BIG_ENDIAN__
	tmpIndex=os::Byteswap::byteswap(tmpIndex);
#endif
	num=tmpIndex;
	if (num >= 0xFF00)
	{
		File->read(&tmpIndex, 2);
#ifndef __BIG_ENDIAN__
		tmpIndex=os::Byteswap::byteswap(tmpIndex);
#endif
		num=((num << 16)|tmpIndex) & ~0xFF000000;
		return 4;
	}
	return 2;
}


bool CLWOMeshFileLoader::readFileHeader()
{
	u32 Id;

	File->read(&Id, 4);
#ifndef __BIG_ENDIAN__
	Id=os::Byteswap::byteswap(Id);
#endif
	if (Id != 0x464f524d) // FORM
		return false;

	//skip the file length
	File->read(&Id, 4);

	File->read(&Id, 4);
#ifndef __BIG_ENDIAN__
	Id=os::Byteswap::byteswap(Id);
#endif
	// Currently supported: LWOB, LWLO, LWO2
	switch (Id)
	{
		case 0x4c574f42:
			FormatVersion = 0; // LWOB
		break;
		case 0x4c574c4f:
			FormatVersion = 1; // LWLO
		break;
		case 0x4c574f32:
			FormatVersion = 2; // LWO2
		break;
		default:
			return false; // unsupported
	}

	return true;
}


video::ITexture* CLWOMeshFileLoader::loadTexture(const core::stringc& file)
{
	video::IVideoDriver* driver = SceneManager->getVideoDriver();

	if (FileSystem->existFile(file))
		return driver->getTexture(file);

	core::stringc strippedName=FileSystem->getFileBasename(file);
	if (FileSystem->existFile(strippedName))
		return driver->getTexture(strippedName);
	core::stringc newpath = FileSystem->getFileDir(File->getFileName());
	newpath.append("/");
	newpath.append(strippedName);
	if (FileSystem->existFile(newpath))
		return driver->getTexture(newpath);
	os::Printer::log("Could not load texture", file.c_str(), ELL_WARNING);

	return 0;
}


} // end namespace scene
} // end namespace irr


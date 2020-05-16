// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_MS3D_LOADER_

#include "IReadFile.h"
#include "os.h"
#include "CMS3DMeshFileLoader.h"
#include "CSkinnedMesh.h"


namespace irr
{
namespace scene
{

#ifdef _DEBUG
#define _IRR_DEBUG_MS3D_LOADER_
#endif

// byte-align structures
#include "irrpack.h"

namespace {
// File header
struct MS3DHeader
{
	char ID[10];
	int Version;
} PACK_STRUCT;

// Vertex information
struct MS3DVertex
{
	u8 Flags;
	float Vertex[3];
	char BoneID;
	u8 RefCount;
} PACK_STRUCT;

// Triangle information
struct MS3DTriangle
{
	u16 Flags;
	u16 VertexIndices[3];
	float VertexNormals[3][3];
	float S[3], T[3];
	u8 SmoothingGroup;
	u8 GroupIndex;
} PACK_STRUCT;

// Material information
struct MS3DMaterial
{
    char Name[32];
    float Ambient[4];
    float Diffuse[4];
    float Specular[4];
    float Emissive[4];
    float Shininess;	// 0.0f - 128.0f
    float Transparency;	// 0.0f - 1.0f
    u8 Mode;	// 0, 1, 2 is unused now
    char Texture[128];
    char Alphamap[128];
} PACK_STRUCT;

// Joint information
struct MS3DJoint
{
	u8 Flags;
	char Name[32];
	char ParentName[32];
	float Rotation[3];
	float Translation[3];
	u16 NumRotationKeyframes;
	u16 NumTranslationKeyframes;
} PACK_STRUCT;

// Keyframe data
struct MS3DKeyframe
{
	float Time;
	float Parameter[3];
} PACK_STRUCT;

// vertex weights in 1.8.x
struct MS3DVertexWeights
{
	char boneIds[3];
	u8 weights[3];
} PACK_STRUCT;

} // end namespace

// Default alignment
#include "irrunpack.h"

struct SGroup
{
	core::stringc Name;
	core::array<u16> VertexIds;
	u16 MaterialIdx;
};

//! Constructor
CMS3DMeshFileLoader::CMS3DMeshFileLoader(video::IVideoDriver *driver)
: Driver(driver), AnimatedMesh(0)
{
	#ifdef _DEBUG
	setDebugName("CMS3DMeshFileLoader");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".bsp")
bool CMS3DMeshFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "ms3d" );
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* CMS3DMeshFileLoader::createMesh(io::IReadFile* file)
{
	if (!file)
		return 0;

	AnimatedMesh = new CSkinnedMesh();

	if ( load(file) )
	{
		AnimatedMesh->finalize();
	}
	else
	{
		AnimatedMesh->drop();
		AnimatedMesh = 0;
	}

	return AnimatedMesh;
}


//! loads a milkshape file
bool CMS3DMeshFileLoader::load(io::IReadFile* file)
{
	if (!file)
		return false;

	// find file size
	const long fileSize = file->getSize();

	// read whole file

	u8* buffer = new u8[fileSize];
	s32 read = file->read(buffer, fileSize);
	if (read != fileSize)
	{
		delete [] buffer;
		os::Printer::log("Could not read full file. Loading failed", file->getFileName(), ELL_ERROR);
		return false;
	}

	// read header

	const u8 *pPtr = (u8*)((void*)buffer);
	MS3DHeader *pHeader = (MS3DHeader*)pPtr;
	pPtr += sizeof(MS3DHeader);

	if ( strncmp( pHeader->ID, "MS3D000000", 10 ) != 0 )
	{
		delete [] buffer;
		os::Printer::log("Not a valid Milkshape3D Model File. Loading failed", file->getFileName(), ELL_ERROR);
		return false;
	}

#ifdef __BIG_ENDIAN__
	pHeader->Version = os::Byteswap::byteswap(pHeader->Version);
#endif
	if ( pHeader->Version < 3 || pHeader->Version > 4 )
	{
		delete [] buffer;
		os::Printer::log("Only Milkshape3D version 3 and 4 (1.3 to 1.8) is supported. Loading failed", file->getFileName(), ELL_ERROR);
		return false;
	}
#ifdef _IRR_DEBUG_MS3D_LOADER_
	os::Printer::log("Loaded header version", core::stringc(pHeader->Version).c_str());
#endif

	// get pointers to data

	// vertices
	u16 numVertices = *(u16*)pPtr;
#ifdef __BIG_ENDIAN__
	numVertices = os::Byteswap::byteswap(numVertices);
#endif
#ifdef _IRR_DEBUG_MS3D_LOADER_
	os::Printer::log("Load vertices", core::stringc(numVertices).c_str());
#endif
	pPtr += sizeof(u16);
	MS3DVertex *vertices = (MS3DVertex*)pPtr;
	pPtr += sizeof(MS3DVertex) * numVertices;
	if (pPtr > buffer+fileSize)
	{
		delete [] buffer;
		os::Printer::log("Loading failed. Corrupted data found.", file->getFileName(), ELL_ERROR);
		return false;
	}
	for (u16 tmp=0; tmp<numVertices; ++tmp)
	{
#ifdef __BIG_ENDIAN__
		vertices[tmp].Vertex[0] = os::Byteswap::byteswap(vertices[tmp].Vertex[0]);
		vertices[tmp].Vertex[1] = os::Byteswap::byteswap(vertices[tmp].Vertex[1]);
		vertices[tmp].Vertex[2] = -os::Byteswap::byteswap(vertices[tmp].Vertex[2]);
#else
		vertices[tmp].Vertex[2] = -vertices[tmp].Vertex[2];
#endif
	}

	// triangles
	u16 numTriangles = *(u16*)pPtr;
#ifdef __BIG_ENDIAN__
	numTriangles = os::Byteswap::byteswap(numTriangles);
#endif
#ifdef _IRR_DEBUG_MS3D_LOADER_
	os::Printer::log("Load Triangles", core::stringc(numTriangles).c_str());
#endif
	pPtr += sizeof(u16);
	MS3DTriangle *triangles = (MS3DTriangle*)pPtr;
	pPtr += sizeof(MS3DTriangle) * numTriangles;
	if (pPtr > buffer+fileSize)
	{
		delete [] buffer;
		os::Printer::log("Loading failed. Corrupted data found.", file->getFileName(), ELL_ERROR);
		return false;
	}
	for (u16 tmp=0; tmp<numTriangles; ++tmp)
	{
#ifdef __BIG_ENDIAN__
		triangles[tmp].Flags = os::Byteswap::byteswap(triangles[tmp].Flags);
		for (u16 j=0; j<3; ++j)
		{
			triangles[tmp].VertexIndices[j] = os::Byteswap::byteswap(triangles[tmp].VertexIndices[j]);
			triangles[tmp].VertexNormals[j][0] = os::Byteswap::byteswap(triangles[tmp].VertexNormals[j][0]);
			triangles[tmp].VertexNormals[j][1] = os::Byteswap::byteswap(triangles[tmp].VertexNormals[j][1]);
			triangles[tmp].VertexNormals[j][2] = -os::Byteswap::byteswap(triangles[tmp].VertexNormals[j][2]);
			triangles[tmp].S[j] = os::Byteswap::byteswap(triangles[tmp].S[j]);
			triangles[tmp].T[j] = os::Byteswap::byteswap(triangles[tmp].T[j]);
		}
#else
		triangles[tmp].VertexNormals[0][2] = -triangles[tmp].VertexNormals[0][2];
		triangles[tmp].VertexNormals[1][2] = -triangles[tmp].VertexNormals[1][2];
		triangles[tmp].VertexNormals[2][2] = -triangles[tmp].VertexNormals[2][2];
#endif
	}

	// groups
	u16 numGroups = *(u16*)pPtr;
#ifdef __BIG_ENDIAN__
	numGroups = os::Byteswap::byteswap(numGroups);
#endif
#ifdef _IRR_DEBUG_MS3D_LOADER_
	os::Printer::log("Load Groups", core::stringc(numGroups).c_str());
#endif
	pPtr += sizeof(u16);

	core::array<SGroup> groups;
	groups.reallocate(numGroups);

	//store groups
	u32 i;
	for (i=0; i<numGroups; ++i)
	{
		groups.push_back(SGroup());
		SGroup& grp = groups.getLast();

		// The byte flag is before the name, so add 1
		grp.Name = ((const c8*) pPtr) + 1;

		pPtr += 33; // name and 1 byte flags
		u16 triangleCount = *(u16*)pPtr;
#ifdef __BIG_ENDIAN__
		triangleCount = os::Byteswap::byteswap(triangleCount);
#endif
		pPtr += sizeof(u16);
		grp.VertexIds.reallocate(triangleCount);

		//pPtr += sizeof(u16) * triangleCount; // triangle indices
		for (u16 j=0; j<triangleCount; ++j)
		{
#ifdef __BIG_ENDIAN__
			grp.VertexIds.push_back(os::Byteswap::byteswap(*(u16*)pPtr));
#else
			grp.VertexIds.push_back(*(u16*)pPtr);
#endif
			pPtr += sizeof (u16);
		}

		grp.MaterialIdx = *(u8*)pPtr;
		if (grp.MaterialIdx == 255)
			grp.MaterialIdx = 0;

		pPtr += sizeof(c8); // material index
		if (pPtr > buffer+fileSize)
		{
			delete [] buffer;
			os::Printer::log("Loading failed. Corrupted data found.", file->getFileName(), ELL_ERROR);
			return false;
		}
	}

	// load materials
	u16 numMaterials = *(u16*)pPtr;
#ifdef __BIG_ENDIAN__
	numMaterials = os::Byteswap::byteswap(numMaterials);
#endif
#ifdef _IRR_DEBUG_MS3D_LOADER_
	os::Printer::log("Load Materials", core::stringc(numMaterials).c_str());
#endif
	pPtr += sizeof(u16);

	if(numMaterials == 0)
	{
		// if there are no materials, add at least one buffer
		AnimatedMesh->addMeshBuffer();
	}

	for (i=0; i<numMaterials; ++i)
	{
		MS3DMaterial *material = (MS3DMaterial*)pPtr;
#ifdef __BIG_ENDIAN__
		for (u16 j=0; j<4; ++j)
			material->Ambient[j] = os::Byteswap::byteswap(material->Ambient[j]);
		for (u16 j=0; j<4; ++j)
			material->Diffuse[j] = os::Byteswap::byteswap(material->Diffuse[j]);
		for (u16 j=0; j<4; ++j)
			material->Specular[j] = os::Byteswap::byteswap(material->Specular[j]);
		for (u16 j=0; j<4; ++j)
			material->Emissive[j] = os::Byteswap::byteswap(material->Emissive[j]);
		material->Shininess = os::Byteswap::byteswap(material->Shininess);
		material->Transparency = os::Byteswap::byteswap(material->Transparency);
#endif
		pPtr += sizeof(MS3DMaterial);
		if (pPtr > buffer+fileSize)
		{
			delete [] buffer;
			os::Printer::log("Loading failed. Corrupted data found.", file->getFileName(), ELL_ERROR);
			return false;
		}

		scene::SSkinMeshBuffer *tmpBuffer = AnimatedMesh->addMeshBuffer();

		tmpBuffer->Material.MaterialType = video::EMT_SOLID;

		tmpBuffer->Material.AmbientColor = video::SColorf(material->Ambient[0], material->Ambient[1], material->Ambient[2], material->Ambient[3]).toSColor ();
		tmpBuffer->Material.DiffuseColor = video::SColorf(material->Diffuse[0], material->Diffuse[1], material->Diffuse[2], material->Diffuse[3]).toSColor ();
		tmpBuffer->Material.EmissiveColor = video::SColorf(material->Emissive[0], material->Emissive[1], material->Emissive[2], material->Emissive[3]).toSColor ();
		tmpBuffer->Material.SpecularColor = video::SColorf(material->Specular[0], material->Specular[1], material->Specular[2], material->Specular[3]).toSColor ();
		tmpBuffer->Material.Shininess = material->Shininess;

		core::stringc TexturePath(material->Texture);
		if (TexturePath.trim()!="")
		{
			TexturePath=stripPathFromString(file->getFileName(),true) + stripPathFromString(TexturePath,false);
			tmpBuffer->Material.setTexture(0, Driver->getTexture(TexturePath));
		}

		core::stringc AlphamapPath=(const c8*)material->Alphamap;
		if (AlphamapPath.trim()!="")
		{
			AlphamapPath=stripPathFromString(file->getFileName(),true) + stripPathFromString(AlphamapPath,false);
			tmpBuffer->Material.setTexture(2, Driver->getTexture(AlphamapPath));
		}
	}

	// animation time
	f32 framesPerSecond = *(float*)pPtr;
#ifdef __BIG_ENDIAN__
	framesPerSecond = os::Byteswap::byteswap(framesPerSecond);
#endif
#ifdef _IRR_DEBUG_MS3D_LOADER_
	os::Printer::log("FPS", core::stringc(framesPerSecond).c_str());
#endif
	pPtr += sizeof(float) * 2; // fps and current time

	if (framesPerSecond<1.f)
		framesPerSecond=1.f;
	AnimatedMesh->setAnimationSpeed(framesPerSecond);

// ignore, calculated inside SkinnedMesh
//	s32 frameCount = *(int*)pPtr;
#ifdef __BIG_ENDIAN__
//	frameCount = os::Byteswap::byteswap(frameCount);
#endif
	pPtr += sizeof(int);

	u16 jointCount = *(u16*)pPtr;
#ifdef __BIG_ENDIAN__
	jointCount = os::Byteswap::byteswap(jointCount);
#endif
#ifdef _IRR_DEBUG_MS3D_LOADER_
	os::Printer::log("Joints", core::stringc(jointCount).c_str());
#endif
	pPtr += sizeof(u16);
	if (pPtr > buffer+fileSize)
	{
		delete [] buffer;
		os::Printer::log("Loading failed. Corrupted data found.", file->getFileName(), ELL_ERROR);
		return false;
	}

	core::array<core::stringc> parentNames;
	parentNames.reallocate(jointCount);

	// load joints
	for (i=0; i<jointCount; ++i)
	{
		u32 j;
		MS3DJoint *pJoint = (MS3DJoint*)pPtr;
#ifdef __BIG_ENDIAN__
		for (j=0; j<3; ++j)
			pJoint->Rotation[j] = os::Byteswap::byteswap(pJoint->Rotation[j]);
		for (j=0; j<3; ++j)
			pJoint->Translation[j] = os::Byteswap::byteswap(pJoint->Translation[j]);
		pJoint->NumRotationKeyframes= os::Byteswap::byteswap(pJoint->NumRotationKeyframes);
		pJoint->NumTranslationKeyframes = os::Byteswap::byteswap(pJoint->NumTranslationKeyframes);
#endif
		pPtr += sizeof(MS3DJoint);
		if (pPtr > buffer+fileSize)
		{
			delete [] buffer;
			os::Printer::log("Loading failed. Corrupted data found.", file->getFileName(), ELL_ERROR);
			return false;
		}

		ISkinnedMesh::SJoint *jnt = AnimatedMesh->addJoint();

		jnt->Name = pJoint->Name;
#ifdef _IRR_DEBUG_MS3D_LOADER_
		os::Printer::log("Joint", jnt->Name.c_str());
		os::Printer::log("Rotation keyframes", core::stringc(pJoint->NumRotationKeyframes).c_str());
		os::Printer::log("Translation keyframes", core::stringc(pJoint->NumTranslationKeyframes).c_str());
#endif
		jnt->LocalMatrix.makeIdentity();
		jnt->LocalMatrix.setRotationRadians(
			core::vector3df(pJoint->Rotation[0], pJoint->Rotation[1], pJoint->Rotation[2]) );
		// convert right-handed to left-handed
		jnt->LocalMatrix[2]=-jnt->LocalMatrix[2];
		jnt->LocalMatrix[6]=-jnt->LocalMatrix[6];
		jnt->LocalMatrix[8]=-jnt->LocalMatrix[8];
		jnt->LocalMatrix[9]=-jnt->LocalMatrix[9];

		jnt->LocalMatrix.setTranslation(
			core::vector3df(pJoint->Translation[0], pJoint->Translation[1], -pJoint->Translation[2]) );
		jnt->Animatedposition.set(jnt->LocalMatrix.getTranslation());
		jnt->Animatedrotation.set(jnt->LocalMatrix.getRotationDegrees());

		parentNames.push_back( (c8*)pJoint->ParentName );

		/*if (pJoint->NumRotationKeyframes ||
			pJoint->NumTranslationKeyframes)
			HasAnimation = true;
		 */

		// get rotation keyframes
		const u16 numRotationKeyframes = pJoint->NumRotationKeyframes;
		for (j=0; j < numRotationKeyframes; ++j)
		{
			MS3DKeyframe* kf = (MS3DKeyframe*)pPtr;
#ifdef __BIG_ENDIAN__
			kf->Time = os::Byteswap::byteswap(kf->Time);
			for (u32 l=0; l<3; ++l)
				kf->Parameter[l] = os::Byteswap::byteswap(kf->Parameter[l]);
#endif
			pPtr += sizeof(MS3DKeyframe);
			if (pPtr > buffer+fileSize)
			{
				delete [] buffer;
				os::Printer::log("Loading failed. Corrupted data found.", file->getFileName(), ELL_ERROR);
				return false;
			}

			ISkinnedMesh::SRotationKey *k=AnimatedMesh->addRotationKey(jnt);
			k->frame = kf->Time * framesPerSecond-1;

			core::matrix4 tmpMatrix;

			tmpMatrix.setRotationRadians(
				core::vector3df(kf->Parameter[0], kf->Parameter[1], kf->Parameter[2]) );
			// convert right-handed to left-handed
			tmpMatrix[2]=-tmpMatrix[2];
			tmpMatrix[6]=-tmpMatrix[6];
			tmpMatrix[8]=-tmpMatrix[8];
			tmpMatrix[9]=-tmpMatrix[9];

			tmpMatrix=jnt->LocalMatrix*tmpMatrix;

			// IRR_TEST_BROKEN_QUATERNION_USE: TODO - switched from tmpMatrix to tmpMatrix.getTransposed() for downward compatibility. 
			//								   Not tested so far if this was correct or wrong before quaternion fix!
			k->rotation  = core::quaternion(tmpMatrix.getTransposed());
		}

		// get translation keyframes
		const u16 numTranslationKeyframes = pJoint->NumTranslationKeyframes;
		for (j=0; j<numTranslationKeyframes; ++j)
		{
			MS3DKeyframe* kf = (MS3DKeyframe*)pPtr;
#ifdef __BIG_ENDIAN__
			kf->Time = os::Byteswap::byteswap(kf->Time);
			for (u32 l=0; l<3; ++l)
				kf->Parameter[l] = os::Byteswap::byteswap(kf->Parameter[l]);
#endif
			pPtr += sizeof(MS3DKeyframe);
			if (pPtr > buffer+fileSize)
			{
				delete [] buffer;
				os::Printer::log("Loading failed. Corrupted data found.", file->getFileName(), ELL_ERROR);
				return false;
			}

			ISkinnedMesh::SPositionKey *k=AnimatedMesh->addPositionKey(jnt);
			k->frame = kf->Time * framesPerSecond-1;

			k->position = core::vector3df
				(kf->Parameter[0]+pJoint->Translation[0],
				 kf->Parameter[1]+pJoint->Translation[1],
				 -kf->Parameter[2]-pJoint->Translation[2]);
		}
	}

	core::array<MS3DVertexWeights> vertexWeights;
	f32 weightFactor=0;

	if (jointCount && (pHeader->Version == 4) && (pPtr < buffer+fileSize))
	{
		s32 subVersion = *(s32*)pPtr; // comment subVersion, always 1
#ifdef __BIG_ENDIAN__
		subVersion = os::Byteswap::byteswap(subVersion);
#endif
		pPtr += sizeof(s32);

		for (u32 j=0; j<4; ++j) // four comment groups
		{
#ifdef _IRR_DEBUG_MS3D_LOADER_
			os::Printer::log("Skipping comment group", core::stringc(j+1).c_str());
#endif
			u32 numComments = *(u32*)pPtr;
#ifdef __BIG_ENDIAN__
			numComments = os::Byteswap::byteswap(numComments);
#endif
			pPtr += sizeof(u32);
			for (i=0; i<numComments; ++i)
			{
				// according to scorpiomidget this field does
				// not exist for model comments. So avoid to
				// read it
				if (j!=3)
					pPtr += sizeof(s32); // index
				s32 commentLength = *(s32*)pPtr;
#ifdef __BIG_ENDIAN__
				commentLength = os::Byteswap::byteswap(commentLength);
#endif
				pPtr += sizeof(s32);
				pPtr += commentLength;
			}

			if (pPtr > buffer+fileSize)
			{
				delete [] buffer;
				os::Printer::log("Loading failed. Corrupted data found.", file->getFileName(), ELL_ERROR);
				return false;
			}
		}

		if (pPtr < buffer+fileSize)
		{
			subVersion = *(s32*)pPtr; // vertex subVersion, 1 or 2
#ifdef __BIG_ENDIAN__
			subVersion = os::Byteswap::byteswap(subVersion);
#endif
			if (subVersion==1)
				weightFactor=1.f/255.f;
			else
				weightFactor=1.f/100.f;
			pPtr += sizeof(s32);

#ifdef _IRR_DEBUG_MS3D_LOADER_
			os::Printer::log("Reading vertex weights");
#endif
			// read vertex weights, ignoring data 'extra' from 1.8.2
			vertexWeights.reallocate(numVertices);
			const char offset = (subVersion==1)?6:10;
			for (i=0; i<numVertices; ++i)
			{
				vertexWeights.push_back(*(MS3DVertexWeights*)pPtr);
				pPtr += offset;
			}

			if (pPtr > buffer+fileSize)
			{
				delete [] buffer;
				os::Printer::log("Loading failed. Corrupted data found.", file->getFileName(), ELL_ERROR);
				return false;
			}
		}

		if (pPtr < buffer+fileSize)
		{
			subVersion = *(s32*)pPtr; // joint subVersion, 1 or 2
#ifdef __BIG_ENDIAN__
			subVersion = os::Byteswap::byteswap(subVersion);
#endif
			pPtr += sizeof(s32);
			// skip joint colors
#ifdef _IRR_DEBUG_MS3D_LOADER_
			os::Printer::log("Skip joint color");
#endif
			pPtr += 3*sizeof(float)*jointCount;

			if (pPtr > buffer+fileSize)
			{
				delete [] buffer;
				os::Printer::log("Loading failed. Corrupted data found", file->getFileName(), ELL_ERROR);
				return false;
			}
		}

		if (pPtr < buffer+fileSize)
		{
			subVersion = *(s32*)pPtr; // model subVersion, 1 or 2
#ifdef __BIG_ENDIAN__
			subVersion = os::Byteswap::byteswap(subVersion);
#endif
			pPtr += sizeof(s32);
#ifdef _IRR_DEBUG_MS3D_LOADER_
			os::Printer::log("Skip model extra information");
#endif
			// now the model extra information would follow
			// we also skip this for now
		}
	}

	//find parent of every joint
	for (u32 jointnum=0; jointnum<AnimatedMesh->getAllJoints().size(); ++jointnum)
	{
		for (u32 j2=0; j2<AnimatedMesh->getAllJoints().size(); ++j2)
		{
			if (jointnum != j2 && parentNames[jointnum] == AnimatedMesh->getAllJoints()[j2]->Name )
			{
				AnimatedMesh->getAllJoints()[j2]->Children.push_back(AnimatedMesh->getAllJoints()[jointnum]);
				break;
			}
		}
	}

	// create vertices and indices, attach them to the joints.
	video::S3DVertex v;
	core::array<video::S3DVertex> *Vertices;
	core::array<u16> Indices;

	for (i=0; i<numTriangles; ++i)
	{
		u32 tmp = groups[triangles[i].GroupIndex].MaterialIdx;
		Vertices = &AnimatedMesh->getMeshBuffers()[tmp]->Vertices_Standard;

		for (s32 j = 2; j!=-1; --j)
		{
			const u32 vertidx = triangles[i].VertexIndices[j];

			v.TCoords.X = triangles[i].S[j];
			v.TCoords.Y = triangles[i].T[j];

			v.Normal.X = triangles[i].VertexNormals[j][0];
			v.Normal.Y = triangles[i].VertexNormals[j][1];
			v.Normal.Z = triangles[i].VertexNormals[j][2];

			if(triangles[i].GroupIndex < groups.size() &&
					groups[triangles[i].GroupIndex].MaterialIdx < AnimatedMesh->getMeshBuffers().size())
				v.Color = AnimatedMesh->getMeshBuffers()[groups[triangles[i].GroupIndex].MaterialIdx]->Material.DiffuseColor;
			else
				v.Color.set(255,255,255,255);

			v.Pos.X = vertices[vertidx].Vertex[0];
			v.Pos.Y = vertices[vertidx].Vertex[1];
			v.Pos.Z = vertices[vertidx].Vertex[2];

			// check if we already have this vertex in our vertex array
			s32 index = -1;
			for (u32 iV = 0; iV < Vertices->size(); ++iV)
			{
				if (v == (*Vertices)[iV])
				{
					index = (s32)iV;
					break;
				}
			}

			if (index == -1)
			{
				index = Vertices->size();
				const u32 matidx = groups[triangles[i].GroupIndex].MaterialIdx;
				if (vertexWeights.size()==0)
				{
					const s32 boneid = vertices[vertidx].BoneID;
					if ((u32)boneid < AnimatedMesh->getAllJoints().size())
					{
						ISkinnedMesh::SWeight *w=AnimatedMesh->addWeight(AnimatedMesh->getAllJoints()[boneid]);
						w->buffer_id = matidx;
						w->strength = 1.0f;
						w->vertex_id = index;
					}
				}
				else if (jointCount) // new weights from 1.8.x
				{
					f32 sum = 1.0f;
					s32 boneid = vertices[vertidx].BoneID;
					if (((u32)boneid < AnimatedMesh->getAllJoints().size()) && (vertexWeights[vertidx].weights[0] != 0))
					{
						ISkinnedMesh::SWeight *w=AnimatedMesh->addWeight(AnimatedMesh->getAllJoints()[boneid]);
						w->buffer_id = matidx;
						sum -= (w->strength = vertexWeights[vertidx].weights[0]*weightFactor);
						w->vertex_id = index;
					}
					boneid = vertexWeights[vertidx].boneIds[0];
					if (((u32)boneid < AnimatedMesh->getAllJoints().size()) && (vertexWeights[vertidx].weights[1] != 0))
					{
						ISkinnedMesh::SWeight *w=AnimatedMesh->addWeight(AnimatedMesh->getAllJoints()[boneid]);
						w->buffer_id = matidx;
						sum -= (w->strength = vertexWeights[vertidx].weights[1]*weightFactor);
						w->vertex_id = index;
					}
					boneid = vertexWeights[vertidx].boneIds[1];
					if (((u32)boneid < AnimatedMesh->getAllJoints().size()) && (vertexWeights[vertidx].weights[2] != 0))
					{
						ISkinnedMesh::SWeight *w=AnimatedMesh->addWeight(AnimatedMesh->getAllJoints()[boneid]);
						w->buffer_id = matidx;
						sum -= (w->strength = vertexWeights[vertidx].weights[2]*weightFactor);
						w->vertex_id = index;
					}
					boneid = vertexWeights[vertidx].boneIds[2];
					if (((u32)boneid < AnimatedMesh->getAllJoints().size()) && (sum > 0.f))
					{
						ISkinnedMesh::SWeight *w=AnimatedMesh->addWeight(AnimatedMesh->getAllJoints()[boneid]);
						w->buffer_id = matidx;
						w->strength = sum;
						w->vertex_id = index;
					}
					// fallback, if no bone chosen. Seems to be an error in the specs
					boneid = vertices[vertidx].BoneID;
					if ((sum == 1.f) && ((u32)boneid < AnimatedMesh->getAllJoints().size()))
					{
						ISkinnedMesh::SWeight *w=AnimatedMesh->addWeight(AnimatedMesh->getAllJoints()[boneid]);
						w->buffer_id = matidx;
						w->strength = 1.f;
						w->vertex_id = index;
					}
				}

				Vertices->push_back(v);
			}
			Indices.push_back(index);
		}
	}

	//create groups
	s32 iIndex = -1;
	for (i=0; i<groups.size(); ++i)
	{
		SGroup& grp = groups[i];

		if (grp.MaterialIdx >= AnimatedMesh->getMeshBuffers().size())
			grp.MaterialIdx = 0;

		core::array<u16>& indices = AnimatedMesh->getMeshBuffers()[grp.MaterialIdx]->Indices;

		for (u32 k=0; k < grp.VertexIds.size(); ++k)
			for (u32 l=0; l<3; ++l)
				indices.push_back(Indices[++iIndex]);
	}

	delete [] buffer;

	return true;
}


core::stringc CMS3DMeshFileLoader::stripPathFromString(const core::stringc& inString, bool returnPath) const
{
	s32 slashIndex=inString.findLast('/'); // forward slash
	s32 backSlash=inString.findLast('\\'); // back slash

	if (backSlash>slashIndex) slashIndex=backSlash;

	if (slashIndex==-1)//no slashes found
	{
		if (returnPath)
			return core::stringc(); //no path to return
		else
			return inString;
	}

	if (returnPath)
		return inString.subString(0, slashIndex + 1);
	else
		return inString.subString(slashIndex+1, inString.size() - (slashIndex+1));
}


} // end namespace scene
} // end namespace irr

#endif

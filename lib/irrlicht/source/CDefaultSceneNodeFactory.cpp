// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CDefaultSceneNodeFactory.h"
#include "ISceneManager.h"
#include "ITextSceneNode.h"
#include "ITerrainSceneNode.h"
#include "IDummyTransformationSceneNode.h"
#include "ICameraSceneNode.h"
#include "IBillboardSceneNode.h"
#include "IAnimatedMeshSceneNode.h"
#include "IParticleSystemSceneNode.h"
#include "ILightSceneNode.h"
#include "IMeshSceneNode.h"

namespace irr
{
namespace scene
{


CDefaultSceneNodeFactory::CDefaultSceneNodeFactory(ISceneManager* mgr)
: Manager(mgr)
{

	#ifdef _DEBUG
	setDebugName("CDefaultSceneNodeFactory");
	#endif

	// don't grab the scene manager here to prevent cyclic references

	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_CUBE, "cube"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_SPHERE, "sphere"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_TEXT, "text"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_WATER_SURFACE, "waterSurface"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_TERRAIN, "terrain"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_SKY_BOX, "skyBox"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_SKY_DOME, "skyDome"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_SHADOW_VOLUME, "shadowVolume"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_OCTREE, "octree"));
	// Legacy support
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_OCTREE, "octTree"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_MESH, "mesh"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_LIGHT, "light"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_EMPTY, "empty"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_DUMMY_TRANSFORMATION, "dummyTransformation"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_CAMERA, "camera"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_BILLBOARD, "billBoard"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_ANIMATED_MESH, "animatedMesh"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_PARTICLE_SYSTEM, "particleSystem"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_VOLUME_LIGHT, "volumeLight"));
	// SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_MD3_SCENE_NODE, "md3"));

	// legacy, for version <= 1.4.x irr files
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_CAMERA_MAYA, "cameraMaya"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_CAMERA_FPS, "cameraFPS"));
	SupportedSceneNodeTypes.push_back(SSceneNodeTypePair(ESNT_Q3SHADER_SCENE_NODE, "quake3Shader"));
}


//! adds a scene node to the scene graph based on its type id
ISceneNode* CDefaultSceneNodeFactory::addSceneNode(ESCENE_NODE_TYPE type, ISceneNode* parent)
{
	switch(type)
	{
	case ESNT_CUBE:
		return Manager->addCubeSceneNode(10, parent);
	case ESNT_SPHERE:
		return Manager->addSphereSceneNode(5, 16, parent);
	case ESNT_TEXT:
		return Manager->addTextSceneNode(0, L"example");
	case ESNT_WATER_SURFACE:
		return Manager->addWaterSurfaceSceneNode(0, 2.0f, 300.0f, 10.0f, parent);
	case ESNT_TERRAIN:
		return Manager->addTerrainSceneNode("", parent, -1,
							core::vector3df(0.0f,0.0f,0.0f),
							core::vector3df(0.0f,0.0f,0.0f),
							core::vector3df(1.0f,1.0f,1.0f),
							video::SColor(255,255,255,255),
							4, ETPS_17, 0, true);
	case ESNT_SKY_BOX:
		return Manager->addSkyBoxSceneNode(0,0,0,0,0,0, parent);
	case ESNT_SKY_DOME:
		return Manager->addSkyDomeSceneNode(0, 16, 8, 0.9f, 2.0f, 1000.0f, parent);
	case ESNT_SHADOW_VOLUME:
		return 0;
	case ESNT_OCTREE:
		return Manager->addOctreeSceneNode((IMesh*)0, parent, -1, 128, true);
	case ESNT_MESH:
		return Manager->addMeshSceneNode(0, parent, -1, core::vector3df(),
										 core::vector3df(), core::vector3df(1,1,1), true);
	case ESNT_LIGHT:
		return Manager->addLightSceneNode(parent);
	case ESNT_EMPTY:
		return Manager->addEmptySceneNode(parent);
	case ESNT_DUMMY_TRANSFORMATION:
		return Manager->addDummyTransformationSceneNode(parent);
	case ESNT_CAMERA:
		return Manager->addCameraSceneNode(parent);
	case ESNT_CAMERA_MAYA:
		return Manager->addCameraSceneNodeMaya(parent);
	case ESNT_CAMERA_FPS:
		return Manager->addCameraSceneNodeFPS(parent);
	case ESNT_BILLBOARD:
		return Manager->addBillboardSceneNode(parent);
	case ESNT_ANIMATED_MESH:
		return Manager->addAnimatedMeshSceneNode(0, parent, -1, core::vector3df(),
												 core::vector3df(), core::vector3df(1,1,1), true);
	case ESNT_PARTICLE_SYSTEM:
		return Manager->addParticleSystemSceneNode(true, parent);
	case ESNT_VOLUME_LIGHT:
		return (ISceneNode*)Manager->addVolumeLightSceneNode(parent);
	default:
		break;
	}

	return 0;
}


//! adds a scene node to the scene graph based on its type name
ISceneNode* CDefaultSceneNodeFactory::addSceneNode(const c8* typeName, ISceneNode* parent)
{
	return addSceneNode( getTypeFromName(typeName), parent );
}


//! returns amount of scene node types this factory is able to create
u32 CDefaultSceneNodeFactory::getCreatableSceneNodeTypeCount() const
{
	return SupportedSceneNodeTypes.size();
}


//! returns type of a createable scene node type
ESCENE_NODE_TYPE CDefaultSceneNodeFactory::getCreateableSceneNodeType(u32 idx) const
{
	if (idx<SupportedSceneNodeTypes.size())
		return SupportedSceneNodeTypes[idx].Type;
	else
		return ESNT_UNKNOWN;
}


//! returns type name of a createable scene node type
const c8* CDefaultSceneNodeFactory::getCreateableSceneNodeTypeName(u32 idx) const
{
	if (idx<SupportedSceneNodeTypes.size())
		return SupportedSceneNodeTypes[idx].TypeName.c_str();
	else
		return 0;
}


//! returns type name of a createable scene node type
const c8* CDefaultSceneNodeFactory::getCreateableSceneNodeTypeName(ESCENE_NODE_TYPE type) const
{
	for (u32 i=0; i<SupportedSceneNodeTypes.size(); ++i)
		if (SupportedSceneNodeTypes[i].Type == type)
			return SupportedSceneNodeTypes[i].TypeName.c_str();

	return 0;
}


ESCENE_NODE_TYPE CDefaultSceneNodeFactory::getTypeFromName(const c8* name) const
{
	for (u32 i=0; i<SupportedSceneNodeTypes.size(); ++i)
		if (SupportedSceneNodeTypes[i].TypeName == name)
			return SupportedSceneNodeTypes[i].Type;

	return ESNT_UNKNOWN;
}


} // end namespace scene
} // end namespace irr


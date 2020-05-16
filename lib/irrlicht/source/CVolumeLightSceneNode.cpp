// Copyright (C) 2007-2012 Dean Wadsworth
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CVolumeLightSceneNode.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "S3DVertex.h"
#include "os.h"

namespace irr
{
namespace scene
{

//! constructor
CVolumeLightSceneNode::CVolumeLightSceneNode(ISceneNode* parent, ISceneManager* mgr,
		s32 id, const u32 subdivU, const u32 subdivV,
		const video::SColor foot,
		const video::SColor tail,
		const core::vector3df& position,
		const core::vector3df& rotation, const core::vector3df& scale)
	: IVolumeLightSceneNode(parent, mgr, id, position, rotation, scale),
		Mesh(0), LPDistance(8.0f),
		SubdivideU(subdivU), SubdivideV(subdivV),
		FootColor(foot), TailColor(tail),
		LightDimensions(core::vector3df(1.0f, 1.2f, 1.0f))
{
	#ifdef _DEBUG
	setDebugName("CVolumeLightSceneNode");
	#endif

	constructLight();
}


CVolumeLightSceneNode::~CVolumeLightSceneNode()
{
	if (Mesh)
		Mesh->drop();
}


void CVolumeLightSceneNode::constructLight()
{
	if (Mesh)
		Mesh->drop();
	Mesh = SceneManager->getGeometryCreator()->createVolumeLightMesh(SubdivideU, SubdivideV, FootColor, TailColor, LPDistance, LightDimensions);
}


//! renders the node.
void CVolumeLightSceneNode::render()
{
	if (!Mesh)
		return;

	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);

	driver->setMaterial(Mesh->getMeshBuffer(0)->getMaterial());
	driver->drawMeshBuffer(Mesh->getMeshBuffer(0));
}


//! returns the axis aligned bounding box of this node
const core::aabbox3d<f32>& CVolumeLightSceneNode::getBoundingBox() const
{
	return Mesh->getBoundingBox();
}


void CVolumeLightSceneNode::OnRegisterSceneNode()
{
	if (IsVisible)
	{
		SceneManager->registerNodeForRendering(this, ESNRP_TRANSPARENT);
	}
	ISceneNode::OnRegisterSceneNode();
}


//! returns the material based on the zero based index i. To get the amount
//! of materials used by this scene node, use getMaterialCount().
//! This function is needed for inserting the node into the scene hirachy on a
//! optimal position for minimizing renderstate changes, but can also be used
//! to directly modify the material of a scene node.
video::SMaterial& CVolumeLightSceneNode::getMaterial(u32 i)
{
	return Mesh->getMeshBuffer(i)->getMaterial();
}


//! returns amount of materials used by this scene node.
u32 CVolumeLightSceneNode::getMaterialCount() const
{
	return 1;
}


void CVolumeLightSceneNode::setSubDivideU (const u32 inU)
{
	if (inU != SubdivideU)
	{
		SubdivideU = inU;
		constructLight();
	}
}


void CVolumeLightSceneNode::setSubDivideV (const u32 inV)
{
	if (inV != SubdivideV)
	{
		SubdivideV = inV;
		constructLight();
	}
}


void CVolumeLightSceneNode::setFootColor(const video::SColor inColor)
{
	if (inColor != FootColor)
	{
		FootColor = inColor;
		constructLight();
	}
}


void CVolumeLightSceneNode::setTailColor(const video::SColor inColor)
{
	if (inColor != TailColor)
	{
		TailColor = inColor;
		constructLight();
	}
}


//! Writes attributes of the scene node.
void CVolumeLightSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	ISceneNode::serializeAttributes(out, options);

	out->addFloat("lpDistance", LPDistance);
	out->addInt("subDivideU", SubdivideU);
	out->addInt("subDivideV", SubdivideV);

	out->addColor("footColor", FootColor);
	out->addColor("tailColor", TailColor);

	out->addVector3d("lightDimension", LightDimensions);
}


//! Reads attributes of the scene node.
void CVolumeLightSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	LPDistance = in->getAttributeAsFloat("lpDistance");
	LPDistance = core::max_(LPDistance, 8.0f);

	SubdivideU = in->getAttributeAsInt("subDivideU");
	SubdivideU = core::max_(SubdivideU, 1u);

	SubdivideV = in->getAttributeAsInt("subDivideV");
	SubdivideV = core::max_(SubdivideV, 1u);

	FootColor = in->getAttributeAsColor("footColor");
	TailColor = in->getAttributeAsColor("tailColor");

	LightDimensions = in->getAttributeAsVector3d("lightDimension");

	constructLight();

	ISceneNode::deserializeAttributes(in, options);
}


//! Creates a clone of this scene node and its children.
ISceneNode* CVolumeLightSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
{
	if (!newParent)
		newParent = Parent;
	if (!newManager)
		newManager = SceneManager;

	CVolumeLightSceneNode* nb = new CVolumeLightSceneNode(newParent,
		newManager, ID, SubdivideU, SubdivideV, FootColor, TailColor, RelativeTranslation);

	nb->cloneMembers(this, newManager);
	nb->getMaterial(0) = Mesh->getMeshBuffer(0)->getMaterial();

	if ( newParent )
		nb->drop();
	return nb;
}


} // end namespace scene
} // end namespace irr


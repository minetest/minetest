// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
// Code for this scene node has been contributed by Anders la Cour-Harbo (alc)

#include "CSkyDomeSceneNode.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"
#include "IAnimatedMesh.h"
#include "os.h"

namespace irr
{
namespace scene
{

/* horiRes and vertRes:
	Controls the number of faces along the horizontal axis (30 is a good value)
	and the number of faces along the vertical axis (8 is a good value).

	texturePercentage:
	Only the top texturePercentage of the image is used, e.g. 0.8 uses the top 80% of the image,
	1.0 uses the entire image. This is useful as some landscape images have a small banner
	at the bottom that you don't want.

	spherePercentage:
	This controls how far around the sphere the sky dome goes. For value 1.0 you get exactly the upper
	hemisphere, for 1.1 you get slightly more, and for 2.0 you get a full sphere. It is sometimes useful
	to use a value slightly bigger than 1 to avoid a gap between some ground place and the sky. This
	parameters stretches the image to fit the chosen "sphere-size". */

CSkyDomeSceneNode::CSkyDomeSceneNode(video::ITexture* sky, u32 horiRes, u32 vertRes,
		f32 texturePercentage, f32 spherePercentage, f32 radius,
		ISceneNode* parent, ISceneManager* mgr, s32 id)
	: ISceneNode(parent, mgr, id), Buffer(0),
	  HorizontalResolution(horiRes), VerticalResolution(vertRes),
	  TexturePercentage(texturePercentage),
	  SpherePercentage(spherePercentage), Radius(radius)
{
	#ifdef _DEBUG
	setDebugName("CSkyDomeSceneNode");
	#endif

	setAutomaticCulling(scene::EAC_OFF);

	Buffer = new SMeshBuffer();
	Buffer->Material.Lighting = false;
	Buffer->Material.ZBuffer = video::ECFN_NEVER;
	Buffer->Material.ZWriteEnable = false;
	Buffer->Material.AntiAliasing = video::EAAM_OFF;
	Buffer->Material.setTexture(0, sky);
	Buffer->BoundingBox.MaxEdge.set(0,0,0);
	Buffer->BoundingBox.MinEdge.set(0,0,0);

	// regenerate the mesh
	generateMesh();
}


CSkyDomeSceneNode::~CSkyDomeSceneNode()
{
	if (Buffer)
		Buffer->drop();
}


void CSkyDomeSceneNode::generateMesh()
{
	f32 azimuth;
	u32 k;

	Buffer->Vertices.clear();
	Buffer->Indices.clear();

	const f32 azimuth_step = (core::PI * 2.f) / HorizontalResolution;
	if (SpherePercentage < 0.f)
		SpherePercentage = -SpherePercentage;
	if (SpherePercentage > 2.f)
		SpherePercentage = 2.f;
	const f32 elevation_step = SpherePercentage * core::HALF_PI / (f32)VerticalResolution;

	Buffer->Vertices.reallocate( (HorizontalResolution + 1) * (VerticalResolution + 1) );
	Buffer->Indices.reallocate(3 * (2*VerticalResolution - 1) * HorizontalResolution);

	video::S3DVertex vtx;
	vtx.Color.set(255,255,255,255);
	vtx.Normal.set(0.0f,-1.f,0.0f);

	const f32 tcV = TexturePercentage / VerticalResolution;
	for (k = 0, azimuth = 0; k <= HorizontalResolution; ++k)
	{
		f32 elevation = core::HALF_PI;
		const f32 tcU = (f32)k / (f32)HorizontalResolution;
		const f32 sinA = sinf(azimuth);
		const f32 cosA = cosf(azimuth);
		for (u32 j = 0; j <= VerticalResolution; ++j)
		{
			const f32 cosEr = Radius * cosf(elevation);
			vtx.Pos.set(cosEr*sinA, Radius*sinf(elevation), cosEr*cosA);
			vtx.TCoords.set(tcU, j*tcV);

			vtx.Normal = -vtx.Pos;
			vtx.Normal.normalize();

			Buffer->Vertices.push_back(vtx);
			elevation -= elevation_step;
		}
		azimuth += azimuth_step;
	}

	for (k = 0; k < HorizontalResolution; ++k)
	{
		Buffer->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k);
		Buffer->Indices.push_back(1 + (VerticalResolution + 1)*k);
		Buffer->Indices.push_back(0 + (VerticalResolution + 1)*k);

		for (u32 j = 1; j < VerticalResolution; ++j)
		{
			Buffer->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k + j);
			Buffer->Indices.push_back(1 + (VerticalResolution + 1)*k + j);
			Buffer->Indices.push_back(0 + (VerticalResolution + 1)*k + j);

			Buffer->Indices.push_back(VerticalResolution + 1 + (VerticalResolution + 1)*k + j);
			Buffer->Indices.push_back(VerticalResolution + 2 + (VerticalResolution + 1)*k + j);
			Buffer->Indices.push_back(0 + (VerticalResolution + 1)*k + j);
		}
	}
	Buffer->setHardwareMappingHint(scene::EHM_STATIC);
}


//! renders the node.
void CSkyDomeSceneNode::render()
{
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();

	if (!camera || !driver)
		return;

	if ( !camera->isOrthogonal() )
	{
		core::matrix4 mat(AbsoluteTransformation);
		mat.setTranslation(camera->getAbsolutePosition());

		driver->setTransform(video::ETS_WORLD, mat);

		driver->setMaterial(Buffer->Material);
		driver->drawMeshBuffer(Buffer);
	}

	// for debug purposes only:
	if ( DebugDataVisible )
	{
		video::SMaterial m;
		m.Lighting = false;
		driver->setMaterial(m);

		if ( DebugDataVisible & scene::EDS_NORMALS )
		{
			// draw normals
			const f32 debugNormalLength = SceneManager->getParameters()->getAttributeAsFloat(DEBUG_NORMAL_LENGTH);
			const video::SColor debugNormalColor = SceneManager->getParameters()->getAttributeAsColor(DEBUG_NORMAL_COLOR);
			driver->drawMeshBufferNormals(Buffer, debugNormalLength, debugNormalColor);
		}

		// show mesh
		if ( DebugDataVisible & scene::EDS_MESH_WIRE_OVERLAY )
		{
			m.Wireframe = true;
			driver->setMaterial(m);

			driver->drawMeshBuffer(Buffer);
		}
	}
}


//! returns the axis aligned bounding box of this node
const core::aabbox3d<f32>& CSkyDomeSceneNode::getBoundingBox() const
{
	return Buffer->BoundingBox;
}


void CSkyDomeSceneNode::OnRegisterSceneNode()
{
	if (IsVisible)
	{
		SceneManager->registerNodeForRendering(this, ESNRP_SKY_BOX );
	}

	ISceneNode::OnRegisterSceneNode();
}


//! returns the material based on the zero based index i. To get the amount
//! of materials used by this scene node, use getMaterialCount().
//! This function is needed for inserting the node into the scene hirachy on a
//! optimal position for minimizing renderstate changes, but can also be used
//! to directly modify the material of a scene node.
video::SMaterial& CSkyDomeSceneNode::getMaterial(u32 i)
{
	return Buffer->Material;
}


//! returns amount of materials used by this scene node.
u32 CSkyDomeSceneNode::getMaterialCount() const
{
	return 1;
}


//! Writes attributes of the scene node.
void CSkyDomeSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	ISceneNode::serializeAttributes(out, options);

	out->addInt  ("HorizontalResolution", HorizontalResolution);
	out->addInt  ("VerticalResolution",   VerticalResolution);
	out->addFloat("TexturePercentage",    TexturePercentage);
	out->addFloat("SpherePercentage",     SpherePercentage);
	out->addFloat("Radius",               Radius);
}


//! Reads attributes of the scene node.
void CSkyDomeSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	HorizontalResolution = in->getAttributeAsInt  ("HorizontalResolution");
	VerticalResolution   = in->getAttributeAsInt  ("VerticalResolution");
	TexturePercentage    = in->getAttributeAsFloat("TexturePercentage");
	SpherePercentage     = in->getAttributeAsFloat("SpherePercentage");
	Radius               = in->getAttributeAsFloat("Radius");

	ISceneNode::deserializeAttributes(in, options);

	// regenerate the mesh
	generateMesh();
}

//! Creates a clone of this scene node and its children.
ISceneNode* CSkyDomeSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
{
	if (!newParent)
		newParent = Parent;
	if (!newManager)
		newManager = SceneManager;

	CSkyDomeSceneNode* nb = new CSkyDomeSceneNode(Buffer->Material.TextureLayer[0].Texture, HorizontalResolution, VerticalResolution, TexturePercentage,
		SpherePercentage, Radius, newParent, newManager, ID);

	nb->cloneMembers(this, newManager);

	if ( newParent )
		nb->drop();
	return nb;
}


} // namespace scene
} // namespace irr

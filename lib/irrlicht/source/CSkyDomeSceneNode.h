// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
// Code for this scene node has been contributed by Anders la Cour-Harbo (alc)

#ifndef __C_SKY_DOME_SCENE_NODE_H_INCLUDED__
#define __C_SKY_DOME_SCENE_NODE_H_INCLUDED__

#include "ISceneNode.h"
#include "SMeshBuffer.h"

namespace irr
{
namespace scene
{

class CSkyDomeSceneNode : public ISceneNode
{
	public:
		CSkyDomeSceneNode(video::ITexture* texture, u32 horiRes, u32 vertRes,
			f32 texturePercentage, f32 spherePercentage, f32 radius,
			ISceneNode* parent, ISceneManager* smgr, s32 id);
		virtual ~CSkyDomeSceneNode();
		virtual void OnRegisterSceneNode();
		virtual void render();
		virtual const core::aabbox3d<f32>& getBoundingBox() const;
		virtual video::SMaterial& getMaterial(u32 i);
		virtual u32 getMaterialCount() const;
		virtual ESCENE_NODE_TYPE getType() const { return ESNT_SKY_DOME; }

		virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;
		virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);
		virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);

	private:

		void generateMesh();

		SMeshBuffer* Buffer;

		u32 HorizontalResolution, VerticalResolution;
		f32 TexturePercentage, SpherePercentage, Radius;
};


}
}

#endif



#pragma once

#include <ISceneManager.h>
#include <ISceneNode.h>
#include <ICameraSceneNode.h>
#include <IVideoDriver.h>
#include <S3DVertex.h>
#include <SMaterial.h>
#include <SColor.h>
#include "irrlichttypes_bloated.h"

using namespace irr;

struct LineVertex
{
	v3f position = v3f(0, 0, 0);
	// The camera offset. Necessary for correct world space placement.
	v3f offset = v3f(0, 0, 0);
	// Target scene node.
	scene::ISceneNode *target = nullptr;
	float width = 0.01f;
	video::SColor color = video::SColor(0xFFFFFFFF);

	v3f getScenePos() const
	{
		if (target) {
			v3f pos_copy = position;
			// The offset has likely been applied to the parent node already.
			target->getAbsoluteTransformation().transformVect(pos_copy);
			return pos_copy;
		}
		return position - offset;
	}
};

class LineSceneNode : public scene::ISceneNode
{
	public:
		aabb3f bounds;
		LineVertex src_point;
		LineVertex dst_point;
		bool simple = false;
		bool wrap_texture = true;
		int alphaMode = 0; // 0 is test/opaque, 1 is blend, 2 is add
		video::SMaterial mat;

		void recalculateBounds() {
			bounds = aabb3f(src_point.getScenePos(), dst_point.getScenePos());
			bounds.repair();
		}
		virtual void render();
		virtual void OnRegisterSceneNode();
		virtual void OnAnimate(u32 t);
		virtual const aabb3f &getBoundingBox() const {
			return bounds;
		}
		LineSceneNode(scene::ISceneManager* mgr, s32 id = -1)
		: scene::ISceneNode(mgr->getRootSceneNode(), mgr, id)
		{
		}
};

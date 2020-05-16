// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_CUBE_SCENE_NODE_H_INCLUDED__
#define __C_CUBE_SCENE_NODE_H_INCLUDED__

#include "IMeshSceneNode.h"
#include "SMesh.h"

namespace irr
{
namespace scene
{
	class CCubeSceneNode : public IMeshSceneNode
	{
	public:

		//! constructor
		CCubeSceneNode(f32 size, ISceneNode* parent, ISceneManager* mgr, s32 id,
			const core::vector3df& position = core::vector3df(0,0,0),
			const core::vector3df& rotation = core::vector3df(0,0,0),
			const core::vector3df& scale = core::vector3df(1.0f, 1.0f, 1.0f));

		virtual ~CCubeSceneNode();

		virtual void OnRegisterSceneNode();

		//! renders the node.
		virtual void render();

		//! returns the axis aligned bounding box of this node
		virtual const core::aabbox3d<f32>& getBoundingBox() const;

		//! returns the material based on the zero based index i. To get the amount
		//! of materials used by this scene node, use getMaterialCount().
		//! This function is needed for inserting the node into the scene hirachy on a
		//! optimal position for minimizing renderstate changes, but can also be used
		//! to directly modify the material of a scene node.
		virtual video::SMaterial& getMaterial(u32 i);

		//! returns amount of materials used by this scene node.
		virtual u32 getMaterialCount() const;

		//! Returns type of the scene node
		virtual ESCENE_NODE_TYPE getType() const { return ESNT_CUBE; }

		//! Creates shadow volume scene node as child of this node
		//! and returns a pointer to it.
		virtual IShadowVolumeSceneNode* addShadowVolumeSceneNode(const IMesh* shadowMesh,
			s32 id, bool zfailmethod=true, f32 infinity=10000.0f);

		//! Writes attributes of the scene node.
		virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const;

		//! Reads attributes of the scene node.
		virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0);

		//! Creates a clone of this scene node and its children.
		virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);

		//! Sets a new mesh to display
		virtual void setMesh(IMesh* mesh) {}

		//! Returns the current mesh
		virtual IMesh* getMesh(void) { return Mesh; }

		//! Sets if the scene node should not copy the materials of the mesh but use them in a read only style.
		/* In this way it is possible to change the materials a mesh causing all mesh scene nodes 
		referencing this mesh to change too. */
		virtual void setReadOnlyMaterials(bool readonly) {}

		//! Returns if the scene node should not copy the materials of the mesh but use them in a read only style
		virtual bool isReadOnlyMaterials() const { return false; }

		//! Removes a child from this scene node.
		//! Implemented here, to be able to remove the shadow properly, if there is one,
		//! or to remove attached childs.
		virtual bool removeChild(ISceneNode* child);

	private:
		void setSize();

		IMesh* Mesh;
		IShadowVolumeSceneNode* Shadow;
		f32 Size;
	};

} // end namespace scene
} // end namespace irr

#endif


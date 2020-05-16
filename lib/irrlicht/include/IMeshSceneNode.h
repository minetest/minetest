// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_MESH_SCENE_NODE_H_INCLUDED__
#define __I_MESH_SCENE_NODE_H_INCLUDED__

#include "ISceneNode.h"

namespace irr
{
namespace scene
{

class IShadowVolumeSceneNode;
class IMesh;


//! A scene node displaying a static mesh
class IMeshSceneNode : public ISceneNode
{
public:

	//! Constructor
	/** Use setMesh() to set the mesh to display.
	*/
	IMeshSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
			const core::vector3df& position = core::vector3df(0,0,0),
			const core::vector3df& rotation = core::vector3df(0,0,0),
			const core::vector3df& scale = core::vector3df(1,1,1))
		: ISceneNode(parent, mgr, id, position, rotation, scale) {}

	//! Sets a new mesh to display
	/** \param mesh Mesh to display. */
	virtual void setMesh(IMesh* mesh) = 0;

	//! Get the currently defined mesh for display.
	/** \return Pointer to mesh which is displayed by this node. */
	virtual IMesh* getMesh(void) = 0;

	//! Creates shadow volume scene node as child of this node.
	/** The shadow can be rendered using the ZPass or the zfail
	method. ZPass is a little bit faster because the shadow volume
	creation is easier, but with this method there occur ugly
	looking artifacs when the camera is inside the shadow volume.
	These error do not occur with the ZFail method.
	\param shadowMesh: Optional custom mesh for shadow volume.
	\param id: Id of the shadow scene node. This id can be used to
	identify the node later.
	\param zfailmethod: If set to true, the shadow will use the
	zfail method, if not, zpass is used.
	\param infinity: Value used by the shadow volume algorithm to
	scale the shadow volume (for zfail shadow volume we support only
	finite shadows, so camera zfar must be larger than shadow back cap,
	which is depend on infinity parameter).
	\return Pointer to the created shadow scene node. This pointer
	should not be dropped. See IReferenceCounted::drop() for more
	information. */
	virtual IShadowVolumeSceneNode* addShadowVolumeSceneNode(const IMesh* shadowMesh=0,
		s32 id=-1, bool zfailmethod=true, f32 infinity=1000.0f) = 0;

	//! Sets if the scene node should not copy the materials of the mesh but use them in a read only style.
	/** In this way it is possible to change the materials of a mesh
	causing all mesh scene nodes referencing this mesh to change, too.
	\param readonly Flag if the materials shall be read-only. */
	virtual void setReadOnlyMaterials(bool readonly) = 0;

	//! Check if the scene node should not copy the materials of the mesh but use them in a read only style
	/** This flag can be set by setReadOnlyMaterials().
	\return Whether the materials are read-only. */
	virtual bool isReadOnlyMaterials() const = 0;
};

} // end namespace scene
} // end namespace irr


#endif


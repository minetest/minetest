// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_ANIMATED_MESH_H_INCLUDED__
#define __I_ANIMATED_MESH_H_INCLUDED__

#include "aabbox3d.h"
#include "IMesh.h"

namespace irr
{
namespace scene
{
	//! Possible types of (animated) meshes.
	enum E_ANIMATED_MESH_TYPE
	{
		//! Unknown animated mesh type.
		EAMT_UNKNOWN = 0,

		//! Quake 2 MD2 model file
		EAMT_MD2,

		//! Quake 3 MD3 model file
		EAMT_MD3,

		//! Maya .obj static model
		EAMT_OBJ,

		//! Quake 3 .bsp static Map
		EAMT_BSP,

		//! 3D Studio .3ds file
		EAMT_3DS,

		//! My3D Mesh, the file format by Zhuck Dimitry
		EAMT_MY3D,

		//! Pulsar LMTools .lmts file. This Irrlicht loader was written by Jonas Petersen
		EAMT_LMTS,

		//! Cartography Shop .csm file. This loader was created by Saurav Mohapatra.
		EAMT_CSM,

		//! .oct file for Paul Nette's FSRad or from Murphy McCauley's Blender .oct exporter.
		/** The oct file format contains 3D geometry and lightmaps and
		can be loaded directly by Irrlicht */
		EAMT_OCT,

		//! Halflife MDL model file
		EAMT_MDL_HALFLIFE,

		//! generic skinned mesh
		EAMT_SKINNED
	};

	//! Interface for an animated mesh.
	/** There are already simple implementations of this interface available so
	you don't have to implement this interface on your own if you need to:
	You might want to use irr::scene::SAnimatedMesh, irr::scene::SMesh,
	irr::scene::SMeshBuffer etc. */
	class IAnimatedMesh : public IMesh
	{
	public:

		//! Gets the frame count of the animated mesh.
		/** \return The amount of frames. If the amount is 1,
		it is a static, non animated mesh. */
		virtual u32 getFrameCount() const = 0;

		//! Gets the animation speed of the animated mesh.
		/** \return The number of frames per second to play the
		animation with by default. If the amount is 0,
		it is a static, non animated mesh. */
		virtual f32 getAnimationSpeed() const = 0;

		//! Sets the animation speed of the animated mesh.
		/** \param fps Number of frames per second to play the
		animation with by default. If the amount is 0,
		it is not animated. The actual speed is set in the
		scene node the mesh is instantiated in.*/
		virtual void setAnimationSpeed(f32 fps) =0;

		//! Returns the IMesh interface for a frame.
		/** \param frame: Frame number as zero based index. The maximum
		frame number is getFrameCount() - 1;
		\param detailLevel: Level of detail. 0 is the lowest, 255 the
		highest level of detail. Most meshes will ignore the detail level.
		\param startFrameLoop: Because some animated meshes (.MD2) are
		blended between 2 static frames, and maybe animated in a loop,
		the startFrameLoop and the endFrameLoop have to be defined, to
		prevent the animation to be blended between frames which are
		outside of this loop.
		If startFrameLoop and endFrameLoop are both -1, they are ignored.
		\param endFrameLoop: see startFrameLoop.
		\return Returns the animated mesh based on a detail level. */
		virtual IMesh* getMesh(s32 frame, s32 detailLevel=255, s32 startFrameLoop=-1, s32 endFrameLoop=-1) = 0;

		//! Returns the type of the animated mesh.
		/** In most cases it is not neccessary to use this method.
		This is useful for making a safe downcast. For example,
		if getMeshType() returns EAMT_MD2 it's safe to cast the
		IAnimatedMesh to IAnimatedMeshMD2.
		\returns Type of the mesh. */
		virtual E_ANIMATED_MESH_TYPE getMeshType() const
		{
			return EAMT_UNKNOWN;
		}
	};

} // end namespace scene
} // end namespace irr

#endif


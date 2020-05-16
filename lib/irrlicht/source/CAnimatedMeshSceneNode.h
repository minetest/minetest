// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_ANIMATED_MESH_SCENE_NODE_H_INCLUDED__
#define __C_ANIMATED_MESH_SCENE_NODE_H_INCLUDED__

#include "IAnimatedMeshSceneNode.h"
#include "IAnimatedMesh.h"

#include "matrix4.h"


namespace irr
{
namespace scene
{
	class IDummyTransformationSceneNode;

	class CAnimatedMeshSceneNode : public IAnimatedMeshSceneNode
	{
	public:

		//! constructor
		CAnimatedMeshSceneNode(IAnimatedMesh* mesh, ISceneNode* parent, ISceneManager* mgr,	s32 id,
			const core::vector3df& position = core::vector3df(0,0,0),
			const core::vector3df& rotation = core::vector3df(0,0,0),
			const core::vector3df& scale = core::vector3df(1.0f, 1.0f, 1.0f));

		//! destructor
		virtual ~CAnimatedMeshSceneNode();

		//! sets the current frame. from now on the animation is played from this frame.
		virtual void setCurrentFrame(f32 frame);

		//! frame
		virtual void OnRegisterSceneNode();

		//! OnAnimate() is called just before rendering the whole scene.
		virtual void OnAnimate(u32 timeMs);

		//! renders the node.
		virtual void render();

		//! returns the axis aligned bounding box of this node
		virtual const core::aabbox3d<f32>& getBoundingBox() const;

		//! sets the frames between the animation is looped.
		//! the default is 0 - MaximalFrameCount of the mesh.
		virtual bool setFrameLoop(s32 begin, s32 end);

		//! Sets looping mode which is on by default. If set to false,
		//! animations will not be looped.
		virtual void setLoopMode(bool playAnimationLooped);

		//! returns the current loop mode
		virtual bool getLoopMode() const;

		//! Sets a callback interface which will be called if an animation
		//! playback has ended. Set this to 0 to disable the callback again.
		virtual void setAnimationEndCallback(IAnimationEndCallBack* callback=0);

		//! sets the speed with which the animation is played
		virtual void setAnimationSpeed(f32 framesPerSecond);

		//! gets the speed with which the animation is played
		virtual f32 getAnimationSpeed() const;

		//! returns the material based on the zero based index i. To get the amount
		//! of materials used by this scene node, use getMaterialCount().
		//! This function is needed for inserting the node into the scene hirachy on a
		//! optimal position for minimizing renderstate changes, but can also be used
		//! to directly modify the material of a scene node.
		virtual video::SMaterial& getMaterial(u32 i);

		//! returns amount of materials used by this scene node.
		virtual u32 getMaterialCount() const;

		//! Creates shadow volume scene node as child of this node
		//! and returns a pointer to it.
		virtual IShadowVolumeSceneNode* addShadowVolumeSceneNode(const IMesh* shadowMesh,
			s32 id, bool zfailmethod=true, f32 infinity=1000.0f);

		//! Returns a pointer to a child node, which has the same transformation as
		//! the corrsesponding joint, if the mesh in this scene node is a skinned mesh.
		virtual IBoneSceneNode* getJointNode(const c8* jointName);

		//! same as getJointNode(const c8* jointName), but based on id
		virtual IBoneSceneNode* getJointNode(u32 jointID);

		//! Gets joint count.
		virtual u32 getJointCount() const;

		//! Deprecated command, please use getJointNode.
		virtual ISceneNode* getMS3DJointNode(const c8* jointName);

		//! Deprecated command, please use getJointNode.
		virtual ISceneNode* getXJointNode(const c8* jointName);

		//! Removes a child from this scene node.
		//! Implemented here, to be able to remove the shadow properly, if there is one,
		//! or to remove attached childs.
		virtual bool removeChild(ISceneNode* child);

		//! Starts a MD2 animation.
		virtual bool setMD2Animation(EMD2_ANIMATION_TYPE anim);

		//! Starts a special MD2 animation.
		virtual bool setMD2Animation(const c8* animationName);

		//! Returns the current displayed frame number.
		virtual f32 getFrameNr() const;
		//! Returns the current start frame number.
		virtual s32 getStartFrame() const;
		//! Returns the current end frame number.
		virtual s32 getEndFrame() const;

		//! Sets if the scene node should not copy the materials of the mesh but use them in a read only style.
		/* In this way it is possible to change the materials a mesh causing all mesh scene nodes
		referencing this mesh to change too. */
		virtual void setReadOnlyMaterials(bool readonly);

		//! Returns if the scene node should not copy the materials of the mesh but use them in a read only style
		virtual bool isReadOnlyMaterials() const;

		//! Sets a new mesh
		virtual void setMesh(IAnimatedMesh* mesh);

		//! Returns the current mesh
		virtual IAnimatedMesh* getMesh(void) { return Mesh; }

		//! Writes attributes of the scene node.
		virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const;

		//! Reads attributes of the scene node.
		virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0);

		//! Returns type of the scene node
		virtual ESCENE_NODE_TYPE getType() const { return ESNT_ANIMATED_MESH; }

		// returns the absolute transformation for a special MD3 Tag if the mesh is a md3 mesh,
		// or the absolutetransformation if it's a normal scenenode
		const SMD3QuaternionTag* getMD3TagTransformation( const core::stringc & tagname);

		//! updates the absolute position based on the relative and the parents position
		virtual void updateAbsolutePosition();


		//! Set the joint update mode (0-unused, 1-get joints only, 2-set joints only, 3-move and set)
		virtual void setJointMode(E_JOINT_UPDATE_ON_RENDER mode);

		//! Sets the transition time in seconds (note: This needs to enable joints, and setJointmode maybe set to 2)
		//! you must call animateJoints(), or the mesh will not animate
		virtual void setTransitionTime(f32 Time);

		//! updates the joint positions of this mesh
		virtual void animateJoints(bool CalculateAbsolutePositions=true);

		//! render mesh ignoring its transformation. Used with ragdolls. (culling is unaffected)
		virtual void setRenderFromIdentity( bool On );

		//! Creates a clone of this scene node and its children.
		/** \param newParent An optional new parent.
		\param newManager An optional new scene manager.
		\return The newly created clone of this node. */
		virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);

	private:

		//! Get a static mesh for the current frame of this animated mesh
		IMesh* getMeshForCurrentFrame();

		void buildFrameNr(u32 timeMs);
		void checkJoints();
		void beginTransition();

		core::array<video::SMaterial> Materials;
		core::aabbox3d<f32> Box;
		IAnimatedMesh* Mesh;

		s32 StartFrame;
		s32 EndFrame;
		f32 FramesPerSecond;
		f32 CurrentFrameNr;

		u32 LastTimeMs;
		u32 TransitionTime; //Transition time in millisecs
		f32 Transiting; //is mesh transiting (plus cache of TransitionTime)
		f32 TransitingBlend; //0-1, calculated on buildFrameNr

		//0-unused, 1-get joints only, 2-set joints only, 3-move and set
		E_JOINT_UPDATE_ON_RENDER JointMode;
		bool JointsUsed;

		bool Looping;
		bool ReadOnlyMaterials;
		bool RenderFromIdentity;

		IAnimationEndCallBack* LoopCallBack;
		s32 PassCount;

		IShadowVolumeSceneNode* Shadow;

		core::array<IBoneSceneNode* > JointChildSceneNodes;
		core::array<core::matrix4> PretransitingSave;

		// Quake3 Model
		struct SMD3Special : public virtual IReferenceCounted
		{
			core::stringc Tagname;
			SMD3QuaternionTagList AbsoluteTagList;

			SMD3Special & operator = (const SMD3Special & copyMe)
			{
				Tagname = copyMe.Tagname;
				AbsoluteTagList = copyMe.AbsoluteTagList;
				return *this;
			}
		};
		SMD3Special *MD3Special;
	};

} // end namespace scene
} // end namespace irr

#endif


// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_CAMERA_SCENE_NODE_H_INCLUDED__
#define __C_CAMERA_SCENE_NODE_H_INCLUDED__

#include "ICameraSceneNode.h"
#include "SViewFrustum.h"

namespace irr
{
namespace scene
{

	class CCameraSceneNode : public ICameraSceneNode
	{
	public:

		//! constructor
		CCameraSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id, 
			const core::vector3df& position = core::vector3df(0,0,0),
			const core::vector3df& lookat = core::vector3df(0,0,100));

		//! Sets the projection matrix of the camera.
		/** The core::matrix4 class has some methods
		to build a projection matrix. e.g: core::matrix4::buildProjectionMatrixPerspectiveFovLH.
		Note that the matrix will only stay as set by this method until one of
		the following Methods are called: setNearValue, setFarValue, setAspectRatio, setFOV.
		\param projection The new projection matrix of the camera.
		\param isOrthogonal Set this to true if the matrix is an orthogonal one (e.g.
		from matrix4::buildProjectionMatrixOrthoLH(). */
		virtual void setProjectionMatrix(const core::matrix4& projection, bool isOrthogonal = false);

		//! Gets the current projection matrix of the camera
		//! \return Returns the current projection matrix of the camera.
		virtual const core::matrix4& getProjectionMatrix() const;

		//! Gets the current view matrix of the camera
		//! \return Returns the current view matrix of the camera.
		virtual const core::matrix4& getViewMatrix() const;

		//! Sets a custom view matrix affector.
		/** \param affector: The affector matrix. */
		virtual void setViewMatrixAffector(const core::matrix4& affector);

		//! Gets the custom view matrix affector.
		virtual const core::matrix4& getViewMatrixAffector() const;

		//! It is possible to send mouse and key events to the camera. Most cameras
		//! may ignore this input, but camera scene nodes which are created for 
		//! example with scene::ISceneManager::addMayaCameraSceneNode or
		//! scene::ISceneManager::addMeshViewerCameraSceneNode, may want to get this input
		//! for changing their position, look at target or whatever. 
		virtual bool OnEvent(const SEvent& event);

		//! Sets the look at target of the camera
		/** If the camera's target and rotation are bound ( @see bindTargetAndRotation() )
		then calling this will also change the camera's scene node rotation to match the target.
		\param pos: Look at target of the camera. */
		virtual void setTarget(const core::vector3df& pos);

		//! Sets the rotation of the node.
		/** This only modifies the relative rotation of the node.
		If the camera's target and rotation are bound ( @see bindTargetAndRotation() )
		then calling this will also change the camera's target to match the rotation.
		\param rotation New rotation of the node in degrees. */
		virtual void setRotation(const core::vector3df& rotation);

		//! Gets the current look at target of the camera
		/** \return The current look at target of the camera */
		virtual const core::vector3df& getTarget() const;

		//! Sets the up vector of the camera.
		//! \param pos: New upvector of the camera.
		virtual void setUpVector(const core::vector3df& pos);

		//! Gets the up vector of the camera.
		//! \return Returns the up vector of the camera.
		virtual const core::vector3df& getUpVector() const;

		//! Gets distance from the camera to the near plane.
		//! \return Value of the near plane of the camera.
		virtual f32 getNearValue() const;

		//! Gets the distance from the camera to the far plane.
		//! \return Value of the far plane of the camera.
		virtual f32 getFarValue() const;

		//! Get the aspect ratio of the camera.
		//! \return The aspect ratio of the camera.
		virtual f32 getAspectRatio() const;

		//! Gets the field of view of the camera.
		//! \return Field of view of the camera
		virtual f32 getFOV() const;

		//! Sets the value of the near clipping plane. (default: 1.0f)
		virtual void setNearValue(f32 zn);

		//! Sets the value of the far clipping plane (default: 2000.0f)
		virtual void setFarValue(f32 zf);

		//! Sets the aspect ratio (default: 4.0f / 3.0f)
		virtual void setAspectRatio(f32 aspect);

		//! Sets the field of view (Default: PI / 3.5f)
		virtual void setFOV(f32 fovy);

		//! PreRender event
		virtual void OnRegisterSceneNode();

		//! Render
		virtual void render();

		//! Returns the axis aligned bounding box of this node
		virtual const core::aabbox3d<f32>& getBoundingBox() const;

		//! Returns the view area. Sometimes needed by bsp or lod render nodes.
		virtual const SViewFrustum* getViewFrustum() const;

		//! Disables or enables the camera to get key or mouse inputs.
		//! If this is set to true, the camera will respond to key inputs
		//! otherwise not.
		virtual void setInputReceiverEnabled(bool enabled);

		//! Returns if the input receiver of the camera is currently enabled.
		virtual bool isInputReceiverEnabled() const;

		//! Writes attributes of the scene node.
		virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const;

		//! Reads attributes of the scene node.
		virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0);

		//! Returns type of the scene node
		virtual ESCENE_NODE_TYPE getType() const { return ESNT_CAMERA; }

		//! Binds the camera scene node's rotation to its target position and vice vera, or unbinds them.
		virtual void bindTargetAndRotation(bool bound);

		//! Queries if the camera scene node's rotation and its target position are bound together.
		virtual bool getTargetAndRotationBinding(void) const;

		//! Creates a clone of this scene node and its children.
		virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);

	protected:

		void recalculateProjectionMatrix();
		void recalculateViewArea();

		core::vector3df Target;
		core::vector3df UpVector;

		f32 Fovy;	// Field of view, in radians. 
		f32 Aspect;	// Aspect ratio. 
		f32 ZNear;	// value of the near view-plane. 
		f32 ZFar;	// Z-value of the far view-plane.

		SViewFrustum ViewArea;
		core::matrix4 Affector;

		bool InputReceiverEnabled;
		bool TargetAndRotationAreBound;
	};

} // end namespace
} // end namespace

#endif


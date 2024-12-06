// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IAnimatedMesh.h"
#include "ISceneManager.h"
#include "SMeshBuffer.h"
#include "SSkinMeshBuffer.h"
#include "quaternion.h"

#include <optional>
#include <string>

namespace irr
{
namespace scene
{

class IAnimatedMeshSceneNode;
class IBoneSceneNode;
class ISceneManager;

class SkinnedMesh : public IAnimatedMesh
{
public:
	//! constructor
	SkinnedMesh() :
		EndFrame(0.f), FramesPerSecond(25.f),
		LastAnimatedFrame(-1), SkinnedLastFrame(false),
		HasAnimation(false), PreparedForSkinning(false),
		AnimateNormals(true), HardwareSkinning(false)
	{
		SkinningBuffers = &LocalBuffers;
	}

	//! destructor
	virtual ~SkinnedMesh();

	//! If the duration is 0, it is a static (=non animated) mesh.
	f32 getMaxFrameNumber() const override;

	//! Gets the default animation speed of the animated mesh.
	/** \return Amount of frames per second. If the amount is 0, it is a static, non animated mesh. */
	f32 getAnimationSpeed() const override;

	//! Gets the frame count of the animated mesh.
	/** \param fps Frames per second to play the animation with. If the amount is 0, it is not animated.
	The actual speed is set in the scene node the mesh is instantiated in.*/
	void setAnimationSpeed(f32 fps) override;

	//! returns the animated mesh for the given frame
	IMesh *getMesh(f32) override;

	//! Animates this mesh's joints based on frame input
	//! blend: {0-old position, 1-New position}
	void animateMesh(f32 frame, f32 blend);

	//! Performs a software skin on this mesh based of joint positions
	void skinMesh();

	//! returns amount of mesh buffers.
	u32 getMeshBufferCount() const override;

	//! returns pointer to a mesh buffer
	IMeshBuffer *getMeshBuffer(u32 nr) const override;

	//! Returns pointer to a mesh buffer which fits a material
	/** \param material: material to search for
	\return Returns the pointer to the mesh buffer or
	NULL if there is no such mesh buffer. */
	IMeshBuffer *getMeshBuffer(const video::SMaterial &material) const override;

	u32 getTextureSlot(u32 meshbufNr) const override;

	void setTextureSlot(u32 meshbufNr, u32 textureSlot);

	//! returns an axis aligned bounding box
	const core::aabbox3d<f32> &getBoundingBox() const override {
		return BoundingBox;
	}

	//! set user axis aligned bounding box
	void setBoundingBox(const core::aabbox3df &box) override {
		BoundingBox = box;
	}

	//! set the hardware mapping hint, for driver
	void setHardwareMappingHint(E_HARDWARE_MAPPING newMappingHint, E_BUFFER_TYPE buffer = EBT_VERTEX_AND_INDEX) override;

	//! flags the meshbuffer as changed, reloads hardware buffers
	void setDirty(E_BUFFER_TYPE buffer = EBT_VERTEX_AND_INDEX) override;

	//! Returns the type of the animated mesh.
	E_ANIMATED_MESH_TYPE getMeshType() const override {
		return EAMT_SKINNED;
	}

	//! Gets joint count.
	u32 getJointCount() const;

	//! Gets the name of a joint.
	/** \param number: Zero based index of joint.
	\return Name of joint and null if an error happened. */
	const std::optional<std::string> &getJointName(u32 number) const;

	//! Gets a joint number from its name
	/** \param name: Name of the joint.
	\return Number of the joint or std::nullopt if not found. */
	std::optional<u32> getJointNumber(const std::string &name) const;

	//! Update Normals when Animating
	/** \param on If false don't animate, which is faster.
	Else update normals, which allows for proper lighting of
	animated meshes (default). */
	void updateNormalsWhenAnimating(bool on) {
		AnimateNormals = on;
	}

	//! converts the vertex type of all meshbuffers to tangents.
	/** E.g. used for bump mapping. */
	void convertMeshToTangents();

	//! Does the mesh have no animation
	bool isStatic() const {
		return !HasAnimation;
	}

	//! Allows to enable hardware skinning.
	/* This feature is not implemented in Irrlicht yet */
	bool setHardwareSkinning(bool on);

	//! Refreshes vertex data cached in joints such as positions and normals
	void refreshJointCache();

	//! Moves the mesh into static position.
	void resetAnimation();

	virtual void updateBoundingBox();

	//! Recovers the joints from the mesh
	void recoverJointsFromMesh(std::vector<IBoneSceneNode *> &jointChildSceneNodes);

	//! Transfers the joint data to the mesh
	void transferJointsToMesh(const std::vector<IBoneSceneNode *> &jointChildSceneNodes);

	//! Transfers the joint hints to the mesh
	void transferOnlyJointsHintsToMesh(const std::vector<IBoneSceneNode *> &jointChildSceneNodes);

	//! Creates an array of joints from this mesh as children of node
	void addJoints(std::vector<IBoneSceneNode *> &jointChildSceneNodes,
			IAnimatedMeshSceneNode *node,
			ISceneManager *smgr);

	//! A vertex weight
	struct SWeight
	{
		//! Index of the mesh buffer
		u16 buffer_id; // I doubt 32bits is needed

		//! Index of the vertex
		u32 vertex_id; // Store global ID here

		//! Weight Strength/Percentage (0-1)
		f32 strength;

	private:
		//! Internal members used by SkinnedMesh
		friend class SkinnedMesh;
		char *Moved;
		core::vector3df StaticPos;
		core::vector3df StaticNormal;
	};

	//! Animation keyframe which describes a new position
	struct SPositionKey
	{
		f32 frame;
		core::vector3df position;
	};

	//! Animation keyframe which describes a new scale
	struct SScaleKey
	{
		f32 frame;
		core::vector3df scale;
	};

	//! Animation keyframe which describes a new rotation
	struct SRotationKey
	{
		f32 frame;
		core::quaternion rotation;
	};

	//! Joints
	struct SJoint
	{
		SJoint() :
				GlobalSkinningSpace(false),
				positionHint(-1), scaleHint(-1), rotationHint(-1)
		{
		}

		//! The name of this joint
		std::optional<std::string> Name;

		//! Local matrix of this joint
		core::matrix4 LocalMatrix;

		//! List of child joints
		std::vector<SJoint *> Children;

		//! List of attached meshes
		std::vector<u32> AttachedMeshes;

		//! Animation keys causing translation change
		core::array<SPositionKey> PositionKeys;

		//! Animation keys causing scale change
		core::array<SScaleKey> ScaleKeys;

		//! Animation keys causing rotation change
		core::array<SRotationKey> RotationKeys;

		//! Skin weights
		std::vector<SWeight> Weights;

		//! Unnecessary for loaders, will be overwritten on finalize
		core::matrix4 GlobalMatrix; // loaders may still choose to set this (temporarily) to calculate absolute vertex data.
		core::matrix4 GlobalAnimatedMatrix;
		core::matrix4 LocalAnimatedMatrix;

		//! These should be set by loaders.
		core::vector3df Animatedposition;
		core::vector3df Animatedscale;
		core::quaternion Animatedrotation;

		// The .x and .gltf formats pre-calculate this
		std::optional<core::matrix4> GlobalInversedMatrix;
	private:
		//! Internal members used by SkinnedMesh
		friend class SkinnedMesh;

		bool GlobalSkinningSpace;

		s32 positionHint;
		s32 scaleHint;
		s32 rotationHint;
	};

	// Interface for the mesh loaders (finalize should lock these functions, and they should have some prefix like loader_
	// these functions will use the needed arrays, set values, etc to help the loaders

	//! exposed for loaders to add mesh buffers
	std::vector<SSkinMeshBuffer *> &getMeshBuffers() {
		return LocalBuffers;
	}

	//! alternative method for adding joints
	std::vector<SJoint *> &getAllJoints() {
		return AllJoints;
	}

	//! alternative method for reading joints
	const std::vector<SJoint *> &getAllJoints() const {
		return AllJoints;
	}

	//! loaders should call this after populating the mesh
	void finalize();

	//! Adds a new meshbuffer to the mesh, access it as last one
	SSkinMeshBuffer *addMeshBuffer();

	//! Adds a new meshbuffer to the mesh, access it as last one
	void addMeshBuffer(SSkinMeshBuffer *meshbuf);

	//! Adds a new joint to the mesh, access it as last one
	SJoint *addJoint(SJoint *parent = 0);

	//! Adds a new position key to the mesh, access it as last one
	SPositionKey *addPositionKey(SJoint *joint);
	//! Adds a new rotation key to the mesh, access it as last one
	SRotationKey *addRotationKey(SJoint *joint);
	//! Adds a new scale key to the mesh, access it as last one
	SScaleKey *addScaleKey(SJoint *joint);

	//! Adds a new weight to the mesh, access it as last one
	SWeight *addWeight(SJoint *joint);

private:
	void checkForAnimation();

	void normalizeWeights();

	void buildAllLocalAnimatedMatrices();

	void buildAllGlobalAnimatedMatrices(SJoint *Joint = 0, SJoint *ParentJoint = 0);

	void getFrameData(f32 frame, SJoint *Node,
			core::vector3df &position, s32 &positionHint,
			core::vector3df &scale, s32 &scaleHint,
			core::quaternion &rotation, s32 &rotationHint);

	void calculateGlobalMatrices(SJoint *Joint, SJoint *ParentJoint);

	void skinJoint(SJoint *Joint, SJoint *ParentJoint);

	void calculateTangents(core::vector3df &normal,
			core::vector3df &tangent, core::vector3df &binormal,
			const core::vector3df &vt1, const core::vector3df &vt2, const core::vector3df &vt3,
			const core::vector2df &tc1, const core::vector2df &tc2, const core::vector2df &tc3);

	std::vector<SSkinMeshBuffer *> *SkinningBuffers; // Meshbuffer to skin, default is to skin localBuffers

	std::vector<SSkinMeshBuffer *> LocalBuffers;
	//! Mapping from meshbuffer number to bindable texture slot
	std::vector<u32> TextureSlots;

	std::vector<SJoint *> AllJoints;
	std::vector<SJoint *> RootJoints;

	// bool can't be used here because std::vector<bool>
	// doesn't allow taking a reference to individual elements.
	std::vector<std::vector<char>> Vertices_Moved;

	core::aabbox3d<f32> BoundingBox;

	f32 EndFrame;
	f32 FramesPerSecond;

	f32 LastAnimatedFrame;
	bool SkinnedLastFrame;

	bool HasAnimation;
	bool PreparedForSkinning;
	bool AnimateNormals;
	bool HardwareSkinning;
};

} // end namespace scene
} // end namespace irr

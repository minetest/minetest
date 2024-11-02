// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CSkinnedMesh.h"
#include <optional>
#include "CBoneSceneNode.h"
#include "IAnimatedMeshSceneNode.h"
#include "SSkinMeshBuffer.h"
#include "irrMath.h"
#include "irrTypes.h"
#include "os.h"
#include "quaternion.h"
#include "vector3d.h"

namespace irr
{
namespace scene
{

//! constructor
CSkinnedMesh::CSkinnedMesh() :
		SkinningBuffers(0), EndFrame(0.f), FramesPerSecond(25.f),
		InterpolationMode(EIM_LINEAR),
		HasAnimation(false), PreparedForSkinning(false),
		AnimateNormals(true), HardwareSkinning(false)
{
	SkinningBuffers = &LocalBuffers;
}

//! destructor
CSkinnedMesh::~CSkinnedMesh()
{
	for (u32 i = 0; i < AllJoints.size(); ++i)
		delete AllJoints[i];

	for (u32 j = 0; j < LocalBuffers.size(); ++j) {
		if (LocalBuffers[j])
			LocalBuffers[j]->drop();
	}
}

f32 CSkinnedMesh::getMaxFrameNumber() const
{
	return EndFrame;
}

//! Gets the default animation speed of the animated mesh.
/** \return Amount of frames per second. If the amount is 0, it is a static, non animated mesh. */
f32 CSkinnedMesh::getAnimationSpeed() const
{
	return FramesPerSecond;
}

//! Gets the frame count of the animated mesh.
/** \param fps Frames per second to play the animation with. If the amount is 0, it is not animated.
The actual speed is set in the scene node the mesh is instantiated in.*/
void CSkinnedMesh::setAnimationSpeed(f32 fps)
{
	FramesPerSecond = fps;
}

//! returns the animated mesh based
IMesh *CSkinnedMesh::getMesh(f32 frame)
{
	if (frame == -1)
		return this;

	animateMesh(frame); // TODO why does this exist?
	skinMesh();
	return this;
}

//--------------------------------------------------------------------------
//			Keyframe Animation
//--------------------------------------------------------------------------

//! Animates this mesh's joints based on frame input
void CSkinnedMesh::animateMesh(f32 frame)
{
	if (!HasAnimation)
		return; // TODO sus

	for (u32 i = 0; i < AllJoints.size(); ++i) {
		auto *joint = AllJoints[i];
		if (const auto *animated_joint = joint->UseAnimationFrom) {
			animated_joint->keys.updateTransform(frame,
					InterpolationMode == EIM_LINEAR, joint->AnimatedTransform);
		}
	}

	// Note:
	// LocalAnimatedMatrix needs to be built at some point, but this function may be called lots of times for
	// one render (to play two animations at the same time) LocalAnimatedMatrix only needs to be built once.
	// a call to buildAllLocalAnimatedMatrices is needed before skinning the mesh, and before the user gets the joints to move
	buildAllLocalAnimatedMatrices();

	updateBoundingBox();
}

void CSkinnedMesh::buildAllLocalAnimatedMatrices()
{
	for (u32 i = 0; i < AllJoints.size(); ++i) {
		SJoint *joint = AllJoints[i];

		// Could be faster:

		if (joint->UseAnimationFrom && !joint->UseAnimationFrom->keys.empty()) {
			joint->GlobalSkinningSpace = false;

			// IRR_TEST_BROKEN_QUATERNION_USE: TODO - switched to getMatrix_transposed instead of getMatrix for downward compatibility.
			//								   Not tested so far if this was correct or wrong before quaternion fix!
			// Note that using getMatrix_transposed inverts the rotation.
			joint->AnimatedTransform.rotation.getMatrix_transposed(joint->LocalAnimatedMatrix);

			// --- joint->LocalAnimatedMatrix *= joint->Animatedrotation.getMatrix() ---
			f32 *m1 = joint->LocalAnimatedMatrix.pointer();
			const core::vector3df Pos = joint->AnimatedTransform.translation;
			m1[0] += Pos.X * m1[3];
			m1[1] += Pos.Y * m1[3];
			m1[2] += Pos.Z * m1[3];
			m1[4] += Pos.X * m1[7];
			m1[5] += Pos.Y * m1[7];
			m1[6] += Pos.Z * m1[7];
			m1[8] += Pos.X * m1[11];
			m1[9] += Pos.Y * m1[11];
			m1[10] += Pos.Z * m1[11];
			m1[12] += Pos.X * m1[15];
			m1[13] += Pos.Y * m1[15];
			m1[14] += Pos.Z * m1[15];
			// -----------------------------------

			const core::vector3df scale = joint->AnimatedTransform.scale;
			if (scale != core::vector3df(1)) {
				/*
				core::matrix4 scaleMatrix;
				scaleMatrix.setScale(scale);
				joint->LocalAnimatedMatrix *= scaleMatrix;
				*/

				// -------- joint->LocalAnimatedMatrix *= scaleMatrix -----------------
				core::matrix4 &mat = joint->LocalAnimatedMatrix;
				mat[0] *= scale.X;
				mat[1] *= scale.X;
				mat[2] *= scale.X;
				mat[3] *= scale.X;
				mat[4] *= scale.Y;
				mat[5] *= scale.Y;
				mat[6] *= scale.Y;
				mat[7] *= scale.Y;
				mat[8] *= scale.Z;
				mat[9] *= scale.Z;
				mat[10] *= scale.Z;
				mat[11] *= scale.Z;
				// -----------------------------------
			}

			joint->LocalAnimatedMatrix = joint->AnimatedTransform.toMatrix(); // HACK
		} else {
			joint->LocalAnimatedMatrix = joint->LocalMatrix;
		}
	}
}

void CSkinnedMesh::buildAllGlobalAnimatedMatrices(SJoint *joint, SJoint *parentJoint)
{
	if (!joint) {
		for (u32 i = 0; i < RootJoints.size(); ++i)
			buildAllGlobalAnimatedMatrices(RootJoints[i], 0);
		return;
	} else {
		// Find global matrix...
		if (!parentJoint || joint->GlobalSkinningSpace)
			joint->GlobalAnimatedMatrix = joint->LocalAnimatedMatrix;
		else
			joint->GlobalAnimatedMatrix = parentJoint->GlobalAnimatedMatrix * joint->LocalAnimatedMatrix;
	}

	for (u32 j = 0; j < joint->Children.size(); ++j)
		buildAllGlobalAnimatedMatrices(joint->Children[j], joint);
}

//--------------------------------------------------------------------------
//				Software Skinning
//--------------------------------------------------------------------------

//! Preforms a software skin on this mesh based of joint positions
void CSkinnedMesh::skinMesh()
{
	if (!HasAnimation)
		return;

	//----------------
	// This is marked as "Temp!".  A shiny dubloon to whomever can tell me why.
	buildAllGlobalAnimatedMatrices();
	//-----------------

	if (!HardwareSkinning) {
		// Software skin....
		u32 i;

		// rigid animation
		for (i = 0; i < AllJoints.size(); ++i) {
			for (u32 j = 0; j < AllJoints[i]->AttachedMeshes.size(); ++j) {
				SSkinMeshBuffer *Buffer = (*SkinningBuffers)[AllJoints[i]->AttachedMeshes[j]];
				Buffer->Transformation = AllJoints[i]->GlobalAnimatedMatrix;
			}
		}

		// clear skinning helper array
		for (i = 0; i < Vertices_Moved.size(); ++i)
			for (u32 j = 0; j < Vertices_Moved[i].size(); ++j)
				Vertices_Moved[i][j] = false;

		// skin starting with the root joints
		for (i = 0; i < RootJoints.size(); ++i)
			skinJoint(RootJoints[i], 0);

		for (i = 0; i < SkinningBuffers->size(); ++i)
			(*SkinningBuffers)[i]->setDirty(EBT_VERTEX);
	}
	updateBoundingBox();
}

void CSkinnedMesh::skinJoint(SJoint *joint, SJoint *parentJoint)
{
	if (joint->Weights.size()) {
		// Find this joints pull on vertices...
		// Note: It is assumed that the global inversed matrix has been calculated at this point.
		core::matrix4 jointVertexPull = joint->GlobalAnimatedMatrix * joint->GlobalInversedMatrix.value();

		core::vector3df thisVertexMove, thisNormalMove;

		core::array<scene::SSkinMeshBuffer *> &buffersUsed = *SkinningBuffers;

		// Skin Vertices Positions and Normals...
		for (u32 i = 0; i < joint->Weights.size(); ++i) {
			SWeight &weight = joint->Weights[i];

			// Pull this vertex...
			jointVertexPull.transformVect(thisVertexMove, weight.StaticPos);

			if (AnimateNormals) {
				thisNormalMove = jointVertexPull.rotateAndScaleVect(weight.StaticNormal);
				thisNormalMove.normalize(); // must renormalize after potentially scaling
			}

			if (!(*(weight.Moved))) {
				*(weight.Moved) = true;

				buffersUsed[weight.buffer_id]->getVertex(weight.vertex_id)->Pos = thisVertexMove * weight.strength;

				if (AnimateNormals)
					buffersUsed[weight.buffer_id]->getVertex(weight.vertex_id)->Normal = thisNormalMove * weight.strength;

				//*(weight._Pos) = thisVertexMove * weight.strength;
			} else {
				buffersUsed[weight.buffer_id]->getVertex(weight.vertex_id)->Pos += thisVertexMove * weight.strength;

				if (AnimateNormals)
					buffersUsed[weight.buffer_id]->getVertex(weight.vertex_id)->Normal += thisNormalMove * weight.strength;

				//*(weight._Pos) += thisVertexMove * weight.strength;
			}

			buffersUsed[weight.buffer_id]->boundingBoxNeedsRecalculated();
		}
	}

	// Skin all children
	for (u32 j = 0; j < joint->Children.size(); ++j)
		skinJoint(joint->Children[j], joint);
}

E_ANIMATED_MESH_TYPE CSkinnedMesh::getMeshType() const
{
	return EAMT_SKINNED;
}

//! Gets joint count.
u32 CSkinnedMesh::getJointCount() const
{
	return AllJoints.size();
}

//! Gets the name of a joint.
const std::optional<std::string> &CSkinnedMesh::getJointName(u32 number) const
{
	if (number >= getJointCount()) {
		static const std::optional<std::string> nullopt;
		return nullopt;
	}
	return AllJoints[number]->Name;
}

//! Gets a joint number from its name
std::optional<u32> CSkinnedMesh::getJointNumber(const std::string &name) const
{
	for (u32 i = 0; i < AllJoints.size(); ++i) {
		if (AllJoints[i]->Name == name)
			return i;
	}

	return std::nullopt;
}

//! returns amount of mesh buffers.
u32 CSkinnedMesh::getMeshBufferCount() const
{
	return LocalBuffers.size();
}

//! returns pointer to a mesh buffer
IMeshBuffer *CSkinnedMesh::getMeshBuffer(u32 nr) const
{
	if (nr < LocalBuffers.size())
		return LocalBuffers[nr];
	else
		return 0;
}

//! Returns pointer to a mesh buffer which fits a material
IMeshBuffer *CSkinnedMesh::getMeshBuffer(const video::SMaterial &material) const
{
	for (u32 i = 0; i < LocalBuffers.size(); ++i) {
		if (LocalBuffers[i]->getMaterial() == material)
			return LocalBuffers[i];
	}
	return 0;
}

u32 CSkinnedMesh::getTextureSlot(u32 meshbufNr) const
{
	return TextureSlots.at(meshbufNr);
}

void CSkinnedMesh::setTextureSlot(u32 meshbufNr, u32 textureSlot) {
	TextureSlots.at(meshbufNr) = textureSlot;
}

//! returns an axis aligned bounding box
const core::aabbox3d<f32> &CSkinnedMesh::getBoundingBox() const
{
	return BoundingBox;
}

//! set user axis aligned bounding box
void CSkinnedMesh::setBoundingBox(const core::aabbox3df &box)
{
	BoundingBox = box;
}

//! set the hardware mapping hint, for driver
void CSkinnedMesh::setHardwareMappingHint(E_HARDWARE_MAPPING newMappingHint,
		E_BUFFER_TYPE buffer)
{
	for (u32 i = 0; i < LocalBuffers.size(); ++i)
		LocalBuffers[i]->setHardwareMappingHint(newMappingHint, buffer);
}

//! flags the meshbuffer as changed, reloads hardware buffers
void CSkinnedMesh::setDirty(E_BUFFER_TYPE buffer)
{
	for (u32 i = 0; i < LocalBuffers.size(); ++i)
		LocalBuffers[i]->setDirty(buffer);
}

//! uses animation from another mesh
bool CSkinnedMesh::useAnimationFrom(const ISkinnedMesh *mesh)
{
	bool unmatched = false;

	for (u32 i = 0; i < AllJoints.size(); ++i) {
		SJoint *joint = AllJoints[i];
		joint->UseAnimationFrom = 0;

		if (joint->Name == "")
			unmatched = true;
		else {
			for (u32 j = 0; j < mesh->getAllJoints().size(); ++j) {
				SJoint *otherJoint = mesh->getAllJoints()[j];
				if (joint->Name == otherJoint->Name) {
					joint->UseAnimationFrom = otherJoint;
				}
			}
			if (!joint->UseAnimationFrom)
				unmatched = true;
		}
	}

	checkForAnimation();

	return !unmatched;
}

//! Update Normals when Animating
//! False= Don't animate them, faster
//! True= Update normals (default)
void CSkinnedMesh::updateNormalsWhenAnimating(bool on)
{
	AnimateNormals = on;
}

//! Sets Interpolation Mode
void CSkinnedMesh::setInterpolationMode(E_INTERPOLATION_MODE mode)
{
	InterpolationMode = mode;
}

core::array<scene::SSkinMeshBuffer *> &CSkinnedMesh::getMeshBuffers()
{
	return LocalBuffers;
}

core::array<CSkinnedMesh::SJoint *> &CSkinnedMesh::getAllJoints()
{
	return AllJoints;
}

const core::array<CSkinnedMesh::SJoint *> &CSkinnedMesh::getAllJoints() const
{
	return AllJoints;
}

//! (This feature is not implemented in irrlicht yet)
bool CSkinnedMesh::setHardwareSkinning(bool on)
{
	if (HardwareSkinning != on) {
		if (on) {

			// set mesh to static pose...
			for (u32 i = 0; i < AllJoints.size(); ++i) {
				SJoint *joint = AllJoints[i];
				for (u32 j = 0; j < joint->Weights.size(); ++j) {
					const u16 buffer_id = joint->Weights[j].buffer_id;
					const u32 vertex_id = joint->Weights[j].vertex_id;
					LocalBuffers[buffer_id]->getVertex(vertex_id)->Pos = joint->Weights[j].StaticPos;
					LocalBuffers[buffer_id]->getVertex(vertex_id)->Normal = joint->Weights[j].StaticNormal;
					LocalBuffers[buffer_id]->boundingBoxNeedsRecalculated();
				}
			}
		}

		HardwareSkinning = on;
	}
	return HardwareSkinning;
}

void CSkinnedMesh::refreshJointCache()
{
	// copy cache from the mesh...
	for (u32 i = 0; i < AllJoints.size(); ++i) {
		SJoint *joint = AllJoints[i];
		for (u32 j = 0; j < joint->Weights.size(); ++j) {
			const u16 buffer_id = joint->Weights[j].buffer_id;
			const u32 vertex_id = joint->Weights[j].vertex_id;
			joint->Weights[j].StaticPos = LocalBuffers[buffer_id]->getVertex(vertex_id)->Pos;
			joint->Weights[j].StaticNormal = LocalBuffers[buffer_id]->getVertex(vertex_id)->Normal;
		}
	}
}

void CSkinnedMesh::resetAnimation()
{
	// copy from the cache to the mesh...
	for (u32 i = 0; i < AllJoints.size(); ++i) {
		SJoint *joint = AllJoints[i];
		for (u32 j = 0; j < joint->Weights.size(); ++j) {
			const u16 buffer_id = joint->Weights[j].buffer_id;
			const u32 vertex_id = joint->Weights[j].vertex_id;
			LocalBuffers[buffer_id]->getVertex(vertex_id)->Pos = joint->Weights[j].StaticPos;
			LocalBuffers[buffer_id]->getVertex(vertex_id)->Normal = joint->Weights[j].StaticNormal;
		}
	}
}

void CSkinnedMesh::calculateGlobalMatrices(SJoint *joint, SJoint *parentJoint)
{
	if (!joint && parentJoint) // bit of protection from endless loops
		return;

	// Go through the root bones
	if (!joint) {
		for (u32 i = 0; i < RootJoints.size(); ++i)
			calculateGlobalMatrices(RootJoints[i], 0);
		return;
	}

	if (!parentJoint)
		joint->GlobalMatrix = joint->LocalMatrix;
	else
		joint->GlobalMatrix = parentJoint->GlobalMatrix * joint->LocalMatrix;

	joint->LocalAnimatedMatrix = joint->LocalMatrix;
	joint->GlobalAnimatedMatrix = joint->GlobalMatrix;

	if (!joint->GlobalInversedMatrix.has_value()) { // might be pre calculated
		joint->GlobalInversedMatrix = joint->GlobalMatrix;
		joint->GlobalInversedMatrix->makeInverse(); // slow
	}

	for (u32 j = 0; j < joint->Children.size(); ++j)
		calculateGlobalMatrices(joint->Children[j], joint);
}

void CSkinnedMesh::checkForAnimation()
{
	u32 i, j;
	// Check for animation...
	HasAnimation = false;
	for (i = 0; i < AllJoints.size(); ++i) {
		if (AllJoints[i]->UseAnimationFrom && !AllJoints[i]->UseAnimationFrom->keys.empty()) {
			HasAnimation = true;
			break;
		}
	}

	// meshes with weights, are still counted as animated for ragdolls, etc
	if (!HasAnimation) {
		for (i = 0; i < AllJoints.size(); ++i) {
			if (AllJoints[i]->Weights.size())
				HasAnimation = true;
		}
	}

	if (HasAnimation) {
		// Find the length of the animation
		EndFrame = 0;
		for (i = 0; i < AllJoints.size(); ++i) {
			if (const auto *joint = AllJoints[i]->UseAnimationFrom) {
				EndFrame = std::max(EndFrame, joint->keys.getEndFrame());
			}
		}
	}

	if (HasAnimation && !PreparedForSkinning) {
		PreparedForSkinning = true;

		// check for bugs:
		for (i = 0; i < AllJoints.size(); ++i) {
			SJoint *joint = AllJoints[i];
			for (j = 0; j < joint->Weights.size(); ++j) {
				const u16 buffer_id = joint->Weights[j].buffer_id;
				const u32 vertex_id = joint->Weights[j].vertex_id;

				// check for invalid ids
				if (buffer_id >= LocalBuffers.size()) {
					os::Printer::log("Skinned Mesh: Weight buffer id too large", ELL_WARNING);
					joint->Weights[j].buffer_id = joint->Weights[j].vertex_id = 0;
				} else if (vertex_id >= LocalBuffers[buffer_id]->getVertexCount()) {
					os::Printer::log("Skinned Mesh: Weight vertex id too large", ELL_WARNING);
					joint->Weights[j].buffer_id = joint->Weights[j].vertex_id = 0;
				}
			}
		}

		// An array used in skinning

		for (i = 0; i < Vertices_Moved.size(); ++i)
			for (j = 0; j < Vertices_Moved[i].size(); ++j)
				Vertices_Moved[i][j] = false;

		// For skinning: cache weight values for speed

		for (i = 0; i < AllJoints.size(); ++i) {
			SJoint *joint = AllJoints[i];
			for (j = 0; j < joint->Weights.size(); ++j) {
				const u16 buffer_id = joint->Weights[j].buffer_id;
				const u32 vertex_id = joint->Weights[j].vertex_id;

				joint->Weights[j].Moved = &Vertices_Moved[buffer_id][vertex_id];
				joint->Weights[j].StaticPos = LocalBuffers[buffer_id]->getVertex(vertex_id)->Pos;
				joint->Weights[j].StaticNormal = LocalBuffers[buffer_id]->getVertex(vertex_id)->Normal;

				// joint->Weights[j]._Pos=&Buffers[buffer_id]->getVertex(vertex_id)->Pos;
			}
		}

		// normalize weights
		normalizeWeights();
	}
}

//! called by loader after populating with mesh and bone data
void CSkinnedMesh::finalize()
{
	os::Printer::log("Skinned Mesh - finalize", ELL_DEBUG);
	u32 i;

	// calculate bounding box
	for (i = 0; i < LocalBuffers.size(); ++i) {
		LocalBuffers[i]->recalculateBoundingBox();
	}

	if (AllJoints.size() || RootJoints.size()) {
		// populate AllJoints or RootJoints, depending on which is empty
		if (!RootJoints.size()) {

			for (u32 CheckingIdx = 0; CheckingIdx < AllJoints.size(); ++CheckingIdx) {

				bool foundParent = false;
				for (i = 0; i < AllJoints.size(); ++i) {
					for (u32 n = 0; n < AllJoints[i]->Children.size(); ++n) {
						if (AllJoints[i]->Children[n] == AllJoints[CheckingIdx])
							foundParent = true;
					}
				}

				if (!foundParent)
					RootJoints.push_back(AllJoints[CheckingIdx]);
			}
		} else {
			AllJoints = RootJoints;
		}
	}

	for (i = 0; i < AllJoints.size(); ++i) {
		AllJoints[i]->UseAnimationFrom = AllJoints[i];
	}

	// Set array sizes...

	for (i = 0; i < LocalBuffers.size(); ++i) {
		Vertices_Moved.push_back(core::array<char>());
		Vertices_Moved[i].set_used(LocalBuffers[i]->getVertexCount());
	}

	checkForAnimation();

	if (HasAnimation) {
		for (i = 0; i < AllJoints.size(); ++i) {
			AllJoints[i]->keys.cleanup();
		}
	}

	// Needed for animation and skinning...

	calculateGlobalMatrices(0, 0);

	// rigid animation for non animated meshes
	for (i = 0; i < AllJoints.size(); ++i) {
		for (u32 j = 0; j < AllJoints[i]->AttachedMeshes.size(); ++j) {
			SSkinMeshBuffer *Buffer = (*SkinningBuffers)[AllJoints[i]->AttachedMeshes[j]];
			Buffer->Transformation = AllJoints[i]->GlobalAnimatedMatrix;
		}
	}

	// calculate bounding box
	if (LocalBuffers.empty())
		BoundingBox.reset(0, 0, 0);
	else {
		irr::core::aabbox3df bb(LocalBuffers[0]->BoundingBox);
		LocalBuffers[0]->Transformation.transformBoxEx(bb);
		BoundingBox.reset(bb);

		for (u32 j = 1; j < LocalBuffers.size(); ++j) {
			bb = LocalBuffers[j]->BoundingBox;
			LocalBuffers[j]->Transformation.transformBoxEx(bb);

			BoundingBox.addInternalBox(bb);
		}
	}
}

void CSkinnedMesh::updateBoundingBox(void)
{
	if (!SkinningBuffers)
		return;

	core::array<SSkinMeshBuffer *> &buffer = *SkinningBuffers;
	BoundingBox.reset(0, 0, 0);

	if (!buffer.empty()) {
		for (u32 j = 0; j < buffer.size(); ++j) {
			buffer[j]->recalculateBoundingBox();
			core::aabbox3df bb = buffer[j]->BoundingBox;
			buffer[j]->Transformation.transformBoxEx(bb);

			BoundingBox.addInternalBox(bb);
		}
	}
}

scene::SSkinMeshBuffer *CSkinnedMesh::addMeshBuffer()
{
	scene::SSkinMeshBuffer *buffer = new scene::SSkinMeshBuffer();
	TextureSlots.push_back(LocalBuffers.size());
	LocalBuffers.push_back(buffer);
	return buffer;
}

void CSkinnedMesh::addMeshBuffer(SSkinMeshBuffer *meshbuf)
{
	TextureSlots.push_back(LocalBuffers.size());
	LocalBuffers.push_back(meshbuf);
}

CSkinnedMesh::SJoint *CSkinnedMesh::addJoint(SJoint *parent)
{
	SJoint *joint = new SJoint;

	AllJoints.push_back(joint);
	if (!parent) {
		// Add root joints to array in finalize()
	} else {
		// Set parent (Be careful of the mesh loader also setting the parent)
		parent->Children.push_back(joint);
	}

	return joint;
}

void CSkinnedMesh::addPositionKey(SJoint *joint, f32 frame, core::vector3df pos)
{
	_IRR_DEBUG_BREAK_IF(!joint);
	joint->keys.position.add(frame, pos);
}

void CSkinnedMesh::addScaleKey(SJoint *joint, f32 frame, core::vector3df scale)
{
	_IRR_DEBUG_BREAK_IF(!joint);
	joint->keys.scale.add(frame, scale);
}

void CSkinnedMesh::addRotationKey(SJoint *joint, f32 frame, core::quaternion rot)
{
	_IRR_DEBUG_BREAK_IF(!joint);
	joint->keys.rotation.add(frame, rot);
}

CSkinnedMesh::SWeight *CSkinnedMesh::addWeight(SJoint *joint)
{
	if (!joint)
		return 0;

	joint->Weights.push_back(SWeight());
	return &joint->Weights.getLast();
}

bool CSkinnedMesh::isStatic()
{
	return !HasAnimation;
}

void CSkinnedMesh::normalizeWeights()
{
	// note: unsure if weights ids are going to be used.

	// Normalise the weights on bones....

	u32 i, j;
	core::array<core::array<f32>> verticesTotalWeight;

	verticesTotalWeight.reallocate(LocalBuffers.size());
	for (i = 0; i < LocalBuffers.size(); ++i) {
		verticesTotalWeight.push_back(core::array<f32>());
		verticesTotalWeight[i].set_used(LocalBuffers[i]->getVertexCount());
	}

	for (i = 0; i < verticesTotalWeight.size(); ++i)
		for (j = 0; j < verticesTotalWeight[i].size(); ++j)
			verticesTotalWeight[i][j] = 0;

	for (i = 0; i < AllJoints.size(); ++i) {
		SJoint *joint = AllJoints[i];
		for (j = 0; j < joint->Weights.size(); ++j) {
			if (joint->Weights[j].strength <= 0) { // Check for invalid weights
				joint->Weights.erase(j);
				--j;
			} else {
				verticesTotalWeight[joint->Weights[j].buffer_id][joint->Weights[j].vertex_id] += joint->Weights[j].strength;
			}
		}
	}

	for (i = 0; i < AllJoints.size(); ++i) {
		SJoint *joint = AllJoints[i];
		for (j = 0; j < joint->Weights.size(); ++j) {
			const f32 total = verticesTotalWeight[joint->Weights[j].buffer_id][joint->Weights[j].vertex_id];
			if (total != 0 && total != 1)
				joint->Weights[j].strength /= total;
		}
	}
}

void CSkinnedMesh::recoverJointsFromMesh(core::array<IBoneSceneNode *> &jointChildSceneNodes)
{
	for (u32 i = 0; i < AllJoints.size(); ++i) {
		IBoneSceneNode *node = jointChildSceneNodes[i];
		SJoint *joint = AllJoints[i];
		node->setPosition(joint->AnimatedTransform.translation);
		core::vector3df euler;
		core::quaternion rot = joint->AnimatedTransform.rotation;
		rot.makeInverse();
		rot.toEuler(euler);
		node->setRotation(core::RADTODEG * euler);
		node->setScale(joint->AnimatedTransform.scale);
		node->updateAbsolutePosition();
	}
}

void CSkinnedMesh::transferJointsToMesh(const core::array<IBoneSceneNode *> &jointChildSceneNodes)
{
	for (u32 i = 0; i < AllJoints.size(); ++i) {
		const IBoneSceneNode *const node = jointChildSceneNodes[i];
		SJoint *joint = AllJoints[i];

		joint->LocalAnimatedMatrix.setRotationDegrees(node->getRotation());
		joint->LocalAnimatedMatrix.setTranslation(node->getPosition());
		joint->LocalAnimatedMatrix *= core::matrix4().setScale(node->getScale());

		joint->GlobalSkinningSpace = (node->getSkinningSpace() == EBSS_GLOBAL);
	}
}

void CSkinnedMesh::addJoints(core::array<IBoneSceneNode *> &jointChildSceneNodes,
		IAnimatedMeshSceneNode *node, ISceneManager *smgr)
{
	// Create new joints
	for (u32 i = 0; i < AllJoints.size(); ++i) {
		jointChildSceneNodes.push_back(new CBoneSceneNode(0, smgr, 0, i, AllJoints[i]->Name));
	}

	// Match up parents
	for (u32 i = 0; i < jointChildSceneNodes.size(); ++i) {
		const SJoint *const joint = AllJoints[i]; // should be fine

		s32 parentID = -1;

		for (u32 j = 0; (parentID == -1) && (j < AllJoints.size()); ++j) {
			if (i != j) {
				const SJoint *const parentTest = AllJoints[j];
				for (u32 n = 0; n < parentTest->Children.size(); ++n) {
					if (parentTest->Children[n] == joint) {
						parentID = j;
						break;
					}
				}
			}
		}

		IBoneSceneNode *bone = jointChildSceneNodes[i];
		if (parentID != -1)
			bone->setParent(jointChildSceneNodes[parentID]);
		else
			bone->setParent(node);

		bone->drop();
	}
}

void CSkinnedMesh::convertMeshToTangents()
{
	// now calculate tangents
	for (u32 b = 0; b < LocalBuffers.size(); ++b) {
		if (LocalBuffers[b]) {
			LocalBuffers[b]->convertToTangents();

			const s32 idxCnt = LocalBuffers[b]->getIndexCount();

			u16 *idx = LocalBuffers[b]->getIndices();
			video::S3DVertexTangents *v =
					(video::S3DVertexTangents *)LocalBuffers[b]->getVertices();

			for (s32 i = 0; i < idxCnt; i += 3) {
				calculateTangents(
						v[idx[i + 0]].Normal,
						v[idx[i + 0]].Tangent,
						v[idx[i + 0]].Binormal,
						v[idx[i + 0]].Pos,
						v[idx[i + 1]].Pos,
						v[idx[i + 2]].Pos,
						v[idx[i + 0]].TCoords,
						v[idx[i + 1]].TCoords,
						v[idx[i + 2]].TCoords);

				calculateTangents(
						v[idx[i + 1]].Normal,
						v[idx[i + 1]].Tangent,
						v[idx[i + 1]].Binormal,
						v[idx[i + 1]].Pos,
						v[idx[i + 2]].Pos,
						v[idx[i + 0]].Pos,
						v[idx[i + 1]].TCoords,
						v[idx[i + 2]].TCoords,
						v[idx[i + 0]].TCoords);

				calculateTangents(
						v[idx[i + 2]].Normal,
						v[idx[i + 2]].Tangent,
						v[idx[i + 2]].Binormal,
						v[idx[i + 2]].Pos,
						v[idx[i + 0]].Pos,
						v[idx[i + 1]].Pos,
						v[idx[i + 2]].TCoords,
						v[idx[i + 0]].TCoords,
						v[idx[i + 1]].TCoords);
			}
		}
	}
}

void CSkinnedMesh::calculateTangents(
		core::vector3df &normal,
		core::vector3df &tangent,
		core::vector3df &binormal,
		const core::vector3df &vt1, const core::vector3df &vt2, const core::vector3df &vt3, // vertices
		const core::vector2df &tc1, const core::vector2df &tc2, const core::vector2df &tc3) // texture coords
{
	core::vector3df v1 = vt1 - vt2;
	core::vector3df v2 = vt3 - vt1;
	normal = v2.crossProduct(v1);
	normal.normalize();

	// binormal

	f32 deltaX1 = tc1.X - tc2.X;
	f32 deltaX2 = tc3.X - tc1.X;
	binormal = (v1 * deltaX2) - (v2 * deltaX1);
	binormal.normalize();

	// tangent

	f32 deltaY1 = tc1.Y - tc2.Y;
	f32 deltaY2 = tc3.Y - tc1.Y;
	tangent = (v1 * deltaY2) - (v2 * deltaY1);
	tangent.normalize();

	// adjust

	core::vector3df txb = tangent.crossProduct(binormal);
	if (txb.dotProduct(normal) < 0.0f) {
		tangent *= -1.0f;
		binormal *= -1.0f;
	}
}

} // end namespace scene
} // end namespace irr

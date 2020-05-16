// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_MD2_LOADER_

#include "CAnimatedMeshMD2.h"
#include "SColor.h"
#include "irrMath.h"

namespace irr
{
namespace scene
{

const s32 MD2_FRAME_SHIFT	= 2;
const f32 MD2_FRAME_SHIFT_RECIPROCAL = 1.f / (1 << MD2_FRAME_SHIFT);

const s32 Q2_VERTEX_NORMAL_TABLE_SIZE = 162;

static const f32 Q2_VERTEX_NORMAL_TABLE[Q2_VERTEX_NORMAL_TABLE_SIZE][3] = {
	{-0.525731f, 0.000000f, 0.850651f},
	{-0.442863f, 0.238856f, 0.864188f},
	{-0.295242f, 0.000000f, 0.955423f},
	{-0.309017f, 0.500000f, 0.809017f},
	{-0.162460f, 0.262866f, 0.951056f},
	{0.000000f, 0.000000f, 1.000000f},
	{0.000000f, 0.850651f, 0.525731f},
	{-0.147621f, 0.716567f, 0.681718f},
	{0.147621f, 0.716567f, 0.681718f},
	{0.000000f, 0.525731f, 0.850651f},
	{0.309017f, 0.500000f, 0.809017f},
	{0.525731f, 0.000000f, 0.850651f},
	{0.295242f, 0.000000f, 0.955423f},
	{0.442863f, 0.238856f, 0.864188f},
	{0.162460f, 0.262866f, 0.951056f},
	{-0.681718f, 0.147621f, 0.716567f},
	{-0.809017f, 0.309017f, 0.500000f},
	{-0.587785f, 0.425325f, 0.688191f},
	{-0.850651f, 0.525731f, 0.000000f},
	{-0.864188f, 0.442863f, 0.238856f},
	{-0.716567f, 0.681718f, 0.147621f},
	{-0.688191f, 0.587785f, 0.425325f},
	{-0.500000f, 0.809017f, 0.309017f},
	{-0.238856f, 0.864188f, 0.442863f},
	{-0.425325f, 0.688191f, 0.587785f},
	{-0.716567f, 0.681718f, -0.147621f},
	{-0.500000f, 0.809017f, -0.309017f},
	{-0.525731f, 0.850651f, 0.000000f},
	{0.000000f, 0.850651f, -0.525731f},
	{-0.238856f, 0.864188f, -0.442863f},
	{0.000000f, 0.955423f, -0.295242f},
	{-0.262866f, 0.951056f, -0.162460f},
	{0.000000f, 1.000000f, 0.000000f},
	{0.000000f, 0.955423f, 0.295242f},
	{-0.262866f, 0.951056f, 0.162460f},
	{0.238856f, 0.864188f, 0.442863f},
	{0.262866f, 0.951056f, 0.162460f},
	{0.500000f, 0.809017f, 0.309017f},
	{0.238856f, 0.864188f, -0.442863f},
	{0.262866f, 0.951056f, -0.162460f},
	{0.500000f, 0.809017f, -0.309017f},
	{0.850651f, 0.525731f, 0.000000f},
	{0.716567f, 0.681718f, 0.147621f},
	{0.716567f, 0.681718f, -0.147621f},
	{0.525731f, 0.850651f, 0.000000f},
	{0.425325f, 0.688191f, 0.587785f},
	{0.864188f, 0.442863f, 0.238856f},
	{0.688191f, 0.587785f, 0.425325f},
	{0.809017f, 0.309017f, 0.500000f},
	{0.681718f, 0.147621f, 0.716567f},
	{0.587785f, 0.425325f, 0.688191f},
	{0.955423f, 0.295242f, 0.000000f},
	{1.000000f, 0.000000f, 0.000000f},
	{0.951056f, 0.162460f, 0.262866f},
	{0.850651f, -0.525731f, 0.000000f},
	{0.955423f, -0.295242f, 0.000000f},
	{0.864188f, -0.442863f, 0.238856f},
	{0.951056f, -0.162460f, 0.262866f},
	{0.809017f, -0.309017f, 0.500000f},
	{0.681718f, -0.147621f, 0.716567f},
	{0.850651f, 0.000000f, 0.525731f},
	{0.864188f, 0.442863f, -0.238856f},
	{0.809017f, 0.309017f, -0.500000f},
	{0.951056f, 0.162460f, -0.262866f},
	{0.525731f, 0.000000f, -0.850651f},
	{0.681718f, 0.147621f, -0.716567f},
	{0.681718f, -0.147621f, -0.716567f},
	{0.850651f, 0.000000f, -0.525731f},
	{0.809017f, -0.309017f, -0.500000f},
	{0.864188f, -0.442863f, -0.238856f},
	{0.951056f, -0.162460f, -0.262866f},
	{0.147621f, 0.716567f, -0.681718f},
	{0.309017f, 0.500000f, -0.809017f},
	{0.425325f, 0.688191f, -0.587785f},
	{0.442863f, 0.238856f, -0.864188f},
	{0.587785f, 0.425325f, -0.688191f},
	{0.688191f, 0.587785f, -0.425325f},
	{-0.147621f, 0.716567f, -0.681718f},
	{-0.309017f, 0.500000f, -0.809017f},
	{0.000000f, 0.525731f, -0.850651f},
	{-0.525731f, 0.000000f, -0.850651f},
	{-0.442863f, 0.238856f, -0.864188f},
	{-0.295242f, 0.000000f, -0.955423f},
	{-0.162460f, 0.262866f, -0.951056f},
	{0.000000f, 0.000000f, -1.000000f},
	{0.295242f, 0.000000f, -0.955423f},
	{0.162460f, 0.262866f, -0.951056f},
	{-0.442863f, -0.238856f, -0.864188f},
	{-0.309017f, -0.500000f, -0.809017f},
	{-0.162460f, -0.262866f, -0.951056f},
	{0.000000f, -0.850651f, -0.525731f},
	{-0.147621f, -0.716567f, -0.681718f},
	{0.147621f, -0.716567f, -0.681718f},
	{0.000000f, -0.525731f, -0.850651f},
	{0.309017f, -0.500000f, -0.809017f},
	{0.442863f, -0.238856f, -0.864188f},
	{0.162460f, -0.262866f, -0.951056f},
	{0.238856f, -0.864188f, -0.442863f},
	{0.500000f, -0.809017f, -0.309017f},
	{0.425325f, -0.688191f, -0.587785f},
	{0.716567f, -0.681718f, -0.147621f},
	{0.688191f, -0.587785f, -0.425325f},
	{0.587785f, -0.425325f, -0.688191f},
	{0.000000f, -0.955423f, -0.295242f},
	{0.000000f, -1.000000f, 0.000000f},
	{0.262866f, -0.951056f, -0.162460f},
	{0.000000f, -0.850651f, 0.525731f},
	{0.000000f, -0.955423f, 0.295242f},
	{0.238856f, -0.864188f, 0.442863f},
	{0.262866f, -0.951056f, 0.162460f},
	{0.500000f, -0.809017f, 0.309017f},
	{0.716567f, -0.681718f, 0.147621f},
	{0.525731f, -0.850651f, 0.000000f},
	{-0.238856f, -0.864188f, -0.442863f},
	{-0.500000f, -0.809017f, -0.309017f},
	{-0.262866f, -0.951056f, -0.162460f},
	{-0.850651f, -0.525731f, 0.000000f},
	{-0.716567f, -0.681718f, -0.147621f},
	{-0.716567f, -0.681718f, 0.147621f},
	{-0.525731f, -0.850651f, 0.000000f},
	{-0.500000f, -0.809017f, 0.309017f},
	{-0.238856f, -0.864188f, 0.442863f},
	{-0.262866f, -0.951056f, 0.162460f},
	{-0.864188f, -0.442863f, 0.238856f},
	{-0.809017f, -0.309017f, 0.500000f},
	{-0.688191f, -0.587785f, 0.425325f},
	{-0.681718f, -0.147621f, 0.716567f},
	{-0.442863f, -0.238856f, 0.864188f},
	{-0.587785f, -0.425325f, 0.688191f},
	{-0.309017f, -0.500000f, 0.809017f},
	{-0.147621f, -0.716567f, 0.681718f},
	{-0.425325f, -0.688191f, 0.587785f},
	{-0.162460f, -0.262866f, 0.951056f},
	{0.442863f, -0.238856f, 0.864188f},
	{0.162460f, -0.262866f, 0.951056f},
	{0.309017f, -0.500000f, 0.809017f},
	{0.147621f, -0.716567f, 0.681718f},
	{0.000000f, -0.525731f, 0.850651f},
	{0.425325f, -0.688191f, 0.587785f},
	{0.587785f, -0.425325f, 0.688191f},
	{0.688191f, -0.587785f, 0.425325f},
	{-0.955423f, 0.295242f, 0.000000f},
	{-0.951056f, 0.162460f, 0.262866f},
	{-1.000000f, 0.000000f, 0.000000f},
	{-0.850651f, 0.000000f, 0.525731f},
	{-0.955423f, -0.295242f, 0.000000f},
	{-0.951056f, -0.162460f, 0.262866f},
	{-0.864188f, 0.442863f, -0.238856f},
	{-0.951056f, 0.162460f, -0.262866f},
	{-0.809017f, 0.309017f, -0.500000f},
	{-0.864188f, -0.442863f, -0.238856f},
	{-0.951056f, -0.162460f, -0.262866f},
	{-0.809017f, -0.309017f, -0.500000f},
	{-0.681718f, 0.147621f, -0.716567f},
	{-0.681718f, -0.147621f, -0.716567f},
	{-0.850651f, 0.000000f, -0.525731f},
	{-0.688191f, 0.587785f, -0.425325f},
	{-0.587785f, 0.425325f, -0.688191f},
	{-0.425325f, 0.688191f, -0.587785f},
	{-0.425325f, -0.688191f, -0.587785f},
	{-0.587785f, -0.425325f, -0.688191f},
	{-0.688191f, -0.587785f, -0.425325f},
	};

struct SMD2AnimationType
{
	s32 begin;
	s32 end;
	s32 fps;
};

static const SMD2AnimationType MD2AnimationTypeList[21] =
{
	{  0,  39,  9}, // STAND
	{ 40,  45, 10}, // RUN
	{ 46,  53, 10}, // ATTACK
	{ 54,  57,  7}, // PAIN_A
	{ 58,  61,  7}, // PAIN_B
	{ 62,  65,  7}, // PAIN_C
	{ 66,  71,  7}, // JUMP
	{ 72,  83,  7}, // FLIP
	{ 84,  94,  7}, // SALUTE
	{ 95, 111, 10}, // FALLBACK
	{112, 122,  7}, // WAVE
	{123, 134,  6}, // POINT
	{135, 153, 10}, // CROUCH_STAND
	{154, 159,  7}, // CROUCH_WALK
	{160, 168, 10}, // CROUCH_ATTACK
	{169, 172,  7}, // CROUCH_PAIN
	{173, 177,  5}, // CROUCH_DEATH
	{178, 183,  7}, // DEATH_FALLBACK
	{184, 189,  7}, // DEATH_FALLFORWARD
	{190, 197,  7}, // DEATH_FALLBACKSLOW
	{198, 198,  5}, // BOOM
};


//! constructor
CAnimatedMeshMD2::CAnimatedMeshMD2()
	: InterpolationBuffer(0), FrameList(0), FrameCount(0), FramesPerSecond((f32)(MD2AnimationTypeList[0].fps << MD2_FRAME_SHIFT))
{
	#ifdef _DEBUG
	IAnimatedMesh::setDebugName("CAnimatedMeshMD2 IAnimatedMesh");
	IMesh::setDebugName("CAnimatedMeshMD2 IMesh");
	#endif
	InterpolationBuffer = new SMeshBuffer;
}


//! destructor
CAnimatedMeshMD2::~CAnimatedMeshMD2()
{
	delete [] FrameList;
	if (InterpolationBuffer)
		InterpolationBuffer->drop();
}


//! returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.
u32 CAnimatedMeshMD2::getFrameCount() const
{
	return FrameCount<<MD2_FRAME_SHIFT;
}


//! returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail. Note, that some Meshes will ignore the detail level.
IMesh* CAnimatedMeshMD2::getMesh(s32 frame, s32 detailLevel, s32 startFrameLoop, s32 endFrameLoop)
{
	if ((u32)frame > getFrameCount())
		frame = (frame % getFrameCount());

	if (startFrameLoop == -1 && endFrameLoop == -1)
	{
		startFrameLoop = 0;
		endFrameLoop = getFrameCount();
	}

	updateInterpolationBuffer(frame, startFrameLoop, endFrameLoop);
	return this;
}


//! returns amount of mesh buffers. MD2 meshes only have one buffer
u32 CAnimatedMeshMD2::getMeshBufferCount() const
{
	return 1;
}


//! returns pointer to a mesh buffer
IMeshBuffer* CAnimatedMeshMD2::getMeshBuffer(u32 nr) const
{
	if (nr == 0)
		return InterpolationBuffer;
	else
		return 0;
}


//! Returns pointer to a mesh buffer which fits a material
IMeshBuffer* CAnimatedMeshMD2::getMeshBuffer(const video::SMaterial &material) const
{
	if (InterpolationBuffer->Material == material)
		return InterpolationBuffer;
	else
		return 0;
}


// updates the interpolation buffer
void CAnimatedMeshMD2::updateInterpolationBuffer(s32 frame, s32 startFrameLoop, s32 endFrameLoop)
{
	u32 firstFrame, secondFrame;
	f32 div;

	// TA: resolve missing ipol in loop between end-start

	if (endFrameLoop - startFrameLoop == 0)
	{
		firstFrame = frame>>MD2_FRAME_SHIFT;
		secondFrame = frame>>MD2_FRAME_SHIFT;
		div = 1.0f;
	}
	else
	{
		// key frames
		u32 s = startFrameLoop >> MD2_FRAME_SHIFT;
		u32 e = endFrameLoop >> MD2_FRAME_SHIFT;

		firstFrame = frame >> MD2_FRAME_SHIFT;
		secondFrame = core::if_c_a_else_b(firstFrame + 1 > e, s, firstFrame + 1);

		firstFrame = core::s32_min(FrameCount - 1, firstFrame);
		secondFrame = core::s32_min(FrameCount - 1, secondFrame);

		//div = (frame % (1<<MD2_FRAME_SHIFT)) / (f32)(1<<MD2_FRAME_SHIFT);
		frame &= (1<<MD2_FRAME_SHIFT) - 1;
		div = frame * MD2_FRAME_SHIFT_RECIPROCAL;
	}

	video::S3DVertex* target = static_cast<video::S3DVertex*>(InterpolationBuffer->getVertices());
	SMD2Vert* first = FrameList[firstFrame].pointer();
	SMD2Vert* second = FrameList[secondFrame].pointer();

	// interpolate both frames
	const u32 count = FrameList[firstFrame].size();
	for (u32 i=0; i<count; ++i)
	{
		const core::vector3df one = core::vector3df(f32(first->Pos.X) * FrameTransforms[firstFrame].scale.X + FrameTransforms[firstFrame].translate.X,
				f32(first->Pos.Y) * FrameTransforms[firstFrame].scale.Y + FrameTransforms[firstFrame].translate.Y,
				f32(first->Pos.Z) * FrameTransforms[firstFrame].scale.Z + FrameTransforms[firstFrame].translate.Z);
		const core::vector3df two = core::vector3df(f32(second->Pos.X) * FrameTransforms[secondFrame].scale.X + FrameTransforms[secondFrame].translate.X,
				f32(second->Pos.Y) * FrameTransforms[secondFrame].scale.Y + FrameTransforms[secondFrame].translate.Y,
				f32(second->Pos.Z) * FrameTransforms[secondFrame].scale.Z + FrameTransforms[secondFrame].translate.Z);
		target->Pos = two.getInterpolated(one, div);
		const core::vector3df n1(
				Q2_VERTEX_NORMAL_TABLE[first->NormalIdx][0],
				Q2_VERTEX_NORMAL_TABLE[first->NormalIdx][2],
				Q2_VERTEX_NORMAL_TABLE[first->NormalIdx][1]);
		const core::vector3df n2(
				Q2_VERTEX_NORMAL_TABLE[second->NormalIdx][0],
				Q2_VERTEX_NORMAL_TABLE[second->NormalIdx][2],
				Q2_VERTEX_NORMAL_TABLE[second->NormalIdx][1]);
		target->Normal = n2.getInterpolated(n1, div);
		++target;
		++first;
		++second;
	}

	//update bounding box
	InterpolationBuffer->setBoundingBox(BoxList[secondFrame].getInterpolated(BoxList[firstFrame], div));
	InterpolationBuffer->setDirty();
}


//! sets a flag of all contained materials to a new value
void CAnimatedMeshMD2::setMaterialFlag(video::E_MATERIAL_FLAG flag, bool newvalue)
{
	InterpolationBuffer->Material.setFlag(flag, newvalue);
}


//! set the hardware mapping hint, for driver
void CAnimatedMeshMD2::setHardwareMappingHint(E_HARDWARE_MAPPING newMappingHint,
		E_BUFFER_TYPE buffer)
{
	InterpolationBuffer->setHardwareMappingHint(newMappingHint, buffer);
}


//! flags the meshbuffer as changed, reloads hardware buffers
void CAnimatedMeshMD2::setDirty(E_BUFFER_TYPE buffer)
{
	InterpolationBuffer->setDirty(buffer);
}


//! returns an axis aligned bounding box
const core::aabbox3d<f32>& CAnimatedMeshMD2::getBoundingBox() const
{
	return InterpolationBuffer->BoundingBox;
}


//! set user axis aligned bounding box
void CAnimatedMeshMD2::setBoundingBox(const core::aabbox3df& box)
{
	InterpolationBuffer->BoundingBox = box;
}


//! Returns the type of the animated mesh.
E_ANIMATED_MESH_TYPE CAnimatedMeshMD2::getMeshType() const
{
	return EAMT_MD2;
}


//! Returns frame loop data for a special MD2 animation type.
void CAnimatedMeshMD2::getFrameLoop(EMD2_ANIMATION_TYPE l,
				s32& outBegin, s32& outEnd, s32& outFPS) const
{
	if (l < 0 || l >= EMAT_COUNT)
		return;

	outBegin = MD2AnimationTypeList[l].begin << MD2_FRAME_SHIFT;
	outEnd = MD2AnimationTypeList[l].end << MD2_FRAME_SHIFT;

	// correct to anim between last->first frame
	outEnd += MD2_FRAME_SHIFT == 0 ? 1 : (1 << MD2_FRAME_SHIFT) - 1;
	outFPS = MD2AnimationTypeList[l].fps << MD2_FRAME_SHIFT;
}


//! Returns frame loop data for a special MD2 animation type.
bool CAnimatedMeshMD2::getFrameLoop(const c8* name,
	s32& outBegin, s32&outEnd, s32& outFPS) const
{
	for (u32 i=0; i < AnimationData.size(); ++i)
	{
		if (AnimationData[i].name == name)
		{
			outBegin = AnimationData[i].begin << MD2_FRAME_SHIFT;
			outEnd = AnimationData[i].end << MD2_FRAME_SHIFT;
			outEnd += MD2_FRAME_SHIFT == 0 ? 1 : (1 << MD2_FRAME_SHIFT) - 1;
			outFPS = AnimationData[i].fps << MD2_FRAME_SHIFT;
			return true;
		}
	}

	return false;
}


//! Returns amount of md2 animations in this file.
s32 CAnimatedMeshMD2::getAnimationCount() const
{
	return AnimationData.size();
}


//! Returns name of md2 animation.
const c8* CAnimatedMeshMD2::getAnimationName(s32 nr) const
{
	if ((u32)nr >= AnimationData.size())
		return 0;

	return AnimationData[nr].name.c_str();
}


} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_MD2_LOADER_

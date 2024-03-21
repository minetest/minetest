// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IMesh.h"
#include "IMeshBuffer.h"
#include "aabbox3d.h"
#include "irrArray.h"

namespace irr
{
namespace scene
{
//! Simple implementation of the IMesh interface.
struct SMesh : public IMesh
{
	//! constructor
	SMesh()
	{
#ifdef _DEBUG
		setDebugName("SMesh");
#endif
	}

	//! destructor
	virtual ~SMesh()
	{
		// drop buffers
		for (u32 i = 0; i < MeshBuffers.size(); ++i)
			MeshBuffers[i]->drop();
	}

	//! clean mesh
	virtual void clear()
	{
		for (u32 i = 0; i < MeshBuffers.size(); ++i)
			MeshBuffers[i]->drop();
		MeshBuffers.clear();
		BoundingBox.reset(0.f, 0.f, 0.f);
	}

	//! returns amount of mesh buffers.
	u32 getMeshBufferCount() const override
	{
		return MeshBuffers.size();
	}

	//! returns pointer to a mesh buffer
	IMeshBuffer *getMeshBuffer(u32 nr) const override
	{
		return MeshBuffers[nr];
	}

	//! returns a meshbuffer which fits a material
	/** reverse search */
	IMeshBuffer *getMeshBuffer(const video::SMaterial &material) const override
	{
		for (s32 i = (s32)MeshBuffers.size() - 1; i >= 0; --i) {
			if (material == MeshBuffers[i]->getMaterial())
				return MeshBuffers[i];
		}

		return 0;
	}

	//! returns an axis aligned bounding box
	const core::aabbox3d<f32> &getBoundingBox() const override
	{
		return BoundingBox;
	}

	//! set user axis aligned bounding box
	void setBoundingBox(const core::aabbox3df &box) override
	{
		BoundingBox = box;
	}

	//! recalculates the bounding box
	void recalculateBoundingBox()
	{
		bool hasMeshBufferBBox = false;
		for (u32 i = 0; i < MeshBuffers.size(); ++i) {
			const core::aabbox3df &bb = MeshBuffers[i]->getBoundingBox();
			if (!bb.isEmpty()) {
				if (!hasMeshBufferBBox) {
					hasMeshBufferBBox = true;
					BoundingBox = bb;
				} else {
					BoundingBox.addInternalBox(bb);
				}
			}
		}

		if (!hasMeshBufferBBox)
			BoundingBox.reset(0.0f, 0.0f, 0.0f);
	}

	//! adds a MeshBuffer
	/** The bounding box is not updated automatically. */
	void addMeshBuffer(IMeshBuffer *buf)
	{
		if (buf) {
			buf->grab();
			MeshBuffers.push_back(buf);
		}
	}

	//! set the hardware mapping hint, for driver
	void setHardwareMappingHint(E_HARDWARE_MAPPING newMappingHint, E_BUFFER_TYPE buffer = EBT_VERTEX_AND_INDEX) override
	{
		for (u32 i = 0; i < MeshBuffers.size(); ++i)
			MeshBuffers[i]->setHardwareMappingHint(newMappingHint, buffer);
	}

	//! flags the meshbuffer as changed, reloads hardware buffers
	void setDirty(E_BUFFER_TYPE buffer = EBT_VERTEX_AND_INDEX) override
	{
		for (u32 i = 0; i < MeshBuffers.size(); ++i)
			MeshBuffers[i]->setDirty(buffer);
	}

	//! The meshbuffers of this mesh
	core::array<IMeshBuffer *> MeshBuffers;

	//! The bounding box of this mesh
	core::aabbox3d<f32> BoundingBox;
};

} // end namespace scene
} // end namespace irr

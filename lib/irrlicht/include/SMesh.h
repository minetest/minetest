// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __S_MESH_H_INCLUDED__
#define __S_MESH_H_INCLUDED__

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
			for (u32 i=0; i<MeshBuffers.size(); ++i)
				MeshBuffers[i]->drop();
		}

		//! clean mesh
		virtual void clear()
		{
			for (u32 i=0; i<MeshBuffers.size(); ++i)
				MeshBuffers[i]->drop();
			MeshBuffers.clear();
			BoundingBox.reset ( 0.f, 0.f, 0.f );
		}


		//! returns amount of mesh buffers.
		virtual u32 getMeshBufferCount() const
		{
			return MeshBuffers.size();
		}

		//! returns pointer to a mesh buffer
		virtual IMeshBuffer* getMeshBuffer(u32 nr) const
		{
			return MeshBuffers[nr];
		}

		//! returns a meshbuffer which fits a material
		/** reverse search */
		virtual IMeshBuffer* getMeshBuffer( const video::SMaterial & material) const
		{
			for (s32 i = (s32)MeshBuffers.size()-1; i >= 0; --i)
			{
				if ( material == MeshBuffers[i]->getMaterial())
					return MeshBuffers[i];
			}

			return 0;
		}

		//! returns an axis aligned bounding box
		virtual const core::aabbox3d<f32>& getBoundingBox() const
		{
			return BoundingBox;
		}

		//! set user axis aligned bounding box
		virtual void setBoundingBox( const core::aabbox3df& box)
		{
			BoundingBox = box;
		}

		//! recalculates the bounding box
		void recalculateBoundingBox()
		{
			if (MeshBuffers.size())
			{
				BoundingBox = MeshBuffers[0]->getBoundingBox();
				for (u32 i=1; i<MeshBuffers.size(); ++i)
					BoundingBox.addInternalBox(MeshBuffers[i]->getBoundingBox());
			}
			else
				BoundingBox.reset(0.0f, 0.0f, 0.0f);
		}

		//! adds a MeshBuffer
		/** The bounding box is not updated automatically. */
		void addMeshBuffer(IMeshBuffer* buf)
		{
			if (buf)
			{
				buf->grab();
				MeshBuffers.push_back(buf);
			}
		}

		//! sets a flag of all contained materials to a new value
		virtual void setMaterialFlag(video::E_MATERIAL_FLAG flag, bool newvalue)
		{
			for (u32 i=0; i<MeshBuffers.size(); ++i)
				MeshBuffers[i]->getMaterial().setFlag(flag, newvalue);
		}

		//! set the hardware mapping hint, for driver
		virtual void setHardwareMappingHint( E_HARDWARE_MAPPING newMappingHint, E_BUFFER_TYPE buffer=EBT_VERTEX_AND_INDEX )
		{
			for (u32 i=0; i<MeshBuffers.size(); ++i)
				MeshBuffers[i]->setHardwareMappingHint(newMappingHint, buffer);
		}

		//! flags the meshbuffer as changed, reloads hardware buffers
		virtual void setDirty(E_BUFFER_TYPE buffer=EBT_VERTEX_AND_INDEX)
		{
			for (u32 i=0; i<MeshBuffers.size(); ++i)
				MeshBuffers[i]->setDirty(buffer);
		}

		//! The meshbuffers of this mesh
		core::array<IMeshBuffer*> MeshBuffers;

		//! The bounding box of this mesh
		core::aabbox3d<f32> BoundingBox;
	};


} // end namespace scene
} // end namespace irr

#endif


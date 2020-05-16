// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_ANIMATED_MESH_MD3_H_INCLUDED__
#define __C_ANIMATED_MESH_MD3_H_INCLUDED__

#include "IAnimatedMeshMD3.h"
#include "IReadFile.h"
#include "IFileSystem.h"
#include "irrArray.h"
#include "irrString.h"
#include "SMesh.h"
#include "SMeshBuffer.h"
#include "IQ3Shader.h"

namespace irr
{
namespace scene
{

	class CAnimatedMeshMD3 : public IAnimatedMeshMD3
	{
	public:

		//! constructor
		CAnimatedMeshMD3();

		//! destructor
		virtual ~CAnimatedMeshMD3();

		//! loads a quake3 md3 file
		virtual bool loadModelFile(u32 modelIndex, io::IReadFile* file, 
				io::IFileSystem* fs, video::IVideoDriver* driver);

		// IAnimatedMeshMD3
		virtual void setInterpolationShift(u32 shift, u32 loopMode);
		virtual SMD3Mesh* getOriginalMesh();
		virtual SMD3QuaternionTagList* getTagList(s32 frame, s32 detailLevel, s32 startFrameLoop, s32 endFrameLoop);

		//IAnimatedMesh
		virtual u32 getFrameCount() const;

		//! Gets the default animation speed of the animated mesh.
		/** \return Amount of frames per second. If the amount is 0, it is a static, non animated mesh. */
		virtual f32 getAnimationSpeed() const
		{
			return FramesPerSecond;
		}

		//! Gets the frame count of the animated mesh.
		/** \param fps Frames per second to play the animation with. If the amount is 0, it is not animated.
		The actual speed is set in the scene node the mesh is instantiated in.*/
		virtual void setAnimationSpeed(f32 fps)
		{
			FramesPerSecond=fps;
		}

		virtual IMesh* getMesh(s32 frame, s32 detailLevel,
				s32 startFrameLoop, s32 endFrameLoop);
		virtual const core::aabbox3d<f32>& getBoundingBox() const;
		virtual E_ANIMATED_MESH_TYPE getMeshType() const;

		//! returns amount of mesh buffers.
		virtual u32 getMeshBufferCount() const;

		//! returns pointer to a mesh buffer
		virtual IMeshBuffer* getMeshBuffer(u32 nr) const;

		//! Returns pointer to a mesh buffer which fits a material
		virtual IMeshBuffer* getMeshBuffer(const video::SMaterial &material) const;

		virtual void setMaterialFlag(video::E_MATERIAL_FLAG flag, bool newvalue);

		//! set user axis aligned bounding box
		virtual void setBoundingBox(const core::aabbox3df& box);

		//! set the hardware mapping hint, for driver
		virtual void setHardwareMappingHint(E_HARDWARE_MAPPING newMappingHint, E_BUFFER_TYPE buffer=EBT_VERTEX_AND_INDEX);

		//! flags the meshbuffer as changed, reloads hardware buffers
		virtual void setDirty(E_BUFFER_TYPE buffer=EBT_VERTEX_AND_INDEX);

	private:
		//! animates one frame
		inline void Animate(u32 frame);

		video::SMaterial Material;

		//! hold original compressed MD3 Info
		SMD3Mesh *Mesh;

		u32 IPolShift;
		u32 LoopMode;
		f32 Scaling;

		//! Cache Info
		struct SCacheInfo
		{
			SCacheInfo(s32 frame=-1, s32 start=-1, s32 end=-1 ) :
					Frame(frame), startFrameLoop(start),
					endFrameLoop(end)
			{}

			bool operator == ( const SCacheInfo &other ) const
			{
				return 0 == memcmp ( this, &other, sizeof ( SCacheInfo ) );
			}
			s32 Frame;
			s32 startFrameLoop;
			s32 endFrameLoop;
		};
		SCacheInfo Current;

		//! return a Mesh per frame
		SMesh* MeshIPol;
		SMD3QuaternionTagList TagListIPol;

		IMeshBuffer* createMeshBuffer(const SMD3MeshBuffer* source,
				io::IFileSystem* fs, video::IVideoDriver* driver);

		void buildVertexArray(u32 frameA, u32 frameB, f32 interpolate,
					const SMD3MeshBuffer* source,
					SMeshBufferLightMap* dest);

		void buildTagArray(u32 frameA, u32 frameB, f32 interpolate);
		f32 FramesPerSecond;
	};


} // end namespace scene
} // end namespace irr

#endif


// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

// The code for the TerrainSceneNode is based on the terrain renderer by
// Soconne and the GeoMipMapSceneNode developed by Spintz. They made their
// code available for Irrlicht and allowed it to be distributed under this
// licence. I only modified some parts. A lot of thanks go to them.

#ifndef __I_TERRAIN_SCENE_NODE_H__
#define __I_TERRAIN_SCENE_NODE_H__

#include "ETerrainElements.h"
#include "ISceneNode.h"
#include "IDynamicMeshBuffer.h"
#include "irrArray.h"

namespace irr
{
namespace io
{
	class IReadFile;
} // end namespace io
namespace scene
{
	class IMesh;

	//! A scene node for displaying terrain using the geo mip map algorithm.
	/** The code for the TerrainSceneNode is based on the Terrain renderer by Soconne and
	 * the GeoMipMapSceneNode developed by Spintz. They made their code available for Irrlicht
	 * and allowed it to be distributed under this licence. I only modified some parts.
	 * A lot of thanks go to them.
	 *
	 * This scene node is capable of very quickly loading
	 * terrains and updating the indices at runtime to enable viewing very large terrains. It uses a
	 * CLOD (Continuous Level of Detail) algorithm which updates the indices for each patch based on
	 * a LOD (Level of Detail) which is determined based on a patch's distance from the camera.
	 *
	 * The Patch Size of the terrain must always be a size of ( 2^N+1, i.e. 8+1(9), 16+1(17), etc. ).
	 * The MaxLOD available is directly dependent on the patch size of the terrain. LOD 0 contains all
	 * of the indices to draw all the triangles at the max detail for a patch. As each LOD goes up by 1
	 * the step taken, in generating indices increases by - 2^LOD, so for LOD 1, the step taken is 2, for
	 * LOD 2, the step taken is 4, LOD 3 - 8, etc. The step can be no larger than the size of the patch,
	 * so having a LOD of 8, with a patch size of 17, is asking the algoritm to generate indices every
	 * 2^8 ( 256 ) vertices, which is not possible with a patch size of 17. The maximum LOD for a patch
	 * size of 17 is 2^4 ( 16 ). So, with a MaxLOD of 5, you'll have LOD 0 ( full detail ), LOD 1 ( every
	 * 2 vertices ), LOD 2 ( every 4 vertices ), LOD 3 ( every 8 vertices ) and LOD 4 ( every 16 vertices ).
	 **/
	class ITerrainSceneNode : public ISceneNode
	{
	public:
		//! Constructor
		ITerrainSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
			const core::vector3df& position = core::vector3df(0.0f, 0.0f, 0.0f),
			const core::vector3df& rotation = core::vector3df(0.0f, 0.0f, 0.0f),
			const core::vector3df& scale = core::vector3df(1.0f, 1.0f, 1.0f) )
			: ISceneNode (parent, mgr, id, position, rotation, scale) {}

		//! Get the bounding box of the terrain.
		/** \return The bounding box of the entire terrain. */
		virtual const core::aabbox3d<f32>& getBoundingBox() const =0;

		//! Get the bounding box of a patch
		/** \return The bounding box of the chosen patch. */
		virtual const core::aabbox3d<f32>& getBoundingBox(s32 patchX, s32 patchZ) const =0;

		//! Get the number of indices currently in the meshbuffer
		/** \return The index count. */
		virtual u32 getIndexCount() const =0;

		//! Get pointer to the mesh
		/** \return Pointer to the mesh. */
		virtual IMesh* getMesh() =0;

		//! Get pointer to the buffer used by the terrain (most users will not need this)
		virtual IMeshBuffer* getRenderBuffer() =0;


		//! Gets the meshbuffer data based on a specified level of detail.
		/** \param mb A reference to an IDynamicMeshBuffer object
		\param LOD The level of detail you want the indices from. */
		virtual void getMeshBufferForLOD(IDynamicMeshBuffer& mb, s32 LOD=0) const =0;

		//! Gets the indices for a specified patch at a specified Level of Detail.
		/** \param indices A reference to an array of u32 indices.
		\param patchX Patch x coordinate.
		\param patchZ Patch z coordinate.
		\param LOD The level of detail to get for that patch. If -1,
		then get the CurrentLOD. If the CurrentLOD is set to -1,
		meaning it's not shown, then it will retrieve the triangles at
		the highest LOD (0).
		\return Number of indices put into the buffer. */
		virtual s32 getIndicesForPatch(core::array<u32>& indices,
			s32 patchX, s32 patchZ, s32 LOD=0) =0;

		//! Populates an array with the CurrentLOD of each patch.
		/** \param LODs A reference to a core::array<s32> to hold the
		values
		\return Number of elements in the array */
		virtual s32 getCurrentLODOfPatches(core::array<s32>& LODs) const =0;

		//! Manually sets the LOD of a patch
		/** \param patchX Patch x coordinate.
		\param patchZ Patch z coordinate.
		\param LOD The level of detail to set the patch to. */
		virtual void setLODOfPatch(s32 patchX, s32 patchZ, s32 LOD=0) =0;

		//! Get center of terrain.
		virtual const core::vector3df& getTerrainCenter() const =0;

		//! Get height of a point of the terrain.
		virtual f32 getHeight(f32 x, f32 y) const =0;

		//! Sets the movement camera threshold.
		/** It is used to determine when to recalculate
		indices for the scene node. The default value is 10.0f. */
		virtual void setCameraMovementDelta(f32 delta) =0;

		//! Sets the rotation camera threshold.
		/** It is used to determine when to recalculate
		indices for the scene node. The default value is 1.0f. */
		virtual void setCameraRotationDelta(f32 delta) =0;

		//! Sets whether or not the node should dynamically update its associated selector when the geomipmap data changes.
		/** \param bVal: Boolean value representing whether or not to update selector dynamically. */
		virtual void setDynamicSelectorUpdate(bool bVal) =0;

		//! Override the default generation of distance thresholds.
		/** For determining the LOD a patch is rendered at. If any LOD
		is overridden, then the scene node will no longer apply scaling
		factors to these values. If you override these distances, and
		then apply a scale to the scene node, it is your responsibility
		to update the new distances to work best with your new terrain
		size. */
		virtual bool overrideLODDistance(s32 LOD, f64 newDistance) =0;

		//! Scales the base texture, similar to makePlanarTextureMapping.
		/** \param scale The scaling amount. Values above 1.0
		increase the number of time the texture is drawn on the
		terrain. Values below 0 will decrease the number of times the
		texture is drawn on the terrain. Using negative values will
		flip the texture, as well as still scaling it.
		\param scale2 If set to 0 (default value), this will set the
		second texture coordinate set to the same values as in the
		first set. If this is another value than zero, it will scale
		the second texture coordinate set by this value. */
		virtual void scaleTexture(f32 scale = 1.0f, f32 scale2=0.0f) =0;

		//! Initializes the terrain data.  Loads the vertices from the heightMapFile.
		/** The file must contain a loadable image of the heightmap. The heightmap
		must be square.
		\param file The file to read the image from. File is not rewinded.
		\param vertexColor Color of all vertices.
		\param smoothFactor Number of smoothing passes. */
		virtual bool loadHeightMap(io::IReadFile* file, 
			video::SColor vertexColor=video::SColor(255,255,255,255),
			s32 smoothFactor=0) =0;

		//! Initializes the terrain data.  Loads the vertices from the heightMapFile.
		/** The data is interpreted as (signed) integers of the given bit size or
		floats (with 32bits, signed). Allowed bitsizes for integers are
		8, 16, and 32. The heightmap must be square.
		\param file The file to read the RAW data from. File is not rewinded.
		\param bitsPerPixel Size of data if integers used, for floats always use 32.
		\param signedData Whether we use signed or unsigned ints, ignored for floats.
		\param floatVals Whether the data is float or int.
		\param width Width (and also Height, as it must be square) of the heightmap. Use 0 for autocalculating from the filesize.
		\param vertexColor Color of all vertices.
		\param smoothFactor Number of smoothing passes. */
		virtual bool loadHeightMapRAW(io::IReadFile* file, s32 bitsPerPixel=16,
			bool signedData=false, bool floatVals=false, s32 width=0,
			video::SColor vertexColor=video::SColor(255,255,255,255),
			s32 smoothFactor=0) =0;

	};

} // end namespace scene
} // end namespace irr


#endif // __I_TERRAIN_SCENE_NODE_H__


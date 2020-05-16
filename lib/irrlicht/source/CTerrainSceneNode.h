// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

// The code for the TerrainSceneNode is based on the GeoMipMapSceneNode
// developed by Spintz. He made it available for Irrlicht and allowed it to be
// distributed under this licence. I only modified some parts. A lot of thanks go to him.

#ifndef __C_TERRAIN_SCENE_NODE_H__
#define __C_TERRAIN_SCENE_NODE_H__

#include "ITerrainSceneNode.h"
#include "IDynamicMeshBuffer.h"
#include "path.h"

namespace irr
{
namespace io
{
	class IFileSystem;
	class IReadFile;
}
namespace scene
{
	struct SMesh;
	class ITextSceneNode;

	//! A scene node for displaying terrain using the geo mip map algorithm.
	class CTerrainSceneNode : public ITerrainSceneNode
	{
	public:

		//! constructor
		//! \param parent: The node which this node is a child of.  Making this node a child of another node, or
		//! making it a parent of another node is yet untested and most likely does not work properly.
		//! \param mgr: Pointer to the scene manager.
		//! \param id: The id of the node
		//! \param maxLOD: The maximum LOD ( Level of Detail ) for the node.
		//! \param patchSize: An E_GEOMIPMAP_PATCH_SIZE enumeration defining the size of each patch of the terrain.
		//! \param position: The absolute position of this node.
		//! \param rotation: The absolute rotation of this node. ( NOT YET IMPLEMENTED )
		//! \param scale: The scale factor for the terrain.  If you're using a heightmap of size 128x128 and would like
		//! your terrain to be 12800x12800 in game units, then use a scale factor of ( core::vector ( 100.0f, 100.0f, 100.0f ).
		//! If you use a Y scaling factor of 0.0f, then your terrain will be flat.
		CTerrainSceneNode(ISceneNode* parent, ISceneManager* mgr, io::IFileSystem* fs, s32 id,
			s32 maxLOD = 4, E_TERRAIN_PATCH_SIZE patchSize = ETPS_17,
			const core::vector3df& position = core::vector3df(0.0f, 0.0f, 0.0f),
			const core::vector3df& rotation = core::vector3df(0.0f, 0.0f, 0.0f),
			const core::vector3df& scale = core::vector3df(1.0f, 1.0f, 1.0f));

		virtual ~CTerrainSceneNode();

		//! Initializes the terrain data.  Loads the vertices from the heightMapFile.
		virtual bool loadHeightMap(io::IReadFile* file,
			video::SColor vertexColor = video::SColor ( 255, 255, 255, 255 ), s32 smoothFactor = 0 );

		//! Initializes the terrain data.  Loads the vertices from the heightMapFile.
		virtual bool loadHeightMapRAW(io::IReadFile* file, s32 bitsPerPixel = 16,
			bool signedData=true, bool floatVals=false, s32 width=0, video::SColor vertexColor = video::SColor ( 255, 255, 255, 255 ), s32 smoothFactor = 0 );

		//! Returns the material based on the zero based index i. This scene node only uses
		//! 1 material.
		//! \param i: Zero based index i. UNUSED, left in for virtual purposes.
		//! \return Returns the single material this scene node uses.
		virtual video::SMaterial& getMaterial(u32 i);

		//! Returns amount of materials used by this scene node ( always 1 )
		//! \return Returns current count of materials used by this scene node ( always 1 )
		virtual u32 getMaterialCount() const;

		//! Gets the last scaling factor applied to the scene node.  This value only represents the
		//! last scaling factor presented to the node.  For instance, if you make create the node
		//! with a scale factor of ( 1.0f, 1.0f, 1.0f ) then call setScale ( 50.0f, 5.0f, 50.0f ),
		//! then make another call to setScale with the values ( 2.0f, 2.0f, 2.0f ), this will return
		//! core::vector3df ( 2.0f, 2.0f, 2.0f ), although the total scaling of the scene node is
		//! core::vector3df ( 100.0f, 10.0f, 100.0f ).
		//! \return Returns the last scaling factor passed to the scene node.
		virtual const core::vector3df& getScale() const
		{
			return TerrainData.Scale;
		}

		//! Scales the scene nodes vertices by the vector specified.
		//! \param scale: Scaling factor to apply to the node.
		virtual void setScale(const core::vector3df& scale);

		//! Gets the last rotation factor applied to the scene node.
		//! \return Returns the last rotation factor applied to the scene node.
		virtual const core::vector3df& getRotation() const
		{
			return TerrainData.Rotation;
		}

		//! Rotates the node. This only modifies the relative rotation of the node.
		//! \param rotation: New rotation of the node in degrees.
		virtual void setRotation(const core::vector3df& rotation);

		//! Sets the pivot point for rotation of this node.
		//! NOTE: The default for the RotationPivot will be the center of the individual tile.
		virtual void setRotationPivot( const core::vector3df& pivot );

		//! Gets the last positioning vector applied to the scene node.
		//! \return Returns the last position vector applied to the scene node.
		virtual const core::vector3df& getPosition() const
		{
			return TerrainData.Position;
		}

		//! Moves the scene nodes vertices by the vector specified.
		//! \param newpos: Vector specifying how much to move each vertex of the scene node.
		virtual void setPosition(const core::vector3df& newpos);

		//! Updates the scene nodes indices if the camera has moved or rotated by a certain
		//! threshold, which can be changed using the SetCameraMovementDeltaThreshold and
		//! SetCameraRotationDeltaThreshold functions.  This also determines if a given patch
		//! for the scene node is within the view frustum and if it's not the indices are not
		//! generated for that patch.
		virtual void OnRegisterSceneNode();

		//! Render the scene node
		virtual void render();

		//! Return the bounding box of the entire terrain.
		virtual const core::aabbox3d<f32>& getBoundingBox() const;

		//! Return the bounding box of a patch
		virtual const core::aabbox3d<f32>& getBoundingBox(s32 patchX, s32 patchZ) const;

		//! Return the number of indices currently used to draw the scene node.
		virtual u32 getIndexCount() const { return IndicesToRender; }

		//! Returns the mesh
		virtual IMesh* getMesh();

		//! Returns a pointer to the buffer used by the terrain (most users will not need this)
		virtual IMeshBuffer* getRenderBuffer() { return RenderBuffer; }

		//! Gets the meshbuffer data based on a specified Level of Detail.
		//! \param mb: A reference to an IDynamicMeshBuffer object
		//! \param LOD: The Level Of Detail you want the indices from.
		virtual void getMeshBufferForLOD(IDynamicMeshBuffer& mb, s32 LOD=0) const;

		//! Gets the indices for a specified patch at a specified Level of Detail.
		//! \param indices: A reference to an array of u32 indices.
		//! \param patchX: Patch x coordinate.
		//! \param patchZ: Patch z coordinate.
		//! \param LOD: The level of detail to get for that patch.  If -1, then get
		//! the CurrentLOD.  If the CurrentLOD is set to -1, meaning it's not shown,
		//! then it will retrieve the triangles at the highest LOD (0).
		//! \return: Number of indices put into the buffer.
		virtual s32 getIndicesForPatch(core::array<u32>& indices,
			s32 patchX, s32 patchZ, s32 LOD=0);

		//! Populates an array with the CurrentLOD of each patch.
		//! \param LODs: A reference to a core::array<s32> to hold the values
		//! \return Returns the number of elements in the array
		virtual s32 getCurrentLODOfPatches(core::array<s32>& LODs) const;

		//! Manually sets the LOD of a patch
		//! \param patchX: Patch x coordinate.
		//! \param patchZ: Patch z coordinate.
		//! \param LOD: The level of detail to set the patch to.
		virtual void setLODOfPatch(s32 patchX, s32 patchZ, s32 LOD=0);

		//! Returns center of terrain.
		virtual const core::vector3df& getTerrainCenter() const
		{
			return TerrainData.Center;
		}

		//! Returns center of terrain.
		virtual f32 getHeight( f32 x, f32 y ) const;

		//! Sets the movement camera threshold which is used to determine when to recalculate
		//! indices for the scene node.  The default value is 10.0f.
		virtual void setCameraMovementDelta(f32 delta)
		{
			CameraMovementDelta = delta;
		}

		//! Sets the rotation camera threshold which is used to determine when to recalculate
		//! indices for the scene node.  The default value is 1.0f.
		virtual void setCameraRotationDelta(f32 delta)
		{
			CameraRotationDelta = delta;
		}

		//! Sets whether or not the node should dynamically update it its associated selector when
		//! the geomipmap data changes.
		//! param bVal: Boolean value representing whether or not to update selector dynamically.
		//! NOTE: Temporarily disabled while working out issues with DynamicSelectorUpdate
		virtual void setDynamicSelectorUpdate(bool bVal ) { DynamicSelectorUpdate = false; }

		//! Override the default generation of distance thresholds for determining the LOD a patch
		//! is rendered at. If any LOD is overridden, then the scene node will no longer apply
		//! scaling factors to these values. If you override these distances and then apply
		//! a scale to the scene node, it is your responsibility to update the new distances to
		//! work best with your new terrain size.
		virtual bool overrideLODDistance( s32 LOD, f64 newDistance );

		//! Scales the two textures
		virtual void scaleTexture(f32 scale = 1.0f, f32 scale2 = 0.0f);

		//! Returns type of the scene node
		virtual ESCENE_NODE_TYPE getType() const {return ESNT_TERRAIN;}

		//! Writes attributes of the scene node.
		virtual void serializeAttributes(io::IAttributes* out,
				io::SAttributeReadWriteOptions* options=0) const;

		//! Reads attributes of the scene node.
		virtual void deserializeAttributes(io::IAttributes* in,
				io::SAttributeReadWriteOptions* options=0);

		//! Creates a clone of this scene node and its children.
		virtual ISceneNode* clone(ISceneNode* newParent,
				ISceneManager* newManager);

	private:
		friend class CTerrainTriangleSelector;

		struct SPatch
		{
			SPatch()
			: Top(0), Bottom(0), Right(0), Left(0), CurrentLOD(-1)
			{
			}

			SPatch* Top;
			SPatch* Bottom;
			SPatch* Right;
			SPatch* Left;
			s32 CurrentLOD;
			core::aabbox3df BoundingBox;
			core::vector3df Center;
		};

		struct STerrainData
		{
			STerrainData(s32 patchSize, s32 maxLOD, const core::vector3df& position, const core::vector3df& rotation, const core::vector3df& scale)
			: Patches(0), Size(0), Position(position), Rotation(rotation),
				Scale(scale), PatchSize(patchSize), CalcPatchSize(patchSize-1),
				PatchCount(0), MaxLOD(maxLOD)
			{
			}

			SPatch*		Patches;
			s32		Size;
			core::vector3df	Position;
			core::vector3df	Rotation;
			core::vector3df RotationPivot;
			core::vector3df	Scale;
			core::vector3df Center;
			s32		PatchSize;
			s32		CalcPatchSize;
			s32		PatchCount;
			s32		MaxLOD;
			core::aabbox3df	BoundingBox;
			core::array<f64> LODDistanceThreshold;
		};
    
    
    virtual void preRenderCalculationsIfNeeded();
    
		virtual void preRenderLODCalculations();
		virtual void preRenderIndicesCalculations();

		//! get indices when generating index data for patches at varying levels of detail.
		u32 getIndex(const s32 PatchX, const s32 PatchZ, const s32 PatchIndex, u32 vX, u32 vZ) const;

		//! smooth the terrain
		void smoothTerrain(IDynamicMeshBuffer* mb, s32 smoothFactor);

		//! calculate smooth normals
		void calculateNormals(IDynamicMeshBuffer* mb);

		//! create patches, stuff that needs to only be done once for patches goes here.
		void createPatches();

		//! calculate the internal STerrainData structure
		void calculatePatchData();

		//! calculate or recalculate the distance thresholds
		void calculateDistanceThresholds(bool scalechanged = false);

		//! sets the CurrentLOD of all patches to the specified LOD
		void setCurrentLODOfPatches(s32 i);

		//! sets the CurrentLOD of TerrainData patches to the LODs specified in the array
		void setCurrentLODOfPatches(const core::array<s32>& lodarray);

		//! Apply transformation changes( scale, position, rotation )
		void applyTransformation();

		STerrainData TerrainData;
		SMesh* Mesh;

		IDynamicMeshBuffer *RenderBuffer;

		u32 VerticesToRender;
		u32 IndicesToRender;

		bool DynamicSelectorUpdate;
		bool OverrideDistanceThreshold;
		bool UseDefaultRotationPivot;
		bool ForceRecalculation;

		core::vector3df	OldCameraPosition;
		core::vector3df	OldCameraRotation;
		core::vector3df	OldCameraUp;
		f32             OldCameraFOV;
		f32 CameraMovementDelta;
		f32 CameraRotationDelta;
		f32 CameraFOVDelta;

		// needed for (de)serialization
		f32 TCoordScale1;
		f32 TCoordScale2;
		s32 SmoothFactor;
		io::path HeightmapFile;
		io::IFileSystem* FileSystem;
	};


} // end namespace scene
} // end namespace irr

#endif // __C_TERRAIN_SCENE_NODE_H__



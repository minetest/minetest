// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

// The code for the TerrainSceneNode is based on the GeoMipMapSceneNode
// developed by Spintz. He made it available for Irrlicht and allowed it to be
// distributed under this licence. I only modified some parts. A lot of thanks
// go to him.

#include "CTerrainSceneNode.h"
#include "CTerrainTriangleSelector.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"
#include "SViewFrustum.h"
#include "irrMath.h"
#include "os.h"
#include "IGUIFont.h"
#include "IFileSystem.h"
#include "IReadFile.h"
#include "ITextSceneNode.h"
#include "IAnimatedMesh.h"
#include "SMesh.h"
#include "CDynamicMeshBuffer.h"

namespace irr
{
namespace scene
{

	//! constructor
	CTerrainSceneNode::CTerrainSceneNode(ISceneNode* parent, ISceneManager* mgr,
			io::IFileSystem* fs, s32 id, s32 maxLOD, E_TERRAIN_PATCH_SIZE patchSize,
			const core::vector3df& position,
			const core::vector3df& rotation,
			const core::vector3df& scale)
	: ITerrainSceneNode(parent, mgr, id, position, rotation, scale),
	TerrainData(patchSize, maxLOD, position, rotation, scale), RenderBuffer(0),
	VerticesToRender(0), IndicesToRender(0), DynamicSelectorUpdate(false),
	OverrideDistanceThreshold(false), UseDefaultRotationPivot(true), ForceRecalculation(true),
	CameraMovementDelta(10.0f), CameraRotationDelta(1.0f),CameraFOVDelta(0.1f),
	TCoordScale1(1.0f), TCoordScale2(1.0f), SmoothFactor(0), FileSystem(fs)
	{
		#ifdef _DEBUG
		setDebugName("CTerrainSceneNode");
		#endif

		Mesh = new SMesh();
		RenderBuffer = new CDynamicMeshBuffer(video::EVT_2TCOORDS, video::EIT_16BIT);
		RenderBuffer->setHardwareMappingHint(scene::EHM_STATIC, scene::EBT_VERTEX);
		RenderBuffer->setHardwareMappingHint(scene::EHM_DYNAMIC, scene::EBT_INDEX);

		if (FileSystem)
			FileSystem->grab();

		setAutomaticCulling(scene::EAC_OFF);
	}


	//! destructor
	CTerrainSceneNode::~CTerrainSceneNode()
	{
		delete [] TerrainData.Patches;

		if (FileSystem)
			FileSystem->drop();

		if (Mesh)
			Mesh->drop();

		if (RenderBuffer)
			RenderBuffer->drop();
	}


	//! Initializes the terrain data. Loads the vertices from the heightMapFile
	bool CTerrainSceneNode::loadHeightMap(io::IReadFile* file, video::SColor vertexColor,
			s32 smoothFactor)
	{
		if (!file)
			return false;

		Mesh->MeshBuffers.clear();
		const u32 startTime = os::Timer::getRealTime();
		video::IImage* heightMap = SceneManager->getVideoDriver()->createImageFromFile(file);

		if (!heightMap)
		{
			os::Printer::log("Unable to load heightmap.");
			return false;
		}

		HeightmapFile = file->getFileName();
		SmoothFactor = smoothFactor;

		// Get the dimension of the heightmap data
		TerrainData.Size = heightMap->getDimension().Width;

		switch (TerrainData.PatchSize)
		{
			case ETPS_9:
				if (TerrainData.MaxLOD > 3)
				{
					TerrainData.MaxLOD = 3;
				}
			break;
			case ETPS_17:
				if (TerrainData.MaxLOD > 4)
				{
					TerrainData.MaxLOD = 4;
				}
			break;
			case ETPS_33:
				if (TerrainData.MaxLOD > 5)
				{
					TerrainData.MaxLOD = 5;
				}
			break;
			case ETPS_65:
				if (TerrainData.MaxLOD > 6)
				{
					TerrainData.MaxLOD = 6;
				}
			break;
			case ETPS_129:
				if (TerrainData.MaxLOD > 7)
				{
					TerrainData.MaxLOD = 7;
				}
			break;
		}

		// --- Generate vertex data from heightmap ----
		// resize the vertex array for the mesh buffer one time (makes loading faster)
		scene::CDynamicMeshBuffer *mb=0;

		const u32 numVertices = TerrainData.Size * TerrainData.Size;
		if (numVertices <= 65536)
		{
			//small enough for 16bit buffers
			mb=new scene::CDynamicMeshBuffer(video::EVT_2TCOORDS, video::EIT_16BIT);
			RenderBuffer->getIndexBuffer().setType(video::EIT_16BIT);
		}
		else
		{
			//we need 32bit buffers
			mb=new scene::CDynamicMeshBuffer(video::EVT_2TCOORDS, video::EIT_32BIT);
			RenderBuffer->getIndexBuffer().setType(video::EIT_32BIT);
		}

		mb->getVertexBuffer().set_used(numVertices);

		// Read the heightmap to get the vertex data
		// Apply positions changes, scaling changes
		const f32 tdSize = 1.0f/(f32)(TerrainData.Size-1);
		s32 index = 0;
		float fx=0.f;
		float fx2=0.f;
		for (s32 x = 0; x < TerrainData.Size; ++x)
		{
			float fz=0.f;
			float fz2=0.f;
			for (s32 z = 0; z < TerrainData.Size; ++z)
			{
				video::S3DVertex2TCoords& vertex= static_cast<video::S3DVertex2TCoords*>(mb->getVertexBuffer().pointer())[index++];
				vertex.Normal.set(0.0f, 1.0f, 0.0f);
				vertex.Color = vertexColor;
				vertex.Pos.X = fx;
				vertex.Pos.Y = (f32) heightMap->getPixel(TerrainData.Size-x-1,z).getLightness();
				vertex.Pos.Z = fz;

				vertex.TCoords.X = vertex.TCoords2.X = 1.f-fx2;
				vertex.TCoords.Y = vertex.TCoords2.Y = fz2;

				++fz;
				fz2 += tdSize;
			}
			++fx;
			fx2 += tdSize;
		}

		// drop heightMap, no longer needed
		heightMap->drop();

		smoothTerrain(mb, smoothFactor);

		// calculate smooth normals for the vertices
		calculateNormals(mb);

		// add the MeshBuffer to the mesh
		Mesh->addMeshBuffer(mb);

		// We copy the data to the renderBuffer, after the normals have been calculated.
		RenderBuffer->getVertexBuffer().set_used(numVertices);

		for (u32 i = 0; i < numVertices; ++i)
		{
			RenderBuffer->getVertexBuffer()[i] = mb->getVertexBuffer()[i];
			RenderBuffer->getVertexBuffer()[i].Pos *= TerrainData.Scale;
			RenderBuffer->getVertexBuffer()[i].Pos += TerrainData.Position;
		}

		// We no longer need the mb
		mb->drop();

		// calculate all the necessary data for the patches and the terrain
		calculateDistanceThresholds();
		createPatches();
		calculatePatchData();

		// set the default rotation pivot point to the terrain nodes center
		TerrainData.RotationPivot = TerrainData.Center;

		// Rotate the vertices of the terrain by the rotation
		// specified. Must be done after calculating the terrain data,
		// so we know what the current center of the terrain is.
		setRotation(TerrainData.Rotation);

		// Pre-allocate memory for indices

		RenderBuffer->getIndexBuffer().set_used(
				TerrainData.PatchCount * TerrainData.PatchCount *
				TerrainData.CalcPatchSize * TerrainData.CalcPatchSize * 6);

		RenderBuffer->setDirty();

		const u32 endTime = os::Timer::getRealTime();

		c8 tmp[255];
		snprintf(tmp, 255, "Generated terrain data (%dx%d) in %.4f seconds",
			TerrainData.Size, TerrainData.Size, (endTime - startTime) / 1000.0f );
		os::Printer::log(tmp);

		return true;
	}


	//! Initializes the terrain data. Loads the vertices from the heightMapFile
	bool CTerrainSceneNode::loadHeightMapRAW(io::IReadFile* file,
			s32 bitsPerPixel, bool signedData, bool floatVals,
			s32 width, video::SColor vertexColor, s32 smoothFactor)
	{
		if (!file)
			return false;
		if (floatVals && bitsPerPixel != 32)
			return false;

		// start reading
		const u32 startTime = os::Timer::getTime();

		Mesh->MeshBuffers.clear();

		const s32 bytesPerPixel = bitsPerPixel / 8;

		// Get the dimension of the heightmap data
		const s32 filesize = file->getSize();
		if (!width)
			TerrainData.Size = core::floor32(sqrtf((f32)(filesize / bytesPerPixel)));
		else
		{
			if ((filesize-file->getPos())/bytesPerPixel>width*width)
			{
				os::Printer::log("Error reading heightmap RAW file", "File is too small.");
				return false;
			}
			TerrainData.Size = width;
		}

		switch (TerrainData.PatchSize)
		{
			case ETPS_9:
				if (TerrainData.MaxLOD > 3)
				{
					TerrainData.MaxLOD = 3;
				}
			break;
			case ETPS_17:
				if (TerrainData.MaxLOD > 4)
				{
					TerrainData.MaxLOD = 4;
				}
			break;
			case ETPS_33:
				if (TerrainData.MaxLOD > 5)
				{
					TerrainData.MaxLOD = 5;
				}
			break;
			case ETPS_65:
				if (TerrainData.MaxLOD > 6)
				{
					TerrainData.MaxLOD = 6;
				}
			break;
			case ETPS_129:
				if (TerrainData.MaxLOD > 7)
				{
					TerrainData.MaxLOD = 7;
				}
			break;
		}

		// --- Generate vertex data from heightmap ----
		// resize the vertex array for the mesh buffer one time (makes loading faster)
		scene::CDynamicMeshBuffer *mb=0;
		const u32 numVertices = TerrainData.Size * TerrainData.Size;
		if (numVertices <= 65536)
		{
			//small enough for 16bit buffers
			mb=new scene::CDynamicMeshBuffer(video::EVT_2TCOORDS, video::EIT_16BIT);
			RenderBuffer->getIndexBuffer().setType(video::EIT_16BIT);
		}
		else
		{
			//we need 32bit buffers
			mb=new scene::CDynamicMeshBuffer(video::EVT_2TCOORDS, video::EIT_32BIT);
			RenderBuffer->getIndexBuffer().setType(video::EIT_32BIT);
		}

		mb->getVertexBuffer().reallocate(numVertices);

		video::S3DVertex2TCoords vertex;
		vertex.Normal.set(0.0f, 1.0f, 0.0f);
		vertex.Color = vertexColor;

		// Read the heightmap to get the vertex data
		// Apply positions changes, scaling changes
		const f32 tdSize = 1.0f/(f32)(TerrainData.Size-1);
		float fx=0.f;
		float fx2=0.f;
		for (s32 x = 0; x < TerrainData.Size; ++x)
		{
			float fz=0.f;
			float fz2=0.f;
			for (s32 z = 0; z < TerrainData.Size; ++z)
			{
				bool failure=false;
				vertex.Pos.X = fx;
				if (floatVals)
				{
					if (file->read(&vertex.Pos.Y, bytesPerPixel) != bytesPerPixel)
						failure=true;
				}
				else if (signedData)
				{
					switch (bytesPerPixel)
					{
						case 1:
						{
							s8 val;
							if (file->read(&val, bytesPerPixel) != bytesPerPixel)
								failure=true;
							vertex.Pos.Y=val;
						}
						break;
						case 2:
						{
							s16 val;
							if (file->read(&val, bytesPerPixel) != bytesPerPixel)
								failure=true;
							vertex.Pos.Y=val/256.f;
						}
						break;
						case 4:
						{
							s32 val;
							if (file->read(&val, bytesPerPixel) != bytesPerPixel)
								failure=true;
							vertex.Pos.Y=val/16777216.f;
						}
						break;
					}
				}
				else
				{
					switch (bytesPerPixel)
					{
						case 1:
						{
							u8 val;
							if (file->read(&val, bytesPerPixel) != bytesPerPixel)
								failure=true;
							vertex.Pos.Y=val;
						}
						break;
						case 2:
						{
							u16 val;
							if (file->read(&val, bytesPerPixel) != bytesPerPixel)
								failure=true;
							vertex.Pos.Y=val/256.f;
						}
						break;
						case 4:
						{
							u32 val;
							if (file->read(&val, bytesPerPixel) != bytesPerPixel)
								failure=true;
							vertex.Pos.Y=val/16777216.f;
						}
						break;
					}
				}
				if (failure)
				{
					os::Printer::log("Error reading heightmap RAW file.");
					mb->drop();
					return false;
				}
				vertex.Pos.Z = fz;

				vertex.TCoords.X = vertex.TCoords2.X = 1.f-fx2;
				vertex.TCoords.Y = vertex.TCoords2.Y = fz2;

				mb->getVertexBuffer().push_back(vertex);
				++fz;
				fz2 += tdSize;
			}
			++fx;
			fx2 += tdSize;
		}

		smoothTerrain(mb, smoothFactor);

		// calculate smooth normals for the vertices
		calculateNormals(mb);

		// add the MeshBuffer to the mesh
		Mesh->addMeshBuffer(mb);
		const u32 vertexCount = mb->getVertexCount();

		// We copy the data to the renderBuffer, after the normals have been calculated.
		RenderBuffer->getVertexBuffer().set_used(vertexCount);

		for (u32 i = 0; i < vertexCount; i++)
		{
			RenderBuffer->getVertexBuffer()[i] = mb->getVertexBuffer()[i];
			RenderBuffer->getVertexBuffer()[i].Pos *= TerrainData.Scale;
			RenderBuffer->getVertexBuffer()[i].Pos += TerrainData.Position;
		}

		// We no longer need the mb
		mb->drop();

		// calculate all the necessary data for the patches and the terrain
		calculateDistanceThresholds();
		createPatches();
		calculatePatchData();

		// set the default rotation pivot point to the terrain nodes center
		TerrainData.RotationPivot = TerrainData.Center;

		// Rotate the vertices of the terrain by the rotation specified. Must be done
		// after calculating the terrain data, so we know what the current center of the
		// terrain is.
		setRotation(TerrainData.Rotation);

		// Pre-allocate memory for indices
		RenderBuffer->getIndexBuffer().set_used(
				TerrainData.PatchCount*TerrainData.PatchCount*
				TerrainData.CalcPatchSize*TerrainData.CalcPatchSize*6);

		const u32 endTime = os::Timer::getTime();

		c8 tmp[255];
		snprintf(tmp, 255, "Generated terrain data (%dx%d) in %.4f seconds",
			TerrainData.Size, TerrainData.Size, (endTime - startTime) / 1000.0f);
		os::Printer::log(tmp);

		return true;
	}


	//! Returns the mesh
	IMesh* CTerrainSceneNode::getMesh() { return Mesh; }


	//! Returns the material based on the zero based index i.
	video::SMaterial& CTerrainSceneNode::getMaterial(u32 i)
	{
		return Mesh->getMeshBuffer(i)->getMaterial();
	}


	//! Returns amount of materials used by this scene node ( always 1 )
	u32 CTerrainSceneNode::getMaterialCount() const
	{
		return Mesh->getMeshBufferCount();
	}


	//! Sets the scale of the scene node.
	//! \param scale: New scale of the node
	void CTerrainSceneNode::setScale(const core::vector3df& scale)
	{
		TerrainData.Scale = scale;
		applyTransformation();
		calculateNormals(RenderBuffer);
		ForceRecalculation = true;
	}


	//! Sets the rotation of the node. This only modifies
	//! the relative rotation of the node.
	//! \param rotation: New rotation of the node in degrees.
	void CTerrainSceneNode::setRotation(const core::vector3df& rotation)
	{
		TerrainData.Rotation = rotation;
		applyTransformation();
		ForceRecalculation = true;
	}


	//! Sets the pivot point for rotation of this node. This is useful for the TiledTerrainManager to
	//! rotate all terrain tiles around a global world point.
	//! NOTE: The default for the RotationPivot will be the center of the individual tile.
	void CTerrainSceneNode::setRotationPivot(const core::vector3df& pivot)
	{
		UseDefaultRotationPivot = false;
		TerrainData.RotationPivot = pivot;
	}


	//! Sets the position of the node.
	//! \param newpos: New postition of the scene node.
	void CTerrainSceneNode::setPosition(const core::vector3df& newpos)
	{
		TerrainData.Position = newpos;
		applyTransformation();
		ForceRecalculation = true;
	}


	//! Apply transformation changes(scale, position, rotation)
	void CTerrainSceneNode::applyTransformation()
	{
		if (!Mesh->getMeshBufferCount())
			return;

		core::matrix4 rotMatrix;
		rotMatrix.setRotationDegrees(TerrainData.Rotation);

		const s32 vtxCount = Mesh->getMeshBuffer(0)->getVertexCount();
		for (s32 i = 0; i < vtxCount; ++i)
		{
			RenderBuffer->getVertexBuffer()[i].Pos = Mesh->getMeshBuffer(0)->getPosition(i) * TerrainData.Scale + TerrainData.Position;

			RenderBuffer->getVertexBuffer()[i].Pos -= TerrainData.RotationPivot;
			rotMatrix.inverseRotateVect(RenderBuffer->getVertexBuffer()[i].Pos);
			RenderBuffer->getVertexBuffer()[i].Pos += TerrainData.RotationPivot;
		}

		calculateDistanceThresholds(true);
		calculatePatchData();

		RenderBuffer->setDirty(EBT_VERTEX);
	}


	//! Updates the scene nodes indices if the camera has moved or rotated by a certain
	//! threshold, which can be changed using the SetCameraMovementDeltaThreshold and
	//! SetCameraRotationDeltaThreshold functions. This also determines if a given patch
	//! for the scene node is within the view frustum and if it's not the indices are not
	//! generated for that patch.
	void CTerrainSceneNode::OnRegisterSceneNode()
	{
		if (!IsVisible || !SceneManager->getActiveCamera())
			return;

		SceneManager->registerNodeForRendering(this);

		preRenderCalculationsIfNeeded();

		// Do Not call ISceneNode::OnRegisterSceneNode(), this node should have no children (luke: is this comment still true, as ISceneNode::OnRegisterSceneNode() is called?)

		ISceneNode::OnRegisterSceneNode();
		ForceRecalculation = false;
	}

	void CTerrainSceneNode::preRenderCalculationsIfNeeded()
	{
		scene::ICameraSceneNode * camera = SceneManager->getActiveCamera();
		if (!camera)
			return;

		// Determine the camera rotation, based on the camera direction.
		const core::vector3df cameraPosition = camera->getAbsolutePosition();
		const core::vector3df cameraRotation = core::line3d<f32>(cameraPosition, camera->getTarget()).getVector().getHorizontalAngle();
		core::vector3df cameraUp = camera->getUpVector();
		cameraUp.normalize();
		const f32 CameraFOV = SceneManager->getActiveCamera()->getFOV();

		// Only check on the Camera's Y Rotation
		if (!ForceRecalculation)
		{
			if ((fabsf(cameraRotation.X - OldCameraRotation.X) < CameraRotationDelta) &&
				(fabsf(cameraRotation.Y - OldCameraRotation.Y) < CameraRotationDelta))
			{
				if ((fabs(cameraPosition.X - OldCameraPosition.X) < CameraMovementDelta) &&
					(fabs(cameraPosition.Y - OldCameraPosition.Y) < CameraMovementDelta) &&
					(fabs(cameraPosition.Z - OldCameraPosition.Z) < CameraMovementDelta))
				{
					if (fabs(CameraFOV-OldCameraFOV) < CameraFOVDelta &&
						cameraUp.dotProduct(OldCameraUp) > (1.f - (cos(core::DEGTORAD * CameraRotationDelta))))
					{
						return;
					}
				}
			}
		}

		//we need to redo calculations...

		OldCameraPosition = cameraPosition;
		OldCameraRotation = cameraRotation;
		OldCameraUp = cameraUp;
		OldCameraFOV = CameraFOV;

		preRenderLODCalculations();
		preRenderIndicesCalculations();
	}

	void CTerrainSceneNode::preRenderLODCalculations()
	{
		scene::ICameraSceneNode * camera = SceneManager->getActiveCamera();

		if (!camera)
			return;

		const core::vector3df cameraPosition = camera->getAbsolutePosition();

		const SViewFrustum* frustum = camera->getViewFrustum();

		// Determine each patches LOD based on distance from camera (and whether or not they are in
		// the view frustum).
		const s32 count = TerrainData.PatchCount * TerrainData.PatchCount;
		for (s32 j = 0; j < count; ++j)
		{
			if (frustum->getBoundingBox().intersectsWithBox(TerrainData.Patches[j].BoundingBox))
			{
				const f32 distance = cameraPosition.getDistanceFromSQ(TerrainData.Patches[j].Center);

				TerrainData.Patches[j].CurrentLOD = 0;
				for (s32 i = TerrainData.MaxLOD - 1; i>0; --i)
				{
					if (distance >= TerrainData.LODDistanceThreshold[i])
					{
						TerrainData.Patches[j].CurrentLOD = i;
						break;
					}
				}
			}
			else
			{
				TerrainData.Patches[j].CurrentLOD = -1;
			}
		}
	}


	void CTerrainSceneNode::preRenderIndicesCalculations()
	{
		scene::IIndexBuffer& indexBuffer = RenderBuffer->getIndexBuffer();
		IndicesToRender = 0;
		indexBuffer.set_used(0);

		s32 index = 0;
		// Then generate the indices for all patches that are visible.
		for (s32 i = 0; i < TerrainData.PatchCount; ++i)
		{
			for (s32 j = 0; j < TerrainData.PatchCount; ++j)
			{
				if (TerrainData.Patches[index].CurrentLOD >= 0)
				{
					s32 x = 0;
					s32 z = 0;

					// calculate the step we take this patch, based on the patches current LOD
					const s32 step = 1 << TerrainData.Patches[index].CurrentLOD;

					// Loop through patch and generate indices
					while (z < TerrainData.CalcPatchSize)
					{
						const s32 index11 = getIndex(j, i, index, x, z);
						const s32 index21 = getIndex(j, i, index, x + step, z);
						const s32 index12 = getIndex(j, i, index, x, z + step);
						const s32 index22 = getIndex(j, i, index, x + step, z + step);

						indexBuffer.push_back(index12);
						indexBuffer.push_back(index11);
						indexBuffer.push_back(index22);
						indexBuffer.push_back(index22);
						indexBuffer.push_back(index11);
						indexBuffer.push_back(index21);
						IndicesToRender+=6;

						// increment index position horizontally
						x += step;

						// we've hit an edge
						if (x >= TerrainData.CalcPatchSize)
						{
							x = 0;
							z += step;
						}
					}
				}
				++index;
			}
		}

		RenderBuffer->setDirty(EBT_INDEX);

		if (DynamicSelectorUpdate && TriangleSelector)
		{
			CTerrainTriangleSelector* selector = (CTerrainTriangleSelector*)TriangleSelector;
			selector->setTriangleData(this, -1);
		}
	}


	//! Render the scene node
	void CTerrainSceneNode::render()
	{
		if (!IsVisible || !SceneManager->getActiveCamera())
			return;

		if (!Mesh->getMeshBufferCount())
			return;

		video::IVideoDriver* driver = SceneManager->getVideoDriver();

		driver->setTransform (video::ETS_WORLD, core::IdentityMatrix);
		driver->setMaterial(Mesh->getMeshBuffer(0)->getMaterial());

		RenderBuffer->getIndexBuffer().set_used(IndicesToRender);

		// For use with geomorphing
		driver->drawMeshBuffer(RenderBuffer);

		RenderBuffer->getIndexBuffer().set_used(RenderBuffer->getIndexBuffer().allocated_size());

		// for debug purposes only:
		if (DebugDataVisible)
		{
			video::SMaterial m;
			m.Lighting = false;
			driver->setMaterial(m);
			if (DebugDataVisible & scene::EDS_BBOX)
				driver->draw3DBox(TerrainData.BoundingBox, video::SColor(255,255,255,255));

			const s32 count = TerrainData.PatchCount * TerrainData.PatchCount;
			s32 visible = 0;
			if (DebugDataVisible & scene::EDS_BBOX_BUFFERS)
			{
				for (s32 j = 0; j < count; ++j)
				{
					driver->draw3DBox(TerrainData.Patches[j].BoundingBox, video::SColor(255,255,0,0));
					visible += (TerrainData.Patches[j].CurrentLOD >= 0);
				}
			}

			if (DebugDataVisible & scene::EDS_NORMALS)
			{
				// draw normals
				const f32 debugNormalLength = SceneManager->getParameters()->getAttributeAsFloat(DEBUG_NORMAL_LENGTH);
				const video::SColor debugNormalColor = SceneManager->getParameters()->getAttributeAsColor(DEBUG_NORMAL_COLOR);
				driver->drawMeshBufferNormals(RenderBuffer, debugNormalLength, debugNormalColor);
			}

			driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);

			static u32 lastTime = 0;

			const u32 now = os::Timer::getRealTime();
			if (now - lastTime > 1000)
			{
				char buf[64];
				snprintf(buf, 64, "Count: %d, Visible: %d", count, visible);
				os::Printer::log(buf);

				lastTime = now;
			}
		}
	}


	//! Return the bounding box of the entire terrain.
	const core::aabbox3d<f32>& CTerrainSceneNode::getBoundingBox() const
	{
		return TerrainData.BoundingBox;
	}


	//! Return the bounding box of a patch
	const core::aabbox3d<f32>& CTerrainSceneNode::getBoundingBox(s32 patchX, s32 patchZ) const
	{
		return TerrainData.Patches[patchX * TerrainData.PatchCount + patchZ].BoundingBox;
	}


	//! Gets the meshbuffer data based on a specified Level of Detail.
	//! \param mb: A reference to an SMeshBuffer object
	//! \param LOD: The Level Of Detail you want the indices from.
	void CTerrainSceneNode::getMeshBufferForLOD(IDynamicMeshBuffer& mb, s32 LOD ) const
	{
		if (!Mesh->getMeshBufferCount())
			return;

		LOD = core::clamp(LOD, 0, TerrainData.MaxLOD - 1);

		const u32 numVertices = Mesh->getMeshBuffer(0)->getVertexCount();
		mb.getVertexBuffer().reallocate(numVertices);
		video::S3DVertex2TCoords* vertices = (video::S3DVertex2TCoords*)Mesh->getMeshBuffer(0)->getVertices();

		for (u32 n=0; n<numVertices; ++n)
			mb.getVertexBuffer().push_back(vertices[n]);

		mb.getIndexBuffer().setType(RenderBuffer->getIndexBuffer().getType());

		// calculate the step we take for all patches, since LOD is the same
		const s32 step = 1 << LOD;

		// Generate the indices for all patches at the specified LOD
		s32 index = 0;
		for (s32 i=0; i<TerrainData.PatchCount; ++i)
		{
			for (s32 j=0; j<TerrainData.PatchCount; ++j)
			{
				s32 x = 0;
				s32 z = 0;

				// Loop through patch and generate indices
				while (z < TerrainData.CalcPatchSize)
				{
					const s32 index11 = getIndex(j, i, index, x, z);
					const s32 index21 = getIndex(j, i, index, x + step, z);
					const s32 index12 = getIndex(j, i, index, x, z + step);
					const s32 index22 = getIndex(j, i, index, x + step, z + step);

					mb.getIndexBuffer().push_back(index12);
					mb.getIndexBuffer().push_back(index11);
					mb.getIndexBuffer().push_back(index22);
					mb.getIndexBuffer().push_back(index22);
					mb.getIndexBuffer().push_back(index11);
					mb.getIndexBuffer().push_back(index21);

					// increment index position horizontally
					x += step;

					if (x >= TerrainData.CalcPatchSize) // we've hit an edge
					{
						x = 0;
						z += step;
					}
				}
				++index;
			}
		}
	}


	//! Gets the indices for a specified patch at a specified Level of Detail.
	//! \param mb: A reference to an array of u32 indices.
	//! \param patchX: Patch x coordinate.
	//! \param patchZ: Patch z coordinate.
	//! \param LOD: The level of detail to get for that patch. If -1, then get
	//! the CurrentLOD. If the CurrentLOD is set to -1, meaning it's not shown,
	//! then it will retrieve the triangles at the highest LOD (0).
	//! \return: Number if indices put into the buffer.
	s32 CTerrainSceneNode::getIndicesForPatch(core::array<u32>& indices, s32 patchX, s32 patchZ, s32 LOD)
	{
		if (patchX < 0 || patchX > TerrainData.PatchCount-1 ||
				patchZ < 0 || patchZ > TerrainData.PatchCount-1)
			return -1;

		if (LOD < -1 || LOD > TerrainData.MaxLOD - 1)
			return -1;

		core::array<s32> cLODs;
		bool setLODs = false;

		// If LOD of -1 was passed in, use the CurrentLOD of the patch specified
		if (LOD == -1)
		{
			LOD = TerrainData.Patches[patchX * TerrainData.PatchCount + patchZ].CurrentLOD;
		}
		else
		{
			getCurrentLODOfPatches(cLODs);
			setCurrentLODOfPatches(LOD);
			setLODs = true;
		}

		if (LOD < 0)
			return -2; // Patch not visible, don't generate indices.

		// calculate the step we take for this LOD
		const s32 step = 1 << LOD;

		// Generate the indices for the specified patch at the specified LOD
		const s32 index = patchX * TerrainData.PatchCount + patchZ;

		s32 x = 0;
		s32 z = 0;

		indices.set_used(TerrainData.PatchSize * TerrainData.PatchSize * 6);

		// Loop through patch and generate indices
		s32 rv=0;
		while (z<TerrainData.CalcPatchSize)
		{
			const s32 index11 = getIndex(patchZ, patchX, index, x, z);
			const s32 index21 = getIndex(patchZ, patchX, index, x + step, z);
			const s32 index12 = getIndex(patchZ, patchX, index, x, z + step);
			const s32 index22 = getIndex(patchZ, patchX, index, x + step, z + step);

			indices[rv++] = index12;
			indices[rv++] = index11;
			indices[rv++] = index22;
			indices[rv++] = index22;
			indices[rv++] = index11;
			indices[rv++] = index21;

			// increment index position horizontally
			x += step;

			if (x >= TerrainData.CalcPatchSize) // we've hit an edge
			{
				x = 0;
				z += step;
			}
		}

		if (setLODs)
			setCurrentLODOfPatches(cLODs);

		return rv;
	}


	//! Populates an array with the CurrentLOD of each patch.
	//! \param LODs: A reference to a core::array<s32> to hold the values
	//! \return Returns the number of elements in the array
	s32 CTerrainSceneNode::getCurrentLODOfPatches(core::array<s32>& LODs) const
	{
		s32 numLODs;
		LODs.clear();

		const s32 count = TerrainData.PatchCount * TerrainData.PatchCount;
		for (numLODs = 0; numLODs < count; numLODs++)
			LODs.push_back(TerrainData.Patches[numLODs].CurrentLOD);

		return LODs.size();
	}


	//! Manually sets the LOD of a patch
	//! \param patchX: Patch x coordinate.
	//! \param patchZ: Patch z coordinate.
	//! \param LOD: The level of detail to set the patch to.
	void CTerrainSceneNode::setLODOfPatch(s32 patchX, s32 patchZ, s32 LOD)
	{
		TerrainData.Patches[patchX * TerrainData.PatchCount + patchZ].CurrentLOD = LOD;
	}


	//! Override the default generation of distance thresholds for determining the LOD a patch
	//! is rendered at.
	bool CTerrainSceneNode::overrideLODDistance(s32 LOD, f64 newDistance)
	{
		OverrideDistanceThreshold = true;

		if (LOD < 0 || LOD > TerrainData.MaxLOD - 1)
			return false;

		TerrainData.LODDistanceThreshold[LOD] = newDistance * newDistance;

		return true;
	}


	//! Creates a planar texture mapping on the terrain
	//! \param resolution: resolution of the planar mapping. This is the value
	//! specifying the relation between world space and texture coordinate space.
	void CTerrainSceneNode::scaleTexture(f32 resolution, f32 resolution2)
	{
		TCoordScale1 = resolution;
		TCoordScale2 = resolution2;

		const f32 resBySize = resolution / (f32)(TerrainData.Size-1);
		const f32 res2BySize = resolution2 / (f32)(TerrainData.Size-1);
		u32 index = 0;
		f32 xval = 0.f;
		f32 x2val = 0.f;
		for (s32 x=0; x<TerrainData.Size; ++x)
		{
			f32 zval=0.f;
			f32 z2val=0.f;
			for (s32 z=0; z<TerrainData.Size; ++z)
			{
				RenderBuffer->getVertexBuffer()[index].TCoords.X = 1.f-xval;
				RenderBuffer->getVertexBuffer()[index].TCoords.Y = zval;

				if (RenderBuffer->getVertexType()==video::EVT_2TCOORDS)
				{
					if (resolution2 == 0)
					{
						((video::S3DVertex2TCoords&)RenderBuffer->getVertexBuffer()[index]).TCoords2 = RenderBuffer->getVertexBuffer()[index].TCoords;
					}
					else
					{
						((video::S3DVertex2TCoords&)RenderBuffer->getVertexBuffer()[index]).TCoords2.X = 1.f-x2val;
						((video::S3DVertex2TCoords&)RenderBuffer->getVertexBuffer()[index]).TCoords2.Y = z2val;
					}
				}

				++index;
				zval += resBySize;
				z2val += res2BySize;
			}
			xval += resBySize;
			x2val += res2BySize;
		}

		RenderBuffer->setDirty(EBT_VERTEX);
	}


	//! used to get the indices when generating index data for patches at varying levels of detail.
	u32 CTerrainSceneNode::getIndex(const s32 PatchX, const s32 PatchZ,
					const s32 PatchIndex, u32 vX, u32 vZ) const
	{
		// top border
		if (vZ == 0)
		{
			if (TerrainData.Patches[PatchIndex].Top &&
				TerrainData.Patches[PatchIndex].CurrentLOD < TerrainData.Patches[PatchIndex].Top->CurrentLOD &&
				(vX % (1 << TerrainData.Patches[PatchIndex].Top->CurrentLOD)) != 0 )
			{
				vX -= vX % (1 << TerrainData.Patches[PatchIndex].Top->CurrentLOD);
			}
		}
		else
		if (vZ == (u32)TerrainData.CalcPatchSize) // bottom border
		{
			if (TerrainData.Patches[PatchIndex].Bottom &&
				TerrainData.Patches[PatchIndex].CurrentLOD < TerrainData.Patches[PatchIndex].Bottom->CurrentLOD &&
				(vX % (1 << TerrainData.Patches[PatchIndex].Bottom->CurrentLOD)) != 0)
			{
				vX -= vX % (1 << TerrainData.Patches[PatchIndex].Bottom->CurrentLOD);
			}
		}

		// left border
		if (vX == 0)
		{
			if (TerrainData.Patches[PatchIndex].Left &&
				TerrainData.Patches[PatchIndex].CurrentLOD < TerrainData.Patches[PatchIndex].Left->CurrentLOD &&
				(vZ % (1 << TerrainData.Patches[PatchIndex].Left->CurrentLOD)) != 0)
			{
				vZ -= vZ % (1 << TerrainData.Patches[PatchIndex].Left->CurrentLOD);
			}
		}
		else
		if (vX == (u32)TerrainData.CalcPatchSize) // right border
		{
			if (TerrainData.Patches[PatchIndex].Right &&
				TerrainData.Patches[PatchIndex].CurrentLOD < TerrainData.Patches[PatchIndex].Right->CurrentLOD &&
				(vZ % (1 << TerrainData.Patches[PatchIndex].Right->CurrentLOD)) != 0)
			{
				vZ -= vZ % (1 << TerrainData.Patches[PatchIndex].Right->CurrentLOD);
			}
		}

		if (vZ >= (u32)TerrainData.PatchSize)
			vZ = TerrainData.CalcPatchSize;

		if (vX >= (u32)TerrainData.PatchSize)
			vX = TerrainData.CalcPatchSize;

		return (vZ + ((TerrainData.CalcPatchSize) * PatchZ)) * TerrainData.Size +
			(vX + ((TerrainData.CalcPatchSize) * PatchX));
	}


	//! smooth the terrain
	void CTerrainSceneNode::smoothTerrain(IDynamicMeshBuffer* mb, s32 smoothFactor)
	{
		for (s32 run = 0; run < smoothFactor; ++run)
		{
			s32 yd = TerrainData.Size;
			for (s32 y = 1; y < TerrainData.Size - 1; ++y)
			{
				for (s32 x = 1; x < TerrainData.Size - 1; ++x)
				{
					mb->getVertexBuffer()[x + yd].Pos.Y =
						(mb->getVertexBuffer()[x-1 + yd].Pos.Y + //left
						mb->getVertexBuffer()[x+1 + yd].Pos.Y + //right
						mb->getVertexBuffer()[x + yd - TerrainData.Size].Pos.Y + //above
						mb->getVertexBuffer()[x + yd + TerrainData.Size].Pos.Y) * 0.25f; //below
				}
				yd += TerrainData.Size;
			}
		}
	}


	//! calculate smooth normals
	void CTerrainSceneNode::calculateNormals(IDynamicMeshBuffer* mb)
	{
		s32 count;
		core::vector3df a, b, c, t;

		for (s32 x=0; x<TerrainData.Size; ++x)
		{
			for (s32 z=0; z<TerrainData.Size; ++z)
			{
				count = 0;
				core::vector3df normal;

				// top left
				if (x>0 && z>0)
				{
					a = mb->getVertexBuffer()[(x-1)*TerrainData.Size+z-1].Pos;
					b = mb->getVertexBuffer()[(x-1)*TerrainData.Size+z].Pos;
					c = mb->getVertexBuffer()[x*TerrainData.Size+z].Pos;
					b -= a;
					c -= a;
					t = b.crossProduct(c);
					t.normalize();
					normal += t;

					a = mb->getVertexBuffer()[(x-1)*TerrainData.Size+z-1].Pos;
					b = mb->getVertexBuffer()[x*TerrainData.Size+z-1].Pos;
					c = mb->getVertexBuffer()[x*TerrainData.Size+z].Pos;
					b -= a;
					c -= a;
					t = b.crossProduct(c);
					t.normalize();
					normal += t;

					count += 2;
				}

				// top right
				if (x>0 && z<TerrainData.Size-1)
				{
					a = mb->getVertexBuffer()[(x-1)*TerrainData.Size+z].Pos;
					b = mb->getVertexBuffer()[(x-1)*TerrainData.Size+z+1].Pos;
					c = mb->getVertexBuffer()[x*TerrainData.Size+z+1].Pos;
					b -= a;
					c -= a;
					t = b.crossProduct(c);
					t.normalize();
					normal += t;

					a = mb->getVertexBuffer()[(x-1)*TerrainData.Size+z].Pos;
					b = mb->getVertexBuffer()[x*TerrainData.Size+z+1].Pos;
					c = mb->getVertexBuffer()[x*TerrainData.Size+z].Pos;
					b -= a;
					c -= a;
					t = b.crossProduct(c);
					t.normalize();
					normal += t;

					count += 2;
				}

				// bottom right
				if (x<TerrainData.Size-1 && z<TerrainData.Size-1)
				{
					a = mb->getVertexBuffer()[x*TerrainData.Size+z+1].Pos;
					b = mb->getVertexBuffer()[x*TerrainData.Size+z].Pos;
					c = mb->getVertexBuffer()[(x+1)*TerrainData.Size+z+1].Pos;
					b -= a;
					c -= a;
					t = b.crossProduct(c);
					t.normalize();
					normal += t;

					a = mb->getVertexBuffer()[x*TerrainData.Size+z+1].Pos;
					b = mb->getVertexBuffer()[(x+1)*TerrainData.Size+z+1].Pos;
					c = mb->getVertexBuffer()[(x+1)*TerrainData.Size+z].Pos;
					b -= a;
					c -= a;
					t = b.crossProduct(c);
					t.normalize();
					normal += t;

					count += 2;
				}

				// bottom left
				if (x<TerrainData.Size-1 && z>0)
				{
					a = mb->getVertexBuffer()[x*TerrainData.Size+z-1].Pos;
					b = mb->getVertexBuffer()[x*TerrainData.Size+z].Pos;
					c = mb->getVertexBuffer()[(x+1)*TerrainData.Size+z].Pos;
					b -= a;
					c -= a;
					t = b.crossProduct(c);
					t.normalize();
					normal += t;

					a = mb->getVertexBuffer()[x*TerrainData.Size+z-1].Pos;
					b = mb->getVertexBuffer()[(x+1)*TerrainData.Size+z].Pos;
					c = mb->getVertexBuffer()[(x+1)*TerrainData.Size+z-1].Pos;
					b -= a;
					c -= a;
					t = b.crossProduct(c);
					t.normalize();
					normal += t;

					count += 2;
				}

				if (count != 0)
				{
					normal.normalize();
				}
				else
				{
					normal.set(0.0f, 1.0f, 0.0f);
				}

				mb->getVertexBuffer()[x * TerrainData.Size + z].Normal = normal;
			}
		}
	}


	//! create patches, stuff that needs to be done only once for patches goes here.
	void CTerrainSceneNode::createPatches()
	{
		TerrainData.PatchCount = (TerrainData.Size - 1) / (TerrainData.CalcPatchSize);

		if (TerrainData.Patches)
			delete [] TerrainData.Patches;

		TerrainData.Patches = new SPatch[TerrainData.PatchCount * TerrainData.PatchCount];
	}


	//! used to calculate the internal STerrainData structure both at creation and after scaling/position calls.
	void CTerrainSceneNode::calculatePatchData()
	{
		// Reset the Terrains Bounding Box for re-calculation
		TerrainData.BoundingBox.reset(RenderBuffer->getPosition(0));

		for (s32 x = 0; x < TerrainData.PatchCount; ++x)
		{
			for (s32 z = 0; z < TerrainData.PatchCount; ++z)
			{
				const s32 index = x * TerrainData.PatchCount + z;
				SPatch& patch = TerrainData.Patches[index];
				patch.CurrentLOD = 0;

				const s32 xstart = x*TerrainData.CalcPatchSize;
				const s32 xend = xstart+TerrainData.CalcPatchSize;
				const s32 zstart = z*TerrainData.CalcPatchSize;
				const s32 zend = zstart+TerrainData.CalcPatchSize;
				// For each patch, calculate the bounding box (mins and maxes)
				patch.BoundingBox.reset(RenderBuffer->getPosition(xstart*TerrainData.Size + zstart));

				for (s32 xx = xstart; xx <= xend; ++xx)
					for (s32 zz = zstart; zz <= zend; ++zz)
						patch.BoundingBox.addInternalPoint(RenderBuffer->getVertexBuffer()[xx * TerrainData.Size + zz].Pos);

				// Reconfigure the bounding box of the terrain as a whole
				TerrainData.BoundingBox.addInternalBox(patch.BoundingBox);

				// get center of Patch
				patch.Center = patch.BoundingBox.getCenter();

				// Assign Neighbours
				// Top
				if (x > 0)
					patch.Top = &TerrainData.Patches[(x-1) * TerrainData.PatchCount + z];
				else
					patch.Top = 0;

				// Bottom
				if (x < TerrainData.PatchCount - 1)
					patch.Bottom = &TerrainData.Patches[(x+1) * TerrainData.PatchCount + z];
				else
					patch.Bottom = 0;

				// Left
				if (z > 0)
					patch.Left = &TerrainData.Patches[x * TerrainData.PatchCount + z - 1];
				else
					patch.Left = 0;

				// Right
				if (z < TerrainData.PatchCount - 1)
					patch.Right = &TerrainData.Patches[x * TerrainData.PatchCount + z + 1];
				else
					patch.Right = 0;
			}
		}

		// get center of Terrain
		TerrainData.Center = TerrainData.BoundingBox.getCenter();

		// if the default rotation pivot is still being used, update it.
		if (UseDefaultRotationPivot)
		{
			TerrainData.RotationPivot = TerrainData.Center;
		}
	}


	//! used to calculate or recalculate the distance thresholds
	void CTerrainSceneNode::calculateDistanceThresholds(bool scalechanged)
	{
		// Only update the LODDistanceThreshold if it's not manually changed
		if (!OverrideDistanceThreshold)
		{
			TerrainData.LODDistanceThreshold.set_used(0);
			// Determine new distance threshold for determining what LOD to draw patches at
			TerrainData.LODDistanceThreshold.reallocate(TerrainData.MaxLOD);

			const f64 size = TerrainData.PatchSize * TerrainData.PatchSize *
					TerrainData.Scale.X * TerrainData.Scale.Z;
			for (s32 i=0; i<TerrainData.MaxLOD; ++i)
			{
				TerrainData.LODDistanceThreshold.push_back(size * ((i+1+ i / 2) * (i+1+ i / 2)));
			}
		}
	}


	void CTerrainSceneNode::setCurrentLODOfPatches(s32 lod)
	{
		const s32 count = TerrainData.PatchCount * TerrainData.PatchCount;
		for (s32 i=0; i< count; ++i)
			TerrainData.Patches[i].CurrentLOD = lod;
	}


	void CTerrainSceneNode::setCurrentLODOfPatches(const core::array<s32>& lodarray)
	{
		const s32 count = TerrainData.PatchCount * TerrainData.PatchCount;
		for (s32 i=0; i<count; ++i)
			TerrainData.Patches[i].CurrentLOD = lodarray[i];
	}


	//! Gets the height
	f32 CTerrainSceneNode::getHeight(f32 x, f32 z) const
	{
		if (!Mesh->getMeshBufferCount())
			return 0;

		core::matrix4 rotMatrix;
		rotMatrix.setRotationDegrees(TerrainData.Rotation);
		core::vector3df pos(x, 0.0f, z);
		rotMatrix.rotateVect(pos);
		pos -= TerrainData.Position;
		pos /= TerrainData.Scale;

		s32 X(core::floor32(pos.X));
		s32 Z(core::floor32(pos.Z));

		f32 height = -FLT_MAX;
		if (X >= 0 && X < TerrainData.Size-1 &&
				Z >= 0 && Z < TerrainData.Size-1)
		{
			const video::S3DVertex2TCoords* Vertices = (const video::S3DVertex2TCoords*)Mesh->getMeshBuffer(0)->getVertices();
			const core::vector3df& a = Vertices[X * TerrainData.Size + Z].Pos;
			const core::vector3df& b = Vertices[(X + 1) * TerrainData.Size + Z].Pos;
			const core::vector3df& c = Vertices[X * TerrainData.Size + (Z + 1)].Pos;
			const core::vector3df& d = Vertices[(X + 1) * TerrainData.Size + (Z + 1)].Pos;

			// offset from integer position
			const f32 dx = pos.X - X;
			const f32 dz = pos.Z - Z;

			if (dx > dz)
				height = a.Y + (d.Y - b.Y)*dz + (b.Y - a.Y)*dx;
			else
				height = a.Y + (d.Y - c.Y)*dx + (c.Y - a.Y)*dz;

			height *= TerrainData.Scale.Y;
			height += TerrainData.Position.Y;
		}

		return height;
	}


	//! Writes attributes of the scene node.
	void CTerrainSceneNode::serializeAttributes(io::IAttributes* out,
				io::SAttributeReadWriteOptions* options) const
	{
		ISceneNode::serializeAttributes(out, options);

		out->addString("Heightmap", HeightmapFile.c_str());
		out->addFloat("TextureScale1", TCoordScale1);
		out->addFloat("TextureScale2", TCoordScale2);
		out->addInt("SmoothFactor", SmoothFactor);
	}


	//! Reads attributes of the scene node.
	void CTerrainSceneNode::deserializeAttributes(io::IAttributes* in,
			io::SAttributeReadWriteOptions* options)
	{
		io::path newHeightmap = in->getAttributeAsString("Heightmap");
		f32 tcoordScale1 = in->getAttributeAsFloat("TextureScale1");
		f32 tcoordScale2 = in->getAttributeAsFloat("TextureScale2");
		s32 smoothFactor = in->getAttributeAsInt("SmoothFactor");

		// set possible new heightmap

		if (newHeightmap.size() != 0 && newHeightmap != HeightmapFile)
		{
			io::IReadFile* file = FileSystem->createAndOpenFile(newHeightmap.c_str());
			if (file)
			{
				loadHeightMap(file, video::SColor(255,255,255,255), smoothFactor);
				file->drop();
			}
			else
				os::Printer::log("could not open heightmap", newHeightmap.c_str());
		}

		// set possible new scale

		if (core::equals(tcoordScale1, 0.f))
			tcoordScale1 = 1.0f;

		if (core::equals(tcoordScale2, 0.f))
			tcoordScale2 = 1.0f;

		if (!core::equals(tcoordScale1, TCoordScale1) ||
			!core::equals(tcoordScale2, TCoordScale2))
		{
			scaleTexture(tcoordScale1, tcoordScale2);
		}

		ISceneNode::deserializeAttributes(in, options);
	}


	//! Creates a clone of this scene node and its children.
	ISceneNode* CTerrainSceneNode::clone(ISceneNode* newParent, ISceneManager* newManager)
	{
		if (!newParent)
			newParent = Parent;
		if (!newManager)
			newManager = SceneManager;

		CTerrainSceneNode* nb = new CTerrainSceneNode(
			newParent, newManager, FileSystem, ID,
			4, ETPS_17, getPosition(), getRotation(), getScale());

		nb->cloneMembers(this, newManager);

		// instead of cloning the data structures, recreate the terrain.
		// (temporary solution)

		// load file

		io::IReadFile* file = FileSystem->createAndOpenFile(HeightmapFile.c_str());
		if (file)
		{
			nb->loadHeightMap(file, video::SColor(255,255,255,255), 0);
			file->drop();
		}

		// scale textures

		nb->scaleTexture(TCoordScale1, TCoordScale2);

		// copy materials

		for (unsigned int m = 0; m<Mesh->getMeshBufferCount(); ++m)
		{
			if (nb->Mesh->getMeshBufferCount()>m &&
				nb->Mesh->getMeshBuffer(m) &&
				Mesh->getMeshBuffer(m))
			{
				nb->Mesh->getMeshBuffer(m)->getMaterial() =
					Mesh->getMeshBuffer(m)->getMaterial();
			}
		}

		nb->RenderBuffer->getMaterial() = RenderBuffer->getMaterial();

		// finish

		if ( newParent )
			nb->drop();
		return nb;
	}

} // end namespace scene
} // end namespace irr



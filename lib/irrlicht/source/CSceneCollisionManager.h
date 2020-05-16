// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_SCENE_COLLISION_MANAGER_H_INCLUDED__
#define __C_SCENE_COLLISION_MANAGER_H_INCLUDED__

#include "ISceneCollisionManager.h"
#include "ISceneManager.h"
#include "IVideoDriver.h"

namespace irr
{
namespace scene
{

	//! The Scene Collision Manager provides methods for performing collision tests and picking on scene nodes.
	class CSceneCollisionManager : public ISceneCollisionManager
	{
	public:

		//! constructor
		CSceneCollisionManager(ISceneManager* smanager, video::IVideoDriver* driver);

		//! destructor
		virtual ~CSceneCollisionManager();

		//! Returns the scene node, which is currently visible at the given
		//! screen coordinates, viewed from the currently active camera.
		virtual ISceneNode* getSceneNodeFromScreenCoordinatesBB(const core::position2d<s32>& pos,
				s32 idBitMask=0, bool bNoDebugObjects=false, ISceneNode* root=0);

		//! Returns the nearest scene node which collides with a 3d ray and
		//! whose id matches a bitmask.
		virtual ISceneNode* getSceneNodeFromRayBB(const core::line3d<f32>& ray,
						s32 idBitMask=0, bool bNoDebugObjects=false,
						ISceneNode* root=0);

		//! Returns the scene node, at which the overgiven camera is looking at and
		//! which id matches the bitmask.
		virtual ISceneNode* getSceneNodeFromCameraBB(ICameraSceneNode* camera,
				s32 idBitMask=0, bool bNoDebugObjects = false);

		//! Finds the collision point of a line and lots of triangles, if there is one.
		virtual bool getCollisionPoint(const core::line3d<f32>& ray,
			ITriangleSelector* selector, core::vector3df& outCollisionPoint,
			core::triangle3df& outTriangle,
			ISceneNode* & outNode);

		//! Collides a moving ellipsoid with a 3d world with gravity and returns
		//! the resulting new position of the ellipsoid.
		virtual core::vector3df getCollisionResultPosition(
			ITriangleSelector* selector,
			const core::vector3df &ellipsoidPosition,
			const core::vector3df& ellipsoidRadius,
			const core::vector3df& ellipsoidDirectionAndSpeed,
			core::triangle3df& triout,
			core::vector3df& hitPosition,
			bool& outFalling,
			ISceneNode*& outNode,
			f32 slidingSpeed,
			const core::vector3df& gravityDirectionAndSpeed);

		//! Returns a 3d ray which would go through the 2d screen coodinates.
		virtual core::line3d<f32> getRayFromScreenCoordinates(
			const core::position2d<s32> & pos, ICameraSceneNode* camera = 0);

		//! Calculates 2d screen position from a 3d position.
		virtual core::position2d<s32> getScreenCoordinatesFrom3DPosition(
			const core::vector3df & pos, ICameraSceneNode* camera=0, bool useViewPort=false);

		//! Gets the scene node and nearest collision point for a ray based on
		//! the nodes' id bitmasks, bounding boxes and triangle selectors.
		virtual ISceneNode* getSceneNodeAndCollisionPointFromRay(
								core::line3df ray,
								core::vector3df & outCollisionPoint,
								core::triangle3df & outTriangle,
								s32 idBitMask = 0,
								ISceneNode * collisionRootNode = 0,
								bool noDebugObjects = false);


	private:

		//! recursive method for going through all scene nodes
		void getPickedNodeBB(ISceneNode* root, core::line3df& ray, s32 bits,
					bool bNoDebugObjects,
					f32& outbestdistance, ISceneNode*& outbestnode);

		//! recursive method for going through all scene nodes
		void getPickedNodeFromBBAndSelector(ISceneNode * root,
						core::line3df & ray,
						s32 bits,
						bool noDebugObjects,
						f32 & outBestDistanceSquared,
						ISceneNode * & outBestNode,
						core::vector3df & outBestCollisionPoint,
						core::triangle3df & outBestTriangle);


		struct SCollisionData
		{
			core::vector3df eRadius;

			core::vector3df R3Velocity;
			core::vector3df R3Position;

			core::vector3df velocity;
			core::vector3df normalizedVelocity;
			core::vector3df basePoint;

			bool foundCollision;
			f32 nearestDistance;
			core::vector3df intersectionPoint;

			core::triangle3df intersectionTriangle;
			s32 triangleIndex;
			s32 triangleHits;

			f32 slidingSpeed;

			ITriangleSelector* selector;
		};

		//! Tests the current collision data against an individual triangle.
		/**
		\param colData: the collision data.
		\param triangle: the triangle to test against.
		\return true if the triangle is hit (and is the closest hit), false otherwise */
		bool testTriangleIntersection(SCollisionData* colData,
			const core::triangle3df& triangle);

		//! recursive method for doing collision response
		core::vector3df collideEllipsoidWithWorld(ITriangleSelector* selector,
			const core::vector3df &position,
			const core::vector3df& radius,  const core::vector3df& velocity,
			f32 slidingSpeed,
			const core::vector3df& gravity, core::triangle3df& triout,
			core::vector3df& hitPosition,
			bool& outFalling,
			ISceneNode*& outNode);

		core::vector3df collideWithWorld(s32 recursionDepth, SCollisionData &colData,
			core::vector3df pos, core::vector3df vel);

		inline bool getLowestRoot(f32 a, f32 b, f32 c, f32 maxR, f32* root);

		ISceneManager* SceneManager;
		video::IVideoDriver* Driver;
		core::array<core::triangle3df> Triangles; // triangle buffer
	};


} // end namespace scene
} // end namespace irr

#endif


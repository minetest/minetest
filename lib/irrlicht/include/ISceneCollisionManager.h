// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_SCENE_COLLISION_MANAGER_H_INCLUDED__
#define __I_SCENE_COLLISION_MANAGER_H_INCLUDED__

#include "IReferenceCounted.h"
#include "vector3d.h"
#include "triangle3d.h"
#include "position2d.h"
#include "line3d.h"

namespace irr
{

namespace scene
{
	class ISceneNode;
	class ICameraSceneNode;
	class ITriangleSelector;

	//! The Scene Collision Manager provides methods for performing collision tests and picking on scene nodes.
	class ISceneCollisionManager : public virtual IReferenceCounted
	{
	public:

		//! Finds the nearest collision point of a line and lots of triangles, if there is one.
		/** \param ray: Line with which collisions are tested.
		\param selector: TriangleSelector containing the triangles. It
		can be created for example using
		ISceneManager::createTriangleSelector() or
		ISceneManager::createTriangleOctreeSelector().
		\param outCollisionPoint: If a collision is detected, this will
		contain the position of the nearest collision to the line-start.
		\param outTriangle: If a collision is detected, this will
		contain the triangle with which the ray collided.
		\param outNode: If a collision is detected, this will contain
		the scene node associated with the triangle that was hit.
		\return True if a collision was detected and false if not. */
		virtual bool getCollisionPoint(const core::line3d<f32>& ray,
				ITriangleSelector* selector, core::vector3df& outCollisionPoint,
				core::triangle3df& outTriangle, ISceneNode*& outNode) =0;

		//! Collides a moving ellipsoid with a 3d world with gravity and returns the resulting new position of the ellipsoid.
		/** This can be used for moving a character in a 3d world: The
		character will slide at walls and is able to walk up stairs.
		The method used how to calculate the collision result position
		is based on the paper "Improved Collision detection and
		Response" by Kasper Fauerby.
		\param selector: TriangleSelector containing the triangles of
		the world. It can be created for example using
		ISceneManager::createTriangleSelector() or
		ISceneManager::createTriangleOctreeSelector().
		\param ellipsoidPosition: Position of the ellipsoid.
		\param ellipsoidRadius: Radius of the ellipsoid.
		\param ellipsoidDirectionAndSpeed: Direction and speed of the
		movement of the ellipsoid.
		\param triout: Optional parameter where the last triangle
		causing a collision is stored, if there is a collision.
		\param hitPosition: Return value for the position of the collision
		\param outFalling: Is set to true if the ellipsoid is falling
		down, caused by gravity.
		\param outNode: the node with which the ellipoid collided (if any)
		\param slidingSpeed: DOCUMENTATION NEEDED.
		\param gravityDirectionAndSpeed: Direction and force of gravity.
		\return New position of the ellipsoid. */
		virtual core::vector3df getCollisionResultPosition(
			ITriangleSelector* selector,
			const core::vector3df &ellipsoidPosition,
			const core::vector3df& ellipsoidRadius,
			const core::vector3df& ellipsoidDirectionAndSpeed,
			core::triangle3df& triout,
			core::vector3df& hitPosition,
			bool& outFalling,
			ISceneNode*& outNode,
			f32 slidingSpeed = 0.0005f,
			const core::vector3df& gravityDirectionAndSpeed
			= core::vector3df(0.0f, 0.0f, 0.0f)) = 0;

		//! Returns a 3d ray which would go through the 2d screen coodinates.
		/** \param pos: Screen coordinates in pixels.
		\param camera: Camera from which the ray starts. If null, the
		active camera is used.
		\return Ray starting from the position of the camera and ending
		at a length of the far value of the camera at a position which
		would be behind the 2d screen coodinates. */
		virtual core::line3d<f32> getRayFromScreenCoordinates(
			const core::position2d<s32>& pos, ICameraSceneNode* camera = 0) = 0;

		//! Calculates 2d screen position from a 3d position.
		/** \param pos: 3D position in world space to be transformed
		into 2d.
		\param camera: Camera to be used. If null, the currently active
		camera is used.
		\param useViewPort: Calculate screen coordinates relative to
		the current view port. Please note that unless the driver does
		not take care of the view port, it is usually best to get the
		result in absolute screen coordinates (flag=false).
		\return 2d screen coordinates which a object in the 3d world
		would have if it would be rendered to the screen. If the 3d
		position is behind the camera, it is set to (-1000,-1000). In
		most cases you can ignore this fact, because if you use this
		method for drawing a decorator over a 3d object, it will be
		clipped by the screen borders. */
		virtual core::position2d<s32> getScreenCoordinatesFrom3DPosition(
			const core::vector3df& pos, ICameraSceneNode* camera=0, bool useViewPort=false) = 0;

		//! Gets the scene node, which is currently visible under the given screencoordinates, viewed from the currently active camera.
		/** The collision tests are done using a bounding box for each
		scene node. You can limit the recursive search so just all children of the specified root are tested.
		\param pos: Position in pixel screen coordinates, under which
		the returned scene node will be.
		\param idBitMask: Only scene nodes with an id with bits set
		like in this mask will be tested. If the BitMask is 0, this
		feature is disabled.
		Please note that the default node id of -1 will match with
		every bitmask != 0
		\param bNoDebugObjects: Doesn't take debug objects into account
		when true. These are scene nodes with IsDebugObject() = true.
		\param root If different from 0, the search is limited to the children of this node.
		\return Visible scene node under screen coordinates with
		matching bits in its id. If there is no scene node under this
		position, 0 is returned. */
		virtual ISceneNode* getSceneNodeFromScreenCoordinatesBB(const core::position2d<s32>& pos,
				s32 idBitMask=0, bool bNoDebugObjects=false, ISceneNode* root=0) =0;

		//! Returns the nearest scene node which collides with a 3d ray and whose id matches a bitmask.
		/** The collision tests are done using a bounding box for each
		scene node. The recursive search can be limited be specifying a scene node.
		\param ray Line with which collisions are tested.
		\param idBitMask Only scene nodes with an id which matches at
		least one of the bits contained in this mask will be tested.
		However, if this parameter is 0, then all nodes are checked.
		\param bNoDebugObjects: Doesn't take debug objects into account when true. These
		are scene nodes with IsDebugObject() = true.
		\param root If different from 0, the search is limited to the children of this node.
		\return Scene node nearest to ray.start, which collides with
		the ray and matches the idBitMask, if the mask is not null. If
		no scene node is found, 0 is returned. */
		virtual ISceneNode* getSceneNodeFromRayBB(const core::line3d<f32>& ray,
							s32 idBitMask=0, bool bNoDebugObjects=false, ISceneNode* root=0) =0;

		//! Get the scene node, which the given camera is looking at and whose id matches the bitmask.
		/** A ray is simply casted from the position of the camera to
		the view target position, and all scene nodes are tested
		against this ray. The collision tests are done using a bounding
		box for each scene node.
		\param camera: Camera from which the ray is casted.
		\param idBitMask: Only scene nodes with an id which matches at least one of the
		bits contained in this mask will be tested. However, if this parameter is 0, then
		all nodes are checked.
		feature is disabled.
		Please note that the default node id of -1 will match with
		every bitmask != 0
		\param bNoDebugObjects: Doesn't take debug objects into account
		when true. These are scene nodes with IsDebugObject() = true.
		\return Scene node nearest to the camera, which collides with
		the ray and matches the idBitMask, if the mask is not null. If
		no scene node is found, 0 is returned. */
		virtual ISceneNode* getSceneNodeFromCameraBB(ICameraSceneNode* camera,
			s32 idBitMask=0, bool bNoDebugObjects = false) = 0;

		//! Perform a ray/box and ray/triangle collision check on a heirarchy of scene nodes.
		/** This checks all scene nodes under the specified one, first by ray/bounding
		box, and then by accurate ray/triangle collision, finding the nearest collision,
		and the scene node containg it.  It returns the node hit, and (via output
		parameters) the position of the collision, and the triangle that was hit.

		All scene nodes in the hierarchy tree under the specified node are checked. Only
		nodes that are visible, with an ID that matches at least one bit in the supplied
		bitmask, and which have a triangle selector are considered as candidates for being hit.
		You do not have to build a meta triangle selector; the individual triangle selectors
		of each candidate scene node are used automatically.

		\param ray: Line with which collisions are tested.
		\param outCollisionPoint: If a collision is detected, this will contain the
		position of the nearest collision.
		\param outTriangle: If a collision is detected, this will contain the triangle
		with which the ray collided.
		\param idBitMask: Only scene nodes with an id which matches at least one of the
		bits contained in this mask will be tested. However, if this parameter is 0, then
		all nodes are checked.
		\param collisionRootNode: the scene node at which to begin checking. Only this
		node and its children will be checked. If you want to check the entire scene,
		pass 0, and the root scene node will be used (this is the default).
		\param noDebugObjects: when true, debug objects are not considered viable targets.
		Debug objects are scene nodes with IsDebugObject() = true.
		\return Returns the scene node containing the hit triangle nearest to ray.start.
		If no collision is detected, then 0 is returned. */
		virtual ISceneNode* getSceneNodeAndCollisionPointFromRay(
								core::line3df ray,
								core::vector3df & outCollisionPoint,
								core::triangle3df & outTriangle,
								s32 idBitMask = 0,
								ISceneNode * collisionRootNode = 0,
								bool noDebugObjects = false) = 0;
	};


} // end namespace scene
} // end namespace irr

#endif


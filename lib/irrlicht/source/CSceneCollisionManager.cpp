// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CSceneCollisionManager.h"
#include "ISceneNode.h"
#include "ICameraSceneNode.h"
#include "ITriangleSelector.h"
#include "SViewFrustum.h"

#include "os.h"
#include "irrMath.h"

namespace irr
{
namespace scene
{

//! constructor
CSceneCollisionManager::CSceneCollisionManager(ISceneManager* smanager, video::IVideoDriver* driver)
: SceneManager(smanager), Driver(driver)
{
	#ifdef _DEBUG
	setDebugName("CSceneCollisionManager");
	#endif

	if (Driver)
		Driver->grab();
}


//! destructor
CSceneCollisionManager::~CSceneCollisionManager()
{
	if (Driver)
		Driver->drop();
}


//! Returns the scene node, which is currently visible at the given
//! screen coordinates, viewed from the currently active camera.
ISceneNode* CSceneCollisionManager::getSceneNodeFromScreenCoordinatesBB(
		const core::position2d<s32>& pos, s32 idBitMask, bool noDebugObjects, scene::ISceneNode* root)
{
	const core::line3d<f32> ln = getRayFromScreenCoordinates(pos, 0);

	if ( ln.start == ln.end )
		return 0;

	return getSceneNodeFromRayBB(ln, idBitMask, noDebugObjects, root);
}


//! Returns the nearest scene node which collides with a 3d ray and
//! which id matches a bitmask.
ISceneNode* CSceneCollisionManager::getSceneNodeFromRayBB(
		const core::line3d<f32>& ray,
		s32 idBitMask, bool noDebugObjects, scene::ISceneNode* root)
{
	ISceneNode* best = 0;
	f32 dist = FLT_MAX;

	core::line3d<f32> truncatableRay(ray);

	getPickedNodeBB((root==0)?SceneManager->getRootSceneNode():root, truncatableRay,
		idBitMask, noDebugObjects, dist, best);

	return best;
}


//! recursive method for going through all scene nodes
void CSceneCollisionManager::getPickedNodeBB(ISceneNode* root,
		core::line3df& ray, s32 bits, bool noDebugObjects,
		f32& outbestdistance, ISceneNode*& outbestnode)
{
	const ISceneNodeList& children = root->getChildren();
	const core::vector3df rayVector = ray.getVector().normalize();

	ISceneNodeList::ConstIterator it = children.begin();
	for (; it != children.end(); ++it)
	{
		ISceneNode* current = *it;

		if (current->isVisible())
		{
			if((noDebugObjects ? !current->isDebugObject() : true) &&
				(bits==0 || (bits != 0 && (current->getID() & bits))))
			{
				// get world to object space transform
				core::matrix4 worldToObject;
				if (!current->getAbsoluteTransformation().getInverse(worldToObject))
					continue;

				// transform vector from world space to object space
				core::line3df objectRay(ray);
				worldToObject.transformVect(objectRay.start);
				worldToObject.transformVect(objectRay.end);

				const core::aabbox3df & objectBox = current->getBoundingBox();

				// Do the initial intersection test in object space, since the
				// object space box test is more accurate.
				if(objectBox.isPointInside(objectRay.start))
				{
					// use fast bbox intersection to find distance to hitpoint
					// algorithm from Kay et al., code from gamedev.net
					const core::vector3df dir = (objectRay.end-objectRay.start).normalize();
					const core::vector3df minDist = (objectBox.MinEdge - objectRay.start)/dir;
					const core::vector3df maxDist = (objectBox.MaxEdge - objectRay.start)/dir;
					const core::vector3df realMin(core::min_(minDist.X, maxDist.X),core::min_(minDist.Y, maxDist.Y),core::min_(minDist.Z, maxDist.Z));
					const core::vector3df realMax(core::max_(minDist.X, maxDist.X),core::max_(minDist.Y, maxDist.Y),core::max_(minDist.Z, maxDist.Z));

					const f32 minmax = core::min_(realMax.X, realMax.Y, realMax.Z);
					// nearest distance to intersection
					const f32 maxmin = core::max_(realMin.X, realMin.Y, realMin.Z);

					const f32 toIntersectionSq = (maxmin>0?maxmin*maxmin:minmax*minmax);
					if (toIntersectionSq < outbestdistance)
					{
						outbestdistance = toIntersectionSq;
						outbestnode = current;

						// And we can truncate the ray to stop us hitting further nodes.
						ray.end = ray.start + (rayVector * sqrtf(toIntersectionSq));
					}
				}
				else
				if (objectBox.intersectsWithLine(objectRay))
				{
					// Now transform into world space, since we need to use world space
					// scales and distances.
					core::aabbox3df worldBox(objectBox);
					current->getAbsoluteTransformation().transformBoxEx(worldBox);

					core::vector3df edges[8];
					worldBox.getEdges(edges);

					/* We need to check against each of 6 faces, composed of these corners:
						  /3--------/7
						 /  |      / |
						/   |     /  |
						1---------5  |
						|   2- - -| -6
						|  /      |  /
						|/        | /
						0---------4/

						Note that we define them as opposite pairs of faces.
					*/
					static const s32 faceEdges[6][3] =
					{
						{ 0, 1, 5 }, // Front
						{ 6, 7, 3 }, // Back
						{ 2, 3, 1 }, // Left
						{ 4, 5, 7 }, // Right
						{ 1, 3, 7 }, // Top
						{ 2, 0, 4 }  // Bottom
					};

					core::vector3df intersection;
					core::plane3df facePlane;
					f32 bestDistToBoxBorder = FLT_MAX;
					f32 bestToIntersectionSq = FLT_MAX;

                    for(s32 face = 0; face < 6; ++face)
					{
						facePlane.setPlane(edges[faceEdges[face][0]],
											edges[faceEdges[face][1]],
											edges[faceEdges[face][2]]);

						// Only consider lines that might be entering through this face, since we
						// already know that the start point is outside the box.
						if(facePlane.classifyPointRelation(ray.start) != core::ISREL3D_FRONT)
							continue;

						// Don't bother using a limited ray, since we already know that it should be long
						// enough to intersect with the box.
						if(facePlane.getIntersectionWithLine(ray.start, rayVector, intersection))
						{
							const f32 toIntersectionSq = ray.start.getDistanceFromSQ(intersection);
							if(toIntersectionSq < outbestdistance)
							{
								// We have to check that the intersection with this plane is actually
								// on the box, so need to go back to object space again.
								worldToObject.transformVect(intersection);

                                // find the closest point on the box borders. Have to do this as exact checks will fail due to floating point problems.
								f32 distToBorder = core::max_ ( core::min_ (core::abs_(objectBox.MinEdge.X-intersection.X), core::abs_(objectBox.MaxEdge.X-intersection.X)),
                                                                core::min_ (core::abs_(objectBox.MinEdge.Y-intersection.Y), core::abs_(objectBox.MaxEdge.Y-intersection.Y)),
                                                                core::min_ (core::abs_(objectBox.MinEdge.Z-intersection.Z), core::abs_(objectBox.MaxEdge.Z-intersection.Z)) );
                                if ( distToBorder < bestDistToBoxBorder )
                                {
                                    bestDistToBoxBorder = distToBorder;
                                    bestToIntersectionSq = toIntersectionSq;
                                }
							}
						}

						// If the ray could be entering through the first face of a pair, then it can't
						// also be entering through the opposite face, and so we can skip that face.
						if (!(face & 0x01))
							++face;
					}

					if ( bestDistToBoxBorder < FLT_MAX )
					{
                        outbestdistance = bestToIntersectionSq;
						outbestnode = current;

                        // If we got a hit, we can now truncate the ray to stop us hitting further nodes.
                        ray.end = ray.start + (rayVector * sqrtf(outbestdistance));
					}
				}
			}

			// Only check the children if this node is visible.
			getPickedNodeBB(current, ray, bits, noDebugObjects, outbestdistance, outbestnode);
		}
	}
}


ISceneNode* CSceneCollisionManager::getSceneNodeAndCollisionPointFromRay(
						core::line3df ray,
						core::vector3df & outCollisionPoint,
						core::triangle3df & outTriangle,
						s32 idBitMask,
						ISceneNode * collisionRootNode,
						bool noDebugObjects)
{
	ISceneNode* bestNode = 0;
	f32 bestDistanceSquared = FLT_MAX;

	if(0 == collisionRootNode)
		collisionRootNode = SceneManager->getRootSceneNode();

	// We don't try to do anything too clever, like sorting the candidate
	// nodes by distance to bounding-box. In the example below, we could do the
	// triangle collision check with node A first, but we'd have to check node B
	// anyway, as the actual collision point could be (and is) closer than the
	// collision point in node A.
	//
	//    ray end
	//       |
	//   AAAAAAAAAA
	//   A   |
	//   A   |  B
	//   A   |  B
	//   A  BBBBB
	//   A   |
	//   A   |
	//       |
	//       |
	//    ray start
	//
	// We therefore have to do a full BB and triangle collision on every scene
	// node in order to find the nearest collision point, so sorting them by
	// bounding box would be pointless.

	getPickedNodeFromBBAndSelector(collisionRootNode, ray, idBitMask,
					noDebugObjects, bestDistanceSquared, bestNode,
					outCollisionPoint, outTriangle);
	return bestNode;
}


void CSceneCollisionManager::getPickedNodeFromBBAndSelector(
				ISceneNode * root,
				core::line3df & ray,
				s32 bits,
				bool noDebugObjects,
				f32 & outBestDistanceSquared,
				ISceneNode * & outBestNode,
				core::vector3df & outBestCollisionPoint,
				core::triangle3df & outBestTriangle)
{
	const ISceneNodeList& children = root->getChildren();

	ISceneNodeList::ConstIterator it = children.begin();
	for (; it != children.end(); ++it)
	{
		ISceneNode* current = *it;
		ITriangleSelector * selector = current->getTriangleSelector();

		if (selector && current->isVisible() &&
			(noDebugObjects ? !current->isDebugObject() : true) &&
			(bits==0 || (bits != 0 && (current->getID() & bits))))
		{
			// get world to object space transform
			core::matrix4 mat;
			if (!current->getAbsoluteTransformation().getInverse(mat))
			continue;

			// transform vector from world space to object space
			core::line3df line(ray);
			mat.transformVect(line.start);
			mat.transformVect(line.end);

			const core::aabbox3df& box = current->getBoundingBox();

			core::vector3df candidateCollisionPoint;
			core::triangle3df candidateTriangle;

			// do intersection test in object space
			ISceneNode * hitNode = 0;
			if (box.intersectsWithLine(line) &&
				getCollisionPoint(ray, selector, candidateCollisionPoint, candidateTriangle, hitNode))
			{
				const f32 distanceSquared = (candidateCollisionPoint - ray.start).getLengthSQ();

				if(distanceSquared < outBestDistanceSquared)
				{
					outBestDistanceSquared = distanceSquared;
					outBestNode = current;
					outBestCollisionPoint = candidateCollisionPoint;
					outBestTriangle = candidateTriangle;
					const core::vector3df rayVector = ray.getVector().normalize();
					ray.end = ray.start + (rayVector * sqrtf(distanceSquared));
				}
			}
		}

		getPickedNodeFromBBAndSelector(current, ray, bits, noDebugObjects,
						outBestDistanceSquared, outBestNode,
						outBestCollisionPoint, outBestTriangle);
	}
}


//! Returns the scene node, at which the overgiven camera is looking at and
//! which id matches the bitmask.
ISceneNode* CSceneCollisionManager::getSceneNodeFromCameraBB(
	ICameraSceneNode* camera, s32 idBitMask, bool noDebugObjects)
{
	if (!camera)
		return 0;

	const core::vector3df start = camera->getAbsolutePosition();
	core::vector3df end = camera->getTarget();

	end = start + ((end - start).normalize() * camera->getFarValue());

	return getSceneNodeFromRayBB(core::line3d<f32>(start, end), idBitMask, noDebugObjects);
}


//! Finds the collision point of a line and lots of triangles, if there is one.
bool CSceneCollisionManager::getCollisionPoint(const core::line3d<f32>& ray,
		ITriangleSelector* selector, core::vector3df& outIntersection,
		core::triangle3df& outTriangle,
		ISceneNode*& outNode)
{
	if (!selector)
	{
		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return false;
	}

	s32 totalcnt = selector->getTriangleCount();
	if ( totalcnt <= 0 )
		return false;

	Triangles.set_used(totalcnt);

	s32 cnt = 0;
	selector->getTriangles(Triangles.pointer(), totalcnt, cnt, ray);

	const core::vector3df linevect = ray.getVector().normalize();
	core::vector3df intersection;
	f32 nearest = FLT_MAX;
	bool found = false;
	const f32 raylength = ray.getLengthSQ();

	const f32 minX = core::min_(ray.start.X, ray.end.X);
	const f32 maxX = core::max_(ray.start.X, ray.end.X);
	const f32 minY = core::min_(ray.start.Y, ray.end.Y);
	const f32 maxY = core::max_(ray.start.Y, ray.end.Y);
	const f32 minZ = core::min_(ray.start.Z, ray.end.Z);
	const f32 maxZ = core::max_(ray.start.Z, ray.end.Z);

	for (s32 i=0; i<cnt; ++i)
	{
		const core::triangle3df & triangle = Triangles[i];

		if(minX > triangle.pointA.X && minX > triangle.pointB.X && minX > triangle.pointC.X)
			continue;
		if(maxX < triangle.pointA.X && maxX < triangle.pointB.X && maxX < triangle.pointC.X)
			continue;
		if(minY > triangle.pointA.Y && minY > triangle.pointB.Y && minY > triangle.pointC.Y)
			continue;
		if(maxY < triangle.pointA.Y && maxY < triangle.pointB.Y && maxY < triangle.pointC.Y)
			continue;
		if(minZ > triangle.pointA.Z && minZ > triangle.pointB.Z && minZ > triangle.pointC.Z)
			continue;
		if(maxZ < triangle.pointA.Z && maxZ < triangle.pointB.Z && maxZ < triangle.pointC.Z)
			continue;

		if (triangle.getIntersectionWithLine(ray.start, linevect, intersection))
		{
			const f32 tmp = intersection.getDistanceFromSQ(ray.start);
			const f32 tmp2 = intersection.getDistanceFromSQ(ray.end);

			if (tmp < raylength && tmp2 < raylength && tmp < nearest)
			{
				nearest = tmp;
				outTriangle = triangle;
				outIntersection = intersection;
				outNode = selector->getSceneNodeForTriangle(i);
				found = true;
			}
		}
	}

	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return found;
}


//! Collides a moving ellipsoid with a 3d world with gravity and returns
//! the resulting new position of the ellipsoid.
core::vector3df CSceneCollisionManager::getCollisionResultPosition(
		ITriangleSelector* selector,
		const core::vector3df &position, const core::vector3df& radius,
		const core::vector3df& direction,
		core::triangle3df& triout,
		core::vector3df& hitPosition,
		bool& outFalling,
		ISceneNode*& outNode,
		f32 slidingSpeed,
		const core::vector3df& gravity)
{
	return collideEllipsoidWithWorld(selector, position,
		radius, direction, slidingSpeed, gravity, triout, hitPosition, outFalling, outNode);
}


bool CSceneCollisionManager::testTriangleIntersection(SCollisionData* colData,
			const core::triangle3df& triangle)
{
	const core::plane3d<f32> trianglePlane = triangle.getPlane();

	// only check front facing polygons
	if ( !trianglePlane.isFrontFacing(colData->normalizedVelocity) )
		return false;

	// get interval of plane intersection

	f32 t1, t0;
	bool embeddedInPlane = false;

	// calculate signed distance from sphere position to triangle plane
	f32 signedDistToTrianglePlane = trianglePlane.getDistanceTo(
		colData->basePoint);

	f32 normalDotVelocity =
		trianglePlane.Normal.dotProduct(colData->velocity);

	if ( core::iszero ( normalDotVelocity ) )
	{
		// sphere is traveling parallel to plane

		if (fabs(signedDistToTrianglePlane) >= 1.0f)
			return false; // no collision possible
		else
		{
			// sphere is embedded in plane
			embeddedInPlane = true;
			t0 = 0.0;
			t1 = 1.0;
		}
	}
	else
	{
		normalDotVelocity = core::reciprocal ( normalDotVelocity );

		// N.D is not 0. Calculate intersection interval
		t0 = (-1.f - signedDistToTrianglePlane) * normalDotVelocity;
		t1 = (1.f - signedDistToTrianglePlane) * normalDotVelocity;

		// Swap so t0 < t1
		if (t0 > t1) { f32 tmp = t1; t1 = t0; t0 = tmp;	}

		// check if at least one value is within the range
		if (t0 > 1.0f || t1 < 0.0f)
			return false; // both t values are outside 1 and 0, no collision possible

		// clamp to 0 and 1
		t0 = core::clamp ( t0, 0.f, 1.f );
		t1 = core::clamp ( t1, 0.f, 1.f );
	}

	// at this point we have t0 and t1, if there is any intersection, it
	// is between this interval
	core::vector3df collisionPoint;
	bool foundCollision = false;
	f32 t = 1.0f;

	// first check the easy case: Collision within the triangle;
	// if this happens, it must be at t0 and this is when the sphere
	// rests on the front side of the triangle plane. This can only happen
	// if the sphere is not embedded in the triangle plane.

	if (!embeddedInPlane)
	{
		core::vector3df planeIntersectionPoint =
			(colData->basePoint - trianglePlane.Normal)
			+ (colData->velocity * t0);

		if (triangle.isPointInside(planeIntersectionPoint))
		{
			foundCollision = true;
			t = t0;
			collisionPoint = planeIntersectionPoint;
		}
	}

	// if we havent found a collision already we will have to sweep
	// the sphere against points and edges of the triangle. Note: A
	// collision inside the triangle will always happen before a
	// vertex or edge collision.

	if (!foundCollision)
	{
		core::vector3df velocity = colData->velocity;
		core::vector3df base = colData->basePoint;

		f32 velocitySqaredLength = velocity.getLengthSQ();
		f32 a,b,c;
		f32 newT;

		// for each edge or vertex a quadratic equation has to be solved:
		// a*t^2 + b*t + c = 0. We calculate a,b, and c for each test.

		// check against points
		a = velocitySqaredLength;

		// p1
		b = 2.0f * (velocity.dotProduct(base - triangle.pointA));
		c = (triangle.pointA-base).getLengthSQ() - 1.f;
		if (getLowestRoot(a,b,c,t, &newT))
		{
			t = newT;
			foundCollision = true;
			collisionPoint = triangle.pointA;
		}

		// p2
		if (!foundCollision)
		{
			b = 2.0f * (velocity.dotProduct(base - triangle.pointB));
			c = (triangle.pointB-base).getLengthSQ() - 1.f;
			if (getLowestRoot(a,b,c,t, &newT))
			{
				t = newT;
				foundCollision = true;
				collisionPoint = triangle.pointB;
			}
		}

		// p3
		if (!foundCollision)
		{
			b = 2.0f * (velocity.dotProduct(base - triangle.pointC));
			c = (triangle.pointC-base).getLengthSQ() - 1.f;
			if (getLowestRoot(a,b,c,t, &newT))
			{
				t = newT;
				foundCollision = true;
				collisionPoint = triangle.pointC;
			}
		}

		// check against edges:

		// p1 --- p2
		core::vector3df edge = triangle.pointB - triangle.pointA;
		core::vector3df baseToVertex = triangle.pointA - base;
		f32 edgeSqaredLength = edge.getLengthSQ();
		f32 edgeDotVelocity = edge.dotProduct(velocity);
		f32 edgeDotBaseToVertex = edge.dotProduct(baseToVertex);

		// calculate parameters for equation
		a = edgeSqaredLength* -velocitySqaredLength +
			edgeDotVelocity*edgeDotVelocity;
		b = edgeSqaredLength* (2.f *velocity.dotProduct(baseToVertex)) -
			2.0f*edgeDotVelocity*edgeDotBaseToVertex;
		c = edgeSqaredLength* (1.f -baseToVertex.getLengthSQ()) +
			edgeDotBaseToVertex*edgeDotBaseToVertex;

		// does the swept sphere collide against infinite edge?
		if (getLowestRoot(a,b,c,t,&newT))
		{
			f32 f = (edgeDotVelocity*newT - edgeDotBaseToVertex) / edgeSqaredLength;
			if (f >=0.0f && f <= 1.0f)
			{
				// intersection took place within segment
				t = newT;
				foundCollision = true;
				collisionPoint = triangle.pointA + (edge*f);
			}
		}

		// p2 --- p3
		edge = triangle.pointC-triangle.pointB;
		baseToVertex = triangle.pointB - base;
		edgeSqaredLength = edge.getLengthSQ();
		edgeDotVelocity = edge.dotProduct(velocity);
		edgeDotBaseToVertex = edge.dotProduct(baseToVertex);

		// calculate parameters for equation
		a = edgeSqaredLength* -velocitySqaredLength +
			edgeDotVelocity*edgeDotVelocity;
		b = edgeSqaredLength* (2*velocity.dotProduct(baseToVertex)) -
			2.0f*edgeDotVelocity*edgeDotBaseToVertex;
		c = edgeSqaredLength* (1-baseToVertex.getLengthSQ()) +
			edgeDotBaseToVertex*edgeDotBaseToVertex;

		// does the swept sphere collide against infinite edge?
		if (getLowestRoot(a,b,c,t,&newT))
		{
			f32 f = (edgeDotVelocity*newT-edgeDotBaseToVertex) /
				edgeSqaredLength;
			if (f >=0.0f && f <= 1.0f)
			{
				// intersection took place within segment
				t = newT;
				foundCollision = true;
				collisionPoint = triangle.pointB + (edge*f);
			}
		}


		// p3 --- p1
		edge = triangle.pointA-triangle.pointC;
		baseToVertex = triangle.pointC - base;
		edgeSqaredLength = edge.getLengthSQ();
		edgeDotVelocity = edge.dotProduct(velocity);
		edgeDotBaseToVertex = edge.dotProduct(baseToVertex);

		// calculate parameters for equation
		a = edgeSqaredLength* -velocitySqaredLength +
			edgeDotVelocity*edgeDotVelocity;
		b = edgeSqaredLength* (2*velocity.dotProduct(baseToVertex)) -
			2.0f*edgeDotVelocity*edgeDotBaseToVertex;
		c = edgeSqaredLength* (1-baseToVertex.getLengthSQ()) +
			edgeDotBaseToVertex*edgeDotBaseToVertex;

		// does the swept sphere collide against infinite edge?
		if (getLowestRoot(a,b,c,t,&newT))
		{
			f32 f = (edgeDotVelocity*newT-edgeDotBaseToVertex) /
				edgeSqaredLength;
			if (f >=0.0f && f <= 1.0f)
			{
				// intersection took place within segment
				t = newT;
				foundCollision = true;
				collisionPoint = triangle.pointC + (edge*f);
			}
		}
	}// end no collision found

	// set result:
	if (foundCollision)
	{
		// distance to collision is t
		f32 distToCollision = t*colData->velocity.getLength();

		// does this triangle qualify for closest hit?
		if (!colData->foundCollision ||
			distToCollision	< colData->nearestDistance)
		{
			colData->nearestDistance = distToCollision;
			colData->intersectionPoint = collisionPoint;
			colData->foundCollision = true;
			colData->intersectionTriangle = triangle;
			++colData->triangleHits;
			return true;
		}
	}// end found collision

	return false;
}


//! Collides a moving ellipsoid with a 3d world with gravity and returns
//! the resulting new position of the ellipsoid.
core::vector3df CSceneCollisionManager::collideEllipsoidWithWorld(
		ITriangleSelector* selector, const core::vector3df &position,
		const core::vector3df& radius,  const core::vector3df& velocity,
		f32 slidingSpeed,
		const core::vector3df& gravity,
		core::triangle3df& triout,
		core::vector3df& hitPosition,
		bool& outFalling,
		ISceneNode*& outNode)
{
	if (!selector || radius.X == 0.0f || radius.Y == 0.0f || radius.Z == 0.0f)
		return position;

	// This code is based on the paper "Improved Collision detection and Response"
	// by Kasper Fauerby, but some parts are modified.

	SCollisionData colData;
	colData.R3Position = position;
	colData.R3Velocity = velocity;
	colData.eRadius = radius;
	colData.nearestDistance = FLT_MAX;
	colData.selector = selector;
	colData.slidingSpeed = slidingSpeed;
	colData.triangleHits = 0;
	colData.triangleIndex = -1;

	core::vector3df eSpacePosition = colData.R3Position / colData.eRadius;
	core::vector3df eSpaceVelocity = colData.R3Velocity / colData.eRadius;

	// iterate until we have our final position

	core::vector3df finalPos = collideWithWorld(
		0, colData, eSpacePosition, eSpaceVelocity);

	outFalling = false;

	// add gravity

	if (gravity != core::vector3df(0,0,0))
	{
		colData.R3Position = finalPos * colData.eRadius;
		colData.R3Velocity = gravity;
		colData.triangleHits = 0;

		eSpaceVelocity = gravity/colData.eRadius;

		finalPos = collideWithWorld(0, colData,
			finalPos, eSpaceVelocity);

		outFalling = (colData.triangleHits == 0);
	}

	if (colData.triangleHits)
	{
		triout = colData.intersectionTriangle;
		triout.pointA *= colData.eRadius;
		triout.pointB *= colData.eRadius;
		triout.pointC *= colData.eRadius;
		outNode = selector->getSceneNodeForTriangle(colData.triangleIndex);
	}

	finalPos *= colData.eRadius;
	hitPosition = colData.intersectionPoint * colData.eRadius;
	return finalPos;
}


core::vector3df CSceneCollisionManager::collideWithWorld(s32 recursionDepth,
	SCollisionData &colData, core::vector3df pos, core::vector3df vel)
{
	f32 veryCloseDistance = colData.slidingSpeed;

	if (recursionDepth > 5)
		return pos;

	colData.velocity = vel;
	colData.normalizedVelocity = vel;
	colData.normalizedVelocity.normalize();
	colData.basePoint = pos;
	colData.foundCollision = false;
	colData.nearestDistance = FLT_MAX;

	//------------------ collide with world

	// get all triangles with which we might collide
	core::aabbox3d<f32> box(colData.R3Position);
	box.addInternalPoint(colData.R3Position + colData.R3Velocity);
	box.MinEdge -= colData.eRadius;
	box.MaxEdge += colData.eRadius;

	s32 totalTriangleCnt = colData.selector->getTriangleCount();
	Triangles.set_used(totalTriangleCnt);

	core::matrix4 scaleMatrix;
	scaleMatrix.setScale(
			core::vector3df(1.0f / colData.eRadius.X,
					1.0f / colData.eRadius.Y,
					1.0f / colData.eRadius.Z));

	s32 triangleCnt = 0;
	colData.selector->getTriangles(Triangles.pointer(), totalTriangleCnt, triangleCnt, box, &scaleMatrix);

	for (s32 i=0; i<triangleCnt; ++i)
		if(testTriangleIntersection(&colData, Triangles[i]))
			colData.triangleIndex = i;

	//---------------- end collide with world

	if (!colData.foundCollision)
		return pos + vel;

	// original destination point
	const core::vector3df destinationPoint = pos + vel;
	core::vector3df newBasePoint = pos;

	// only update if we are not already very close
	// and if so only move very close to intersection, not to the
	// exact point
	if (colData.nearestDistance >= veryCloseDistance)
	{
		core::vector3df v = vel;
		v.setLength( colData.nearestDistance - veryCloseDistance );
		newBasePoint = colData.basePoint + v;

		v.normalize();
		colData.intersectionPoint -= (v * veryCloseDistance);
	}

	// calculate sliding plane

	const core::vector3df slidePlaneOrigin = colData.intersectionPoint;
	const core::vector3df slidePlaneNormal = (newBasePoint - colData.intersectionPoint).normalize();
	core::plane3d<f32> slidingPlane(slidePlaneOrigin, slidePlaneNormal);

	core::vector3df newDestinationPoint =
		destinationPoint -
		(slidePlaneNormal * slidingPlane.getDistanceTo(destinationPoint));

	// generate slide vector

	const core::vector3df newVelocityVector = newDestinationPoint -
		colData.intersectionPoint;

	if (newVelocityVector.getLength() < veryCloseDistance)
		return newBasePoint;

	return collideWithWorld(recursionDepth+1, colData,
		newBasePoint, newVelocityVector);
}


//! Returns a 3d ray which would go through the 2d screen coodinates.
core::line3d<f32> CSceneCollisionManager::getRayFromScreenCoordinates(
	const core::position2d<s32> & pos, ICameraSceneNode* camera)
{
	core::line3d<f32> ln(0,0,0,0,0,0);

	if (!SceneManager)
		return ln;

	if (!camera)
		camera = SceneManager->getActiveCamera();

	if (!camera)
		return ln;

	const scene::SViewFrustum* f = camera->getViewFrustum();

	core::vector3df farLeftUp = f->getFarLeftUp();
	core::vector3df lefttoright = f->getFarRightUp() - farLeftUp;
	core::vector3df uptodown = f->getFarLeftDown() - farLeftUp;

	const core::rect<s32>& viewPort = Driver->getViewPort();
	core::dimension2d<u32> screenSize(viewPort.getWidth(), viewPort.getHeight());

	f32 dx = pos.X / (f32)screenSize.Width;
	f32 dy = pos.Y / (f32)screenSize.Height;

	if (camera->isOrthogonal())
		ln.start = f->cameraPosition + (lefttoright * (dx-0.5f)) + (uptodown * (dy-0.5f));
	else
		ln.start = f->cameraPosition;

	ln.end = farLeftUp + (lefttoright * dx) + (uptodown * dy);

	return ln;
}


//! Calculates 2d screen position from a 3d position.
core::position2d<s32> CSceneCollisionManager::getScreenCoordinatesFrom3DPosition(
	const core::vector3df & pos3d, ICameraSceneNode* camera, bool useViewPort)
{
	if (!SceneManager || !Driver)
		return core::position2d<s32>(-1000,-1000);

	if (!camera)
		camera = SceneManager->getActiveCamera();

	if (!camera)
		return core::position2d<s32>(-1000,-1000);

	core::dimension2d<u32> dim;
	if (useViewPort)
		dim.set(Driver->getViewPort().getWidth(), Driver->getViewPort().getHeight());
	else
		dim=(Driver->getCurrentRenderTargetSize());

	dim.Width /= 2;
	dim.Height /= 2;

	core::matrix4 trans = camera->getProjectionMatrix();
	trans *= camera->getViewMatrix();

	f32 transformedPos[4] = { pos3d.X, pos3d.Y, pos3d.Z, 1.0f };

	trans.multiplyWith1x4Matrix(transformedPos);

	if (transformedPos[3] < 0)
		return core::position2d<s32>(-10000,-10000);

	const f32 zDiv = transformedPos[3] == 0.0f ? 1.0f :
		core::reciprocal(transformedPos[3]);

	return core::position2d<s32>(
			dim.Width + core::round32(dim.Width * (transformedPos[0] * zDiv)),
			dim.Height - core::round32(dim.Height * (transformedPos[1] * zDiv)));
}


inline bool CSceneCollisionManager::getLowestRoot(f32 a, f32 b, f32 c, f32 maxR, f32* root)
{
	// check if solution exists
	const f32 determinant = b*b - 4.0f*a*c;

	// if determinant is negative, no solution
	if (determinant < 0.0f || a == 0.f )
		return false;

	// calculate two roots: (if det==0 then x1==x2
	// but lets disregard that slight optimization)

	const f32 sqrtD = sqrtf(determinant);
	const f32 invDA = core::reciprocal(2*a);
	f32 r1 = (-b - sqrtD) * invDA;
	f32 r2 = (-b + sqrtD) * invDA;

	// sort so x1 <= x2
	if (r1 > r2)
		core::swap(r1,r2);

	// get lowest root
	if (r1 > 0 && r1 < maxR)
	{
		*root = r1;
		return true;
	}

	// its possible that we want x2, this can happen if x1 < 0
	if (r2 > 0 && r2 < maxR)
	{
		*root = r2;
		return true;
	}

	return false;
}


} // end namespace scene
} // end namespace irr


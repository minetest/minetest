// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CSceneNodeAnimatorFlyCircle.h"

namespace irr
{
namespace scene
{


//! constructor
CSceneNodeAnimatorFlyCircle::CSceneNodeAnimatorFlyCircle(u32 time,
		const core::vector3df& center, f32 radius, f32 speed,
		const core::vector3df& direction, f32 radiusEllipsoid)
	: Center(center), Direction(direction), Radius(radius),
	RadiusEllipsoid(radiusEllipsoid), Speed(speed), StartTime(time)
{
	#ifdef _DEBUG
	setDebugName("CSceneNodeAnimatorFlyCircle");
	#endif
	init();
}


void CSceneNodeAnimatorFlyCircle::init()
{
	Direction.normalize();

	if (Direction.Y != 0)
		VecV = core::vector3df(50,0,0).crossProduct(Direction).normalize();
	else
		VecV = core::vector3df(0,50,0).crossProduct(Direction).normalize();
	VecU = VecV.crossProduct(Direction).normalize();
}


//! animates a scene node
void CSceneNodeAnimatorFlyCircle::animateNode(ISceneNode* node, u32 timeMs)
{
	if ( 0 == node )
		return;

	f32 time;

	// Check for the condition where the StartTime is in the future.
	if(StartTime > timeMs)
		time = ((s32)timeMs - (s32)StartTime) * Speed;
	else
		time = (timeMs-StartTime) * Speed;

//	node->setPosition(Center + Radius * ((VecU*cosf(time)) + (VecV*sinf(time))));
	f32 r2 = RadiusEllipsoid == 0.f ? Radius : RadiusEllipsoid;
	node->setPosition(Center + (Radius*cosf(time)*VecU) + (r2*sinf(time)*VecV ) );
}


//! Writes attributes of the scene node animator.
void CSceneNodeAnimatorFlyCircle::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	out->addVector3d("Center", Center);
	out->addFloat("Radius", Radius);
	out->addFloat("Speed", Speed);
	out->addVector3d("Direction", Direction);
	out->addFloat("RadiusEllipsoid", RadiusEllipsoid);
}


//! Reads attributes of the scene node animator.
void CSceneNodeAnimatorFlyCircle::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	Center = in->getAttributeAsVector3d("Center");
	Radius = in->getAttributeAsFloat("Radius");
	Speed = in->getAttributeAsFloat("Speed");
	Direction = in->getAttributeAsVector3d("Direction");
	StartTime = 0;
	
	if (Direction.equals(core::vector3df(0,0,0)))
		Direction.set(0,1,0); // irrlicht 1.1 backwards compatibility
	else
		Direction.normalize();

	RadiusEllipsoid = in->getAttributeAsFloat("RadiusEllipsoid");
	init();
}


ISceneNodeAnimator* CSceneNodeAnimatorFlyCircle::createClone(ISceneNode* node, ISceneManager* newManager)
{
	CSceneNodeAnimatorFlyCircle * newAnimator = 
		new CSceneNodeAnimatorFlyCircle(StartTime, Center, Radius, Speed, Direction, RadiusEllipsoid);

	return newAnimator;
}


} // end namespace scene
} // end namespace irr


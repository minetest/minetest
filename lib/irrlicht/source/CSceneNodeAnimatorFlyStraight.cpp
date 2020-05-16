// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CSceneNodeAnimatorFlyStraight.h"

namespace irr
{
namespace scene
{


//! constructor
CSceneNodeAnimatorFlyStraight::CSceneNodeAnimatorFlyStraight(const core::vector3df& startPoint,
				const core::vector3df& endPoint, u32 timeForWay,
				bool loop, u32 now, bool pingpong)
: ISceneNodeAnimatorFinishing(now + timeForWay),
	Start(startPoint), End(endPoint), TimeFactor(0.0f), StartTime(now),
	TimeForWay(timeForWay), Loop(loop), PingPong(pingpong)
{
	#ifdef _DEBUG
	setDebugName("CSceneNodeAnimatorFlyStraight");
	#endif

	recalculateIntermediateValues();
}


void CSceneNodeAnimatorFlyStraight::recalculateIntermediateValues()
{
	Vector = End - Start;
	TimeFactor = (f32)Vector.getLength() / TimeForWay;
	Vector.normalize();
}


//! animates a scene node
void CSceneNodeAnimatorFlyStraight::animateNode(ISceneNode* node, u32 timeMs)
{
	if (!node)
		return;

	u32 t = (timeMs-StartTime);

	core::vector3df pos;

	if (!Loop && !PingPong && t >= TimeForWay)
	{
		pos = End;
		HasFinished = true;
	}
	else if (!Loop && PingPong && t >= TimeForWay * 2.f )
	{
		pos = Start;
		HasFinished = true;
	}
	else
	{
		f32 phase = fmodf( (f32) t, (f32) TimeForWay );
		core::vector3df rel = Vector * phase * TimeFactor;
		const bool pong = PingPong && fmodf( (f32) t, (f32) TimeForWay*2.f ) >= TimeForWay;

		if ( !pong )
		{
			pos += Start + rel;
		}
		else
		{
			pos = End - rel;
		}
	}

	node->setPosition(pos);
}


//! Writes attributes of the scene node animator.
void CSceneNodeAnimatorFlyStraight::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	out->addVector3d("Start", Start);
	out->addVector3d("End", End);
	out->addInt("TimeForWay", TimeForWay);
	out->addBool("Loop", Loop);
	out->addBool("PingPong", PingPong);
}


//! Reads attributes of the scene node animator.
void CSceneNodeAnimatorFlyStraight::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	Start = in->getAttributeAsVector3d("Start");
	End = in->getAttributeAsVector3d("End");
	TimeForWay = in->getAttributeAsInt("TimeForWay");
	Loop = in->getAttributeAsBool("Loop");
	PingPong = in->getAttributeAsBool("PingPong");

	recalculateIntermediateValues();
}


ISceneNodeAnimator* CSceneNodeAnimatorFlyStraight::createClone(ISceneNode* node, ISceneManager* newManager)
{
	CSceneNodeAnimatorFlyStraight * newAnimator =
		new CSceneNodeAnimatorFlyStraight(Start, End, TimeForWay, Loop, StartTime, PingPong);

	return newAnimator;
}


} // end namespace scene
} // end namespace irr


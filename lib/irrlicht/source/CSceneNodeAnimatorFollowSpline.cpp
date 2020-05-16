// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CSceneNodeAnimatorFollowSpline.h"

namespace irr
{
namespace scene
{


//! constructor
CSceneNodeAnimatorFollowSpline::CSceneNodeAnimatorFollowSpline(u32 time,
	const core::array<core::vector3df>& points, f32 speed,
	f32 tightness, bool loop, bool pingpong)
: ISceneNodeAnimatorFinishing(0), Points(points), Speed(speed), Tightness(tightness), StartTime(time)
, Loop(loop), PingPong(pingpong)
{
	#ifdef _DEBUG
	setDebugName("CSceneNodeAnimatorFollowSpline");
	#endif
}


inline s32 CSceneNodeAnimatorFollowSpline::clamp(s32 idx, s32 size)
{
	return ( idx<0 ? size+idx : ( idx>=size ? idx-size : idx ) );
}


//! animates a scene node
void CSceneNodeAnimatorFollowSpline::animateNode(ISceneNode* node, u32 timeMs)
{
	if(!node)
		return;

	const u32 pSize = Points.size();
	if (pSize==0)
	{
		if ( !Loop )
			HasFinished = true;
		return;
	}
	if (pSize==1)
	{
		if ( timeMs > StartTime )
		{
			node->setPosition(Points[0]);
			if ( !Loop )
				HasFinished = true;
		}
		return;
	}

	const f32 dt = ( (timeMs-StartTime) * Speed * 0.001f );
	const s32 unwrappedIdx = core::floor32( dt );
	if ( !Loop && unwrappedIdx >= (s32)pSize-1 )
	{
		node->setPosition(Points[pSize-1]);
		HasFinished = true;
		return;
	}
	const bool pong = PingPong && (unwrappedIdx/(pSize-1))%2;
	const f32 u =  pong ? 1.f-core::fract ( dt ) : core::fract ( dt );
	const s32 idx = pong ?	(pSize-2) - (unwrappedIdx % (pSize-1))
						: (PingPong ? unwrappedIdx % (pSize-1)
									: unwrappedIdx % pSize);
	//const f32 u = 0.001f * fmodf( dt, 1000.0f );

	const core::vector3df& p0 = Points[ clamp( idx - 1, pSize ) ];
	const core::vector3df& p1 = Points[ clamp( idx + 0, pSize ) ]; // starting point
	const core::vector3df& p2 = Points[ clamp( idx + 1, pSize ) ]; // end point
	const core::vector3df& p3 = Points[ clamp( idx + 2, pSize ) ];

	// hermite polynomials
	const f32 h1 = 2.0f * u * u * u - 3.0f * u * u + 1.0f;
	const f32 h2 = -2.0f * u * u * u + 3.0f * u * u;
	const f32 h3 = u * u * u - 2.0f * u * u + u;
	const f32 h4 = u * u * u - u * u;

	// tangents
	const core::vector3df t1 = ( p2 - p0 ) * Tightness;
	const core::vector3df t2 = ( p3 - p1 ) * Tightness;

	// interpolated point
	node->setPosition(p1 * h1 + p2 * h2 + t1 * h3 + t2 * h4);
}


//! Writes attributes of the scene node animator.
void CSceneNodeAnimatorFollowSpline::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	out->addFloat("Speed", Speed);
	out->addFloat("Tightness", Tightness);
	out->addBool("Loop", Loop);
	out->addBool("PingPong", PingPong);

	u32 count = Points.size();

	if ( options && (options->Flags & io::EARWF_FOR_EDITOR))
	{
		// add one point in addition when serializing for editors
		// to make it easier to add points quickly
		count += 1;
	}

	for (u32 i=0; i<count; ++i)
	{
		core::stringc tname = "Point";
		tname += (int)(i+1);

		out->addVector3d(tname.c_str(), i<Points.size() ? Points[i] : core::vector3df(0,0,0) );
	}
}


//! Reads attributes of the scene node animator.
void CSceneNodeAnimatorFollowSpline::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	Speed = in->getAttributeAsFloat("Speed");
	Tightness = in->getAttributeAsFloat("Tightness");
	Loop = in->getAttributeAsBool("Loop");
	PingPong = in->getAttributeAsBool("PingPong");
	Points.clear();

	for(u32 i=1; true; ++i)
	{
		core::stringc pname = "Point";
		pname += i;

		if (in->existsAttribute(pname.c_str()))
			Points.push_back(in->getAttributeAsVector3d(pname.c_str()));
		else
			break;
	}

	// remove last point if double entry from editor
	if ( options && (options->Flags & io::EARWF_FOR_EDITOR) &&
		Points.size() > 2 && Points.getLast() == core::vector3df(0,0,0))
	{
		Points.erase(Points.size()-1);

		if (Points.size() > 2 && Points.getLast() == core::vector3df(0,0,0))
			Points.erase(Points.size()-1);
	}
}


ISceneNodeAnimator* CSceneNodeAnimatorFollowSpline::createClone(ISceneNode* node, ISceneManager* newManager)
{
	CSceneNodeAnimatorFollowSpline * newAnimator =
		new CSceneNodeAnimatorFollowSpline(StartTime, Points, Speed, Tightness);

	return newAnimator;
}


} // end namespace scene
} // end namespace irr


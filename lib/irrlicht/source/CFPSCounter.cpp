// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CFPSCounter.h"
#include "irrMath.h"

namespace irr
{
namespace video  
{


CFPSCounter::CFPSCounter()
:	FPS(60), Primitive(0), StartTime(0), FramesCounted(0),
	PrimitivesCounted(0), PrimitiveAverage(0), PrimitiveTotal(0)
{

}


//! returns current fps
s32 CFPSCounter::getFPS() const
{
	return FPS;
}


//! returns current primitive count
u32 CFPSCounter::getPrimitive() const
{
	return Primitive;
}


//! returns average primitive count of last period
u32 CFPSCounter::getPrimitiveAverage() const
{
	return PrimitiveAverage;
}


//! returns accumulated primitive count since start
u32 CFPSCounter::getPrimitiveTotal() const
{
	return PrimitiveTotal;
}


//! to be called every frame
void CFPSCounter::registerFrame(u32 now, u32 primitivesDrawn)
{
	++FramesCounted;
	PrimitiveTotal += primitivesDrawn;
	PrimitivesCounted += primitivesDrawn;
	Primitive = primitivesDrawn;

	const u32 milliseconds = now - StartTime;

	if (milliseconds >= 1500 )
	{
		const f32 invMilli = core::reciprocal ( (f32) milliseconds );
		
		FPS = core::ceil32 ( ( 1000 * FramesCounted ) * invMilli );
		PrimitiveAverage = core::ceil32 ( ( 1000 * PrimitivesCounted ) * invMilli );

		FramesCounted = 0;
		PrimitivesCounted = 0;
		StartTime = now;
	}
}


} // end namespace video
} // end namespace irr


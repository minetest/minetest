// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CParticlePointEmitter.h"
#include "os.h"
#include "IAttributes.h"

namespace irr
{
namespace scene
{

//! constructor
CParticlePointEmitter::CParticlePointEmitter(
	const core::vector3df& direction, u32 minParticlesPerSecond,
	u32 maxParticlesPerSecond, video::SColor minStartColor,
	video::SColor maxStartColor, u32 lifeTimeMin, u32 lifeTimeMax,
	s32 maxAngleDegrees,
	const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize)
 : Direction(direction),
	MinStartSize(minStartSize), MaxStartSize(maxStartSize),
	MinParticlesPerSecond(minParticlesPerSecond),
	MaxParticlesPerSecond(maxParticlesPerSecond),
	MinStartColor(minStartColor), MaxStartColor(maxStartColor),
	MinLifeTime(lifeTimeMin), MaxLifeTime(lifeTimeMax),
	MaxAngleDegrees(maxAngleDegrees), Time(0), Emitted(0)
{
	#ifdef _DEBUG
	setDebugName("CParticlePointEmitter");
	#endif
}


//! Prepares an array with new particles to emitt into the system
//! and returns how much new particles there are.
s32 CParticlePointEmitter::emitt(u32 now, u32 timeSinceLastCall, SParticle*& outArray)
{
	Time += timeSinceLastCall;

	const u32 pps = (MaxParticlesPerSecond - MinParticlesPerSecond);
	const f32 perSecond = pps ? ((f32)MinParticlesPerSecond + os::Randomizer::frand() * pps) : MinParticlesPerSecond;
	const f32 everyWhatMillisecond = 1000.0f / perSecond;

	if (Time > everyWhatMillisecond)
	{
		Time = 0;
		Particle.startTime = now;
		Particle.vector = Direction;

		if (MaxAngleDegrees)
		{
			core::vector3df tgt = Direction;
			tgt.rotateXYBy(os::Randomizer::frand() * MaxAngleDegrees);
			tgt.rotateYZBy(os::Randomizer::frand() * MaxAngleDegrees);
			tgt.rotateXZBy(os::Randomizer::frand() * MaxAngleDegrees);
			Particle.vector = tgt;
		}

		Particle.endTime = now + MinLifeTime;
		if (MaxLifeTime != MinLifeTime)
			Particle.endTime += os::Randomizer::rand() % (MaxLifeTime - MinLifeTime);

		if (MinStartColor==MaxStartColor)
			Particle.color=MinStartColor;
		else
			Particle.color = MinStartColor.getInterpolated(MaxStartColor, os::Randomizer::frand());

		Particle.startColor = Particle.color;
		Particle.startVector = Particle.vector;

		if (MinStartSize==MaxStartSize)
			Particle.startSize = MinStartSize;
		else
			Particle.startSize = MinStartSize.getInterpolated(MaxStartSize, os::Randomizer::frand());
		Particle.size = Particle.startSize;

		outArray = &Particle;
		return 1;
	}

	return 0;
}


//! Writes attributes of the object.
void CParticlePointEmitter::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	out->addVector3d("Direction", Direction);
	out->addFloat("MinStartSizeWidth", MinStartSize.Width);
	out->addFloat("MinStartSizeHeight", MinStartSize.Height);
	out->addFloat("MaxStartSizeWidth", MaxStartSize.Width);
	out->addFloat("MaxStartSizeHeight", MaxStartSize.Height); 
	out->addInt("MinParticlesPerSecond", MinParticlesPerSecond);
	out->addInt("MaxParticlesPerSecond", MaxParticlesPerSecond);
	out->addColor("MinStartColor", MinStartColor);
	out->addColor("MaxStartColor", MaxStartColor);
	out->addInt("MinLifeTime", MinLifeTime);
	out->addInt("MaxLifeTime", MaxLifeTime);
	out->addInt("MaxAngleDegrees", MaxAngleDegrees);
}

//! Reads attributes of the object.
void CParticlePointEmitter::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	Direction = in->getAttributeAsVector3d("Direction");
	if (Direction.getLength() == 0)
		Direction.set(0,0.01f,0);

	int idx = -1;
	idx = in->findAttribute("MinStartSizeWidth");
	if ( idx >= 0 )
		MinStartSize.Width = in->getAttributeAsFloat(idx);
	idx = in->findAttribute("MinStartSizeHeight");
	if ( idx >= 0 )
		MinStartSize.Height = in->getAttributeAsFloat(idx);
	idx = in->findAttribute("MaxStartSizeWidth");
	if ( idx >= 0 )
		MaxStartSize.Width = in->getAttributeAsFloat(idx);
	idx = in->findAttribute("MaxStartSizeHeight");
	if ( idx >= 0 )
		MaxStartSize.Height = in->getAttributeAsFloat(idx);

	MinParticlesPerSecond = in->getAttributeAsInt("MinParticlesPerSecond");
	MaxParticlesPerSecond = in->getAttributeAsInt("MaxParticlesPerSecond");

	MinParticlesPerSecond = core::max_(1u, MinParticlesPerSecond);
	MaxParticlesPerSecond = core::max_(MaxParticlesPerSecond, 1u);
	MaxParticlesPerSecond = core::min_(MaxParticlesPerSecond, 200u);
	MinParticlesPerSecond = core::min_(MinParticlesPerSecond, MaxParticlesPerSecond);

	MinStartColor = in->getAttributeAsColor("MinStartColor");
	MaxStartColor = in->getAttributeAsColor("MaxStartColor");
	MinLifeTime = in->getAttributeAsInt("MinLifeTime");
	MaxLifeTime = in->getAttributeAsInt("MaxLifeTime");
	MaxAngleDegrees = in->getAttributeAsInt("MaxAngleDegrees");

	MinLifeTime = core::max_(0u, MinLifeTime);
	MaxLifeTime = core::max_(MaxLifeTime, MinLifeTime);
	MinLifeTime = core::min_(MinLifeTime, MaxLifeTime);
}


} // end namespace scene
} // end namespace irr


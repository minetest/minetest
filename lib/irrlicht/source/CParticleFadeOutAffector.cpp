// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CParticleFadeOutAffector.h"
#include "IAttributes.h"
#include "os.h"

namespace irr
{
namespace scene
{

//! constructor
CParticleFadeOutAffector::CParticleFadeOutAffector(
	const video::SColor& targetColor, u32 fadeOutTime)
	: IParticleFadeOutAffector(), TargetColor(targetColor)
{

	#ifdef _DEBUG
	setDebugName("CParticleFadeOutAffector");
	#endif

	FadeOutTime = fadeOutTime ? static_cast<f32>(fadeOutTime) : 1.0f;
}


//! Affects an array of particles.
void CParticleFadeOutAffector::affect(u32 now, SParticle* particlearray, u32 count)
{
	if (!Enabled)
		return;
	f32 d;

	for (u32 i=0; i<count; ++i)
	{
		if (particlearray[i].endTime - now < FadeOutTime)
		{
			d = (particlearray[i].endTime - now) / FadeOutTime;	// FadeOutTime probably f32 to save casts here (just guessing)
			particlearray[i].color = particlearray[i].startColor.getInterpolated(
				TargetColor, d);
		}
	}
}


//! Writes attributes of the object.
//! Implement this to expose the attributes of your scene node animator for
//! scripting languages, editors, debuggers or xml serialization purposes.
void CParticleFadeOutAffector::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	out->addColor("TargetColor", TargetColor);
	out->addFloat("FadeOutTime", FadeOutTime);
}

//! Reads attributes of the object.
//! Implement this to set the attributes of your scene node animator for
//! scripting languages, editors, debuggers or xml deserialization purposes.
//! \param startIndex: start index where to start reading attributes.
//! \return: returns last index of an attribute read by this affector
void CParticleFadeOutAffector::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	TargetColor = in->getAttributeAsColor("TargetColor");
	FadeOutTime = in->getAttributeAsFloat("FadeOutTime");
}


} // end namespace scene
} // end namespace irr


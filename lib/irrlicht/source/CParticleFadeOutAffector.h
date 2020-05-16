// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_PARTICLE_FADE_OUT_AFFECTOR_H_INCLUDED__
#define __C_PARTICLE_FADE_OUT_AFFECTOR_H_INCLUDED__

#include "IParticleFadeOutAffector.h"
#include "SColor.h"

namespace irr
{
namespace scene
{

//! Particle Affector for fading out a color
class CParticleFadeOutAffector : public IParticleFadeOutAffector
{
public:

	CParticleFadeOutAffector(const video::SColor& targetColor, u32 fadeOutTime);

	//! Affects a particle.
	virtual void affect(u32 now, SParticle* particlearray, u32 count);

	//! Sets the targetColor, i.e. the color the particles will interpolate
	//! to over time.
	virtual void setTargetColor( const video::SColor& targetColor ) { TargetColor = targetColor; }

	//! Sets the amount of time it takes for each particle to fade out.
	virtual void setFadeOutTime( u32 fadeOutTime ) { FadeOutTime = fadeOutTime ? static_cast<f32>(fadeOutTime) : 1.0f; }

	//! Sets the targetColor, i.e. the color the particles will interpolate
	//! to over time.
	virtual const video::SColor& getTargetColor() const { return TargetColor; }

	//! Sets the amount of time it takes for each particle to fade out.
	virtual u32 getFadeOutTime() const { return static_cast<u32>(FadeOutTime); }

	//! Writes attributes of the object.
	//! Implement this to expose the attributes of your scene node animator for
	//! scripting languages, editors, debuggers or xml serialization purposes.
	virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;

	//! Reads attributes of the object.
	//! Implement this to set the attributes of your scene node animator for
	//! scripting languages, editors, debuggers or xml deserialization purposes.
	//! \param startIndex: start index where to start reading attributes.
	//! \return: returns last index of an attribute read by this affector
	virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);

private:

	video::SColor TargetColor;
	f32 FadeOutTime;
};

} // end namespace scene
} // end namespace irr


#endif


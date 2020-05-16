// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_PARTICLE_RING_EMITTER_H_INCLUDED__
#define __I_PARTICLE_RING_EMITTER_H_INCLUDED__

#include "IParticleEmitter.h"

namespace irr
{
namespace scene
{

//! A particle emitter which emits particles along a ring shaped area.
class IParticleRingEmitter : public IParticleEmitter
{
public:

	//! Set the center of the ring
	virtual void setCenter( const core::vector3df& center ) = 0;

	//! Set the radius of the ring
	virtual void setRadius( f32 radius ) = 0;

	//! Set the thickness of the ring
	virtual void setRingThickness( f32 ringThickness ) = 0;

	//! Get the center of the ring
	virtual const core::vector3df& getCenter() const = 0;

	//! Get the radius of the ring
	virtual f32 getRadius() const = 0;

	//! Get the thickness of the ring
	virtual f32 getRingThickness() const = 0;

	//! Get emitter type
	virtual E_PARTICLE_EMITTER_TYPE getType() const { return EPET_RING; }
};

} // end namespace scene
} // end namespace irr


#endif


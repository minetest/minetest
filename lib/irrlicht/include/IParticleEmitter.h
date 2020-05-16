// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_PARTICLE_EMITTER_H_INCLUDED__
#define __I_PARTICLE_EMITTER_H_INCLUDED__

#include "IAttributeExchangingObject.h"
#include "SParticle.h"

namespace irr
{
namespace scene
{

//! Types of built in particle emitters
enum E_PARTICLE_EMITTER_TYPE
{
	EPET_POINT = 0,
	EPET_ANIMATED_MESH,
	EPET_BOX,
	EPET_CYLINDER,
	EPET_MESH,
	EPET_RING,
	EPET_SPHERE,
	EPET_COUNT
};

//! Names for built in particle emitters
const c8* const ParticleEmitterTypeNames[] =
{
	"Point",
	"AnimatedMesh",
	"Box",
	"Cylinder",
	"Mesh",
	"Ring",
	"Sphere",
	0
};

//! A particle emitter for using with particle systems.
/** A Particle emitter emitts new particles into a particle system.
*/
class IParticleEmitter : public virtual io::IAttributeExchangingObject
{
public:

	//! Prepares an array with new particles to emitt into the system
	/** \param now Current time.
	\param timeSinceLastCall Time elapsed since last call, in milliseconds.
	\param outArray Pointer which will point to the array with the new
	particles to add into the system.
	\return Amount of new particles in the array. Can be 0. */
	virtual s32 emitt(u32 now, u32 timeSinceLastCall, SParticle*& outArray) = 0;

	//! Set direction the emitter emits particles
	virtual void setDirection( const core::vector3df& newDirection ) = 0;

	//! Set minimum number of particles the emitter emits per second
	virtual void setMinParticlesPerSecond( u32 minPPS ) = 0;

	//! Set maximum number of particles the emitter emits per second
	virtual void setMaxParticlesPerSecond( u32 maxPPS ) = 0;

	//! Set minimum starting color for particles
	virtual void setMinStartColor( const video::SColor& color ) = 0;

	//! Set maximum starting color for particles
	virtual void setMaxStartColor( const video::SColor& color ) = 0;

	//! Set the maximum starting size for particles
	virtual void setMaxStartSize( const core::dimension2df& size ) = 0;

	//! Set the minimum starting size for particles
	virtual void setMinStartSize( const core::dimension2df& size ) = 0;

	//! Set the minimum particle life-time in milliseconds
	virtual void setMinLifeTime( u32 lifeTimeMin ) = 0;

	//! Set the maximum particle life-time in milliseconds
	virtual void setMaxLifeTime( u32 lifeTimeMax ) = 0;

	//!	Set maximal random derivation from the direction
	virtual void setMaxAngleDegrees( s32 maxAngleDegrees ) = 0;

	//! Get direction the emitter emits particles
	virtual const core::vector3df& getDirection() const = 0;

	//! Get the minimum number of particles the emitter emits per second
	virtual u32 getMinParticlesPerSecond() const = 0;

	//! Get the maximum number of particles the emitter emits per second
	virtual u32 getMaxParticlesPerSecond() const = 0;

	//! Get the minimum starting color for particles
	virtual const video::SColor& getMinStartColor() const = 0;

	//! Get the maximum starting color for particles
	virtual const video::SColor& getMaxStartColor() const = 0;

	//! Get the maximum starting size for particles
	virtual const core::dimension2df& getMaxStartSize() const = 0;

	//! Get the minimum starting size for particles
	virtual const core::dimension2df& getMinStartSize() const = 0;

	//! Get the minimum particle life-time in milliseconds
	virtual u32 getMinLifeTime() const = 0;

	//! Get the maximum particle life-time in milliseconds
	virtual u32 getMaxLifeTime() const = 0;

	//!	Get maximal random derivation from the direction
	virtual s32 getMaxAngleDegrees() const = 0;


	//! Get emitter type
	virtual E_PARTICLE_EMITTER_TYPE getType() const { return EPET_POINT; }
};

typedef IParticleEmitter IParticlePointEmitter;

} // end namespace scene
} // end namespace irr


#endif


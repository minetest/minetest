// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_PARTICLE_GRAVITY_AFFECTOR_H_INCLUDED__
#define __C_PARTICLE_GRAVITY_AFFECTOR_H_INCLUDED__

#include "IParticleGravityAffector.h"
#include "SColor.h"

namespace irr
{
namespace scene
{

//! Particle Affector for affecting direction of particle
class CParticleGravityAffector : public IParticleGravityAffector
{
public:

	CParticleGravityAffector(
		const core::vector3df& gravity = core::vector3df(0.0f,-0.03f,0.0f),
		u32 timeForceLost = 1000);

	//! Affects a particle.
	virtual void affect(u32 now, SParticle* particlearray, u32 count);

	//! Set the time in milliseconds when the gravity force is totally
	//! lost and the particle does not move any more.
	virtual void setTimeForceLost( f32 timeForceLost ) { TimeForceLost = timeForceLost; }

	//! Set the direction and force of gravity.
	virtual void setGravity( const core::vector3df& gravity ) { Gravity = gravity; }

	//! Set the time in milliseconds when the gravity force is totally
	//! lost and the particle does not move any more.
	virtual f32 getTimeForceLost() const { return TimeForceLost; }

	//! Set the direction and force of gravity.
	virtual const core::vector3df& getGravity() const { return Gravity; }

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
	f32 TimeForceLost;
	core::vector3df Gravity;
};

} // end namespace scene
} // end namespace irr


#endif


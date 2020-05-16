// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_PARTICLE_ROTATION_AFFECTOR_H_INCLUDED__
#define __C_PARTICLE_ROTATION_AFFECTOR_H_INCLUDED__

#include "IParticleRotationAffector.h"

namespace irr
{
namespace scene
{

//! Particle Affector for rotating particles about a point
class CParticleRotationAffector : public IParticleRotationAffector
{
public:

	CParticleRotationAffector( const core::vector3df& speed = core::vector3df(5.0f, 5.0f, 5.0f),
		const core::vector3df& point = core::vector3df() );

	//! Affects a particle.
	virtual void affect(u32 now, SParticle* particlearray, u32 count);

	//! Set the point that particles will attract to
	virtual void setPivotPoint( const core::vector3df& point ) { PivotPoint = point; }

	//! Set the speed in degrees per second
	virtual void setSpeed( const core::vector3df& speed ) { Speed = speed; }

	//! Get the point that particles are attracted to
	virtual const core::vector3df& getPivotPoint() const { return PivotPoint; }

	//! Get the speed in degrees per second
	virtual const core::vector3df& getSpeed() const { return Speed; }

	//! Writes attributes of the object.
	virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;

	//! Reads attributes of the object.
	virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);

private:

	core::vector3df PivotPoint;
	core::vector3df Speed;
	u32 LastTime;
};

} // end namespace scene
} // end namespace irr


#endif // __C_PARTICLE_ROTATION_AFFECTOR_H_INCLUDED__


// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_PARTICLE_ATTRACTION_AFFECTOR_H_INCLUDED__
#define __C_PARTICLE_ATTRACTION_AFFECTOR_H_INCLUDED__

#include "IParticleAttractionAffector.h"

namespace irr
{
namespace scene
{

//! Particle Affector for attracting particles to a point
class CParticleAttractionAffector : public IParticleAttractionAffector
{
public:

	CParticleAttractionAffector(
		const core::vector3df& point = core::vector3df(), f32 speed = 1.0f,
		bool attract = true, bool affectX = true,
		bool affectY = true, bool affectZ = true );

	//! Affects a particle.
	virtual void affect(u32 now, SParticle* particlearray, u32 count);

	//! Set the point that particles will attract to
	virtual void setPoint( const core::vector3df& point ) { Point = point; }

	//! Set the speed, in game units per second that the particles will attract to
	//! the specified point
	virtual void setSpeed( f32 speed ) { Speed = speed; }

	//! Set whether or not the particles are attracting or detracting
	virtual void setAttract( bool attract ) { Attract = attract; }

	//! Set whether or not this will affect particles in the X direction
	virtual void setAffectX( bool affect ) { AffectX = affect; }

	//! Set whether or not this will affect particles in the Y direction
	virtual void setAffectY( bool affect ) { AffectY = affect; }

	//! Set whether or not this will affect particles in the Z direction
	virtual void setAffectZ( bool affect ) { AffectZ = affect; }

	//! Get the point that particles are attracted to
	virtual const core::vector3df& getPoint() const { return Point; }

	//! Get the speed that points attract to the specified point
	virtual f32 getSpeed() const { return Speed; }

	//! Get whether or not the particles are attracting or detracting
	virtual bool getAttract() const { return Attract; }

	//! Get whether or not the particles X position are affected
	virtual bool getAffectX() const { return AffectX; }

	//! Get whether or not the particles Y position are affected
	virtual bool getAffectY() const { return AffectY; }

	//! Get whether or not the particles Z position are affected
	virtual bool getAffectZ() const { return AffectZ; }

	//! Writes attributes of the object.
	virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;

	//! Reads attributes of the object.
	virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);

private:

	core::vector3df Point;
	f32 Speed;
	bool AffectX;
	bool AffectY;
	bool AffectZ;
	bool Attract;
	u32 LastTime;
};

} // end namespace scene
} // end namespace irr


#endif // __C_PARTICLE_ATTRACTION_AFFECTOR_H_INCLUDED__


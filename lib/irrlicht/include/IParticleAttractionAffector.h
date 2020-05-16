// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_PARTICLE_ATTRACTION_AFFECTOR_H_INCLUDED__
#define __I_PARTICLE_ATTRACTION_AFFECTOR_H_INCLUDED__

#include "IParticleAffector.h"

namespace irr
{
namespace scene
{

//! A particle affector which attracts or detracts particles.
class IParticleAttractionAffector : public IParticleAffector
{
public:

	//! Set the point that particles will attract to
	virtual void setPoint( const core::vector3df& point ) = 0;

	//! Set whether or not the particles are attracting or detracting
	virtual void setAttract( bool attract ) = 0;

	//! Set whether or not this will affect particles in the X direction
	virtual void setAffectX( bool affect ) = 0;

	//! Set whether or not this will affect particles in the Y direction
	virtual void setAffectY( bool affect ) = 0;

	//! Set whether or not this will affect particles in the Z direction
	virtual void setAffectZ( bool affect ) = 0;

	//! Get the point that particles are attracted to
	virtual const core::vector3df& getPoint() const = 0;

	//! Get whether or not the particles are attracting or detracting
	virtual bool getAttract() const = 0;

	//! Get whether or not the particles X position are affected
	virtual bool getAffectX() const = 0;

	//! Get whether or not the particles Y position are affected
	virtual bool getAffectY() const = 0;

	//! Get whether or not the particles Z position are affected
	virtual bool getAffectZ() const = 0;

	//! Get emitter type
	virtual E_PARTICLE_AFFECTOR_TYPE getType() const { return EPAT_ATTRACT; }
};

} // end namespace scene
} // end namespace irr


#endif // __I_PARTICLE_ATTRACTION_AFFECTOR_H_INCLUDED__


// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_PARTICLE_CYLINDER_EMITTER_H_INCLUDED__
#define __C_PARTICLE_CYLINDER_EMITTER_H_INCLUDED__

#include "IParticleCylinderEmitter.h"
#include "irrArray.h"

namespace irr
{
namespace scene
{

//! A default box emitter
class CParticleCylinderEmitter : public IParticleCylinderEmitter
{
public:

	//! constructor
	CParticleCylinderEmitter(
		const core::vector3df& center, f32 radius,
		const core::vector3df& normal, f32 length,
		bool outlineOnly = false, const core::vector3df& direction = core::vector3df(0.0f,0.03f,0.0f),
		u32 minParticlesPerSecond = 20,
		u32 maxParticlesPerSecond = 40,
		const video::SColor& minStartColor = video::SColor(255,0,0,0),
		const video::SColor& maxStartColor = video::SColor(255,255,255,255),
		u32 lifeTimeMin=2000,
		u32 lifeTimeMax=4000,
		s32 maxAngleDegrees=0,
		const core::dimension2df& minStartSize = core::dimension2df(5.0f,5.0f),
		const core::dimension2df& maxStartSize = core::dimension2df(5.0f,5.0f)
		);

	//! Prepares an array with new particles to emitt into the system
	//! and returns how much new particles there are.
	virtual s32 emitt(u32 now, u32 timeSinceLastCall, SParticle*& outArray);

	//! Set the center of the radius for the cylinder, at one end of the cylinder
	virtual void setCenter( const core::vector3df& center ) { Center = center; }

	//! Set the normal of the cylinder
	virtual void setNormal( const core::vector3df& normal ) { Normal = normal; }

	//! Set the radius of the cylinder
	virtual void setRadius( f32 radius ) { Radius = radius; }

	//! Set the length of the cylinder
	virtual void setLength( f32 length ) { Length = length; }

	//! Set whether or not to draw points inside the cylinder
	virtual void setOutlineOnly( bool outlineOnly ) { OutlineOnly = outlineOnly; }

	//! Set direction the emitter emits particles
	virtual void setDirection( const core::vector3df& newDirection ) { Direction = newDirection; }

	//! Set direction the emitter emits particles
	virtual void setMinParticlesPerSecond( u32 minPPS ) { MinParticlesPerSecond = minPPS; }

	//! Set direction the emitter emits particles
	virtual void setMaxParticlesPerSecond( u32 maxPPS ) { MaxParticlesPerSecond = maxPPS; }

	//! Set direction the emitter emits particles
	virtual void setMinStartColor( const video::SColor& color ) { MinStartColor = color; }

	//! Set direction the emitter emits particles
	virtual void setMaxStartColor( const video::SColor& color ) { MaxStartColor = color; }

	//! Set the maximum starting size for particles
	virtual void setMaxStartSize( const core::dimension2df& size ) { MaxStartSize = size; }

	//! Set the minimum starting size for particles
	virtual void setMinStartSize( const core::dimension2df& size ) { MinStartSize = size; }

	//! Set the minimum particle life-time in milliseconds
	virtual void setMinLifeTime( u32 lifeTimeMin ) { MinLifeTime = lifeTimeMin; }

	//! Set the maximum particle life-time in milliseconds
	virtual void setMaxLifeTime( u32 lifeTimeMax ) { MaxLifeTime = lifeTimeMax; }

	//!	Maximal random derivation from the direction
	virtual void setMaxAngleDegrees( s32 maxAngleDegrees ) { MaxAngleDegrees = maxAngleDegrees; }

	//! Get the center of the cylinder
	virtual const core::vector3df& getCenter() const { return Center; }

	//! Get the normal of the cylinder
	virtual const core::vector3df& getNormal() const { return Normal; }

	//! Get the radius of the cylinder
	virtual f32 getRadius() const { return Radius; }

	//! Get the center of the cylinder
	virtual f32 getLength() const { return Length; }

	//! Get whether or not to draw points inside the cylinder
	virtual bool getOutlineOnly() const { return OutlineOnly; }

	//! Gets direction the emitter emits particles
	virtual const core::vector3df& getDirection() const { return Direction; }

	//! Gets direction the emitter emits particles
	virtual u32 getMinParticlesPerSecond() const { return MinParticlesPerSecond; }

	//! Gets direction the emitter emits particles
	virtual u32 getMaxParticlesPerSecond() const { return MaxParticlesPerSecond; }

	//! Gets direction the emitter emits particles
	virtual const video::SColor& getMinStartColor() const { return MinStartColor; }

	//! Gets direction the emitter emits particles
	virtual const video::SColor& getMaxStartColor() const { return MaxStartColor; }

	//! Gets the maximum starting size for particles
	virtual const core::dimension2df& getMaxStartSize() const { return MaxStartSize; }

	//! Gets the minimum starting size for particles
	virtual const core::dimension2df& getMinStartSize() const { return MinStartSize; }

	//! Get the minimum particle life-time in milliseconds
	virtual u32 getMinLifeTime() const { return MinLifeTime; }

	//! Get the maximum particle life-time in milliseconds
	virtual u32 getMaxLifeTime() const { return MaxLifeTime; }

	//!	Maximal random derivation from the direction
	virtual s32 getMaxAngleDegrees() const { return MaxAngleDegrees; }

	//! Writes attributes of the object.
	virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;

	//! Reads attributes of the object.
	virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);

private:

	core::array<SParticle> Particles;

	core::vector3df	Center;
	core::vector3df	Normal;
	core::vector3df Direction;
	core::dimension2df MaxStartSize, MinStartSize;
	u32 MinParticlesPerSecond, MaxParticlesPerSecond;
	video::SColor MinStartColor, MaxStartColor;
	u32 MinLifeTime, MaxLifeTime;

	f32 Radius;
	f32 Length;

	u32 Time;
	u32 Emitted;
	s32 MaxAngleDegrees;

	bool OutlineOnly;
};

} // end namespace scene
} // end namespace irr


#endif


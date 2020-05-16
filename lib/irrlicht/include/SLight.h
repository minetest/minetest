// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __S_LIGHT_H_INCLUDED__
#define __S_LIGHT_H_INCLUDED__

#include "SColor.h"

namespace irr
{
namespace video
{

//! Enumeration for different types of lights
enum E_LIGHT_TYPE
{
	//! point light, it has a position in space and radiates light in all directions
	ELT_POINT,
	//! spot light, it has a position in space, a direction, and a limited cone of influence
	ELT_SPOT,
	//! directional light, coming from a direction from an infinite distance
	ELT_DIRECTIONAL,

	//! Only used for counting the elements of this enum
	ELT_COUNT
};

//! Names for light types
const c8* const LightTypeNames[] =
{
	"Point",
	"Spot",
	"Directional",
	0
};

//! structure for holding data describing a dynamic point light.
/** Irrlicht supports point lights, spot lights, and directional lights.
*/
struct SLight
{
	SLight() : AmbientColor(0.f,0.f,0.f), DiffuseColor(1.f,1.f,1.f),
		SpecularColor(1.f,1.f,1.f), Attenuation(1.f,0.f,0.f),
		OuterCone(45.f), InnerCone(0.f), Falloff(2.f),
		Position(0.f,0.f,0.f), Direction(0.f,0.f,1.f),
		Radius(100.f), Type(ELT_POINT), CastShadows(true)
		{}

	//! Ambient color emitted by the light
	SColorf AmbientColor;

	//! Diffuse color emitted by the light.
	/** This is the primary color you want to set. */
	SColorf DiffuseColor;

	//! Specular color emitted by the light.
	/** For details how to use specular highlights, see SMaterial::Shininess */
	SColorf SpecularColor;

	//! Attenuation factors (constant, linear, quadratic)
	/** Changes the light strength fading over distance.
	Can also be altered by setting the radius, Attenuation will change to
	(0,1.f/radius,0). Can be overridden after radius was set. */
	core::vector3df Attenuation;

	//! The angle of the spot's outer cone. Ignored for other lights.
	f32 OuterCone;

	//! The angle of the spot's inner cone. Ignored for other lights.
	f32 InnerCone;

	//! The light strength's decrease between Outer and Inner cone.
	f32 Falloff;

	//! Read-ONLY! Position of the light.
	/** If Type is ELT_DIRECTIONAL, it is ignored. Changed via light scene node's position. */
	core::vector3df Position;

	//! Read-ONLY! Direction of the light.
	/** If Type is ELT_POINT, it is ignored. Changed via light scene node's rotation. */
	core::vector3df Direction;

	//! Read-ONLY! Radius of light. Everything within this radius will be lighted.
	f32 Radius;

	//! Read-ONLY! Type of the light. Default: ELT_POINT
	E_LIGHT_TYPE Type;

	//! Read-ONLY! Does the light cast shadows?
	bool CastShadows:1;
};

} // end namespace video
} // end namespace irr

#endif


// Copyright (C) 2015 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "irrTypes.h"

namespace irr
{
namespace video
{

class COpenGLCoreFeature
{
public:
	COpenGLCoreFeature() :
			BlendOperation(false), ColorAttachment(0), MultipleRenderTarget(0), MaxTextureUnits(1)
	{
	}

	virtual ~COpenGLCoreFeature()
	{
	}

	bool BlendOperation;

	u8 ColorAttachment;
	u8 MultipleRenderTarget;
	u8 MaxTextureUnits;
};

}
}

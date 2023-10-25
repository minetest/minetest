// Copyright (C) 2015 Patryk Nadrowski
// Copyright (C) 2009-2010 Amundis
// 2017 modified by Michael Zeilfelder (unifying extension handlers)
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#include "COGLES2ExtensionHandler.h"

#ifdef _IRR_COMPILE_WITH_OGLES2_

#include "irrString.h"
#include "SMaterial.h"
#include "fast_atof.h"

namespace irr
{
namespace video
{
	void COGLES2ExtensionHandler::initExtensions()
	{
		getGLVersion();

		getGLExtensions();

		GLint val=0;
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &val);
		Feature.MaxTextureUnits = static_cast<u8>(val);

	#ifdef GL_EXT_texture_filter_anisotropic
		if (FeatureAvailable[IRR_GL_EXT_texture_filter_anisotropic])
		{
			glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &val);
			MaxAnisotropy = static_cast<u8>(val);
		}
	#endif
	#ifdef GL_MAX_ELEMENTS_INDICES
		glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &val);
		MaxIndices=val;
	#endif
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &val);
		MaxTextureSize=static_cast<u32>(val);
	#ifdef GL_EXT_texture_lod_bias
		if (FeatureAvailable[IRR_EXT_texture_lod_bias])
			glGetFloatv(GL_MAX_TEXTURE_LOD_BIAS_EXT, &MaxTextureLODBias);
	#endif
		glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, DimAliasedLine);
		glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, DimAliasedPoint);

		Feature.MaxTextureUnits = core::min_(Feature.MaxTextureUnits, static_cast<u8>(MATERIAL_MAX_TEXTURES));
		Feature.ColorAttachment = 1;
	}

} // end namespace video
} // end namespace irr


#endif // _IRR_COMPILE_WITH_OGLES2_

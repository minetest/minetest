// Copyright (C) 2002-2008 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#ifdef _IRR_COMPILE_WITH_OGLES1_

#include "COGLESDriver.h"
#include "IMaterialRenderer.h"

namespace irr
{
namespace video
{

//! Base class for all internal OGLES1 material renderers
class COGLES1MaterialRenderer : public IMaterialRenderer
{
public:
	//! Constructor
	COGLES1MaterialRenderer(video::COGLES1Driver *driver) :
			Driver(driver)
	{
	}

protected:
	video::COGLES1Driver *Driver;
};

//! Solid material renderer
class COGLES1MaterialRenderer_SOLID : public COGLES1MaterialRenderer
{
public:
	COGLES1MaterialRenderer_SOLID(video::COGLES1Driver *d) :
			COGLES1MaterialRenderer(d) {}

	virtual void OnSetMaterial(const SMaterial &material, const SMaterial &lastMaterial,
			bool resetAllRenderstates, IMaterialRendererServices *services)
	{
		Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		if (resetAllRenderstates || (material.MaterialType != lastMaterial.MaterialType)) {
			// thanks to Murphy, the following line removed some
			// bugs with several OGLES1 implementations.
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}
	}
};

//! Generic Texture Blend
class COGLES1MaterialRenderer_ONETEXTURE_BLEND : public COGLES1MaterialRenderer
{
public:
	COGLES1MaterialRenderer_ONETEXTURE_BLEND(video::COGLES1Driver *d) :
			COGLES1MaterialRenderer(d) {}

	virtual void OnSetMaterial(const SMaterial &material, const SMaterial &lastMaterial,
			bool resetAllRenderstates, IMaterialRendererServices *services)
	{
		Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		//		if (material.MaterialType != lastMaterial.MaterialType ||
		//			material.MaterialTypeParam != lastMaterial.MaterialTypeParam ||
		//			resetAllRenderstates)
		{
			E_BLEND_FACTOR srcRGBFact, dstRGBFact, srcAlphaFact, dstAlphaFact;
			E_MODULATE_FUNC modulate;
			u32 alphaSource;
			unpack_textureBlendFuncSeparate(srcRGBFact, dstRGBFact, srcAlphaFact, dstAlphaFact, modulate, alphaSource, material.MaterialTypeParam);

			Driver->getCacheHandler()->setBlend(true);

			if (Driver->queryFeature(EVDF_BLEND_SEPARATE)) {
				Driver->getCacheHandler()->setBlendFuncSeparate(Driver->getGLBlend(srcRGBFact), Driver->getGLBlend(dstRGBFact),
						Driver->getGLBlend(srcAlphaFact), Driver->getGLBlend(dstAlphaFact));
			} else {
				Driver->getCacheHandler()->setBlendFunc(Driver->getGLBlend(srcRGBFact), Driver->getGLBlend(dstRGBFact));
			}

			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);

			glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, (f32)modulate);

			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.f);

			if (textureBlendFunc_hasAlpha(srcRGBFact) || textureBlendFunc_hasAlpha(dstRGBFact) ||
					textureBlendFunc_hasAlpha(srcAlphaFact) || textureBlendFunc_hasAlpha(dstAlphaFact)) {
				glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
				glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);

				glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PRIMARY_COLOR);
			}
		}
	}

	virtual void OnUnsetMaterial()
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 1.f);
		glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);

		Driver->getCacheHandler()->setBlend(false);
		glDisable(GL_ALPHA_TEST);
	}

	//! Returns if the material is transparent.
	/** Is not always transparent, but mostly. */
	virtual bool isTransparent() const
	{
		return true;
	}

private:
	u32 getGLBlend(E_BLEND_FACTOR factor) const
	{
		u32 r = 0;
		switch (factor) {
		case EBF_ZERO:
			r = GL_ZERO;
			break;
		case EBF_ONE:
			r = GL_ONE;
			break;
		case EBF_DST_COLOR:
			r = GL_DST_COLOR;
			break;
		case EBF_ONE_MINUS_DST_COLOR:
			r = GL_ONE_MINUS_DST_COLOR;
			break;
		case EBF_SRC_COLOR:
			r = GL_SRC_COLOR;
			break;
		case EBF_ONE_MINUS_SRC_COLOR:
			r = GL_ONE_MINUS_SRC_COLOR;
			break;
		case EBF_SRC_ALPHA:
			r = GL_SRC_ALPHA;
			break;
		case EBF_ONE_MINUS_SRC_ALPHA:
			r = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case EBF_DST_ALPHA:
			r = GL_DST_ALPHA;
			break;
		case EBF_ONE_MINUS_DST_ALPHA:
			r = GL_ONE_MINUS_DST_ALPHA;
			break;
		case EBF_SRC_ALPHA_SATURATE:
			r = GL_SRC_ALPHA_SATURATE;
			break;
		}
		return r;
	}
};

//! Transparent vertex alpha material renderer
class COGLES1MaterialRenderer_TRANSPARENT_VERTEX_ALPHA : public COGLES1MaterialRenderer
{
public:
	COGLES1MaterialRenderer_TRANSPARENT_VERTEX_ALPHA(video::COGLES1Driver *d) :
			COGLES1MaterialRenderer(d) {}

	virtual void OnSetMaterial(const SMaterial &material, const SMaterial &lastMaterial,
			bool resetAllRenderstates, IMaterialRendererServices *services)
	{
		Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		Driver->getCacheHandler()->setBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		Driver->getCacheHandler()->setBlend(true);

		if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates) {
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PRIMARY_COLOR);

			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PRIMARY_COLOR);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_TEXTURE);
		}
	}

	virtual void OnUnsetMaterial()
	{
		// default values
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
		glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PREVIOUS);
		glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
		glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);

		Driver->getCacheHandler()->setBlend(false);
	}

	//! Returns if the material is transparent.
	virtual bool isTransparent() const
	{
		return true;
	}
};

//! Transparent alpha channel material renderer
class COGLES1MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL : public COGLES1MaterialRenderer
{
public:
	COGLES1MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL(video::COGLES1Driver *d) :
			COGLES1MaterialRenderer(d) {}

	virtual void OnSetMaterial(const SMaterial &material, const SMaterial &lastMaterial,
			bool resetAllRenderstates, IMaterialRendererServices *services)
	{
		Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		Driver->getCacheHandler()->setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		Driver->getCacheHandler()->setBlend(true);

		if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates || material.MaterialTypeParam != lastMaterial.MaterialTypeParam) {
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);

			glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
			glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);

			glEnable(GL_ALPHA_TEST);

			glAlphaFunc(GL_GREATER, material.MaterialTypeParam);
		}
	}

	virtual void OnUnsetMaterial()
	{
		glDisable(GL_ALPHA_TEST);
		Driver->getCacheHandler()->setBlend(false);
	}

	//! Returns if the material is transparent.
	virtual bool isTransparent() const
	{
		return true;
	}
};

//! Transparent alpha channel material renderer
class COGLES1MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL_REF : public COGLES1MaterialRenderer
{
public:
	COGLES1MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL_REF(video::COGLES1Driver *d) :
			COGLES1MaterialRenderer(d) {}

	virtual void OnSetMaterial(const SMaterial &material, const SMaterial &lastMaterial,
			bool resetAllRenderstates, IMaterialRendererServices *services)
	{
		Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);

		if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates) {
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.5f);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}
	}

	virtual void OnUnsetMaterial()
	{
		glDisable(GL_ALPHA_TEST);
	}

	//! Returns if the material is transparent.
	virtual bool isTransparent() const
	{
		return false; // this material is not really transparent because it does no blending.
	}
};

} // end namespace video
} // end namespace irr

#endif

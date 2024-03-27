// Copyright (C) 2017 Michael Zeilfelder
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#pragma once

// Can be included from different ES versions
// (this is also the reason why this file is header-only as correct OGL ES headers have to be included first)

#include "irrMath.h"
#include "COpenGLCoreFeature.h"

namespace irr
{
namespace video
{

class COGLESCoreExtensionHandler
{
public:
	// Enums used internally to check for ES extensions quickly.
	// We buffer all extensions on start once in an array.
	// All OpenGL ES versions share the same extensions (WebGL adds it's own extensions on top of ES2)
	enum EOGLESFeatures
	{
		// If you update this enum also update the corresponding OGLESFeatureStrings string-array
		IRR_GL_APPLE_texture_2D_limited_npot,
		IRR_GL_APPLE_texture_format_BGRA8888,
		IRR_GL_EXT_blend_minmax,
		IRR_GL_EXT_read_format_bgra,
		IRR_GL_EXT_texture_filter_anisotropic,
		IRR_GL_EXT_texture_format_BGRA8888,
		IRR_GL_EXT_texture_lod_bias,
		IRR_GL_EXT_texture_rg,
		IRR_GL_IMG_read_format,
		IRR_GL_IMG_texture_format_BGRA8888,
		IRR_GL_IMG_user_clip_plane,
		IRR_GL_OES_blend_func_separate,
		IRR_GL_OES_blend_subtract,
		IRR_GL_OES_depth_texture,
		IRR_GL_OES_depth24,
		IRR_GL_OES_depth32,
		IRR_GL_OES_element_index_uint,
		IRR_GL_OES_framebuffer_object,
		IRR_GL_OES_packed_depth_stencil,
		IRR_GL_OES_point_size_array,
		IRR_GL_OES_point_sprite,
		IRR_GL_OES_read_format,
		IRR_GL_OES_stencil_wrap,
		IRR_GL_OES_texture_float,
		IRR_GL_OES_texture_half_float,
		IRR_GL_OES_texture_mirrored_repeat,
		IRR_GL_OES_texture_npot,

		IRR_OGLES_Feature_Count
	};

	COGLESCoreExtensionHandler() :
			Version(0), MaxAnisotropy(1), MaxIndices(0xffff),
			MaxTextureSize(1), MaxTextureLODBias(0.f), StencilBuffer(false)
	{
		for (u32 i = 0; i < IRR_OGLES_Feature_Count; ++i)
			FeatureAvailable[i] = false;

		DimAliasedLine[0] = 1.f;
		DimAliasedLine[1] = 1.f;
		DimAliasedPoint[0] = 1.f;
		DimAliasedPoint[1] = 1.f;
	}

	virtual ~COGLESCoreExtensionHandler() {}

	const COpenGLCoreFeature &getFeature() const
	{
		return Feature;
	}

	void dump() const
	{
		for (u32 i = 0; i < IRR_OGLES_Feature_Count; ++i)
			os::Printer::log(getFeatureString(i), FeatureAvailable[i] ? " true" : " false");
	}

	bool queryGLESFeature(EOGLESFeatures feature) const
	{
		return FeatureAvailable[feature];
	}

protected:
	const char *getFeatureString(size_t index) const
	{
		// One for each EOGLESFeatures
		static const char *const OGLESFeatureStrings[IRR_OGLES_Feature_Count] = {
				"GL_APPLE_texture_2D_limited_npot",
				"GL_APPLE_texture_format_BGRA8888",
				"GL_EXT_blend_minmax",
				"GL_EXT_read_format_bgra",
				"GL_EXT_texture_filter_anisotropic",
				"GL_EXT_texture_format_BGRA8888",
				"GL_EXT_texture_lod_bias",
				"GL_EXT_texture_rg",
				"GL_IMG_read_format",
				"GL_IMG_texture_format_BGRA8888",
				"GL_IMG_user_clip_plane",
				"GL_OES_blend_func_separate",
				"GL_OES_blend_subtract",
				"GL_OES_depth_texture",
				"GL_OES_depth24",
				"GL_OES_depth32",
				"GL_OES_element_index_uint",
				"GL_OES_framebuffer_object",
				"GL_OES_packed_depth_stencil",
				"GL_OES_point_size_array",
				"GL_OES_point_sprite",
				"GL_OES_read_format",
				"GL_OES_stencil_wrap",
				"GL_OES_texture_float",
				"GL_OES_texture_half_float",
				"GL_OES_texture_mirrored_repeat",
				"GL_OES_texture_npot",
			};

		return OGLESFeatureStrings[index];
	}

	void getGLVersion()
	{
		Version = 0;
		s32 multiplier = 100;

		core::stringc version(glGetString(GL_VERSION));

		for (u32 i = 0; i < version.size(); ++i) {
			if (version[i] >= '0' && version[i] <= '9') {
				if (multiplier > 1) {
					Version += static_cast<u16>(core::floor32(atof(&(version[i]))) * multiplier);
					multiplier /= 10;
				} else {
					break;
				}
			}
		}
	}

	void getGLExtensions()
	{
		core::stringc extensions = glGetString(GL_EXTENSIONS);
		os::Printer::log(extensions.c_str());

		const u32 size = extensions.size() + 1;
		c8 *str = new c8[size];
		strncpy(str, extensions.c_str(), extensions.size());
		str[extensions.size()] = ' ';
		c8 *p = str;

		for (u32 i = 0; i < size; ++i) {
			if (str[i] == ' ') {
				str[i] = 0;
				if (*p)
					for (size_t j = 0; j < IRR_OGLES_Feature_Count; ++j) {
						if (!strcmp(getFeatureString(j), p)) {
							FeatureAvailable[j] = true;
							break;
						}
					}

				p = p + strlen(p) + 1;
			}
		}

		delete[] str;
	}

	COpenGLCoreFeature Feature;

	u16 Version;
	u8 MaxAnisotropy;
	u32 MaxIndices;
	u32 MaxTextureSize;
	f32 MaxTextureLODBias;
	//! Minimal and maximal supported thickness for lines without smoothing
	float DimAliasedLine[2];
	//! Minimal and maximal supported thickness for points without smoothing
	float DimAliasedPoint[2];
	bool StencilBuffer;
	bool FeatureAvailable[IRR_OGLES_Feature_Count];
};
}
}

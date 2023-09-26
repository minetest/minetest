// Copyright (C) 2017 Michael Zeilfelder
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#pragma once

#if defined(_IRR_COMPILE_WITH_WEBGL1_)	// Note: should also work with WebGL2 once we add that to Irrlicht

#include "COpenGLCoreFeature.h"

namespace irr
{
namespace video
{
	// Extension handling for WebGL.
	class CWebGLExtensionHandler
	{
	public:
		// Enums used internally to check for WebGL extensions quickly.
		// We buffer all extensions on start once in an array.
		enum EWebGLFeatures
		{
			// If you update this enum also update the corresponding WebGLFeatureStrings string-array
			// Last updated was up to (including) extension number 35 (EXT_float_blend)

			// Khronos ratified WebGL Extensions
			IRR_OES_texture_float, // 1
			IRR_OES_texture_half_float, // 2
			IRR_WEBGL_lose_context, // 3
			IRR_OES_standard_derivatives, // 4
			IRR_OES_vertex_array_object, // 5
			IRR_WEBGL_debug_renderer_info, // 6
			IRR_WEBGL_debug_shaders, // 7
			IRR_WEBGL_compressed_texture_s3tc, // 8
			IRR_WEBGL_depth_texture, // 9
			IRR_OES_element_index_uint, // 10
			IRR_EXT_texture_filter_anisotropic, // 11
			IRR_EXT_frag_depth, // 16
			IRR_WEBGL_draw_buffers, // 18
			IRR_ANGLE_instanced_arrays, // 19
			IRR_OES_texture_float_linear, // 20
			IRR_OES_texture_half_float_linear, // 21
			IRR_EXT_blend_minmax, // 25
			IRR_EXT_shader_texture_lod, // 27

			// Community approved WebGL Extensions
			IRR_WEBGL_compressed_texture_atc, // 12
			IRR_WEBGL_compressed_texture_pvrtc, // 13
			IRR_EXT_color_buffer_half_float, // 14
			IRR_WEBGL_color_buffer_float, // 15
			IRR_EXT_sRGB, // 17
			IRR_WEBGL_compressed_texture_etc1, // 24
			IRR_EXT_disjoint_timer_query, // 26
			IRR_WEBGL_compressed_texture_etc, // 29
			IRR_WEBGL_compressed_texture_astc, // 30
			IRR_EXT_color_buffer_float, // 31
			IRR_WEBGL_compressed_texture_s3tc_srgb, // 32
			IRR_EXT_disjoint_timer_query_webgl2, // 33

			// Draft WebGL Extensions
			IRR_WEBGL_shared_resources, // 22
			IRR_WEBGL_security_sensitive_resources, // 23
			IRR_OES_fbo_render_mipmap, // 28
			IRR_WEBGL_get_buffer_sub_data_async, // 34
			IRR_EXT_float_blend, // 35

			IRR_WEBGL_Feature_Count
		};

		CWebGLExtensionHandler()
		{
			for (u32 i = 0; i < IRR_WEBGL_Feature_Count; ++i)
				FeatureAvailable[i] = false;
		}

		virtual ~CWebGLExtensionHandler() {}

		void dump() const
		{
			for (u32 i = 0; i < IRR_WEBGL_Feature_Count; ++i)
				os::Printer::log(getFeatureString(i), FeatureAvailable[i] ? " true" : " false");
		}

		bool queryWebGLFeature(EWebGLFeatures feature) const
		{
			return FeatureAvailable[feature];
		}

		void getGLExtensions()
		{
			core::stringc extensions = glGetString(GL_EXTENSIONS);
			os::Printer::log(extensions.c_str());

			const u32 size = extensions.size() + 1;
			c8* str = new c8[size];
			strncpy(str, extensions.c_str(), extensions.size());
			str[extensions.size()] = ' ';
			c8* p = str;

			for (u32 i=0; i<size; ++i)
			{
				if (str[i] == ' ')
				{
					str[i] = 0;
					if (*p)
						for (size_t j=0; j<IRR_WEBGL_Feature_Count; ++j)
						{
							if (!strcmp(getFeatureString(j), p))
							{
								FeatureAvailable[j] = true;
								break;
							}
						}

					p = p + strlen(p) + 1;
				}
			}

			delete[] str;
		}


	protected:

		const char* getFeatureString(size_t index) const
		{
			// Based on https://www.khronos.org/registry/webgl/extensions
			// One for each EWebGLFeatures
			static const char* const WebGLFeatureStrings[IRR_WEBGL_Feature_Count] =
			{
				"OES_texture_float",
				"OES_texture_half_float",
				"WEBGL_lose_context",
				"OES_standard_derivatives",
				"OES_vertex_array_object",
				"WEBGL_debug_renderer_info",
				"WEBGL_debug_shaders",
				"WEBGL_compressed_texture_s3tc",
				"WEBGL_depth_texture",
				"OES_element_index_uint",
				"EXT_texture_filter_anisotropic",
				"EXT_frag_depth",
				"WEBGL_draw_buffers",
				"ANGLE_instanced_arrays",
				"OES_texture_float_linear",
				"OES_texture_half_float_linear",
				"EXT_blend_minmax",
				"EXT_shader_texture_lod",

				"WEBGL_compressed_texture_atc",
				"WEBGL_compressed_texture_pvrtc",
				"EXT_color_buffer_half_float",
				"WEBGL_color_buffer_float",
				"EXT_sRGB",
				"WEBGL_compressed_texture_etc1",
				"EXT_disjoint_timer_query",
				"WEBGL_compressed_texture_etc",
				"WEBGL_compressed_texture_astc",
				"EXT_color_buffer_float",
				"WEBGL_compressed_texture_s3tc_srgb",
				"EXT_disjoint_timer_query_webgl2",

				"WEBGL_shared_resources",
				"WEBGL_security_sensitive_resources",
				"OES_fbo_render_mipmap",
				"WEBGL_get_buffer_sub_data_async",
				"EXT_float_blend"
			};

			return WebGLFeatureStrings[index];
		}

		bool FeatureAvailable[IRR_WEBGL_Feature_Count];
	};
}
}

#endif // defined(_IRR_COMPILE_WITH_WEBGL1_)

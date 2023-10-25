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
			// Last updated was up to (including) extension number 290 (GL_EXT_clip_control)
			IRR_GLX_ARB_context_flush_control, // 191
			IRR_GL_AMD_compressed_3DC_texture, // 39
			IRR_GL_AMD_compressed_ATC_texture, // 40
			IRR_GL_AMD_performance_monitor, // 50
			IRR_GL_AMD_program_binary_Z400, // 48
			IRR_GL_ANDROID_extension_pack_es31a, // 187
			IRR_GL_ANGLE_depth_texture, // 138
			IRR_GL_ANGLE_framebuffer_blit, // 83
			IRR_GL_ANGLE_framebuffer_multisample, // 84
			IRR_GL_ANGLE_instanced_arrays, // 109
			IRR_GL_ANGLE_pack_reverse_row_order, // 110
			IRR_GL_ANGLE_program_binary, //139
			IRR_GL_ANGLE_texture_compression_dxt1, // 111
			IRR_GL_ANGLE_texture_compression_dxt3, // 111
			IRR_GL_ANGLE_texture_compression_dxt5, // 111
			IRR_GL_ANGLE_texture_usage, // 112
			IRR_GL_ANGLE_translated_shader_source, // 113
			IRR_GL_APPLE_clip_distance, // 193
			IRR_GL_APPLE_color_buffer_packed_float, // 194
			IRR_GL_APPLE_copy_texture_levels, // 123
			IRR_GL_APPLE_framebuffer_multisample, // 78
			IRR_GL_APPLE_rgb_422, // 76
			IRR_GL_APPLE_sync, // 124
			IRR_GL_APPLE_texture_2D_limited_npot, // 59
			IRR_GL_APPLE_texture_format_BGRA8888, // 79
			IRR_GL_APPLE_texture_max_level, // 80
			IRR_GL_APPLE_texture_packed_float, // 195
			IRR_ARB_texture_env_combine, //ogl, IMG simulator
			IRR_ARB_texture_env_dot3, //ogl, IMG simulator
			IRR_GL_ARM_mali_program_binary, // 120
			IRR_GL_ARM_mali_shader_binary, // 81
			IRR_GL_ARM_rgba8, // 82
			IRR_GL_ARM_shader_framebuffer_fetch, // 165
			IRR_GL_ARM_shader_framebuffer_fetch_depth_stencil, // 166
			IRR_GL_DMP_program_binary, // 192
			IRR_GL_DMP_shader_binary, // 88
			IRR_GL_EXT_EGL_image_array, // 278
			IRR_GL_EXT_YUV_target, // 222
			IRR_GL_EXT_base_instance, // 203
			IRR_GL_EXT_blend_func_extended, // 247
			IRR_GL_EXT_blend_minmax, // 65
			IRR_GL_EXT_buffer_storage, // 239
			IRR_GL_EXT_clear_texture, // 269
			IRR_GL_EXT_clip_control, // 290
			IRR_GL_EXT_clip_cull_distance, // 257
			IRR_GL_EXT_color_buffer_float, // 137
			IRR_GL_EXT_color_buffer_half_float, // 97
			IRR_GL_EXT_compressed_ETC1_RGB8_sub_texture, // 188
			IRR_GL_EXT_conservative_depth, // 268
			IRR_GL_EXT_copy_image, // 175
			IRR_GL_EXT_debug_label, // 98
			IRR_GL_EXT_debug_marker, // 99
			IRR_GL_EXT_discard_framebuffer, // 64
			IRR_GL_EXT_disjoint_timer_query, // 150
			IRR_GL_EXT_draw_buffers, // 151
			IRR_GL_EXT_draw_buffers_indexed, // 176
			IRR_GL_EXT_draw_elements_base_vertex, // 204
			IRR_GL_EXT_draw_instanced, // 157
			IRR_GL_EXT_draw_transform_feedback, // 272
			IRR_GL_EXT_external_buffer, // 284
			IRR_GL_EXT_float_blend, // 225
			IRR_GL_EXT_frag_depth, // 86
			IRR_GL_EXT_geometry_point_size, // 177
			IRR_GL_EXT_geometry_shader, // 177
			IRR_GL_EXT_gpu_shader5, // 178
			IRR_GL_EXT_instanced_arrays, // 156
			IRR_GL_EXT_map_buffer_range, // 121
			IRR_GL_EXT_memory_object, // 280
			IRR_GL_EXT_memory_object_fd, // 281
			IRR_GL_EXT_memory_object_win32, // 282
			IRR_GL_EXT_multi_draw_arrays, // 69
			IRR_GL_EXT_multi_draw_indirect, // 205
			IRR_GL_EXT_multisample_compatibility, // 248
			IRR_GL_EXT_multisampled_render_to_texture, // 106
			IRR_GL_EXT_multisampled_render_to_texture2, // 275
			IRR_GL_EXT_multiview_draw_buffers, // 125
			IRR_GL_EXT_occlusion_query_boolean, // 100
			IRR_GL_EXT_polygon_offset_clamp, // 252
			IRR_GL_EXT_post_depth_coverage, // 225
			IRR_GL_EXT_primitive_bounding_box, // 186
			IRR_GL_EXT_protected_textures, // 256
			IRR_GL_EXT_pvrtc_sRGB, // 155
			IRR_GL_EXT_raster_multisample, // 226
			IRR_GL_EXT_read_format_bgra, // 66
			IRR_GL_EXT_render_snorm, // 206
			IRR_GL_EXT_robustness, // 107
			IRR_GL_EXT_sRGB, // 105
			IRR_GL_EXT_sRGB_write_control, // 153
			IRR_GL_EXT_semaphore, // 280
			IRR_GL_EXT_semaphore_fd, // 281
			IRR_GL_EXT_semaphore_win32, // 282
			IRR_GL_EXT_separate_shader_objects, // 101
			IRR_GL_EXT_shader_framebuffer_fetch, // 122
			IRR_GL_EXT_shader_group_vote, // 254
			IRR_GL_EXT_shader_implicit_conversions, // 179
			IRR_GL_EXT_shader_integer_mix, // 161
			IRR_GL_EXT_shader_io_blocks, // 180
			IRR_GL_EXT_shader_non_constant_global_initializers, // 264
			IRR_GL_EXT_shader_pixel_local_storage, // 167
			IRR_GL_EXT_shader_pixel_local_storage2, // 253
			IRR_GL_EXT_shader_texture_lod, // 77
			IRR_GL_EXT_shadow_samplers, // 102
			IRR_GL_EXT_sparse_texture, // 240
			IRR_GL_EXT_sparse_texture2, // 259
			IRR_GL_EXT_tessellation_point_size, // 181
			IRR_GL_EXT_tessellation_shader, // 181
			IRR_GL_EXT_texture_border_clamp, // 182
			IRR_GL_EXT_texture_buffer, // 183
			IRR_GL_EXT_texture_compression_astc_decode_mode, // 276
			IRR_GL_EXT_texture_compression_astc_decode_mode_rgb9e5, // 276
			IRR_GL_EXT_texture_compression_bptc, // 287
			IRR_GL_EXT_texture_compression_dxt1, // 49
			IRR_GL_EXT_texture_compression_rgtc, // 286
			IRR_GL_EXT_texture_compression_s3tc, // 154
			IRR_GL_EXT_texture_compression_s3tc_srgb, // 289
			IRR_GL_EXT_texture_cube_map_array, // 184
			IRR_GL_EXT_texture_filter_anisotropic, // 41
			IRR_GL_EXT_texture_filter_minmax, // 227
			IRR_GL_EXT_texture_format_BGRA8888, // 51
			IRR_GL_EXT_texture_lod_bias, // 60
			IRR_GL_EXT_texture_norm16, // 207
			IRR_GL_EXT_texture_rg, // 103
			IRR_GL_EXT_texture_sRGB_R8, // 221
			IRR_GL_EXT_texture_sRGB_RG8, // 223
			IRR_GL_EXT_texture_sRGB_decode, // 152
			IRR_GL_EXT_texture_storage, // 108
			IRR_GL_EXT_texture_type_2_10_10_10_REV, // 42
			IRR_GL_EXT_texture_view, // 185
			IRR_GL_EXT_unpack_subimage, // 90
			IRR_GL_EXT_win32_keyed_mutex, // 283
			IRR_GL_EXT_window_rectangles, // 263
			IRR_GL_FJ_shader_binary_GCCSO, // 114
			IRR_GL_IMG_bindless_texture, // 270
			IRR_GL_IMG_framebuffer_downsample, // 255
			IRR_GL_IMG_multisampled_render_to_texture, // 74
			IRR_GL_IMG_program_binary, // 67
			IRR_GL_IMG_read_format, // 53
			IRR_GL_IMG_shader_binary, // 68
			IRR_GL_IMG_texture_compression_pvrtc, // 54
			IRR_GL_IMG_texture_compression_pvrtc2, // 140
			IRR_GL_IMG_texture_env_enhanced_fixed_function, // 58
			IRR_GL_IMG_texture_format_BGRA8888, // replaced by EXT version
			IRR_GL_IMG_texture_filter_cubic, // 251
			IRR_GL_IMG_user_clip_plane, // 57, was clip_planes
			IRR_GL_IMG_vertex_program, // non-standard
			IRR_GL_INTEL_conservative_rasterization, // 265
			IRR_GL_INTEL_framebuffer_CMAA, // 246
			IRR_GL_INTEL_performance_query, // 164
			IRR_GL_KHR_blend_equation_advanced, // 168
			IRR_GL_KHR_blend_equation_advanced_coherent, // 168
			IRR_GL_KHR_context_flush_control, // 191
			IRR_GL_KHR_debug, // 118
			IRR_GL_KHR_no_error, // 243
			IRR_GL_KHR_parallel_shader_compile, // 288
			IRR_GL_KHR_robust_buffer_access_behavior, // 189
			IRR_GL_KHR_robustness, // 190
			IRR_GL_KHR_texture_compression_astc_hdr, // 117
			IRR_GL_KHR_texture_compression_astc_ldr, // 117
			IRR_GL_KHR_texture_compression_astc_sliced_3d, // 249
			IRR_GL_NVX_blend_equation_advanced_multi_draw_buffers, // 266
			IRR_GL_NV_3dvision_settings, // 129
			IRR_GL_NV_EGL_stream_consumer_external, // 104
			IRR_GL_NV_bgr, // 135
			IRR_GL_NV_bindless_texture, // 197
			IRR_GL_NV_blend_equation_advanced, // 163
			IRR_GL_NV_blend_equation_advanced_coherent, // 163
			IRR_GL_NV_blend_minmax_factor, // 285
			IRR_GL_NV_conditional_render, // 198
			IRR_GL_NV_conservative_raster, // 228
			IRR_GL_NV_conservative_raster_pre_snap_triangles, // 262
			IRR_GL_NV_copy_buffer, // 158
			IRR_GL_NV_coverage_sample, // 72
			IRR_GL_NV_depth_nonlinear, // 73
			IRR_GL_NV_draw_buffers, // 91
			IRR_GL_NV_draw_instanced, // 141
			IRR_GL_NV_draw_texture, // 126
			IRR_GL_NV_draw_vulkan_image, // 274
			IRR_GL_NV_explicit_attrib_location, // 159
			IRR_GL_NV_fbo_color_attachments, // 92
			IRR_GL_NV_fence, // 52
			IRR_GL_NV_fill_rectangle, // 232
			IRR_GL_NV_fragment_coverage_to_color, // 229
			IRR_GL_NV_fragment_shader_interlock, // 230
			IRR_GL_NV_framebuffer_blit, // 142
			IRR_GL_NV_framebuffer_mixed_samples, // 231
			IRR_GL_NV_framebuffer_multisample, // 143
			IRR_GL_NV_generate_mipmap_sRGB, // 144
			IRR_GL_NV_geometry_shader_passthrough, // 233
			IRR_GL_NV_gpu_shader5, // 260
			IRR_GL_NV_image_formats, // 200
			IRR_GL_NV_instanced_arrays, // 145
			IRR_GL_NV_internalformat_sample_query, // 196
			IRR_GL_NV_non_square_matrices, // 160
			IRR_GL_NV_pack_subimage, // 132
			IRR_GL_NV_packed_float, // 127
			IRR_GL_NV_path_rendering, // 199
			IRR_GL_NV_path_rendering_shared_edge, // 234
			IRR_GL_NV_pixel_buffer_object, // 134
			IRR_GL_NV_platform_binary, // 131
			IRR_GL_NV_polygon_mode, // 238
			IRR_GL_NV_read_buffer, // 93
			IRR_GL_NV_read_buffer_front, // part of 93 (non standard)
			IRR_GL_NV_read_depth, // part of 94 (non standard)
			IRR_GL_NV_read_depth_stencil, // 94
			IRR_GL_NV_read_stencil, // part of 94 (non standard)
			IRR_GL_NV_sRGB_formats, // 148
			IRR_GL_NV_sample_locations, // 235
			IRR_GL_NV_sample_mask_override_coverage, // 236
			IRR_GL_NV_shader_atomic_fp16_vector, // 261
			IRR_GL_NV_shader_noperspective_interpolation, // 201
			IRR_GL_NV_shadow_samplers_array, // 146
			IRR_GL_NV_shadow_samplers_cube, // 147
			IRR_GL_NV_texture_array, // 133
			IRR_GL_NV_texture_barrier, // 271
			IRR_GL_NV_texture_border_clamp, // 149
			IRR_GL_NV_texture_compression_latc, // 130
			IRR_GL_NV_texture_compression_s3tc, // 128
			IRR_GL_NV_texture_compression_s3tc_update, // 95
			IRR_GL_NV_texture_npot_2D_mipmap, // 96
			IRR_GL_NV_viewport_array, // 202
			IRR_GL_NV_viewport_array2, // 237
			IRR_GL_NV_viewport_swizzle, // 258
			IRR_GL_OES_EGL_image, // 23
			IRR_GL_OES_EGL_image_external, // 87
			IRR_GL_OES_EGL_image_external_essl3, // 220
			IRR_GL_OES_EGL_sync, // 75
			IRR_GL_OES_blend_equation_separate, // 1
			IRR_GL_OES_blend_func_separate, // 2
			IRR_GL_OES_blend_subtract, // 3
			IRR_GL_OES_byte_coordinates, // 4
			IRR_GL_OES_compressed_ETC1_RGB8_texture, // 5
			IRR_GL_OES_compressed_paletted_texture, // 6
			IRR_GL_OES_copy_image, // 208
			IRR_GL_OES_depth24, // 24
			IRR_GL_OES_depth32, // 25
			IRR_GL_OES_depth_texture, // 43
			IRR_GL_OES_depth_texture_cube_map, // 136
			IRR_GL_OES_draw_buffers_indexed, // 209
			IRR_GL_OES_draw_elements_base_vertex, // 219
			IRR_GL_OES_draw_texture, // 7
			IRR_GL_OES_element_index_uint, // 26
			IRR_GL_OES_extended_matrix_palette, // 8
			IRR_GL_OES_fbo_render_mipmap, // 27
			IRR_GL_OES_fixed_point, // 9
			IRR_GL_OES_fragment_precision_high, // 28
			IRR_GL_OES_framebuffer_object, // 10
			IRR_GL_OES_geometry_shader, // 210
			IRR_GL_OES_get_program_binary, // 47
			IRR_GL_OES_gpu_shader5, // 211
			IRR_GL_OES_mapbuffer, // 29
			IRR_GL_OES_matrix_get, // 11
			IRR_GL_OES_matrix_palette, // 12
			IRR_GL_OES_packed_depth_stencil, // 44
			IRR_GL_OES_point_size_array, // 14
			IRR_GL_OES_point_sprite, // 15
			IRR_GL_OES_primitive_bounding_box, // 212
			IRR_GL_OES_query_matrix, // 16
			IRR_GL_OES_read_format, // 17
			IRR_GL_OES_required_internalformat, // 115
			IRR_GL_OES_rgb8_rgba8, // 30
			IRR_GL_OES_sample_shading, // 169
			IRR_GL_OES_sample_variables, // 170
			IRR_GL_OES_shader_image_atomic, // 171
			IRR_GL_OES_shader_io_blocks, // 213
			IRR_GL_OES_shader_multisample_interpolation, // 172
			IRR_GL_OES_single_precision, // 18
			IRR_GL_OES_standard_derivatives, // 45
			IRR_GL_OES_stencil1, // 31
			IRR_GL_OES_stencil4, // 32
			IRR_GL_OES_stencil8, // 33
			IRR_GL_OES_stencil_wrap, // 19
			IRR_GL_OES_surfaceless_context, // 116
			IRR_GL_OES_tessellation_shader, // 214
			IRR_GL_OES_texture_3D, // 34
			IRR_GL_OES_texture_border_clamp, // 215
			IRR_GL_OES_texture_buffer, // 216
			IRR_GL_OES_texture_compression_astc, // 162
			IRR_GL_OES_texture_cube_map, // 20
			IRR_GL_OES_texture_cube_map_array, // 217
			IRR_GL_OES_texture_env_crossbar, // 21
			IRR_GL_OES_texture_float, // 36
			IRR_GL_OES_texture_float_linear, // 35
			IRR_GL_OES_texture_half_float, // 36
			IRR_GL_OES_texture_half_float_linear, // 35
			IRR_GL_OES_texture_mirrored_repeat, // 22
			IRR_GL_OES_texture_npot, // 37
			IRR_GL_OES_texture_stencil8, // 173
			IRR_GL_OES_texture_storage_multisample_2d_array, // 174
			IRR_GL_OES_texture_view, // 218
			IRR_GL_OES_vertex_array_object, // 71
			IRR_GL_OES_vertex_half_float, // 38
			IRR_GL_OES_vertex_type_10_10_10_2, // 46
			IRR_GL_OES_viewport_array, // 267
			IRR_GL_OVR_multiview, // 241
			IRR_GL_OVR_multiview2, // 242
			IRR_GL_OVR_multiview_multisampled_render_to_texture, // 250
			IRR_GL_QCOM_alpha_test, // 89
			IRR_GL_QCOM_binning_control, // 119
			IRR_GL_QCOM_driver_control, // 55
			IRR_GL_QCOM_extended_get, // 62
			IRR_GL_QCOM_extended_get2, // 63
			IRR_GL_QCOM_framebuffer_foveated, // 273
			IRR_GL_QCOM_performance_monitor_global_mode, // 56
			IRR_GL_QCOM_shader_framebuffer_fetch_noncoherent, // 277
			IRR_GL_QCOM_tiled_rendering, // 70
			IRR_GL_QCOM_writeonly_rendering, // 61
			IRR_GL_SUN_multi_draw_arrays, // 69
			IRR_GL_VIV_shader_binary, // 85
			WGL_ARB_context_flush_control, // 191

			IRR_OGLES_Feature_Count
		};

		COGLESCoreExtensionHandler()
		:	Version(0), MaxAnisotropy(1), MaxIndices(0xffff),
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

		const COpenGLCoreFeature& getFeature() const
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

		const char* getFeatureString(size_t index) const
		{
			// Extension names from https://www.khronos.org/registry/OpenGL/index_es.php
			// One for each EOGLESFeatures
			static const char* const OGLESFeatureStrings[IRR_OGLES_Feature_Count] =
			{
				"GLX_ARB_context_flush_control",
				"GL_AMD_compressed_3DC_texture",
				"GL_AMD_compressed_ATC_texture",
				"GL_AMD_performance_monitor",
				"GL_AMD_program_binary_Z400",
				"GL_ANDROID_extension_pack_es31a",
				"GL_ANGLE_depth_texture",
				"GL_ANGLE_framebuffer_blit",
				"GL_ANGLE_framebuffer_multisample",
				"GL_ANGLE_instanced_arrays",
				"GL_ANGLE_pack_reverse_row_order",
				"GL_ANGLE_program_binary",
				"GL_ANGLE_texture_compression_dxt1",
				"GL_ANGLE_texture_compression_dxt3",
				"GL_ANGLE_texture_compression_dxt5",
				"GL_ANGLE_texture_usage",
				"GL_ANGLE_translated_shader_source",
				"GL_APPLE_clip_distance",
				"GL_APPLE_color_buffer_packed_float",
				"GL_APPLE_copy_texture_levels",
				"GL_APPLE_framebuffer_multisample",
				"GL_APPLE_rgb_422",
				"GL_APPLE_sync",
				"GL_APPLE_texture_2D_limited_npot",
				"GL_APPLE_texture_format_BGRA8888",
				"GL_APPLE_texture_max_level",
				"GL_APPLE_texture_packed_float",
				"GL_ARB_texture_env_combine",
				"GL_ARB_texture_env_dot3",
				"GL_ARM_mali_program_binary",
				"GL_ARM_mali_shader_binary",
				"GL_ARM_rgba8",
				"GL_ARM_shader_framebuffer_fetch",
				"GL_ARM_shader_framebuffer_fetch_depth_stencil",
				"GL_DMP_program_binary",
				"GL_DMP_shader_binary",
				"GL_EXT_EGL_image_array",
				"GL_EXT_YUV_target",
				"GL_EXT_base_instance",
				"GL_EXT_blend_func_extended",
				"GL_EXT_blend_minmax",
				"GL_EXT_buffer_storage",
				"GL_EXT_clear_texture",
				"GL_EXT_clip_control",
				"GL_EXT_clip_cull_distance",
				"GL_EXT_color_buffer_float",
				"GL_EXT_color_buffer_half_float",
				"GL_EXT_compressed_ETC1_RGB8_sub_texture",
				"GL_EXT_conservative_depth",
				"GL_EXT_copy_image",
				"GL_EXT_debug_label",
				"GL_EXT_debug_marker",
				"GL_EXT_discard_framebuffer",
				"GL_EXT_disjoint_timer_query",
				"GL_EXT_draw_buffers",
				"GL_EXT_draw_buffers_indexed",
				"GL_EXT_draw_elements_base_vertex",
				"GL_EXT_draw_instanced",
				"GL_EXT_draw_transform_feedback",
				"GL_EXT_external_buffer",
				"GL_EXT_float_blend",
				"GL_EXT_frag_depth",
				"GL_EXT_geometry_point_size",
				"GL_EXT_geometry_shader",
				"GL_EXT_gpu_shader5",
				"GL_EXT_instanced_arrays",
				"GL_EXT_map_buffer_range",
				"GL_EXT_memory_object",
				"GL_EXT_memory_object_fd",
				"GL_EXT_memory_object_win32",
				"GL_EXT_multi_draw_arrays",
				"GL_EXT_multi_draw_indirect",
				"GL_EXT_multisample_compatibility",
				"GL_EXT_multisampled_render_to_texture",
				"GL_EXT_multisampled_render_to_texture2",
				"GL_EXT_multiview_draw_buffers",
				"GL_EXT_occlusion_query_boolean",
				"GL_EXT_polygon_offset_clamp",
				"GL_EXT_post_depth_coverage",
				"GL_EXT_primitive_bounding_box",
				"GL_EXT_protected_textures",
				"GL_EXT_pvrtc_sRGB",
				"GL_EXT_raster_multisample",
				"GL_EXT_read_format_bgra",
				"GL_EXT_render_snorm",
				"GL_EXT_robustness",
				"GL_EXT_sRGB",
				"GL_EXT_sRGB_write_control",
				"GL_EXT_semaphore",
				"GL_EXT_semaphore_fd",
				"GL_EXT_semaphore_win32",
				"GL_EXT_separate_shader_objects",
				"GL_EXT_shader_framebuffer_fetch",
				"GL_EXT_shader_group_vote",
				"GL_EXT_shader_implicit_conversions",
				"GL_EXT_shader_integer_mix",
				"GL_EXT_shader_io_blocks",
				"GL_EXT_shader_non_constant_global_initializers",
				"GL_EXT_shader_pixel_local_storage",
				"GL_EXT_shader_pixel_local_storage2",
				"GL_EXT_shader_texture_lod",
				"GL_EXT_shadow_samplers",
				"GL_EXT_sparse_texture",
				"GL_EXT_sparse_texture2",
				"GL_EXT_tessellation_point_size",
				"GL_EXT_tessellation_shader",
				"GL_EXT_texture_border_clamp",
				"GL_EXT_texture_buffer",
				"GL_EXT_texture_compression_astc_decode_mode",
				"GL_EXT_texture_compression_astc_decode_mode_rgb9e5",
				"GL_EXT_texture_compression_bptc",
				"GL_EXT_texture_compression_dxt1",
				"GL_EXT_texture_compression_rgtc",
				"GL_EXT_texture_compression_s3tc",
				"GL_EXT_texture_compression_s3tc_srgb",
				"GL_EXT_texture_cube_map_array",
				"GL_EXT_texture_filter_anisotropic",
				"GL_EXT_texture_filter_minmax",
				"GL_EXT_texture_format_BGRA8888",
				"GL_EXT_texture_lod_bias",
				"GL_EXT_texture_norm16",
				"GL_EXT_texture_rg",
				"GL_EXT_texture_sRGB_R8",
				"GL_EXT_texture_sRGB_RG8",
				"GL_EXT_texture_sRGB_decode",
				"GL_EXT_texture_storage",
				"GL_EXT_texture_type_2_10_10_10_REV",
				"GL_EXT_texture_view",
				"GL_EXT_unpack_subimage",
				"GL_EXT_win32_keyed_mutex",
				"GL_EXT_window_rectangles",
				"GL_FJ_shader_binary_GCCSO",
				"GL_IMG_bindless_texture",
				"GL_IMG_framebuffer_downsample",
				"GL_IMG_multisampled_render_to_texture",
				"GL_IMG_program_binary",
				"GL_IMG_read_format",
				"GL_IMG_shader_binary",
				"GL_IMG_texture_compression_pvrtc",
				"GL_IMG_texture_compression_pvrtc2",
				"GL_IMG_texture_env_enhanced_fixed_function",
				"GL_IMG_texture_format_BGRA8888",
				"GL_IMG_texture_filter_cubic",
				"GL_IMG_user_clip_plane",
				"GL_IMG_vertex_program",
				"GL_INTEL_conservative_rasterization",
				"GL_INTEL_framebuffer_CMAA",
				"GL_INTEL_performance_query",
				"GL_KHR_blend_equation_advanced",
				"GL_KHR_blend_equation_advanced_coherent",
				"GL_KHR_context_flush_control",
				"GL_KHR_debug",
				"GL_KHR_no_error",
				"GL_KHR_parallel_shader_compile",
				"GL_KHR_robust_buffer_access_behavior",
				"GL_KHR_robustness",
				"GL_KHR_texture_compression_astc_hdr",
				"GL_KHR_texture_compression_astc_ldr",
				"GL_KHR_texture_compression_astc_sliced_3d",
				"GL_NVX_blend_equation_advanced_multi_draw_buffers",
				"GL_NV_3dvision_settings",
				"GL_NV_EGL_stream_consumer_external",
				"GL_NV_bgr",
				"GL_NV_bindless_texture",
				"GL_NV_blend_equation_advanced",
				"GL_NV_blend_equation_advanced_coherent",
				"GL_NV_blend_minmax_factor",
				"GL_NV_conditional_render",
				"GL_NV_conservative_raster",
				"GL_NV_conservative_raster_pre_snap_triangles",
				"GL_NV_copy_buffer",
				"GL_NV_coverage_sample",
				"GL_NV_depth_nonlinear",
				"GL_NV_draw_buffers",
				"GL_NV_draw_instanced",
				"GL_NV_draw_texture",
				"GL_NV_draw_vulkan_image",
				"GL_NV_explicit_attrib_location",
				"GL_NV_fbo_color_attachments",
				"GL_NV_fence",
				"GL_NV_fill_rectangle",
				"GL_NV_fragment_coverage_to_color",
				"GL_NV_fragment_shader_interlock",
				"GL_NV_framebuffer_blit",
				"GL_NV_framebuffer_mixed_samples",
				"GL_NV_framebuffer_multisample",
				"GL_NV_generate_mipmap_sRGB",
				"GL_NV_geometry_shader_passthrough",
				"GL_NV_gpu_shader5",
				"GL_NV_image_formats",
				"GL_NV_instanced_arrays",
				"GL_NV_internalformat_sample_query",
				"GL_NV_non_square_matrices",
				"GL_NV_pack_subimage",
				"GL_NV_packed_float",
				"GL_NV_path_rendering",
				"GL_NV_path_rendering_shared_edge",
				"GL_NV_pixel_buffer_object",
				"GL_NV_platform_binary",
				"GL_NV_polygon_mode",
				"GL_NV_read_buffer",
				"GL_NV_read_buffer_front",
				"GL_NV_read_depth",
				"GL_NV_read_depth_stencil",
				"GL_NV_read_stencil",
				"GL_NV_sRGB_formats",
				"GL_NV_sample_locations",
				"GL_NV_sample_mask_override_coverage",
				"GL_NV_shader_atomic_fp16_vector",
				"GL_NV_shader_noperspective_interpolation",
				"GL_NV_shadow_samplers_array",
				"GL_NV_shadow_samplers_cube",
				"GL_NV_texture_array",
				"GL_NV_texture_barrier",
				"GL_NV_texture_border_clamp",
				"GL_NV_texture_compression_latc",
				"GL_NV_texture_compression_s3tc",
				"GL_NV_texture_compression_s3tc_update",
				"GL_NV_texture_npot_2D_mipmap",
				"GL_NV_viewport_array",
				"GL_NV_viewport_array2",
				"GL_NV_viewport_swizzle",
				"GL_OES_EGL_image",
				"GL_OES_EGL_image_external",
				"GL_OES_EGL_image_external_essl3",
				"GL_OES_EGL_sync",
				"GL_OES_blend_equation_separate",
				"GL_OES_blend_func_separate",
				"GL_OES_blend_subtract",
				"GL_OES_byte_coordinates",
				"GL_OES_compressed_ETC1_RGB8_texture",
				"GL_OES_compressed_paletted_texture",
				"GL_OES_copy_image",
				"GL_OES_depth24",
				"GL_OES_depth32",
				"GL_OES_depth_texture",
				"GL_OES_depth_texture_cube_map",
				"GL_OES_draw_buffers_indexed",
				"GL_OES_draw_elements_base_vertex",
				"GL_OES_draw_texture",
				"GL_OES_element_index_uint",
				"GL_OES_extended_matrix_palette",
				"GL_OES_fbo_render_mipmap",
				"GL_OES_fixed_point",
				"GL_OES_fragment_precision_high",
				"GL_OES_framebuffer_object",
				"GL_OES_geometry_shader",
				"GL_OES_get_program_binary",
				"GL_OES_gpu_shader5",
				"GL_OES_mapbuffer",
				"GL_OES_matrix_get",
				"GL_OES_matrix_palette",
				"GL_OES_packed_depth_stencil",
				"GL_OES_point_size_array",
				"GL_OES_point_sprite",
				"GL_OES_primitive_bounding_box",
				"GL_OES_query_matrix",
				"GL_OES_read_format",
				"GL_OES_required_internalformat",
				"GL_OES_rgb8_rgba8",
				"GL_OES_sample_shading",
				"GL_OES_sample_variables",
				"GL_OES_shader_image_atomic",
				"GL_OES_shader_io_blocks",
				"GL_OES_shader_multisample_interpolation",
				"GL_OES_single_precision",
				"GL_OES_standard_derivatives",
				"GL_OES_stencil1",
				"GL_OES_stencil4",
				"GL_OES_stencil8",
				"GL_OES_stencil_wrap",
				"GL_OES_surfaceless_context",
				"GL_OES_tessellation_shader",
				"GL_OES_texture_3D",
				"GL_OES_texture_border_clamp",
				"GL_OES_texture_buffer",
				"GL_OES_texture_compression_astc",
				"GL_OES_texture_cube_map",
				"GL_OES_texture_cube_map_array",
				"GL_OES_texture_env_crossbar",
				"GL_OES_texture_float",
				"GL_OES_texture_float_linear",
				"GL_OES_texture_half_float",
				"GL_OES_texture_half_float_linear",
				"GL_OES_texture_mirrored_repeat",
				"GL_OES_texture_npot",
				"GL_OES_texture_stencil8",
				"GL_OES_texture_storage_multisample_2d_array",
				"GL_OES_texture_view",
				"GL_OES_vertex_array_object",
				"GL_OES_vertex_half_float",
				"GL_OES_vertex_type_10_10_10_2",
				"GL_OES_viewport_array",
				"GL_OVR_multiview",
				"GL_OVR_multiview2",
				"GL_OVR_multiview_multisampled_render_to_texture",
				"GL_QCOM_alpha_test",
				"GL_QCOM_binning_control",
				"GL_QCOM_driver_control",
				"GL_QCOM_extended_get",
				"GL_QCOM_extended_get2",
				"GL_QCOM_framebuffer_foveated",
				"GL_QCOM_performance_monitor_global_mode",
				"GL_QCOM_shader_framebuffer_fetch_noncoherent",
				"GL_QCOM_tiled_rendering",
				"GL_QCOM_writeonly_rendering",
				"GL_SUN_multi_draw_arrays",
				"GL_VIV_shader_binary",
				"WGL_ARB_context_flush_control"
			};

			return OGLESFeatureStrings[index];
		}


		void getGLVersion()
		{
			Version = 0;
			s32 multiplier = 100;

			core::stringc version(glGetString(GL_VERSION));

			for (u32 i = 0; i < version.size(); ++i)
			{
				if (version[i] >= '0' && version[i] <= '9')
				{
					if (multiplier > 1)
					{
						Version += static_cast<u16>(core::floor32(atof(&(version[i]))) * multiplier);
						multiplier /= 10;
					}
					else
					{
						break;
					}
				}
			}
		}

		void getGLExtensions()
		{
			core::stringc extensions = glGetString(GL_EXTENSIONS);
			os::Printer::log(extensions.c_str());

			// typo in the simulator (note the postfixed s)
			if (extensions.find("GL_IMG_user_clip_planes"))
				FeatureAvailable[IRR_GL_IMG_user_clip_plane] = true;

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
						for (size_t j=0; j<IRR_OGLES_Feature_Count; ++j)
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

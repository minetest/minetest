/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>
Copyright (C) 2020 appgurueu, Lars Mueller <appgurulars@gmx.de>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "secondstage.h"
#include "client/client.h"
#include "client/shader.h"
#include "client/tile.h"

PostProcessingStep::PostProcessingStep(u32 _shader_id, const std::vector<u8> &_texture_map) :
	shader_id(_shader_id), texture_map(_texture_map)
{
	assert(texture_map.size() <= video::MATERIAL_MAX_TEXTURES);
	configureMaterial();
}

void PostProcessingStep::configureMaterial()
{
	material.UseMipMaps = false;
	material.ZBuffer = true;
	material.ZWriteEnable = video::EZW_ON;
	for (u32 k = 0; k < texture_map.size(); ++k) {
		material.TextureLayer[k].AnisotropicFilter = false;
		material.TextureLayer[k].BilinearFilter = false;
		material.TextureLayer[k].TrilinearFilter = false;
		material.TextureLayer[k].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
		material.TextureLayer[k].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	}
}

void PostProcessingStep::setRenderSource(RenderSource *_source)
{
	source = _source;
}

void PostProcessingStep::setRenderTarget(RenderTarget *_target)
{
	target = _target;
}

void PostProcessingStep::reset(PipelineContext &context)
{
}

void PostProcessingStep::run(PipelineContext &context)
{
	if (target)
		target->activate(context);

	// attach the shader
	material.MaterialType = context.client->getShaderSource()->getShaderInfo(shader_id).material;

	auto driver = context.device->getVideoDriver();

	for (u32 i = 0; i < texture_map.size(); i++)
		material.TextureLayer[i].Texture = source->getTexture(texture_map[i]);

	static const video::SColor color = video::SColor(0, 0, 0, 255);
	static const video::S3DVertex vertices[4] = {
			video::S3DVertex(1.0, -1.0, 0.0, 0.0, 0.0, -1.0,
					color, 1.0, 0.0),
			video::S3DVertex(-1.0, -1.0, 0.0, 0.0, 0.0, -1.0,
					color, 0.0, 0.0),
			video::S3DVertex(-1.0, 1.0, 0.0, 0.0, 0.0, -1.0,
					color, 0.0, 1.0),
			video::S3DVertex(1.0, 1.0, 0.0, 0.0, 0.0, -1.0,
					color, 1.0, 1.0),
	};
	static const u16 indices[6] = {0, 1, 2, 2, 3, 0};
	driver->setMaterial(material);
	driver->drawVertexPrimitiveList(&vertices, 4, &indices, 2);
}

void PostProcessingStep::setBilinearFilter(u8 index, bool value)
{
	assert(index < video::MATERIAL_MAX_TEXTURES);
	material.TextureLayer[index].BilinearFilter = value;
}

RenderStep *addPostProcessing(RenderPipeline *pipeline, RenderStep *previousStep, v2f scale, Client *client)
{
	auto buffer = pipeline->createOwned<TextureBuffer>();
	auto driver = client->getSceneManager()->getVideoDriver();

	// configure texture formats
	video::ECOLOR_FORMAT color_format = video::ECF_A8R8G8B8;
	if (driver->queryTextureFormat(video::ECF_A16B16G16R16F))
		color_format = video::ECF_A16B16G16R16F;

	video::ECOLOR_FORMAT depth_format = video::ECF_D16; // fallback depth format
	if (driver->queryTextureFormat(video::ECF_D32))
		depth_format = video::ECF_D32;
	else if (driver->queryTextureFormat(video::ECF_D24S8))
		depth_format = video::ECF_D24S8;


	// init post-processing buffer
	static const u8 TEXTURE_COLOR = 0;
	static const u8 TEXTURE_DEPTH = 1;
	static const u8 TEXTURE_BLOOM = 2;
	static const u8 TEXTURE_BLOOM_DOWN = 10;
	static const u8 TEXTURE_BLOOM_UP = 20;

	buffer->setTexture(TEXTURE_COLOR, scale, "3d_render", color_format);
	buffer->setTexture(TEXTURE_DEPTH, scale, "3d_depthmap", depth_format);

	// attach buffer to the previous step
	previousStep->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, std::vector<u8> { TEXTURE_COLOR }, TEXTURE_DEPTH));

	// shared variables
	u32 shader_id;

	// post-processing stage
	// set up bloom
	if (g_settings->getBool("enable_bloom")) {


		buffer->setTexture(TEXTURE_BLOOM, scale, "bloom", color_format);

		const u8 MIPMAP_LEVELS = 4;
		v2f downscale = scale * 0.5;
		for (u8 i = 0; i < MIPMAP_LEVELS; i++) {
			buffer->setTexture(TEXTURE_BLOOM_DOWN + i, downscale, std::string("bloom_down") + std::to_string(i), color_format);
			buffer->setTexture(TEXTURE_BLOOM_UP + i, downscale, std::string("bloom_up") + std::to_string(i), color_format);
			downscale *= 0.5;
		}

		// get bright spots
		u32 shader_id = client->getShaderSource()->getShader("extract_bloom", TILE_MATERIAL_PLAIN, NDT_MESH);
		RenderStep *extract_bloom = pipeline->addStep<PostProcessingStep>(shader_id, std::vector<u8> { TEXTURE_COLOR });
		extract_bloom->setRenderSource(buffer);
		extract_bloom->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, TEXTURE_BLOOM));

		// downsample
		shader_id = client->getShaderSource()->getShader("bloom_downsample", TILE_MATERIAL_PLAIN, NDT_MESH);
		u8 source = TEXTURE_BLOOM;
		for (u8 i = 0; i < MIPMAP_LEVELS; i++) {
			auto step = pipeline->addStep<PostProcessingStep>(shader_id, std::vector<u8> { source });
			step->setRenderSource(buffer);
			step->setBilinearFilter(0, true);
			step->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, TEXTURE_BLOOM_DOWN + i));
			source = TEXTURE_BLOOM_DOWN + i;
		}

		// upsample
		shader_id = client->getShaderSource()->getShader("bloom_upsample", TILE_MATERIAL_PLAIN, NDT_MESH);
		for (u8 i = MIPMAP_LEVELS - 1; i > 0; i--) {
			auto step = pipeline->addStep<PostProcessingStep>(shader_id, std::vector<u8> { u8(TEXTURE_BLOOM_DOWN + i - 1), source });
			step->setRenderSource(buffer);
			step->setBilinearFilter(0, true);
			step->setBilinearFilter(1, true);
			step->setRenderTarget(pipeline->createOwned<TextureBufferOutput>(buffer, u8(TEXTURE_BLOOM_UP + i - 1)));
			source = TEXTURE_BLOOM_UP + i - 1;
		}
	}

	// final post-processing
	shader_id = client->getShaderSource()->getShader("second_stage", TILE_MATERIAL_PLAIN, NDT_MESH);
	PostProcessingStep *effect = pipeline->createOwned<PostProcessingStep>(shader_id, std::vector<u8> { TEXTURE_COLOR, TEXTURE_BLOOM_UP });
	pipeline->addStep(effect);
	effect->setBilinearFilter(1, true); // apply filter to the bloom
	effect->setRenderSource(buffer);
	return effect;
}

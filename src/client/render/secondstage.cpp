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

RenderStep *addPostProcessing(RenderPipeline *pipeline, RenderStep *previousStep, v2f scale, Client *client)
{
	auto buffer = pipeline->createOwned<TextureBuffer>();
	static const u8 TEXTURE_COLOR = 0;
	static const u8 TEXTURE_DEPTH = 3;

	// init post-processing buffer
	buffer->setTexture(TEXTURE_COLOR, scale, "3d_render", video::ECF_A8R8G8B8);

	video::ECOLOR_FORMAT depth_format = video::ECF_D16; // fallback depth format
	auto driver = client->getSceneManager()->getVideoDriver();
	if (driver->queryTextureFormat(video::ECF_D32))
		depth_format = video::ECF_D32;
	else if (driver->queryTextureFormat(video::ECF_D24S8))
		depth_format = video::ECF_D24S8;
	buffer->setDepthTexture(TEXTURE_DEPTH, scale, "3d_depthmap", depth_format);

	// attach buffer to the previous step
	previousStep->setRenderTarget(buffer);

	// post-processing stage
	// set up shader
	u32 shader_id = client->getShaderSource()->getShader("second_stage", TILE_MATERIAL_PLAIN, NDT_MESH);

	RenderStep *effect = pipeline->addStep<PostProcessingStep>(shader_id, std::vector<u8> { TEXTURE_COLOR });
	effect->setRenderSource(buffer);
	return effect;
}

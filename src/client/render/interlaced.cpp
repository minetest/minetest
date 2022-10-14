/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

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

#include "interlaced.h"
#include "secondstage.h"
#include "client/client.h"
#include "client/shader.h"
#include "client/camera.h"

InitInterlacedMaskStep::InitInterlacedMaskStep(TextureBuffer *_buffer, u8 _index)	: 
	buffer(_buffer), index(_index)
{
}

void InitInterlacedMaskStep::run(PipelineContext &context)
{
	video::ITexture *mask = buffer->getTexture(index);
	if (!mask)
		return;
	if (mask == last_mask)
		return;
	last_mask = mask;

	auto size = mask->getSize();
	u8 *data = reinterpret_cast<u8 *>(mask->lock());
	for (u32 j = 0; j < size.Height; j++) {
		u8 val = j % 2 ? 0xff : 0x00;
		memset(data, val, 4 * size.Width);
		data += 4 * size.Width;
	}
	mask->unlock();
}

void populateInterlacedPipeline(RenderPipeline *pipeline, Client *client)
{
	static const u8 TEXTURE_LEFT = 0;
	static const u8 TEXTURE_RIGHT = 1;
	static const u8 TEXTURE_MASK = 2;

	TextureBuffer *buffer = pipeline->createOwned<TextureBuffer>();
	buffer->setTexture(TEXTURE_LEFT, v2f(1.0f, 0.5f), "3d_render_left", video::ECF_A8R8G8B8);
	buffer->setTexture(TEXTURE_RIGHT, v2f(1.0f, 0.5f), "3d_render_right", video::ECF_A8R8G8B8);
	buffer->setTexture(TEXTURE_MASK, v2f(1.0f, 1.0f), "3d_render_mask", video::ECF_A8R8G8B8);

	pipeline->addStep<InitInterlacedMaskStep>(buffer, TEXTURE_MASK);

	auto step3D = pipeline->own(create3DStage(client, v2f(1.0f, 0.5f)));

	// eyes
	for (bool right : { false, true }) {
		pipeline->addStep<OffsetCameraStep>(right);
		auto output = pipeline->createOwned<TextureBufferOutput>(buffer, right ? TEXTURE_RIGHT : TEXTURE_LEFT);
		pipeline->addStep<SetRenderTargetStep>(step3D, output);
		pipeline->addStep(step3D);
		pipeline->addStep<MapPostFxStep>();
	}

	pipeline->addStep<OffsetCameraStep>(0.0f);
	IShaderSource *s = client->getShaderSource();
	u32 shader = s->getShader("3d_interlaced_merge", TILE_MATERIAL_BASIC);
	video::E_MATERIAL_TYPE material = s->getShaderInfo(shader).material;
	auto texture_map = { TEXTURE_LEFT, TEXTURE_RIGHT, TEXTURE_MASK };
	auto merge = pipeline->addStep<PostProcessingStep>(material, texture_map);
	merge->setRenderSource(buffer);
	merge->setRenderTarget(pipeline->createOwned<ScreenTarget>());
	pipeline->addStep<DrawHUD>();
}
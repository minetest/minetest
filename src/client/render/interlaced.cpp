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

RenderingCoreInterlaced::RenderingCoreInterlaced(
	IrrlichtDevice *_device, Client *_client, Hud *_hud)
	: RenderingCoreStereo(_device, _client, _hud),
	buffer(_device->getVideoDriver())
{
}

void RenderingCoreInterlaced::initTextures()
{
	v2u32 image_size{screensize.X, screensize.Y / 2};
	buffer.setTexture(TEXTURE_LEFT, image_size.X, image_size.Y, "3d_render_left", video::ECF_A8R8G8B8);
	buffer.setTexture(TEXTURE_RIGHT, image_size.X, image_size.Y, "3d_render_right", video::ECF_A8R8G8B8);
	buffer.setTexture(TEXTURE_MASK, screensize.X, screensize.Y, "3d_render_mask", video::ECF_A8R8G8B8);
	initMask();
}

void RenderingCoreInterlaced::initMask()
{
	auto mask = buffer.getTexture(TEXTURE_MASK);
	u8 *data = reinterpret_cast<u8 *>(mask->lock());
	for (u32 j = 0; j < screensize.Y; j++) {
		u8 val = j % 2 ? 0xff : 0x00;
		memset(data, val, 4 * screensize.X);
		data += 4 * screensize.X;
	}
	mask->unlock();
}

void RenderingCoreInterlaced::createPipeline()
{
	auto cam_node = camera->getCameraNode();

	// eyes
	for (bool right : { false, true }) {
		pipeline.addStep(pipeline.own(new OffsetCameraStep(cam_node, right)));
		auto step3D = new Draw3D(&pipelineState, smgr, driver, hud, camera);
		pipeline.addStep(pipeline.own(step3D));
		auto output = new TextureBufferOutput(driver, &buffer, right ? TEXTURE_RIGHT : TEXTURE_LEFT);
		output->setClearColor(&skycolor);
		step3D->setRenderTarget(pipeline.own(output));
	}

	IShaderSource *s = client->getShaderSource();
	u32 shader = s->getShader("3d_interlaced_merge", TILE_MATERIAL_BASIC);
	auto merge = new PostProcessingStep(driver, s->getShaderInfo(shader).material, { TEXTURE_LEFT, TEXTURE_RIGHT, TEXTURE_MASK });
	merge->setRenderSource(&buffer);
	merge->setRenderTarget(screen);
	pipeline.addStep(pipeline.own(merge));
	pipeline.addStep(stepHUD);
}
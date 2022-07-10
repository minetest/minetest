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

#include "sidebyside.h"
#include "client/hud.h"
#include "client/camera.h"

DrawImageStep::DrawImageStep(u8 texture_index, v2s32 *pos) :
	texture_index(texture_index), pos(pos)
{}

void DrawImageStep::setRenderSource(RenderSource *_source)
{
	source = _source;
}
void DrawImageStep::setRenderTarget(RenderTarget *_target)
{
	target = _target;
}

void DrawImageStep::run(PipelineContext *context)
{
	if (target)
		target->activate(context);
	
	auto texture = source->getTexture(texture_index);
	context->device->getVideoDriver()->draw2DImage(texture, pos ? *pos : v2s32 {});
}

RenderingCoreSideBySide::RenderingCoreSideBySide(
	IrrlichtDevice *_device, Client *_client, Hud *_hud, bool _horizontal, bool _flipped)
	: RenderingCoreStereo(_device, _client, _hud), buffer(driver), horizontal(_horizontal), flipped(_flipped)
{
}

void RenderingCoreSideBySide::initTextures()
{
	core::dimension2du image_size;
	if (horizontal) {
		image_size = {screensize.X, screensize.Y / 2};
		rpos = v2s32(0, screensize.Y / 2);
	} else {
		image_size = {screensize.X / 2, screensize.Y};
		rpos = v2s32(screensize.X / 2, 0);
	}
	virtual_size = image_size;
	buffer.setTexture(TEXTURE_LEFT, image_size.Width, image_size.Height, "3d_render_left", video::ECF_A8R8G8B8);
	buffer.setTexture(TEXTURE_RIGHT, image_size.Width, image_size.Height, "3d_render_right", video::ECF_A8R8G8B8);
}

void RenderingCoreSideBySide::createPipeline()
{
	// eyes
	for (bool right : { false, true }) {
		pipeline.addStep(pipeline.own(new OffsetCameraStep(flipped ? !right : right)));
		auto step3D = new Draw3D(&pipelineState);
		pipeline.addStep(pipeline.own(step3D));
		auto output = new TextureBufferOutput(&buffer, right ? TEXTURE_RIGHT : TEXTURE_LEFT);
		output->setClearColor(&skycolor);
		step3D->setRenderTarget(pipeline.own(output));
		pipeline.addStep(stepPostFx);
		pipeline.addStep(stepHUD);
	}

	pipeline.addStep(pipeline.own(new OffsetCameraStep(0.0f)));
	for (bool right : { false, true }) {
		auto step = new DrawImageStep(right ? TEXTURE_RIGHT : TEXTURE_LEFT, right ? &rpos : nullptr);
		step->setRenderSource(&buffer);
		step->setRenderTarget(screen);
		pipeline.addStep(pipeline.own(step));
	}
}

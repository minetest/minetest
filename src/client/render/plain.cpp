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

#include "plain.h"
#include "settings.h"

inline u32 scaledown(u32 coef, u32 size)
{
	return (size + coef - 1) / coef;
}

RenderingCorePlain::RenderingCorePlain(IrrlichtDevice *_device, Client *_client, Hud *_hud) : 
		RenderingCore(_device, _client, _hud), buffer(_device->getVideoDriver())
{
	scale = g_settings->getU16("undersampling");
}

void RenderingCorePlain::initTextures()
{
	if (scale <= 1)
		return;
	v2u32 size{scaledown(scale, screensize.X), scaledown(scale, screensize.Y)};
	buffer.setTexture(TEXTURE_UPSCALE, size.X, size.Y, "upscale", video::ECF_A8R8G8B8);
}

void RenderingCorePlain::createPipeline()
{
	pipeline.addStep(step3D);
	pipeline.addStep(stepPostFx);
	if (scale > 1) {
	    TextureBufferOutput *buffer_output = new TextureBufferOutput(driver, &buffer, TEXTURE_UPSCALE);
		buffer_output->setClearColor(&skycolor);
		step3D->setRenderTarget(pipeline.own(buffer_output));
		upscale.setRenderSource(&buffer);
		upscale.setRenderTarget(screen);
		pipeline.addStep(&upscale);
	}
	pipeline.addStep(stepHUD);
}

// class UpscaleStep

void UpscaleStep::run(PipelineContext *context)
{
	video::ITexture *lowres = m_source->getTexture(0);
	m_target->activate(context);
	context->device->getVideoDriver()->draw2DImage(lowres,
			core::rect<s32>(0, 0, context->target_size.X, context->target_size.Y),
			core::rect<s32>(0, 0, lowres->getSize().Width, lowres->getSize().Height));
}

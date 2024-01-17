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

#include <memory>

#include "core.h"
#include "IImage.h"
#include "plain.h"
#include "client/shadows/dynamicshadowsrender.h"
#include "settings.h"
#include "client/renderingengine.h"
#include "client/client.h"

RenderingCore::RenderingCore(IrrlichtDevice *_device, Client *_client, Hud *_hud,
		ShadowRenderer *_shadow_renderer, RenderPipeline *_pipeline, v2f _virtual_size_scale)
	: device(_device), client(_client), hud(_hud), shadow_renderer(_shadow_renderer),
	pipeline(_pipeline), virtual_size_scale(_virtual_size_scale)
{
	if (client->getRenderingEngine()->headless) {
		headless_buffer = pipeline->createOwned<TextureBuffer>();
		headless_buffer->setTexture(0, v2f(1.0f, 1.0f), "idk_lol", video::ECF_R8G8B8);
	}
}

RenderingCore::~RenderingCore()
{
	delete pipeline;
	delete shadow_renderer;
}

void RenderingCore::draw(video::SColor _skycolor, bool _show_hud, bool _show_minimap,
		bool _draw_wield_tool, bool _draw_crosshair)
{
	v2u32 screensize = device->getVideoDriver()->getScreenSize();
	virtual_size = v2u32(screensize.X * virtual_size_scale.X, screensize.Y * virtual_size_scale.Y);

	PipelineContext context(device, client, hud, shadow_renderer, _skycolor, screensize);
	context.draw_crosshair = _draw_crosshair;
	context.draw_wield_tool = _draw_wield_tool;
	context.show_hud = _show_hud;
	context.show_minimap = _show_minimap;

	if (client->getRenderingEngine()->headless) {
		auto tex = std::make_unique<TextureBufferOutput>(headless_buffer, 0);
		pipeline->setRenderTarget(tex.get());
		pipeline->reset(context);
		pipeline->run(context);

		std::unique_ptr<irr::video::ITexture, std::function<void(irr::video::ITexture*)>> t(
			headless_buffer->getTexture(0),
			[](irr::video::ITexture* t) {
				if (t) t->unlock();
			});
		std::unique_ptr<irr::video::IImage, DropDeleter> raw_image(
			device->getVideoDriver()->createImageFromData(
				t->getColorFormat(), screensize,
				t->lock(irr::video::E_TEXTURE_LOCK_MODE::ETLM_READ_ONLY),
				false /* copy mem*/),
			DropDeleter{});
		screenshot.reset(device->getVideoDriver()->createImage(video::ECF_R8G8B8, screensize));
		raw_image->copyTo(screenshot.get());
	} else {
		pipeline->reset(context);
		pipeline->run(context);
	}
}

video::IImage *RenderingCore::get_screenshot()
{
	if (!screenshot)
		return nullptr;
	auto copy = device->getVideoDriver()->createImage(
			video::ECF_R8G8B8, screenshot->getDimension());
	screenshot->copyTo(copy);
	return copy;
}

v2u32 RenderingCore::getVirtualSize() const
{
	return virtual_size;
}

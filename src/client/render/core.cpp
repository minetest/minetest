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

#include "core.h"
#include "plain.h"
#include "client/shadows/dynamicshadowsrender.h"
#include "settings.h"


RenderingCore::RenderingCore(IrrlichtDevice *_device, Client *_client, Hud *_hud)
	: device(_device), client(_client), hud(_hud), shadow_renderer(nullptr), pipeline(new RenderPipeline())
{
	// disable if unsupported
	if (g_settings->getBool("enable_dynamic_shadows") && (
		g_settings->get("video_driver") != "opengl" ||
		!g_settings->getBool("enable_shaders"))) {
		g_settings->setBool("enable_dynamic_shadows", false);
	}

	if (g_settings->getBool("enable_shaders") &&
			g_settings->getBool("enable_dynamic_shadows")) {
		shadow_renderer = new ShadowRenderer(device, client);
	}

	screen = new ScreenTarget();
	pipeline->own(screen);
	scene_output = screen;
}

RenderingCore::~RenderingCore()
{
	delete pipeline;
	delete shadow_renderer;
}

void RenderingCore::initialize()
{
	if (shadow_renderer) {
		shadow_renderer->initialize();
		pipeline->addStep(pipeline->own(new RenderShadowMapStep()));
	}

	createPipeline();
}

void RenderingCore::draw(video::SColor _skycolor, bool _show_hud, bool _show_minimap,
		bool _draw_wield_tool, bool _draw_crosshair)
{
	v2u32 screensize = device->getVideoDriver()->getScreenSize();

	PipelineContext context(device, client, hud, shadow_renderer, _skycolor, screensize);
	context.draw_crosshair = _draw_crosshair;
	context.draw_wield_tool = _draw_wield_tool;
	context.show_hud = _show_hud;
	context.show_minimap = _show_minimap;

	pipeline->reset(&context);
	pipeline->run(&context);
}

v2u32 RenderingCore::getVirtualSize() const
{
	return scene_output->getSize();
}
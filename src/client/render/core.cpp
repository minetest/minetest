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
#include "client/camera.h"
#include "client/client.h"
#include "client/clientmap.h"
#include "client/hud.h"
#include "client/minimap.h"
#include "client/shadows/dynamicshadowsrender.h"

RenderingCore::RenderingCore(IrrlichtDevice *_device, Client *_client, Hud *_hud)
	: device(_device), driver(device->getVideoDriver()), smgr(device->getSceneManager()),
	guienv(device->getGUIEnvironment()), client(_client), camera(client->getCamera()),
	mapper(client->getMinimap()), hud(_hud),
	shadow_renderer(nullptr)
{
	screensize = driver->getScreenSize();
	virtual_size = screensize;

	if (g_settings->getBool("enable_shaders") &&
			g_settings->getBool("enable_dynamic_shadows")) {
		shadow_renderer = new ShadowRenderer(device, client);
	}
}

RenderingCore::~RenderingCore()
{
	clearTextures();
	delete shadow_renderer;
}

void RenderingCore::initialize()
{
	// have to be called late as the VMT is not ready in the constructor:
	initTextures();
	if (shadow_renderer)
		shadow_renderer->initialize();
}

void RenderingCore::updateScreenSize()
{
	virtual_size = screensize;
	clearTextures();
	initTextures();
}

void RenderingCore::draw(video::SColor _skycolor, bool _show_hud, bool _show_minimap,
		bool _draw_wield_tool, bool _draw_crosshair)
{
	v2u32 ss = driver->getScreenSize();
	if (screensize != ss) {
		screensize = ss;
		updateScreenSize();
	}
	skycolor = _skycolor;
	show_hud = _show_hud;
	show_minimap = _show_minimap;
	draw_wield_tool = _draw_wield_tool;
	draw_crosshair = _draw_crosshair;

	if (shadow_renderer)
		shadow_renderer->update();

	beforeDraw();
	drawAll();
}

void RenderingCore::draw3D()
{
	smgr->drawAll();
	if (shadow_renderer)
		shadow_renderer->drawDebug();

	driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	if (!show_hud)
		return;
	hud->drawBlockBounds();
	hud->drawSelectionMesh();
	if (draw_wield_tool)
		camera->drawWieldedTool();
}

void RenderingCore::drawHUD()
{
	if (show_hud) {
		if (draw_crosshair)
			hud->drawCrosshair();
	
		hud->drawHotbar(client->getEnv().getLocalPlayer()->getWieldIndex());
		hud->drawLuaElements(camera->getOffset());
		camera->drawNametags();
		if (mapper && show_minimap)
			mapper->drawMinimap();
	}
	guienv->drawAll();
}

void RenderingCore::drawPostFx()
{
	client->getEnv().getClientMap().renderPostFx(camera->getCameraMode());
}

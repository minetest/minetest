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

/// Draw3D pipeline step
void Draw3D::run()
{
	m_smgr->drawAll();
	m_driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	if (!m_state->show_hud)
		return;
	m_hud->drawBlockBounds();
	m_hud->drawSelectionMesh();
	if (m_state->draw_wield_tool)
		m_camera->drawWieldedTool();
}

void DrawHUD::run()
{
	if (m_state->show_hud) {
		if (m_shadow_renderer)
			m_shadow_renderer->drawDebug();

		if (m_state->draw_crosshair)
			m_hud->drawCrosshair();

		m_hud->drawHotbar(m_client->getEnv().getLocalPlayer()->getWieldIndex());
		m_hud->drawLuaElements(m_camera->getOffset());
		m_camera->drawNametags();
		if (m_mapper && m_state->show_minimap)
			m_mapper->drawMinimap();
	}
	m_guienv->drawAll();
}



RenderingCore::RenderingCore(IrrlichtDevice *_device, Client *_client, Hud *_hud)
	: device(_device), driver(device->getVideoDriver()), smgr(device->getSceneManager()),
	guienv(device->getGUIEnvironment()), client(_client), camera(client->getCamera()),
	mapper(client->getMinimap()), hud(_hud),
	shadow_renderer(nullptr)
{
	screensize = driver->getScreenSize();
	virtual_size = screensize;

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

	screen = new ScreenTarget(driver);
	step3D = new Draw3D(&pipelineState, smgr, driver, hud, camera);
	stepHUD = new DrawHUD(&pipelineState, hud, camera, mapper, client, guienv, shadow_renderer);
}

RenderingCore::~RenderingCore()
{
	delete step3D;
	delete stepHUD;
	clearTextures();
	delete shadow_renderer;
	delete screen;
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

	pipelineState.draw_crosshair = draw_crosshair;
	pipelineState.draw_wield_tool = draw_wield_tool;
	pipelineState.show_hud = show_hud;
	pipelineState.show_minimap = show_minimap;

	step3D->reset();
	stepHUD->reset();

	if (shadow_renderer) {
		// This is necessary to render shadows for animations correctly
		smgr->getRootSceneNode()->OnAnimate(device->getTimer()->getTime());
		shadow_renderer->update();
	}

	beforeDraw();
	drawAll();
}

void RenderingCore::draw3D()
{
	step3D->run();
}

void RenderingCore::drawHUD()
{
	stepHUD->run();
}

void RenderingCore::drawPostFx()
{
	client->getEnv().getClientMap().renderPostFx(camera->getCameraMode());
}

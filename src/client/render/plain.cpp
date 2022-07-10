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
#include "client/camera.h"
#include "client/client.h"
#include "client/clientmap.h"
#include "client/hud.h"
#include "client/minimap.h"
#include "client/shadows/dynamicshadowsrender.h"

/// Draw3D pipeline step
void Draw3D::run(PipelineContext *context)
{
	if (m_target)
		m_target->activate(context);

	context->device->getSceneManager()->drawAll();
	context->device->getVideoDriver()->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	if (!context->show_hud)
		return;
	context->hud->drawBlockBounds();
	context->hud->drawSelectionMesh();
	if (context->draw_wield_tool)
		context->client->getCamera()->drawWieldedTool();
}

void DrawHUD::run(PipelineContext *context)
{
	if (context->show_hud) {
		if (context->shadow_renderer)
			context->shadow_renderer->drawDebug();

		if (context->draw_crosshair)
			context->hud->drawCrosshair();

		context->hud->drawHotbar(context->client->getEnv().getLocalPlayer()->getWieldIndex());
		context->hud->drawLuaElements(context->client->getCamera()->getOffset());
		context->client->getCamera()->drawNametags();
		auto mapper = context->client->getMinimap();
		if (mapper && context->show_minimap)
			mapper->drawMinimap();
	}
	context->device->getGUIEnvironment()->drawAll();
}


void MapPostFxStep::setRenderTarget(RenderTarget * _target)
{
	target = _target;
}

void MapPostFxStep::run(PipelineContext *context)
{
	if (target)
		target->activate(context);

	context->client->getEnv().getClientMap().renderPostFx(context->client->getCamera()->getCameraMode());
}

void RenderShadowMapStep::run(PipelineContext *context)
{
	// This is necessary to render shadows for animations correctly
	context->device->getSceneManager()->getRootSceneNode()->OnAnimate(context->device->getTimer()->getTime());
	context->shadow_renderer->update();
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

RenderingCorePlain::RenderingCorePlain(IrrlichtDevice *_device, Client *_client, Hud *_hud) : 
		RenderingCore(_device, _client, _hud)
{
	scale = g_settings->getU16("undersampling");
}

void RenderingCorePlain::createPipeline()
{
	auto step3D = pipeline->own(new Draw3D());
	pipeline->addStep(step3D);
	pipeline->addStep(pipeline->own(new MapPostFxStep()));

	if (scale > 1) {
		TextureBuffer *buffer = new TextureBuffer();
		buffer->setTexture(TEXTURE_UPSCALE, v2f(1.0f / MYMAX(scale, 1), 1.0f / MYMAX(scale, 1)), "upscale", video::ECF_A8R8G8B8);
		pipeline->own(static_cast<RenderTarget *>(buffer));

	    TextureBufferOutput *buffer_output = new TextureBufferOutput(buffer, TEXTURE_UPSCALE);
		step3D->setRenderTarget(pipeline->own(buffer_output));
		RenderStep *upscale = new UpscaleStep();
		upscale->setRenderSource(buffer);
		upscale->setRenderTarget(screen);
		pipeline->addStep(pipeline->own(upscale));
	}
	pipeline->addStep(pipeline->own(new DrawHUD()));
}

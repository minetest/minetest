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
#include "secondstage.h"
#include "client/camera.h"
#include "client/client.h"
#include "client/clientmap.h"
#include "client/hud.h"
#include "client/minimap.h"
#include "client/shadows/dynamicshadowsrender.h"

/// Draw3D pipeline step
void Draw3D::run(PipelineContext &context)
{
	if (m_target)
		m_target->activate(context);

	context.device->getSceneManager()->drawAll();
	context.device->getVideoDriver()->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	if (!context.show_hud)
		return;
	context.hud->drawBlockBounds();
	context.hud->drawSelectionMesh();
	if (context.draw_wield_tool)
		context.client->getCamera()->drawWieldedTool();
}

void DrawHUD::run(PipelineContext &context)
{
	if (context.show_hud) {
		if (context.shadow_renderer)
			context.shadow_renderer->drawDebug();

		context.hud->resizeHotbar();

		if (context.draw_crosshair)
			context.hud->drawCrosshair();

		context.hud->drawHotbar(context.client->getEnv().getLocalPlayer()->getWieldIndex());
		context.hud->drawLuaElements(context.client->getCamera()->getOffset());
		context.client->getCamera()->drawNametags();
		auto mapper = context.client->getMinimap();
		if (mapper && context.show_minimap)
			mapper->drawMinimap();
	}
	context.device->getGUIEnvironment()->drawAll();
}


void MapPostFxStep::setRenderTarget(RenderTarget * _target)
{
	target = _target;
}

void MapPostFxStep::run(PipelineContext &context)
{
	if (target)
		target->activate(context);

	context.client->getEnv().getClientMap().renderPostFx(context.client->getCamera()->getCameraMode());
}

void RenderShadowMapStep::run(PipelineContext &context)
{
	// This is necessary to render shadows for animations correctly
	context.device->getSceneManager()->getRootSceneNode()->OnAnimate(context.device->getTimer()->getTime());
	context.shadow_renderer->update();
}

// class UpscaleStep

void UpscaleStep::run(PipelineContext &context)
{
	video::ITexture *lowres = m_source->getTexture(0);
	m_target->activate(context);
	context.device->getVideoDriver()->draw2DImage(lowres,
			core::rect<s32>(0, 0, context.target_size.X, context.target_size.Y),
			core::rect<s32>(0, 0, lowres->getSize().Width, lowres->getSize().Height));
}

std::unique_ptr<RenderStep> create3DStage(Client *client, v2f scale)
{
	RenderStep *step = new Draw3D();
	if (g_settings->getBool("enable_shaders")) {
		RenderPipeline *pipeline = new RenderPipeline();
		pipeline->addStep(pipeline->own(std::unique_ptr<RenderStep>(step)));

		auto effect = addPostProcessing(pipeline, step, scale, client);
		effect->setRenderTarget(pipeline->getOutput());
		step = pipeline;
	}
	return std::unique_ptr<RenderStep>(step);
}

static v2f getDownscaleFactor()
{
	u16 undersampling = MYMAX(g_settings->getU16("undersampling"), 1);
	return v2f(1.0f / undersampling);
}

RenderStep* addUpscaling(RenderPipeline *pipeline, RenderStep *previousStep, v2f downscale_factor)
{
	const int TEXTURE_UPSCALE = 0;

	if (downscale_factor.X == 1.0f && downscale_factor.Y == 1.0f)
		return previousStep;

	// Initialize buffer
	TextureBuffer *buffer = pipeline->createOwned<TextureBuffer>();
	buffer->setTexture(TEXTURE_UPSCALE, downscale_factor, "upscale", video::ECF_A8R8G8B8);

	// Attach previous step to the buffer
	TextureBufferOutput *buffer_output = pipeline->createOwned<TextureBufferOutput>(buffer, TEXTURE_UPSCALE);
	previousStep->setRenderTarget(buffer_output);

	// Add upscaling step
	RenderStep *upscale = pipeline->createOwned<UpscaleStep>();
	upscale->setRenderSource(buffer);
	pipeline->addStep(upscale);

	return upscale;
}

void populatePlainPipeline(RenderPipeline *pipeline, Client *client)
{
	auto downscale_factor = getDownscaleFactor();
	auto step3D = pipeline->own(create3DStage(client, downscale_factor));
	pipeline->addStep(step3D);
	pipeline->addStep<MapPostFxStep>();

	step3D = addUpscaling(pipeline, step3D, downscale_factor);

	step3D->setRenderTarget(pipeline->createOwned<ScreenTarget>());

	pipeline->addStep<DrawHUD>();
}

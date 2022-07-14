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

#include "factory.h"
#include "log.h"
#include "plain.h"
#include "anaglyph.h"
#include "interlaced.h"
#include "sidebyside.h"
#include "secondstage.h"
#include "client/shadows/dynamicshadowsrender.h"

struct CreatePipelineResult
{
	v2f virtual_size_scale;
	ShadowRenderer *shadow_renderer;
	RenderPipeline *pipeline;
};

bool createPipeline(const std::string &stereo_mode, IrrlichtDevice *device, Client *client, Hud *hud, CreatePipelineResult *result);

RenderingCore *createRenderingCore(const std::string &stereo_mode, IrrlichtDevice *device,
		Client *client, Hud *hud)
{
	CreatePipelineResult created_pipeline;
	if (createPipeline(stereo_mode, device, client, hud, &created_pipeline))
		return new RenderingCore(device, client, hud, 
				created_pipeline.shadow_renderer, created_pipeline.pipeline, created_pipeline.virtual_size_scale);

	if (stereo_mode == "none")
		return new RenderingCorePlain(device, client, hud);
	if (stereo_mode == "anaglyph")
		return new RenderingCoreAnaglyph(device, client, hud);
	if (stereo_mode == "interlaced")
		return new RenderingCoreInterlaced(device, client, hud);
	if (stereo_mode == "sidebyside")
		return new RenderingCoreSideBySide(device, client, hud);
	if (stereo_mode == "topbottom")
		return new RenderingCoreSideBySide(device, client, hud, true);
	if (stereo_mode == "crossview")
		return new RenderingCoreSideBySide(device, client, hud, false, true);
	if (stereo_mode == "secondstage")
		return new RenderingCoreSecondStage(device, client, hud);

	// fallback to plain renderer
	errorstream << "Invalid rendering mode: " << stereo_mode << std::endl;
	return new RenderingCorePlain(device, client, hud);
}

bool createPipeline(const std::string &stereo_mode, IrrlichtDevice *device, Client *client, Hud *hud, CreatePipelineResult *result)
{
	if (stereo_mode == "none") {
		result->shadow_renderer = createShadowRenderer(device, client);
		result->virtual_size_scale = v2f(1.0f);
		result->pipeline = new RenderPipeline();
		if (result->shadow_renderer)
			result->pipeline->addStep(result->pipeline->own(new RenderShadowMapStep()));
		populatePlainPipeline(result->pipeline);
		return true;
	}
	if (stereo_mode == "anaglyph") {
		result->shadow_renderer = createShadowRenderer(device, client);
		result->virtual_size_scale = v2f(1.0f);
		result->pipeline = new RenderPipeline();
		if (result->shadow_renderer)
			result->pipeline->addStep(result->pipeline->own(new RenderShadowMapStep()));
		populateAnaglyphPipeline(result->pipeline);
		return true;
	}
	if (stereo_mode == "interlaced") {
		result->shadow_renderer = createShadowRenderer(device, client);
		result->virtual_size_scale = v2f(1.0f);
		result->pipeline = new RenderPipeline();
		if (result->shadow_renderer)
			result->pipeline->addStep(result->pipeline->own(new RenderShadowMapStep()));
		populateInterlacedPipeline(result->pipeline, client);
		return true;
	}
	if (stereo_mode == "sidebyside") {
		result->shadow_renderer = createShadowRenderer(device, client);
		result->virtual_size_scale = v2f(1.0f);
		result->pipeline = new RenderPipeline();
		if (result->shadow_renderer)
			result->pipeline->addStep(result->pipeline->own(new RenderShadowMapStep()));
		populateSideBySidePipeline(result->pipeline, false, false, result->virtual_size_scale);
		return true;
	}
	if (stereo_mode == "topbottom") {
		result->shadow_renderer = createShadowRenderer(device, client);
		result->virtual_size_scale = v2f(1.0f);
		result->pipeline = new RenderPipeline();
		if (result->shadow_renderer)
			result->pipeline->addStep(result->pipeline->own(new RenderShadowMapStep()));
		populateSideBySidePipeline(result->pipeline, true, false, result->virtual_size_scale);
		return true;
	}
	if (stereo_mode == "crossview") {
		result->shadow_renderer = createShadowRenderer(device, client);
		result->virtual_size_scale = v2f(1.0f);
		result->pipeline = new RenderPipeline();
		if (result->shadow_renderer)
			result->pipeline->addStep(result->pipeline->own(new RenderShadowMapStep()));
		populateSideBySidePipeline(result->pipeline, false, true, result->virtual_size_scale);
		return true;
	}
	return false;
}
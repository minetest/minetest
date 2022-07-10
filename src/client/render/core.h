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

#pragma once
#include "irrlichttypes_extrabloated.h"
#include "pipeline.h"

class ShadowRenderer;
class Camera;
class Client;
class Hud;
class Minimap;

/**
 * Represents the pipeline state of the rendering core.
 */
struct PipelineState
{
	bool show_hud {true};
	bool show_minimap {true};
	bool draw_wield_tool {true};
	bool draw_crosshair {true};
};

/**
 * Implements a pipeline step that renders the 3D scene
 */
class Draw3D : public RenderStep
{
public:
	Draw3D(PipelineState *state) :
			m_state(state)
	{}

	virtual void setRenderSource(RenderSource *) override {}
	virtual void setRenderTarget(RenderTarget *target) override { m_target = target; }

	virtual void reset(PipelineContext *context) override {}
	virtual void run(PipelineContext *context) override;

private:
	PipelineState *m_state;
	RenderTarget *m_target {nullptr};
};

/**
 * Implements a pipeline step that renders the game HUD
 */
class DrawHUD : public RenderStep
{
public:
	DrawHUD(PipelineState *state, ShadowRenderer *shadow_renderer) :
			m_state(state),
			m_shadow_renderer(shadow_renderer)
	{}

	virtual void setRenderSource(RenderSource *) override {}
	virtual void setRenderTarget(RenderTarget *) override {}

	virtual void reset(PipelineContext *context) override {}
	virtual void run(PipelineContext *context) override;
private:
	PipelineState *m_state;
	ShadowRenderer *m_shadow_renderer;
};

class MapPostFxStep : public TrivialRenderStep
{
public:
	virtual void setRenderTarget(RenderTarget *) override;
	virtual void run(PipelineContext *context) override;
private:
	RenderTarget *target;
};

class RenderingCore
{
protected:
	v2u32 screensize;
	video::SColor skycolor;
	bool show_hud;
	bool show_minimap;
	bool draw_wield_tool;
	bool draw_crosshair;

	IrrlichtDevice *device;
	video::IVideoDriver *driver;
	scene::ISceneManager *smgr;
	gui::IGUIEnvironment *guienv;

	Client *client;
	Camera *camera;
	Minimap *mapper;
	Hud *hud;

	ShadowRenderer *shadow_renderer;

	PipelineState pipelineState;
	RenderStep *step3D;
	RenderStep *stepHUD;
	RenderStep *stepPostFx;
	RenderTarget *screen;
	RenderTarget *scene_output;

	RenderPipeline pipeline;

	virtual void createPipeline() {}

public:
	RenderingCore(IrrlichtDevice *_device, Client *_client, Hud *_hud);
	RenderingCore(const RenderingCore &) = delete;
	RenderingCore(RenderingCore &&) = delete;
	virtual ~RenderingCore();

	RenderingCore &operator=(const RenderingCore &) = delete;
	RenderingCore &operator=(RenderingCore &&) = delete;

	void initialize();
	void draw(video::SColor _skycolor, bool _show_hud, bool _show_minimap,
			bool _draw_wield_tool, bool _draw_crosshair);

	inline v2u32 getVirtualSize() const { return scene_output->getSize(); }

	ShadowRenderer *get_shadow_renderer() { return shadow_renderer; };
};

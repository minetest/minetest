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

class ShadowRenderer;
class Camera;
class Client;
class Hud;
class Minimap;
class RenderPipeline;
class RenderTarget;

class RenderingCore
{
protected:
	IrrlichtDevice *device;
	Client *client;
	Hud *hud;
	ShadowRenderer *shadow_renderer;

	RenderPipeline *pipeline;

	v2f virtual_size_scale;
	v2u32 virtual_size { 0, 0 };

	virtual void createPipeline() {}

public:
	RenderingCore(IrrlichtDevice *device, Client *client, Hud *hud, 
			ShadowRenderer *shadow_renderer, RenderPipeline *pipeline,
			v2f virtual_size_scale);
	RenderingCore(const RenderingCore &) = delete;
	RenderingCore(RenderingCore &&) = delete;
	virtual ~RenderingCore();

	RenderingCore &operator=(const RenderingCore &) = delete;
	RenderingCore &operator=(RenderingCore &&) = delete;

	void initialize();
	void draw(video::SColor _skycolor, bool _show_hud, bool _show_minimap,
			bool _draw_wield_tool, bool _draw_crosshair);

	v2u32 getVirtualSize() const;

	ShadowRenderer *get_shadow_renderer() { return shadow_renderer; };
};

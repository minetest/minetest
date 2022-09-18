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
#include "stereo.h"
#include "pipeline.h"

/**
 * Set color mask when rendering the next steps
 */
class SetColorMaskStep : public TrivialRenderStep
{
public:
	SetColorMaskStep(int color_mask);

	void run(PipelineContext &context) override;
private:
	int color_mask;
};

/**
 * Resets depth buffer of the current render target
 * 
 */
class ClearDepthBufferTarget : public RenderTarget
{
public:
	ClearDepthBufferTarget(RenderTarget *target);

	void reset(PipelineContext &context) override {}
	void activate(PipelineContext &context) override;
private:
	RenderTarget *target;
};


/**
 * Enables or disables override material when activated
 * 
 */
class ConfigureOverrideMaterialTarget : public RenderTarget
{
public:
	ConfigureOverrideMaterialTarget(RenderTarget *upstream, bool enable);

	virtual void activate(PipelineContext &context) override;
private:
	RenderTarget *upstream;
	bool enable;
};

void populateAnaglyphPipeline(RenderPipeline *pipeline, Client *client);

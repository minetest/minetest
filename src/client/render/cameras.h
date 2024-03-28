/*
Minetest
Copyright (C) 2022 x2048, Dmitry Kostenko <codeforsmile@gmail.com>
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

#include "pipeline.h"
#include <memory>

class SelectPrimaryCamera : public TrivialRenderStep
{
public:
	virtual void run(PipelineContext &context) override;
};

class DynamicTextureSource;
/**
 * @brief Implements a pipeline step that renders the secondary cameras.
 *
 * @note This class attaches to Client on construction and can only be
 * created as part of the rendering pipeline
 */
class DrawSecondaryCameras : public RenderStep
{
public:
	DrawSecondaryCameras(RenderStep *draw, Client *client);
	~DrawSecondaryCameras() override;

	virtual void setRenderSource(RenderSource *) override {}
	virtual void setRenderTarget(RenderTarget *_target) override { target = _target; }

	virtual void reset(PipelineContext &context) override;
	virtual void run(PipelineContext &context) override;
private:
	std::unique_ptr<TextureBuffer> textures;
	RenderStep *draw;
	RenderTarget *target {nullptr};
	DynamicTextureSource *overrides; // cannot use unique_ptr here without exposing DynamicTextureSource class
};

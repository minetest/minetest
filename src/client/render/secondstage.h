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
 *  Step to apply post-processing filter to the rendered image
 */
class PostProcessingStep : public RenderStep
{
public:
	/**
	 * Construct a new PostProcessingStep object
	 * 
	 * @param shader_id ID of the shader in IShaderSource
	 * @param texture_map Map of textures to be chosen from the render source
	 */
	PostProcessingStep(u32 shader_id, const std::vector<u8> &texture_map);


	void setRenderSource(RenderSource *source) override;
	void setRenderTarget(RenderTarget *target) override;
	void reset(PipelineContext &context) override;
	void run(PipelineContext &context) override;

	/**
	 * Configure bilinear filtering for a specific texture layer
	 * 
	 * @param index Index of the texture layer
	 * @param value true to enable the bilinear filter, false to disable
	 */
	void setBilinearFilter(u8 index, bool value);
private:
	u32 shader_id;
	std::vector<u8> texture_map;
	RenderSource *source { nullptr };
	RenderTarget *target { nullptr };
	video::SMaterial material;

	void configureMaterial();
};

RenderStep *addPostProcessing(RenderPipeline *pipeline, RenderStep *previousStep, v2f scale, Client *client);

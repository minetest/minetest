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
#include "core.h"

/**
 * UpscaleStep step performs rescaling of the image 
 * in the source texture 0 to the size of the target.
 */
class UpscaleStep : public RenderStep
{
public:

    virtual void setRenderSource(RenderSource *source) override { m_source = source; }
    virtual void setRenderTarget(RenderTarget *target) override { m_target = target; }
    virtual void reset(PipelineContext *context) override {};
    virtual void run(PipelineContext *context) override;
private:
	RenderSource *m_source;
	RenderTarget *m_target;
};

class RenderingCorePlain : public RenderingCore
{
protected:
	const int TEXTURE_UPSCALE = 0;

	int scale = 0;
	TextureBuffer buffer;
	UpscaleStep upscale;

	void initTextures() override;
	void createPipeline() override;
public:
	RenderingCorePlain(IrrlichtDevice *_device, Client *_client, Hud *_hud);
};

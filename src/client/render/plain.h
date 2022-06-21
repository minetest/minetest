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
	UpscaleStep(video::IVideoDriver *driver) :
		m_driver(driver)
	{}

    virtual void setRenderSource(RenderSource *source) override { m_source = source; }
    virtual void setRenderTarget(RenderTarget *target) override { m_target = target; }
    virtual void reset() override {};
    virtual void run() override;

	/**
	 * Set the dimensions of the source image
	 * @param sourceSize dimensions of the source buffer in pixels
	 */
	void setSourceSize(v2u32 sourceSize) { m_sourceSize = sourceSize; }

	/**
	 * Set the dimensions of the render target
	 * 
	 * @param targetSize dimensions of the render target in pixels
	 */
	void setTargetSize(v2u32 targetSize) { m_targetSize = targetSize; }
private:
	video::IVideoDriver *m_driver;
	RenderSource *m_source;
	RenderTarget *m_target;
	v2u32 m_sourceSize;
	v2u32 m_targetSize;
};

class RenderingCorePlain : public RenderingCore
{
protected:
	int scale = 0;
	ColorBuffer buffer;
	UpscaleStep upscale;

	void initTextures() override;

public:
	RenderingCorePlain(IrrlichtDevice *_device, Client *_client, Hud *_hud);
private:
	void setSkyColor();
};

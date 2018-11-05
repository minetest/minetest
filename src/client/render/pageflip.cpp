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

#include "pageflip.h"

#ifdef STEREO_PAGEFLIP_SUPPORTED

void RenderingCorePageflip::initTextures()
{
	hud = driver->addRenderTargetTexture(
			screensize, "3d_render_hud", video::ECF_A8R8G8B8);
}

void RenderingCorePageflip::clearTextures()
{
	driver->removeTexture(hud);
}

void RenderingCorePageflip::drawAll()
{
	driver->setRenderTarget(hud, true, true, video::SColor(0, 0, 0, 0));
	drawHUD();
	driver->setRenderTarget(nullptr, false, false, skycolor);
	renderBothImages();
}

void RenderingCorePageflip::useEye(bool _right)
{
	driver->setRenderTarget(_right ? video::ERT_STEREO_RIGHT_BUFFER
				       : video::ERT_STEREO_LEFT_BUFFER,
			true, true, skycolor);
	RenderingCoreStereo::useEye(_right);
}

void RenderingCorePageflip::resetEye()
{
	driver->draw2DImage(hud, v2s32(0, 0));
	driver->setRenderTarget(video::ERT_FRAME_BUFFER, false, false, skycolor);
	RenderingCoreStereo::resetEye();
}

#endif // STEREO_PAGEFLIP_SUPPORTED

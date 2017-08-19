/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 numzero, Lobachesky Vitaly <numzer0@yandex.ru>

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

void RenderingCorePageflip::init_textures()
{
	hud = driver->addRenderTargetTexture(screensize, "3d_render_hud", video::ECF_A8R8G8B8);
}

void RenderingCorePageflip::clear_textures()
{
	driver->removeTexture(hud);
}

void RenderingCorePageflip::draw_all()
{
	driver->setRenderTarget(hud, true, true, video::SColor(0, 0, 0, 0));
	draw_hud();
	driver->setRenderTarget(nullptr, false, false, skycolor);
	render_two();
}

void RenderingCorePageflip::use_eye(bool _right)
{
	driver->setRenderTarget(
		_right ? video::ERT_STEREO_RIGHT_BUFFER : video::ERT_STEREO_LEFT_BUFFER,
		 true, true, skycolor);
	RenderingCoreStereo::use_eye(_right);
}

void RenderingCorePageflip::reset_eye()
{
	driver->draw2DImage(hud, v2s32(0, 0));
	driver->setRenderTarget(video::ERT_FRAME_BUFFER, false, false, skycolor);
	RenderingCoreStereo::reset_eye();
}

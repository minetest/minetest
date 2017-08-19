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

#include "sidebyside.h"

void RenderingCoreSideBySide::initTextures()
{
	v2u32 image_size{screensize.X / 2, screensize.Y};
	left = driver->addRenderTargetTexture(image_size, "3d_render_left", video::ECF_A8R8G8B8);
	right = driver->addRenderTargetTexture(image_size, "3d_render_right", video::ECF_A8R8G8B8);
	hud = driver->addRenderTargetTexture(screensize, "3d_render_hud", video::ECF_A8R8G8B8);
}

void RenderingCoreSideBySide::clearTextures()
{
	driver->removeTexture(left);
	driver->removeTexture(right);
	driver->removeTexture(hud);
}

void RenderingCoreSideBySide::drawAll()
{
	renderBothImages();
	driver->setRenderTarget(hud, true, true, video::SColor(0, 0, 0, 0));
	drawHUD();
	driver->setRenderTarget(nullptr, false, false, skycolor);

	driver->draw2DImage(left, v2s32(0, 0));
	driver->draw2DImage(right, v2s32(screensize.X / 2, 0));

	driver->draw2DImage(hud,
			core::rect<s32>(0, 0, screensize.X / 2, screensize.Y),
			core::rect<s32>(0, 0, screensize.X, screensize.Y), 0, 0,
			true);

	driver->draw2DImage(hud,
			core::rect<s32>(screensize.X / 2, 0, screensize.X, screensize.Y),
			core::rect<s32>(0, 0, screensize.X, screensize.Y), 0, 0,
			true);
}

void RenderingCoreSideBySide::useEye(bool _right)
{
	driver->setRenderTarget(_right ? right : left, true, true, skycolor);
	RenderingCoreStereo::useEye(_right);
}

void RenderingCoreSideBySide::resetEye()
{
	driver->setRenderTarget(nullptr, false, false, skycolor);
	RenderingCoreStereo::resetEye();
}

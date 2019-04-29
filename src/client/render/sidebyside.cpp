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

#include "sidebyside.h"
#include <ICameraSceneNode.h>
#include "client/hud.h"

RenderingCoreSideBySide::RenderingCoreSideBySide(IrrlichtDevice *_device, Client *_client,
		Hud *_hud, bool _horizontal, bool _flipped) :
		RenderingCoreStereo(_device, _client, _hud),
		horizontal(_horizontal), flipped(_flipped)
{
}

void RenderingCoreSideBySide::initTextures()
{
	if (horizontal) {
		image_size = {screensize.X, screensize.Y / 2};
		rpos = v2s32(0, screensize.Y / 2);
	} else {
		image_size = {screensize.X / 2, screensize.Y};
		rpos = v2s32(screensize.X / 2, 0);
	}
	virtual_size = image_size;
	left = driver->addRenderTargetTexture(
			image_size, "3d_render_left", video::ECF_A8R8G8B8);
	right = driver->addRenderTargetTexture(
			image_size, "3d_render_right", video::ECF_A8R8G8B8);
}

void RenderingCoreSideBySide::clearTextures()
{
	driver->removeTexture(left);
	driver->removeTexture(right);
}

void RenderingCoreSideBySide::drawAll()
{
	driver->OnResize(image_size); // HACK to make GUI smaller
	renderBothImages();
	driver->OnResize(screensize);
	driver->draw2DImage(left, {});
	driver->draw2DImage(right, rpos);
}

void RenderingCoreSideBySide::useEye(bool _right)
{
	driver->setRenderTarget(_right ? right : left, true, true, skycolor);
	RenderingCoreStereo::useEye(_right ^ flipped);
}

void RenderingCoreSideBySide::resetEye()
{
	hud->resizeHotbar();
	drawHUD();
	driver->setRenderTarget(nullptr, false, false, skycolor);
	RenderingCoreStereo::resetEye();
}

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

#include "stereo.h"
#include "client/camera.h"
#include "constants.h"
#include "settings.h"

RenderingCoreStereo::RenderingCoreStereo(
	IrrlichtDevice *_device, Client *_client, Hud *_hud)
	: RenderingCore(_device, _client, _hud)
{
	eye_offset = BS * g_settings->getFloat("3d_paralax_strength");
}

void RenderingCoreStereo::beforeDraw()
{
	cam = camera->getCameraNode();
	base_transform = cam->getRelativeTransformation();
}

void RenderingCoreStereo::useEye(bool right)
{
	core::matrix4 move;
	move.setTranslation(
			core::vector3df(right ? eye_offset : -eye_offset, 0.0f, 0.0f));
	cam->setPosition((base_transform * move).getTranslation());
}

void RenderingCoreStereo::resetEye()
{
	cam->setPosition(base_transform.getTranslation());
}

void RenderingCoreStereo::renderBothImages()
{
	useEye(false);
	draw3D();
	resetEye();
	useEye(true);
	draw3D();
	resetEye();
}

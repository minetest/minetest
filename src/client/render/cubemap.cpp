/*
Minetest
Copyright (C) 2019 srifqi, Muhammad Rifqi Priyo Susanto
		<muhammadrifqipriyosusanto@gmail.com>

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

#include "cubemap.h"
#include <ICameraSceneNode.h>
#include "client/camera.h"
#include "client/hud.h"

RenderingCoreCubeMap::RenderingCoreCubeMap(
	IrrlichtDevice *_device, Client *_client, Hud *_hud)
	: RenderingCore(_device, _client, _hud)
{}

void RenderingCoreCubeMap::initTextures()
{
	u32 size = screensize.Y / 3;
	core::dimension2du sz = {size, size};
	initTextures(sz);
}

void RenderingCoreCubeMap::initTextures(core::dimension2du size)
{
	image_size = {screensize.X, screensize.Y};
	render_size = size;

	for (int i = 0; i < face_count; i ++)
		faces[i] = driver->addRenderTargetTexture(
				render_size, "3d_render_f" + i, video::ECF_A8R8G8B8);
}

void RenderingCoreCubeMap::clearTextures()
{
	for (int i = 0; i < face_count; i ++)
		driver->removeTexture(faces[i]);
}

void RenderingCoreCubeMap::beforeDraw()
{
	cam = camera->getCameraNode();
}

// Structure of cube's faces
//  Layout      Axis
//    2          Y+
//  1 4 0 5   X- Z+ X+ Z-
//    3          Y-
//
// See https://www.nvidia.com/object/cube_map_ogl_tutorial.html for more info.

void RenderingCoreCubeMap::useFace(int face)
{
	driver->setRenderTarget(faces[face], true, true, skycolor);

	cam->setUpVector(core::vector3df(0.0f, 1.0f, 0.0f));
	cam->setRotation(core::vector3df()); // case 4
	cam->setAspectRatio(1 / 1); // square
	cam->setFOV(core::PI / 2);

	switch (face) {
		case 0:
			cam->setRotation(core::vector3df(0.0f, 90.0f, 0.0f));
			break;
		case 1:
			cam->setRotation(core::vector3df(0.0f, -90.0f, 0.0f));
			break;
		case 2:
			cam->setRotation(core::vector3df(-90.0f, 0.0f, 0.0f));
			break;
		case 3:
			cam->setRotation(core::vector3df(90.0f, 0.0f, 0.0f));
			break;
		case 5:
			cam->setRotation(core::vector3df(0.0f, 180.0f, 0.0f));
			break;
		default:
			break;
	}
}

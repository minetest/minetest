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
	if (screensize.X / screensize.Y > 4 / 3)
		screensize.X = screensize.Y * 4 / 3;
	else if (screensize.X / screensize.Y < 4 / 3)
		screensize.Y = screensize.X * 3 / 4;

	image_size = {screensize.X / 4, screensize.Y / 3};
	virtual_size = image_size;

	for (int i = 0; i < 6; i ++)
		faces[i] = driver->addRenderTargetTexture(
				image_size, "3d_render_f" + i, video::ECF_A8R8G8B8);
}

void RenderingCoreCubeMap::clearTextures()
{
	for (int i = 0; i < 6; i ++)
		driver->removeTexture(faces[i]);
}

void RenderingCoreCubeMap::beforeDraw()
{
	cam = camera->getCameraNode();
}

// See https://www.nvidia.com/object/cube_map_ogl_tutorial.html for more info.

void RenderingCoreCubeMap::useFace(int face)
{
	driver->setRenderTarget(faces[face], true, true, skycolor);

	cam->setUpVector(core::vector3df(0.0f, 1.0f, 0.0f));
	cam->setRotation(core::vector3df());
	cam->setAspectRatio(1 / 1); // square
	cam->setFOV(core::PI / 2);

	v3f cam_pos = camera->getPosition();
	core::vector3df target(cam_pos.X, cam_pos.Y, cam_pos.Z);
	switch (face) {
		case 0:
			target.X += 100;
			break;
		case 1:
			target.X -= 100;
			break;
		case 2:
			target.Y += 100;
			break;
		case 3:
			target.Y -= 100;
			break;
		case 4:
			target.Z += 100;
			break;
		case 5:
			target.Z -= 100;
			break;
		default:
			break;
	}
	cam->setTarget(target);
}

void RenderingCoreCubeMap::drawFace(int face)
{
	v2s32 pos;
	switch (face) {
		case 0:
			pos = v2s32(image_size.Width * 2, image_size.Height * 1);
			break;
		case 1:
			pos = v2s32(image_size.Width * 0, image_size.Height * 1);
			break;
		case 2:
			pos = v2s32(image_size.Width * 1, image_size.Height * 0);
			break;
		case 3:
			pos = v2s32(image_size.Width * 1, image_size.Height * 2);
			break;
		case 4:
			pos = v2s32(image_size.Width * 1, image_size.Height * 1);
			break;
		case 5:
			pos = v2s32(image_size.Width * 3, image_size.Height * 1);
			break;
		default:
			break;
	}
	driver->draw2DImage(faces[face], pos);
}

void RenderingCoreCubeMap::drawAll()
{
	for (int i = 5; i >= 0; i --) {
		useFace(i);
		draw3D();
	}

	driver->setRenderTarget(nullptr, false, false, skycolor);

	for (int i = 5; i >= 0; i --)
		drawFace(i);

	// drawHUD();
}

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

#include "equirectangular.h"
#include <ICameraSceneNode.h>
#include "client/camera.h"
#include "client/hud.h"

RenderingCoreEquirectangular::RenderingCoreEquirectangular(
	IrrlichtDevice *_device, Client *_client, Hud *_hud)
	: RenderingCoreCubeMap(_device, _client, _hud)
{}

void RenderingCoreEquirectangular::initTextures()
{
	RenderingCoreCubeMap::initTextures();

	image_size = {screensize.X, screensize.Y};

	renderOutput = driver->addRenderTargetTexture(
				image_size, "3d_render_eq", video::ECF_A8R8G8B8);
}

void RenderingCoreEquirectangular::clearTextures()
{
	RenderingCoreCubeMap::clearTextures();

	driver->removeTexture(renderOutput);
}

void RenderingCoreEquirectangular::processImages()
{
	u32 *pixel = (u32 *) renderOutput->lock();
	u32 *pixelf[6];

	for (int i = 0; i < 6; i ++)
		pixelf[i] = (u32 *) faces[i]->lock();

	u32 dims = image_size.Width / 4;
	for (u32 j = 0; j < dims * 2; j ++) {
		double v = 1.0 - (double) j / (dims * 2);
		double phi = v * core::PI;

		for (u32 i = 0; i < dims * 4; i ++) {
			double u = (double) i / (dims * 4);
			double theta = u * 2 * core::PI;

			double x = sin(theta) * sin(phi);
			double y = cos(phi);
			double z = cos(theta) * sin(phi);

			double a = fmax(fmax(abs(x), abs(y)), abs(z));

			double xx = x / a;
			double yy = y / a;
			double zz = z / a;

			u32 xPixel = 0;
			u32 yPixel = 0;
			u32 yTemp = 0;
			int selected_face = -1;

			if (xx == 1) {
				xPixel = ((-1 * tan(atan(z / x)) + 1.0) / 2.0) * dims;
				yTemp = ((tan(atan(y / x)) + 1.0) / 2.0) * dims;
				selected_face = 1; // X-
			} else if (xx == -1) {
				xPixel = ((-1 * tan(atan(z / x)) + 1.0) / 2.0) * dims;
				yTemp = ((-1 * tan(atan(y / x)) + 1.0) / 2.0) * dims;
				selected_face = 0; // X+
			} else if (yy == 1) {
				xPixel = ((tan(atan(x / y)) + 1.0) / 2.0) * dims;
				yTemp = ((-1 * tan(atan(z / y)) + 1.0) / 2.0) * dims;
				selected_face = 2; // Y+
			} else if (yy == -1) {
				xPixel = ((-1 * tan(atan(x / y)) + 1.0) / 2.0) * dims;
				yTemp = ((-1 * tan(atan(z / y)) + 1.0) / 2.0) * dims;
				selected_face = 3; // Y-
			} else if (zz == 1) {
				xPixel = ((tan(atan(x / z)) + 1.0) / 2.0) * dims;
				yTemp = ((tan(atan(y / z)) + 1.0) / 2.0) * dims;
				selected_face = 5; // Z-
			} else if (zz == -1) {
				xPixel = ((tan(atan(x / z)) + 1.0) / 2.0) * dims;
				yTemp = ((-1 * tan(atan(y / z)) + 1.0) / 2.0) * dims;
				selected_face = 4; // Z+
			}

			xPixel = fmin(xPixel, dims - 1);
			yPixel = fmin(yTemp, dims - 1);
			yPixel = dims - 1 - yPixel;

			if (selected_face == 2 || selected_face == 3) {
				pixel[j * dims * 4 + i] = pixelf[selected_face][xPixel * dims + (dims - 1 - yPixel)];
			} else {
				pixel[j * dims * 4 + i] = pixelf[selected_face][yPixel * dims + xPixel];
			}
		}
	}

	for (int i = 0; i < 6; i ++)
		faces[i]->unlock();

	renderOutput->unlock();
}

void RenderingCoreEquirectangular::drawAll()
{
	for (int i = 5; i >= 0; i --) {
		useFace(i);
		draw3D();
	}

	driver->setRenderTarget(nullptr, false, false, skycolor);

	processImages();

	driver->draw2DImage(renderOutput, v2s32(0, 0));
}

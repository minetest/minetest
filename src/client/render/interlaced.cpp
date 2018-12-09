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

#include "interlaced.h"
#include "client/client.h"
#include "client/shader.h"
#include "client/tile.h"

RenderingCoreInterlaced::RenderingCoreInterlaced(
	IrrlichtDevice *_device, Client *_client, Hud *_hud)
	: RenderingCoreStereo(_device, _client, _hud)
{
	initMaterial();
}

void RenderingCoreInterlaced::initMaterial()
{
	IShaderSource *s = client->getShaderSource();
	mat.UseMipMaps = false;
	mat.ZBuffer = false;
	mat.ZWriteEnable = false;
	u32 shader = s->getShader("3d_interlaced_merge", TILE_MATERIAL_BASIC, 0);
	mat.MaterialType = s->getShaderInfo(shader).material;
	for (int k = 0; k < 3; ++k) {
		mat.TextureLayer[k].AnisotropicFilter = false;
		mat.TextureLayer[k].BilinearFilter = false;
		mat.TextureLayer[k].TrilinearFilter = false;
		mat.TextureLayer[k].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
		mat.TextureLayer[k].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	}
}

void RenderingCoreInterlaced::initTextures()
{
	v2u32 image_size{screensize.X, screensize.Y / 2};
	left = driver->addRenderTargetTexture(
			image_size, "3d_render_left", video::ECF_A8R8G8B8);
	right = driver->addRenderTargetTexture(
			image_size, "3d_render_right", video::ECF_A8R8G8B8);
	mask = driver->addTexture(screensize, "3d_render_mask", video::ECF_A8R8G8B8);
	initMask();
	mat.TextureLayer[0].Texture = left;
	mat.TextureLayer[1].Texture = right;
	mat.TextureLayer[2].Texture = mask;
}

void RenderingCoreInterlaced::clearTextures()
{
	driver->removeTexture(left);
	driver->removeTexture(right);
	driver->removeTexture(mask);
}

void RenderingCoreInterlaced::initMask()
{
	u8 *data = reinterpret_cast<u8 *>(mask->lock());
	for (u32 j = 0; j < screensize.Y; j++) {
		u8 val = j % 2 ? 0xff : 0x00;
		memset(data, val, 4 * screensize.X);
		data += 4 * screensize.X;
	}
	mask->unlock();
}

void RenderingCoreInterlaced::drawAll()
{
	renderBothImages();
	merge();
	drawHUD();
}

void RenderingCoreInterlaced::merge()
{
	static const video::S3DVertex vertices[4] = {
			video::S3DVertex(1.0, -1.0, 0.0, 0.0, 0.0, -1.0,
					video::SColor(255, 0, 255, 255), 1.0, 0.0),
			video::S3DVertex(-1.0, -1.0, 0.0, 0.0, 0.0, -1.0,
					video::SColor(255, 255, 0, 255), 0.0, 0.0),
			video::S3DVertex(-1.0, 1.0, 0.0, 0.0, 0.0, -1.0,
					video::SColor(255, 255, 255, 0), 0.0, 1.0),
			video::S3DVertex(1.0, 1.0, 0.0, 0.0, 0.0, -1.0,
					video::SColor(255, 255, 255, 255), 1.0, 1.0),
	};
	static const u16 indices[6] = {0, 1, 2, 2, 3, 0};
	driver->setMaterial(mat);
	driver->drawVertexPrimitiveList(&vertices, 4, &indices, 2);
}

void RenderingCoreInterlaced::useEye(bool _right)
{
	driver->setRenderTarget(_right ? right : left, true, true, skycolor);
	RenderingCoreStereo::useEye(_right);
}

void RenderingCoreInterlaced::resetEye()
{
	driver->setRenderTarget(nullptr, false, false, skycolor);
	RenderingCoreStereo::resetEye();
}

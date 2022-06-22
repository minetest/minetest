/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>
Copyright (C) 2020 appgurueu, Lars Mueller <appgurulars@gmx.de>

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

#include "secondstage.h"
#include "client/client.h"
#include "client/shader.h"
#include "client/tile.h"

RenderingCoreSecondStage::RenderingCoreSecondStage(
		IrrlichtDevice *_device, Client *_client, Hud *_hud) :
		RenderingCoreStereo(_device, _client, _hud),
		buffer(_device->getVideoDriver())
{
	initMaterial();
	step3D->setRenderTarget(&buffer);
}

void RenderingCoreSecondStage::initMaterial()
{
	IWritableShaderSource *s = client->getShaderSource();
	s->global_additional_headers = "#define SECONDSTAGE 1\n";
	s->rebuildShaders(); // TODO remove this if possible, use shader const setters?
	mat.UseMipMaps = false;
	mat.ZBuffer = true;
	mat.ZWriteEnable = video::EZW_ON;
	u32 shader = s->getShader("3d_secondstage", TILE_MATERIAL_BASIC, NDT_NORMAL);
	mat.MaterialType = s->getShaderInfo(shader).material;
	for (int k = 0; k < 4; ++k) {
		mat.TextureLayer[k].AnisotropicFilter = false;
		mat.TextureLayer[k].BilinearFilter = false;
		mat.TextureLayer[k].TrilinearFilter = false;
		mat.TextureLayer[k].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
		mat.TextureLayer[k].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	}
}

void RenderingCoreSecondStage::initTextures()
{
	buffer.setTexture(0, screensize.X, screensize.Y, "3d_render", video::ECF_A8R8G8B8);
	buffer.setTexture(1, screensize.X, screensize.Y, "3d_normalmap", video::ECF_A8R8G8B8);
	buffer.setDepthTexture(2, screensize.X, screensize.Y, "3d_depthmap", video::ECF_D32);
}

void RenderingCoreSecondStage::clearTextures()
{
}

void RenderingCoreSecondStage::drawAll()
{
	buffer.RenderTarget::reset();
	buffer.setClearColor(skycolor);
	draw3D();
	drawPostFx();
	driver->setRenderTarget(nullptr, false, false, skycolor);
	applyEffects();
	drawHUD();
}

void RenderingCoreSecondStage::applyEffects()
{
	mat.TextureLayer[0].Texture = buffer.getTexture(0);
	mat.TextureLayer[1].Texture = buffer.getTexture(1);
	mat.TextureLayer[3].Texture = buffer.getTexture(2);
	// driver->setTransform(video::ETS_VIEW, core::matrix4::EM4CONST_IDENTITY);
	// driver->setTransform(video::ETS_PROJECTION, core::matrix4::EM4CONST_IDENTITY);
	// driver->setTransform(video::ETS_WORLD, core::matrix4::EM4CONST_IDENTITY);
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

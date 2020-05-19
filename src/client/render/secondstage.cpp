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
		RenderingCoreStereo(_device, _client, _hud)
{
	initMaterial();
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
	for (int k = 0; k < 3; ++k) {
		mat.TextureLayer[k].AnisotropicFilter = false;
		mat.TextureLayer[k].BilinearFilter = false;
		mat.TextureLayer[k].TrilinearFilter = false;
		mat.TextureLayer[k].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
		mat.TextureLayer[k].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	}
}

void RenderingCoreSecondStage::initTextures()
{
	rendered = driver->addRenderTargetTexture(
			screensize, "3d_render", video::ECF_A8R8G8B8);
	normalmap = driver->addRenderTargetTexture(
			screensize, "3d_normalmap", video::ECF_A8R8G8B8);
	depthmap = driver->addRenderTargetTexture(
			screensize, "3d_depthmap", video::ECF_R32F);
	renderTargets.push_back(rendered);
	renderTargets.push_back(normalmap);
	renderTargets.push_back(depthmap);
	renderTarget = driver->addRenderTarget();
	renderTarget->setTexture(renderTargets, nullptr);
	mat.TextureLayer[0].Texture = rendered;
	mat.TextureLayer[1].Texture = normalmap;
	mat.TextureLayer[2].Texture = depthmap;
}

void RenderingCoreSecondStage::clearTextures()
{
	driver->removeRenderTarget(renderTarget);
	driver->removeTexture(rendered);
	driver->removeTexture(normalmap);
	driver->removeTexture(depthmap);
}

void RenderingCoreSecondStage::drawAll()
{
    driver->setRenderTargetEx(renderTarget, video::ECBF_ALL, skycolor);
	draw3D();
	driver->setRenderTarget(nullptr, false, false, skycolor);
	draw();
	drawHUD();
}

void RenderingCoreSecondStage::draw()
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

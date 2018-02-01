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

#include "plain.h"
#include "camera.h"
#include "client.h"
#include "client/tile.h"
#include "shader.h"
#include "settings.h"

inline u32 scaledown(u32 coef, u32 size)
{
	return (size + coef - 1) / coef;
}

RenderingCorePlain::RenderingCorePlain(
	IrrlichtDevice *_device, Client *_client, Hud *_hud)
	: RenderingCore(_device, _client, _hud)
{
	scale = g_settings->getU16("undersampling");
	if(scale == 0)
		scale = 1;
}

void RenderingCorePlain::initTextures()
{
	if (!scale)
		return;
	v2u32 size{scaledown(scale, screensize.X), scaledown(scale, screensize.Y)};
	lowres = driver->addRenderTargetTexture(
			size, "render_lowres", video::ECF_A8R8G8B8);
	postimg = driver->addRenderTargetTexture(
			size, "render_postimg", video::ECF_A8R8G8B8);
	zbuffer = driver->addRenderTargetTexture(
			size, "render_zbuffer", video::ECF_R16F);
}

void RenderingCorePlain::clearTextures()
{
	if (!scale)
		return;
	driver->removeTexture(lowres);
	driver->removeTexture(postimg);
	driver->removeTexture(zbuffer);
}

void RenderingCorePlain::beforeDraw()
{
	if (!scale)
		return;
	core::array<video::IRenderTarget> rts;
	rts.push_back(lowres);
	rts.push_back(zbuffer);
	driver->setRenderTarget(rts, true, true, skycolor);
}

void RenderingCorePlain::postProcess()
{
	if(!scale || !postprocess){
		errorstream << "returning..." << std::endl;
		return;
	}
	if(!shaders){
		upscale();
		errorstream << "upscaling..." << std::endl;
		return;
	}
	errorstream << "drawing..." << std::endl;
	driver->setRenderTarget(postimg, true, true, skycolor);
	static const video::S3DVertex vertices[4] = {
		video::S3DVertex(1.0, -1.0, 0.0, 0.0, 0.0, -1.0, video::SColor(255, 0, 255, 255), 1.0, 0.0),
		video::S3DVertex(-1.0, -1.0, 0.0, 0.0, 0.0, -1.0, video::SColor(255, 255, 0, 255), 0.0, 0.0),
		video::S3DVertex(-1.0, 1.0, 0.0, 0.0, 0.0, -1.0, video::SColor(255, 255, 255, 0), 0.0, 1.0),
		video::S3DVertex(1.0, 1.0, 0.0, 0.0, 0.0, -1.0, video::SColor(255, 255, 255, 255), 1.0, 1.0),
	};
	static const u16 indices[6] = { 0, 1, 2, 2, 3, 0 };
	IShaderSource *s = client->getShaderSource();
	video::SMaterial mat;
	mat.UseMipMaps = false;
	mat.ZBuffer = false;
	mat.ZWriteEnable = false;
	mat.TextureLayer[0].AnisotropicFilter = false;
	mat.TextureLayer[0].TrilinearFilter = false;
	mat.TextureLayer[0].TextureWrapU = irr::video::ETC_CLAMP_TO_EDGE;
	mat.TextureLayer[0].TextureWrapV = irr::video::ETC_CLAMP_TO_EDGE;
	if (postprocess) {
		u32 shader = s->getShader("postprocessing", TILE_MATERIAL_BASIC, 0);
		mat.MaterialType = s->getShaderInfo(shader).material;
		mat.TextureLayer[0].BilinearFilter = true;
		mat.TextureLayer[0].Texture = lowres;
		mat.TextureLayer[2].BilinearFilter = true;
		mat.TextureLayer[2].Texture = zbuffer;
		if (scale)
			driver->setRenderTarget(postimg, false, false);
		driver->setMaterial(mat);
		driver->drawVertexPrimitiveList(&vertices, 4, &indices, 2);
		if (draw_wield_tool)
			camera->drawWieldedTool();
		if (scale)
			driver->setRenderTarget(0, false, false);
	}
	if (scale) {
		u32 shader = s->getShader("merging", TILE_MATERIAL_BASIC, 0);
		mat.MaterialType = s->getShaderInfo(shader).material;
		mat.TextureLayer[0].BilinearFilter = scale;
		mat.TextureLayer[0].Texture = postprocess ? postimg : lowres;
		driver->setMaterial(mat);
		driver->drawVertexPrimitiveList(&vertices, 4, &indices, 2);
	}
	
}


void RenderingCorePlain::upscale()
{
	if (!scale)
		return;
	driver->setRenderTarget(0, true, true);
	v2u32 size{scaledown(scale, screensize.X), scaledown(scale, screensize.Y)};
	v2u32 dest_size{scale * size.X, scale * size.Y};
	driver->draw2DImage(lowres, core::rect<s32>(0, 0, dest_size.X, dest_size.Y),
			core::rect<s32>(0, 0, size.X, size.Y));
}

void RenderingCorePlain::drawAll()
{
	draw3D();
	postProcess();
	drawPostFx();
	drawHUD();
}

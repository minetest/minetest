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
#include "client/client.h"
#include "client/camera.h"
#include "client/hud.h"
#include "client/shader.h"
#include "filesys.h"
#include "porting.h"
#include "settings.h"

RenderingCoreEquirectangular::RenderingCoreEquirectangular(
	IrrlichtDevice *_device, Client *_client, Hud *_hud)
	: RenderingCoreCubeMap(_device, _client, _hud), client(_client)
{
	image_size = {screensize.X, screensize.Y};

	saveAsImage = g_settings->getBool("360video_save");
	u32 sz = saveAsImage ?
			g_settings->getU32("360video_size") : screensize.Y;
	output_size = {sz * 2, sz};
	cubemap_size = {sz / 2, sz / 2};
	cubeatlas_size = {sz * 3 / 2, sz};

	IWritableShaderSource *s = client->getShaderSource();
	mat.UseMipMaps = false;
	mat.ZBuffer = false;
	mat.ZWriteEnable = false;
	u32 shader = s->getShader("equirectangular_merge", TILE_MATERIAL_BASIC);
	mat.MaterialType = s->getShaderInfo(shader).material;
	mat.TextureLayer[0].AnisotropicFilter = false;
	mat.TextureLayer[0].BilinearFilter = false;
	mat.TextureLayer[0].TrilinearFilter = false;
	mat.TextureLayer[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
	mat.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
}

void RenderingCoreEquirectangular::initTextures()
{
	RenderingCoreCubeMap::initTextures(cubemap_size);

	cubeAtlas = driver->addRenderTargetTexture(
			cubeatlas_size, "3d_render_cubeatlas", video::ECF_A8R8G8B8);
	mat.TextureLayer[0].Texture = cubeAtlas;

	renderOutput = driver->addRenderTargetTexture(
			output_size, "3d_render_eq", video::ECF_A8R8G8B8);
}

void RenderingCoreEquirectangular::clearTextures()
{
	RenderingCoreCubeMap::clearTextures();

	driver->removeTexture(cubeAtlas);
	driver->removeTexture(renderOutput);
}

void RenderingCoreEquirectangular::drawAll()
{
	if (!client->is360VideoMode()) {
		driver->setRenderTarget(nullptr, false, false, skycolor);
		draw3D();
		drawHUD();
		return;
	}

	scene::ICameraSceneNode *cam = camera->getCameraNode();
	core::vector3df camTarget = cam->getTarget();
	f32 camAspect = cam->getAspectRatio();
	f32 camFOV = cam->getFOV();

	camera->m_enable_draw_wielded_tool = false;
	for (int i = 0; i < 6; i ++) {
		useFace(i);
		draw3D();
	}
	camera->m_enable_draw_wielded_tool = true;

	driver->setRenderTarget(cubeAtlas, false, false, skycolor);
	for (int i = 0; i < 6; i ++)
		driver->draw2DImage(faces[i], v2s32(
				(i % 3) * cubemap_size.Width, (i / 3) * cubemap_size.Height));

	driver->setRenderTarget(renderOutput, false, false, skycolor);
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

	if (saveAsImage) {
		video::IImage *const raw_image = driver->createImageFromData(
				renderOutput->getColorFormat(), renderOutput->getSize(),
				renderOutput->lock());

		if (!raw_image)
			return;

		time_t t = time(NULL);
		struct tm *tm = localtime(&t);
		char timetstamp_c[17]; // YYYYMMDD_HHMMSS_ + '\0'
		strftime(timetstamp_c, sizeof(timetstamp_c), "%Y%m%d_%H%M%S_", tm);

		std::string filename_base = g_settings->get("360video_path") +
				DIR_DELIM + "360shot_" +
				std::string(timetstamp_c) +
				std::to_string(porting::getTimeMs() % 1000);
		std::string filename_ext = "." + g_settings->get("360video_format");
		std::string filename;

		u32 quality = (u32)g_settings->getS32("360video_quality");
		quality = MYMIN(MYMAX(quality, 0), 100) / 100.0 * 255;

		// Try to find a unique filename
		unsigned serial = 0;

		while (serial < SCREENSHOT_MAX_SERIAL_TRIES) {
			filename = filename_base + (serial > 0 ? ("_" + itos(serial)) : "") + filename_ext;
			std::ifstream tmp(filename.c_str());
			if (!tmp.good())
				break; // File did not apparently exist, so we'll go with it.
			serial ++;
		}

		if (serial == SCREENSHOT_MAX_SERIAL_TRIES) {
			infostream << "Could not find suitable filename for 360 shot." << std::endl;
		} else {
			irr::video::IImage *const image =
					driver->createImage(video::ECF_R8G8B8, raw_image->getDimension());

			if (image) {
				raw_image->copyTo(image);

				std::ostringstream sstr;
				if (driver->writeImageToFile(image, filename.c_str(), quality)) {
					sstr << "Saved 360 shot to '" << filename << "'.";
				} else {
					sstr << "Failed to save 360 shot '" << filename << "'.";
				}
				infostream << sstr.str() << std::endl;
				image->drop();
			}
		}
		raw_image->drop();
		renderOutput->unlock();

		// Restore any changes made by calling RenderingCoreCubeMap::useFace()
		cam->setTarget(camTarget);
		cam->setAspectRatio(camAspect);
		cam->setFOV(camFOV);

		// Render normal view after rendering and saving the output
		driver->setRenderTarget(nullptr, false, false, skycolor);
		draw3D();
		drawHUD();
	} else {
		driver->setRenderTarget(nullptr, false, false, skycolor);
		driver->draw2DImage(renderOutput, v2s32(0, 0));
	}
}

/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "mesh.h"
#include "log.h"
#include <cassert>
#include <iostream>
#include <IAnimatedMesh.h>
#include <SAnimatedMesh.h>
#include <ICameraSceneNode.h>

// In Irrlicht 1.8 the signature of ITexture::lock was changed from
// (bool, u32) to (E_TEXTURE_LOCK_MODE, u32).
#if IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR <= 7
#define MY_ETLM_READ_ONLY true
#else
#define MY_ETLM_READ_ONLY video::ETLM_READ_ONLY
#endif

scene::IAnimatedMesh* createCubeMesh(v3f scale)
{
	video::SColor c(255,255,255,255);
	video::S3DVertex vertices[24] =
	{
		// Up
		video::S3DVertex(-0.5,+0.5,-0.5, 0,1,0, c, 0,1),
		video::S3DVertex(-0.5,+0.5,+0.5, 0,1,0, c, 0,0),
		video::S3DVertex(+0.5,+0.5,+0.5, 0,1,0, c, 1,0),
		video::S3DVertex(+0.5,+0.5,-0.5, 0,1,0, c, 1,1),
		// Down
		video::S3DVertex(-0.5,-0.5,-0.5, 0,-1,0, c, 0,0),
		video::S3DVertex(+0.5,-0.5,-0.5, 0,-1,0, c, 1,0),
		video::S3DVertex(+0.5,-0.5,+0.5, 0,-1,0, c, 1,1),
		video::S3DVertex(-0.5,-0.5,+0.5, 0,-1,0, c, 0,1),
		// Right
		video::S3DVertex(+0.5,-0.5,-0.5, 1,0,0, c, 0,1),
		video::S3DVertex(+0.5,+0.5,-0.5, 1,0,0, c, 0,0),
		video::S3DVertex(+0.5,+0.5,+0.5, 1,0,0, c, 1,0),
		video::S3DVertex(+0.5,-0.5,+0.5, 1,0,0, c, 1,1),
		// Left
		video::S3DVertex(-0.5,-0.5,-0.5, -1,0,0, c, 1,1),
		video::S3DVertex(-0.5,-0.5,+0.5, -1,0,0, c, 0,1),
		video::S3DVertex(-0.5,+0.5,+0.5, -1,0,0, c, 0,0),
		video::S3DVertex(-0.5,+0.5,-0.5, -1,0,0, c, 1,0),
		// Back
		video::S3DVertex(-0.5,-0.5,+0.5, 0,0,1, c, 1,1),
		video::S3DVertex(+0.5,-0.5,+0.5, 0,0,1, c, 0,1),
		video::S3DVertex(+0.5,+0.5,+0.5, 0,0,1, c, 0,0),
		video::S3DVertex(-0.5,+0.5,+0.5, 0,0,1, c, 1,0),
		// Front
		video::S3DVertex(-0.5,-0.5,-0.5, 0,0,-1, c, 0,1),
		video::S3DVertex(-0.5,+0.5,-0.5, 0,0,-1, c, 0,0),
		video::S3DVertex(+0.5,+0.5,-0.5, 0,0,-1, c, 1,0),
		video::S3DVertex(+0.5,-0.5,-0.5, 0,0,-1, c, 1,1),
	};

	u16 indices[6] = {0,1,2,2,3,0};

	scene::SMesh *mesh = new scene::SMesh();
	for (u32 i=0; i<6; ++i)
	{
		scene::IMeshBuffer *buf = new scene::SMeshBuffer();
		buf->append(vertices + 4 * i, 4, indices, 6);
		// Set default material
		buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
		buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
		buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
		// Add mesh buffer to mesh
		mesh->addMeshBuffer(buf);
		buf->drop();
	}

	scene::SAnimatedMesh *anim_mesh = new scene::SAnimatedMesh(mesh);
	mesh->drop();
	scaleMesh(anim_mesh, scale);  // also recalculates bounding box
	return anim_mesh;
}

static scene::IAnimatedMesh* extrudeARGB(u32 twidth, u32 theight, u8 *data)
{
	const s32 argb_wstep = 4 * twidth;
	const s32 alpha_threshold = 1;

	scene::IMeshBuffer *buf = new scene::SMeshBuffer();
	video::SColor c(255,255,255,255);

	// Front and back
	{
		video::S3DVertex vertices[8] =
		{
			video::S3DVertex(-0.5,-0.5,-0.5, 0,0,-1, c, 0,1),
			video::S3DVertex(-0.5,+0.5,-0.5, 0,0,-1, c, 0,0),
			video::S3DVertex(+0.5,+0.5,-0.5, 0,0,-1, c, 1,0),
			video::S3DVertex(+0.5,-0.5,-0.5, 0,0,-1, c, 1,1),
			video::S3DVertex(+0.5,-0.5,+0.5, 0,0,+1, c, 1,1),
			video::S3DVertex(+0.5,+0.5,+0.5, 0,0,+1, c, 1,0),
			video::S3DVertex(-0.5,+0.5,+0.5, 0,0,+1, c, 0,0),
			video::S3DVertex(-0.5,-0.5,+0.5, 0,0,+1, c, 0,1),
		};
		u16 indices[12] = {0,1,2,2,3,0,4,5,6,6,7,4};
		buf->append(vertices, 8, indices, 12);
	}

	// "Interior"
	// (add faces where a solid pixel is next to a transparent one)
	u8 *solidity = new u8[(twidth+2) * (theight+2)];
	u32 wstep = twidth + 2;
	for (u32 y = 0; y < theight + 2; ++y)
	{
		u8 *scanline = solidity + y * wstep;
		if (y == 0 || y == theight + 1)
		{
			for (u32 x = 0; x < twidth + 2; ++x)
				scanline[x] = 0;
		}
		else
		{
			scanline[0] = 0;
			u8 *argb_scanline = data + (y - 1) * argb_wstep;
			for (u32 x = 0; x < twidth; ++x)
				scanline[x+1] = (argb_scanline[x*4+3] >= alpha_threshold);
			scanline[twidth + 1] = 0;
		}
	}

	// without this, there would be occasional "holes" in the mesh
	f32 eps = 0.01;

	for (u32 y = 0; y <= theight; ++y)
	{
		u8 *scanline = solidity + y * wstep + 1;
		for (u32 x = 0; x <= twidth; ++x)
		{
			if (scanline[x] && !scanline[x + wstep])
			{
				u32 xx = x + 1;
				while (scanline[xx] && !scanline[xx + wstep])
					++xx;
				f32 vx1 = (x - eps) / (f32) twidth - 0.5;
				f32 vx2 = (xx + eps) / (f32) twidth - 0.5;
				f32 vy = 0.5 - (y - eps) / (f32) theight;
				f32 tx1 = x / (f32) twidth;
				f32 tx2 = xx / (f32) twidth;
				f32 ty = (y - 0.5) / (f32) theight;
				video::S3DVertex vertices[8] =
				{
					video::S3DVertex(vx1,vy,-0.5, 0,-1,0, c, tx1,ty),
					video::S3DVertex(vx2,vy,-0.5, 0,-1,0, c, tx2,ty),
					video::S3DVertex(vx2,vy,+0.5, 0,-1,0, c, tx2,ty),
					video::S3DVertex(vx1,vy,+0.5, 0,-1,0, c, tx1,ty),
				};
				u16 indices[6] = {0,1,2,2,3,0};
				buf->append(vertices, 4, indices, 6);
				x = xx - 1;
			}
			if (!scanline[x] && scanline[x + wstep])
			{
				u32 xx = x + 1;
				while (!scanline[xx] && scanline[xx + wstep])
					++xx;
				f32 vx1 = (x - eps) / (f32) twidth - 0.5;
				f32 vx2 = (xx + eps) / (f32) twidth - 0.5;
				f32 vy = 0.5 - (y + eps) / (f32) theight;
				f32 tx1 = x / (f32) twidth;
				f32 tx2 = xx / (f32) twidth;
				f32 ty = (y + 0.5) / (f32) theight;
				video::S3DVertex vertices[8] =
				{
					video::S3DVertex(vx1,vy,-0.5, 0,1,0, c, tx1,ty),
					video::S3DVertex(vx1,vy,+0.5, 0,1,0, c, tx1,ty),
					video::S3DVertex(vx2,vy,+0.5, 0,1,0, c, tx2,ty),
					video::S3DVertex(vx2,vy,-0.5, 0,1,0, c, tx2,ty),
				};
				u16 indices[6] = {0,1,2,2,3,0};
				buf->append(vertices, 4, indices, 6);
				x = xx - 1;
			}
		}
	}

	for (u32 x = 0; x <= twidth; ++x)
	{
		u8 *scancol = solidity + x + wstep;
		for (u32 y = 0; y <= theight; ++y)
		{
			if (scancol[y * wstep] && !scancol[y * wstep + 1])
			{
				u32 yy = y + 1;
				while (scancol[yy * wstep] && !scancol[yy * wstep + 1])
					++yy;
				f32 vx = (x - eps) / (f32) twidth - 0.5;
				f32 vy1 = 0.5 - (y - eps) / (f32) theight;
				f32 vy2 = 0.5 - (yy + eps) / (f32) theight;
				f32 tx = (x - 0.5) / (f32) twidth;
				f32 ty1 = y / (f32) theight;
				f32 ty2 = yy / (f32) theight;
				video::S3DVertex vertices[8] =
				{
					video::S3DVertex(vx,vy1,-0.5, 1,0,0, c, tx,ty1),
					video::S3DVertex(vx,vy1,+0.5, 1,0,0, c, tx,ty1),
					video::S3DVertex(vx,vy2,+0.5, 1,0,0, c, tx,ty2),
					video::S3DVertex(vx,vy2,-0.5, 1,0,0, c, tx,ty2),
				};
				u16 indices[6] = {0,1,2,2,3,0};
				buf->append(vertices, 4, indices, 6);
				y = yy - 1;
			}
			if (!scancol[y * wstep] && scancol[y * wstep + 1])
			{
				u32 yy = y + 1;
				while (!scancol[yy * wstep] && scancol[yy * wstep + 1])
					++yy;
				f32 vx = (x + eps) / (f32) twidth - 0.5;
				f32 vy1 = 0.5 - (y - eps) / (f32) theight;
				f32 vy2 = 0.5 - (yy + eps) / (f32) theight;
				f32 tx = (x + 0.5) / (f32) twidth;
				f32 ty1 = y / (f32) theight;
				f32 ty2 = yy / (f32) theight;
				video::S3DVertex vertices[8] =
				{
					video::S3DVertex(vx,vy1,-0.5, -1,0,0, c, tx,ty1),
					video::S3DVertex(vx,vy2,-0.5, -1,0,0, c, tx,ty2),
					video::S3DVertex(vx,vy2,+0.5, -1,0,0, c, tx,ty2),
					video::S3DVertex(vx,vy1,+0.5, -1,0,0, c, tx,ty1),
				};
				u16 indices[6] = {0,1,2,2,3,0};
				buf->append(vertices, 4, indices, 6);
				y = yy - 1;
			}
		}
	}

	// Add to mesh
	scene::SMesh *mesh = new scene::SMesh();
	mesh->addMeshBuffer(buf);
	buf->drop();
	scene::SAnimatedMesh *anim_mesh = new scene::SAnimatedMesh(mesh);
	mesh->drop();
	return anim_mesh;
}

scene::IAnimatedMesh* createExtrudedMesh(video::ITexture *texture,
		video::IVideoDriver *driver, v3f scale)
{
	scene::IAnimatedMesh *mesh = NULL;
	core::dimension2d<u32> size = texture->getSize();
	video::ECOLOR_FORMAT format = texture->getColorFormat();
	if (format == video::ECF_A8R8G8B8)
	{
		// Texture is in the correct color format, we can pass it
		// to extrudeARGB right away.
		void *data = texture->lock(MY_ETLM_READ_ONLY);
		if (data == NULL)
			return NULL;
		mesh = extrudeARGB(size.Width, size.Height, (u8*) data);
		texture->unlock();
	}
	else
	{
		video::IImage *img1 = driver->createImageFromData(format, size, texture->lock(MY_ETLM_READ_ONLY));
		if (img1 == NULL)
			return NULL;

		// img1 is in the texture's color format, convert to 8-bit ARGB
		video::IImage *img2 = driver->createImage(video::ECF_A8R8G8B8, size);
		if (img2 != NULL)
		{
			img1->copyTo(img2);
			img1->drop();

			mesh = extrudeARGB(size.Width, size.Height, (u8*) img2->lock());
			img2->unlock();
			img2->drop();
		}
		img1->drop();
	}

	// Set default material
	mesh->getMeshBuffer(0)->getMaterial().setTexture(0, texture);
	mesh->getMeshBuffer(0)->getMaterial().setFlag(video::EMF_LIGHTING, false);
	mesh->getMeshBuffer(0)->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
	mesh->getMeshBuffer(0)->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;

	scaleMesh(mesh, scale);  // also recalculates bounding box
	return mesh;
}

void scaleMesh(scene::IMesh *mesh, v3f scale)
{
	if(mesh == NULL)
		return;

	core::aabbox3d<f32> bbox;
	bbox.reset(0,0,0);

	u16 mc = mesh->getMeshBufferCount();
	for(u16 j=0; j<mc; j++)
	{
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
		video::S3DVertex *vertices = (video::S3DVertex*)buf->getVertices();
		u16 vc = buf->getVertexCount();
		for(u16 i=0; i<vc; i++)
		{
			vertices[i].Pos *= scale;
		}
		buf->recalculateBoundingBox();

		// calculate total bounding box
		if(j == 0)
			bbox = buf->getBoundingBox();
		else
			bbox.addInternalBox(buf->getBoundingBox());
	}
	mesh->setBoundingBox(bbox);
}

void translateMesh(scene::IMesh *mesh, v3f vec)
{
	if(mesh == NULL)
		return;

	core::aabbox3d<f32> bbox;
	bbox.reset(0,0,0);

	u16 mc = mesh->getMeshBufferCount();
	for(u16 j=0; j<mc; j++)
	{
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
		video::S3DVertex *vertices = (video::S3DVertex*)buf->getVertices();
		u16 vc = buf->getVertexCount();
		for(u16 i=0; i<vc; i++)
		{
			vertices[i].Pos += vec;
		}
		buf->recalculateBoundingBox();

		// calculate total bounding box
		if(j == 0)
			bbox = buf->getBoundingBox();
		else
			bbox.addInternalBox(buf->getBoundingBox());
	}
	mesh->setBoundingBox(bbox);
}

void setMeshColor(scene::IMesh *mesh, const video::SColor &color)
{
	if(mesh == NULL)
		return;
	
	u16 mc = mesh->getMeshBufferCount();
	for(u16 j=0; j<mc; j++)
	{
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
		video::S3DVertex *vertices = (video::S3DVertex*)buf->getVertices();
		u16 vc = buf->getVertexCount();
		for(u16 i=0; i<vc; i++)
		{
			vertices[i].Color = color;
		}
	}
}

void setMeshColorByNormalXYZ(scene::IMesh *mesh,
		const video::SColor &colorX,
		const video::SColor &colorY,
		const video::SColor &colorZ)
{
	if(mesh == NULL)
		return;
	
	u16 mc = mesh->getMeshBufferCount();
	for(u16 j=0; j<mc; j++)
	{
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
		video::S3DVertex *vertices = (video::S3DVertex*)buf->getVertices();
		u16 vc = buf->getVertexCount();
		for(u16 i=0; i<vc; i++)
		{
			f32 x = fabs(vertices[i].Normal.X);
			f32 y = fabs(vertices[i].Normal.Y);
			f32 z = fabs(vertices[i].Normal.Z);
			if(x >= y && x >= z)
				vertices[i].Color = colorX;
			else if(y >= z)
				vertices[i].Color = colorY;
			else
				vertices[i].Color = colorZ;

		}
	}
}

video::ITexture *generateTextureFromMesh(scene::IMesh *mesh,
		IrrlichtDevice *device,
		core::dimension2d<u32> dim,
		std::string texture_name,
		v3f camera_position,
		v3f camera_lookat,
		core::CMatrix4<f32> camera_projection_matrix,
		video::SColorf ambient_light,
		v3f light_position,
		video::SColorf light_color,
		f32 light_radius)
{
	video::IVideoDriver *driver = device->getVideoDriver();
	if(driver->queryFeature(video::EVDF_RENDER_TO_TARGET) == false)
	{
		static bool warned = false;
		if(!warned)
		{
			errorstream<<"generateTextureFromMesh(): EVDF_RENDER_TO_TARGET"
					" not supported."<<std::endl;
			warned = true;
		}
		return NULL;
	}

	// Create render target texture
	video::ITexture *rtt = driver->addRenderTargetTexture(
			dim, texture_name.c_str(), video::ECF_A8R8G8B8);
	if(rtt == NULL)
	{
		errorstream<<"generateTextureFromMesh(): addRenderTargetTexture"
				" returned NULL."<<std::endl;
		return NULL;
	}

	// Set render target
	driver->setRenderTarget(rtt, true, true, video::SColor(0,0,0,0));

	// Get a scene manager
	scene::ISceneManager *smgr_main = device->getSceneManager();
	assert(smgr_main);
	scene::ISceneManager *smgr = smgr_main->createNewSceneManager();
	assert(smgr);

	scene::IMeshSceneNode* meshnode = smgr->addMeshSceneNode(mesh, NULL, -1, v3f(0,0,0), v3f(0,0,0), v3f(1,1,1), true);
	meshnode->setMaterialFlag(video::EMF_LIGHTING, true);
	meshnode->setMaterialFlag(video::EMF_ANTI_ALIASING, true);
	meshnode->setMaterialFlag(video::EMF_BILINEAR_FILTER, true);

	scene::ICameraSceneNode* camera = smgr->addCameraSceneNode(0,
			camera_position, camera_lookat);
	// second parameter of setProjectionMatrix (isOrthogonal) is ignored
	camera->setProjectionMatrix(camera_projection_matrix, false);

	smgr->setAmbientLight(ambient_light);
	smgr->addLightSceneNode(0, light_position, light_color, light_radius);

	// Render scene
	driver->beginScene(true, true, video::SColor(0,0,0,0));
	smgr->drawAll();
	driver->endScene();

	// NOTE: The scene nodes should not be dropped, otherwise
	//       smgr->drop() segfaults
	/*cube->drop();
	camera->drop();
	light->drop();*/
	// Drop scene manager
	smgr->drop();

	// Unset render target
	driver->setRenderTarget(0, true, true, 0);

	return rtt;
}

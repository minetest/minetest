/*
Minetest
Copyright (C) 2010-2015 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "minimap.h"
#include "logoutputbuffer.h"
#include "jthread/jmutexautolock.h"
#include "jthread/jsemaphore.h"
#include "clientmap.h"
#include "settings.h"
#include "nodedef.h"
#include "porting.h"
#include "util/numeric.h"
#include "util/string.h"
#include <math.h>

QueuedMinimapUpdate::QueuedMinimapUpdate():
	pos(-1337,-1337,-1337),
	data(NULL)
{
}

QueuedMinimapUpdate::~QueuedMinimapUpdate()
{
	delete data;
}

MinimapUpdateQueue::MinimapUpdateQueue()
{
}

MinimapUpdateQueue::~MinimapUpdateQueue()
{
	JMutexAutoLock lock(m_mutex);

	for (std::list<QueuedMinimapUpdate*>::iterator
			i = m_queue.begin();
			i != m_queue.end(); ++i) {
		QueuedMinimapUpdate *q = *i;
		delete q;
	}
}

bool MinimapUpdateQueue::addBlock(v3s16 pos, MinimapMapblock *data)
{
	DSTACK(__FUNCTION_NAME);

	JMutexAutoLock lock(m_mutex);

	/*
		Find if block is already in queue.
		If it is, update the data and quit.
	*/
	for (std::list<QueuedMinimapUpdate*>::iterator
			i = m_queue.begin();
			i != m_queue.end(); ++i) {
		QueuedMinimapUpdate *q = *i;
		if (q->pos == pos) {
			delete q->data;
			q->data = data;
			return false;
		}
	}

	/*
		Add the block
	*/
	QueuedMinimapUpdate *q = new QueuedMinimapUpdate;
	q->pos = pos;
	q->data = data;
	m_queue.push_back(q);
	return true;
}

QueuedMinimapUpdate * MinimapUpdateQueue::pop()
{
	JMutexAutoLock lock(m_mutex);

	for (std::list<QueuedMinimapUpdate*>::iterator
			i = m_queue.begin();
			i != m_queue.end(); i++) {
		QueuedMinimapUpdate *q = *i;
		m_queue.erase(i);
		return q;
	}
	return NULL;
}

/*
	Minimap update thread
*/

void MinimapUpdateThread::Stop()
{
	JThread::Stop();

	// give us a nudge
	m_queue_sem.Post();
}

void MinimapUpdateThread::enqueue_Block(v3s16 pos, MinimapMapblock *data)
{
	if (m_queue.addBlock(pos, data))
		// we had to allocate a new block
		m_queue_sem.Post();
}

void MinimapUpdateThread::forceUpdate()
{
	m_queue_sem.Post();
}

void *MinimapUpdateThread::Thread()
{
	ThreadStarted();

	log_register_thread("MinimapUpdateThread");

	DSTACK(__FUNCTION_NAME);

	BEGIN_DEBUG_EXCEPTION_HANDLER

	porting::setThreadName("MinimapUpdateThread");

	while (!StopRequested()) {

		m_queue_sem.Wait();
		if (StopRequested()) break;

		while (m_queue.size()) {
			QueuedMinimapUpdate *q = m_queue.pop();
			if (!q)
				break;
			std::map<v3s16, MinimapMapblock *>::iterator it;
			it = m_blocks_cache.find(q->pos);
			if (q->data) {
				m_blocks_cache[q->pos] = q->data;
			} else if (it != m_blocks_cache.end()) {
				delete it->second;
				m_blocks_cache.erase(it);
			}
		}

		if (data->map_invalidated) {
			if (data->mode != MINIMAP_MODE_OFF) {
				getMap(data->pos, data->map_size, data->scan_height, data->radar);
				data->map_invalidated = false;
			}
		}
	}
	END_DEBUG_EXCEPTION_HANDLER(errorstream)

	return NULL;
}

MinimapUpdateThread::~MinimapUpdateThread()
{
	for (std::map<v3s16, MinimapMapblock *>::iterator
			it = m_blocks_cache.begin();
			it != m_blocks_cache.end(); it++) {
		delete it->second;
	}
}

MinimapPixel *MinimapUpdateThread::getMinimapPixel (v3s16 pos, s16 height, s16 &pixel_height)
{
	pixel_height = height - MAP_BLOCKSIZE;
	v3s16 blockpos_max, blockpos_min, relpos;
	getNodeBlockPosWithOffset(v3s16(pos.X, pos.Y - height / 2, pos.Z), blockpos_min, relpos);
	getNodeBlockPosWithOffset(v3s16(pos.X, pos.Y + height / 2, pos.Z), blockpos_max, relpos);
	std::map<v3s16, MinimapMapblock *>::iterator it;
	for (s16 i = blockpos_max.Y; i > blockpos_min.Y - 1; i--) {
		it = m_blocks_cache.find(v3s16(blockpos_max.X, i, blockpos_max.Z));
		if (it != m_blocks_cache.end()) {
			MinimapPixel *pixel = &it->second->data[relpos.X + relpos.Z * MAP_BLOCKSIZE];
			if (pixel->id != CONTENT_AIR) {
				pixel_height += pixel->height;
				return pixel;
			}
		}
		pixel_height -= MAP_BLOCKSIZE;
	}
	return NULL;
}

s16 MinimapUpdateThread::getAirCount (v3s16 pos, s16 height)
{
	s16 air_count = 0;
	v3s16 blockpos_max, blockpos_min, relpos;
	getNodeBlockPosWithOffset(v3s16(pos.X, pos.Y - height / 2, pos.Z), blockpos_min, relpos);
	getNodeBlockPosWithOffset(v3s16(pos.X, pos.Y + height / 2, pos.Z), blockpos_max, relpos);
	std::map<v3s16, MinimapMapblock *>::iterator it;
	for (s16 i = blockpos_max.Y; i > blockpos_min.Y - 1; i--) {
		it = m_blocks_cache.find(v3s16(blockpos_max.X, i, blockpos_max.Z));
		if (it != m_blocks_cache.end()) {
			MinimapPixel *pixel = &it->second->data[relpos.X + relpos.Z * MAP_BLOCKSIZE];
			air_count += pixel->air_count;
		}
	}
	return air_count;
}

void MinimapUpdateThread::getMap (v3s16 pos, s16 size, s16 height, bool radar)
{
	v3s16 p = v3s16 (pos.X - size / 2, pos.Y, pos.Z - size / 2);

	for (s16 x = 0; x < size; x++) {
		for (s16 z = 0; z < size; z++){
			u16 id = CONTENT_AIR;
			MinimapPixel* minimap_pixel = &data->minimap_scan[x + z * size];
			if (!radar) {
				s16 pixel_height = 0;
				MinimapPixel* cached_pixel =
					getMinimapPixel(v3s16(p.X + x, p.Y, p.Z + z), height, pixel_height);
				if (cached_pixel) {
					id = cached_pixel->id;
					minimap_pixel->height = pixel_height;
				}
			} else {
				minimap_pixel->air_count = getAirCount (v3s16(p.X + x, p.Y, p.Z + z), height);
			}
			minimap_pixel->id = id;
		}
	}
}

Mapper::Mapper(IrrlichtDevice *device, Client *client)
{
	this->device = device;
	this->client = client;
	this->driver = device->getVideoDriver();
	this->tsrc = client->getTextureSource();
	this->player = client->getEnv().getLocalPlayer();
	this->shdrsrc = client->getShaderSource();

	m_enable_shaders = g_settings->getBool("enable_shaders");
	m_enable_shaders = g_settings->getBool("enable_shaders");
	if (g_settings->getBool("minimap_double_scan_height")) {
		m_surface_mode_scan_height = 256;
	} else {
		m_surface_mode_scan_height = 128;
	}
	data = new MinimapData;
	data->mode = MINIMAP_MODE_OFF;
	data->radar = false;
	data->map_invalidated = true;
	data->heightmap_image = NULL;
	data->minimap_image = NULL;
	data->texture = NULL;
	data->minimap_shape_round = g_settings->getBool("minimap_shape_round");
	std::string fname1 = "minimap_mask_round.png";
	std::string fname2 = "minimap_overlay_round.png";
	data->minimap_mask_round = driver->createImage (tsrc->getTexture(fname1),
			core::position2d<s32>(0,0), core::dimension2d<u32>(512,512));
	data->minimap_overlay_round = tsrc->getTexture(fname2);
	fname1 = "minimap_mask_square.png";
	fname2 = "minimap_overlay_square.png";
	data->minimap_mask_square = driver->createImage (tsrc->getTexture(fname1),
			core::position2d<s32>(0,0), core::dimension2d<u32>(512,512));
	data->minimap_overlay_square = tsrc->getTexture(fname2);
	data->player_marker = tsrc->getTexture("player_marker.png");
	m_meshbuffer = getMinimapMeshBuffer();
	m_minimap_update_thread = new MinimapUpdateThread(device, client);
	m_minimap_update_thread->data = data;
	m_minimap_update_thread->Start();
}

Mapper::~Mapper()
{
	m_minimap_update_thread->Stop();
	m_minimap_update_thread->Wait();
	m_meshbuffer->drop();
	data->minimap_mask_round->drop();
	data->minimap_mask_square->drop();
	driver->removeTexture(data->texture);
	driver->removeTexture(data->heightmap_texture);
	driver->removeTexture(data->minimap_overlay_round);
	driver->removeTexture(data->minimap_overlay_square);
	delete data;
	delete m_minimap_update_thread;
}

void Mapper::addBlock (v3s16 pos, MinimapMapblock *data)
{
	m_minimap_update_thread->enqueue_Block(pos, data);
}

MinimapMode Mapper::getMinimapMode()
{
	return data->mode;
}

void Mapper::toggleMinimapShape()
{
	data->minimap_shape_round = !data->minimap_shape_round;
	g_settings->setBool(("minimap_shape_round"), data->minimap_shape_round);
}

void Mapper::setMinimapMode(MinimapMode mode)
{
	static const u16 modeDefs[7 * 3] = {
		0, 0, 0,
		0, m_surface_mode_scan_height, 256,
		0, m_surface_mode_scan_height, 128,
		0, m_surface_mode_scan_height, 64,
		1, 32, 128,
		1, 32, 64,
		1, 32, 32};

	JMutexAutoLock lock(m_mutex);
	data->radar = (bool)modeDefs[(int)mode * 3];
	data->scan_height = modeDefs[(int)mode * 3 + 1];
	data->map_size = modeDefs[(int)mode * 3 + 2];
	data->mode = mode;
	m_minimap_update_thread->forceUpdate();
}

void Mapper::setPos(v3s16 pos)
{
	JMutexAutoLock lock(m_mutex);
	if (pos != data->old_pos) {
		data->old_pos = data->pos;
		data->pos = pos;
		m_minimap_update_thread->forceUpdate();
	}
}

video::ITexture *Mapper::getMinimapTexture()
{
	// update minimap textures when new scan is ready
	if (!data->map_invalidated) {
		// create minimap and heightmap image
		core::dimension2d<u32> dim(data->map_size,data->map_size);
		video::IImage *map_image = driver->createImage(video::ECF_A8R8G8B8, dim);
		video::IImage *heightmap_image = driver->createImage(video::ECF_A8R8G8B8, dim);
		video::IImage *minimap_image = driver->createImage(video::ECF_A8R8G8B8,
			core::dimension2d<u32>(512, 512));

		video::SColor c;
		if (!data->radar) {
			// surface mode
			for (s16 x = 0; x < data->map_size; x++) {
				for (s16 z = 0; z < data->map_size; z++) {
					MinimapPixel* minimap_pixel = &data->minimap_scan[x + z * data->map_size];
					const ContentFeatures &f = client->getNodeDefManager()->get(minimap_pixel->id);
					c = f.minimap_color;
					c.setAlpha(240);
					map_image->setPixel(x, data->map_size - z -1, c);
					u32 h = minimap_pixel->height;
					heightmap_image->setPixel(x,data->map_size -z -1,
						video::SColor(255, h, h, h));
				}
			}
		} else {
			// radar mode
			c = video::SColor (240, 0 , 0, 0);
			for (s16 x = 0; x < data->map_size; x++) {
				for (s16 z = 0; z < data->map_size; z++) {
					MinimapPixel* minimap_pixel = &data->minimap_scan[x + z * data->map_size];
					if (minimap_pixel->air_count > 0) {
						c.setGreen(core::clamp(core::round32(32 + minimap_pixel->air_count * 8), 0, 255));
					} else {
						c.setGreen(0);
					}
					map_image->setPixel(x, data->map_size - z -1, c);
				}
			}
		}

		map_image->copyToScaling(minimap_image);
		map_image->drop();

		video::IImage *minimap_mask;
		if (data->minimap_shape_round) {
			minimap_mask = data->minimap_mask_round;
		} else {
			minimap_mask = data->minimap_mask_square;
		}
		for (s16 x = 0; x < 512; x++) {
			for (s16 y = 0; y < 512; y++) {
				video::SColor mask_col = minimap_mask->getPixel(x, y);
				if (!mask_col.getAlpha()) {
					minimap_image->setPixel(x, y, video::SColor(0,0,0,0));
				}
			}
		}

		if (data->texture) {
			driver->removeTexture(data->texture);
		}
		if (data->heightmap_texture) {
			driver->removeTexture(data->heightmap_texture);
		}
		data->texture = driver->addTexture("minimap__", minimap_image);
		data->heightmap_texture = driver->addTexture("minimap_heightmap__", heightmap_image);
		minimap_image->drop();
		heightmap_image->drop();

		data->map_invalidated = true;
	}
	return data->texture;
}

v3f Mapper::getYawVec()
{
	if (data->minimap_shape_round) {
		return v3f(cos(player->getYaw()* core::DEGTORAD),
			sin(player->getYaw()* core::DEGTORAD), 1.0);
	} else {
		return v3f(1.0, 0.0, 1.0);
	}
}

scene::SMeshBuffer *Mapper::getMinimapMeshBuffer()
{
	scene::SMeshBuffer *buf = new scene::SMeshBuffer();
	buf->Vertices.set_used(4);
	buf->Indices .set_used(6);
	video::SColor c(255, 255, 255, 255);

	buf->Vertices[0] = video::S3DVertex(-1, -1, 0, 0, 0, 1, c, 0, 1);
	buf->Vertices[1] = video::S3DVertex(-1,  1, 0, 0, 0, 1, c, 0, 0);
	buf->Vertices[2] = video::S3DVertex( 1,  1, 0, 0, 0, 1, c, 1, 0);
	buf->Vertices[3] = video::S3DVertex( 1, -1, 0, 0, 0, 1, c, 1, 1);

	buf->Indices[0] = 0;
	buf->Indices[1] = 1;
	buf->Indices[2] = 2;
	buf->Indices[3] = 2;
	buf->Indices[4] = 3;
	buf->Indices[5] = 0;

	return buf;
}

void Mapper::drawMinimap()
{
	v2u32 screensize = porting::getWindowSize();
	u32 size = 0.25 * screensize.Y;
	video::ITexture* minimap_texture = getMinimapTexture();
	core::matrix4 matrix;

	core::rect<s32> oldViewPort = driver->getViewPort();
	driver->setViewPort(core::rect<s32>(screensize.X - size - 10, 10,
		screensize.X - 10, size + 10));
	core::matrix4 oldProjMat = driver->getTransform(video::ETS_PROJECTION);
	driver->setTransform(video::ETS_PROJECTION, core::matrix4());
	core::matrix4 oldViewMat = driver->getTransform(video::ETS_VIEW);
	driver->setTransform(video::ETS_VIEW, core::matrix4());
	matrix.makeIdentity();

	if (minimap_texture) {
		video::SMaterial& material = m_meshbuffer->getMaterial();
		material.setFlag(video::EMF_TRILINEAR_FILTER, true);
		material.Lighting = false;
		material.TextureLayer[0].Texture = minimap_texture;
		material.TextureLayer[1].Texture = data->heightmap_texture;
		if (m_enable_shaders && !data->radar) {
			u16 sid = shdrsrc->getShader("minimap_shader", 1, 1);
			material.MaterialType = shdrsrc->getShaderInfo(sid).material;
		} else {
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		}

		if (data->minimap_shape_round)
			matrix.setRotationDegrees(core::vector3df(0, 0, 360 - player->getYaw()));
		driver->setTransform(video::ETS_WORLD, matrix);
		driver->setMaterial(material);
		driver->drawMeshBuffer(m_meshbuffer);
		video::ITexture *minimap_overlay;
		if (data->minimap_shape_round) {
			minimap_overlay = data->minimap_overlay_round;
		} else {
			minimap_overlay = data->minimap_overlay_square;
		}
		material.TextureLayer[0].Texture = minimap_overlay;
		material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		driver->setMaterial(material);
		driver->drawMeshBuffer(m_meshbuffer);

		if (!data->minimap_shape_round) {
			matrix.setRotationDegrees(core::vector3df(0, 0, player->getYaw()));
			driver->setTransform(video::ETS_WORLD, matrix);
			material.TextureLayer[0].Texture = data->player_marker;
			driver->setMaterial(material);
			driver->drawMeshBuffer(m_meshbuffer);
		}
	}

	driver->setTransform(video::ETS_VIEW, oldViewMat);
	driver->setTransform(video::ETS_PROJECTION, oldProjMat);
	driver->setViewPort(oldViewPort);
}

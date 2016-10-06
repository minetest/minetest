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
#include "threading/mutex_auto_lock.h"
#include "threading/semaphore.h"
#include "clientmap.h"
#include "settings.h"
#include "nodedef.h"
#include "porting.h"
#include "util/numeric.h"
#include "util/string.h"
#include <math.h>


////
//// MinimapUpdateThread
////

MinimapUpdateThread::~MinimapUpdateThread()
{
	for (std::map<v3s16, MinimapMapblock *>::iterator
			it = m_blocks_cache.begin();
			it != m_blocks_cache.end(); ++it) {
		delete it->second;
	}

	for (std::deque<QueuedMinimapUpdate>::iterator
			it = m_update_queue.begin();
			it != m_update_queue.end(); ++it) {
		QueuedMinimapUpdate &q = *it;
		delete q.data;
	}
}

bool MinimapUpdateThread::pushBlockUpdate(v3s16 pos, MinimapMapblock *data)
{
	MutexAutoLock lock(m_queue_mutex);

	// Find if block is already in queue.
	// If it is, update the data and quit.
	for (std::deque<QueuedMinimapUpdate>::iterator
			it = m_update_queue.begin();
			it != m_update_queue.end(); ++it) {
		QueuedMinimapUpdate &q = *it;
		if (q.pos == pos) {
			delete q.data;
			q.data = data;
			return false;
		}
	}

	// Add the block
	QueuedMinimapUpdate q;
	q.pos  = pos;
	q.data = data;
	m_update_queue.push_back(q);

	return true;
}

bool MinimapUpdateThread::popBlockUpdate(QueuedMinimapUpdate *update)
{
	MutexAutoLock lock(m_queue_mutex);

	if (m_update_queue.empty())
		return false;

	*update = m_update_queue.front();
	m_update_queue.pop_front();

	return true;
}

void MinimapUpdateThread::enqueueBlock(v3s16 pos, MinimapMapblock *data)
{
	pushBlockUpdate(pos, data);
	deferUpdate();
}


void MinimapUpdateThread::doUpdate()
{
	QueuedMinimapUpdate update;

	while (popBlockUpdate(&update)) {
		if (update.data) {
			// Swap two values in the map using single lookup
			std::pair<std::map<v3s16, MinimapMapblock*>::iterator, bool>
			    result = m_blocks_cache.insert(std::make_pair(update.pos, update.data));
			if (result.second == false) {
				delete result.first->second;
				result.first->second = update.data;
			}
		} else {
			std::map<v3s16, MinimapMapblock *>::iterator it;
			it = m_blocks_cache.find(update.pos);
			if (it != m_blocks_cache.end()) {
				delete it->second;
				m_blocks_cache.erase(it);
			}
		}
	}

	MutexAutoLock(m_data->mutex);
	MinimapModeDef *mode = m_data->mode;
	if (m_data->map_invalidated && mode->is_minimap_shown) {
		getMap(m_data->pos, mode->map_size, mode->scan_height, mode->is_radar);
		m_data->map_invalidated = false;
	}
}

MinimapPixel *MinimapUpdateThread::getMinimapPixel(v3s16 pos,
	s16 scan_height, s16 *pixel_height)
{
	s16 height = scan_height - MAP_BLOCKSIZE;
	v3s16 blockpos_max, blockpos_min, relpos;

	getNodeBlockPosWithOffset(
		v3s16(pos.X, pos.Y - scan_height / 2, pos.Z),
		blockpos_min, relpos);
	getNodeBlockPosWithOffset(
		v3s16(pos.X, pos.Y + scan_height / 2, pos.Z),
		blockpos_max, relpos);

	for (s16 i = blockpos_max.Y; i > blockpos_min.Y - 1; i--) {
		std::map<v3s16, MinimapMapblock *>::iterator it =
			m_blocks_cache.find(v3s16(blockpos_max.X, i, blockpos_max.Z));
		if (it != m_blocks_cache.end()) {
			MinimapMapblock *mmblock = it->second;
			MinimapPixel *pixel = &mmblock->data[relpos.Z * MAP_BLOCKSIZE + relpos.X];
			if (pixel->id != CONTENT_AIR) {
				*pixel_height = height + pixel->height;
				return pixel;
			}
		}

		height -= MAP_BLOCKSIZE;
	}

	return NULL;
}

s16 MinimapUpdateThread::getAirCount(v3s16 pos, s16 height)
{
	s16 air_count = 0;
	v3s16 blockpos_max, blockpos_min, relpos;

	getNodeBlockPosWithOffset(
		v3s16(pos.X, pos.Y - height / 2, pos.Z),
		blockpos_min, relpos);
	getNodeBlockPosWithOffset(
		v3s16(pos.X, pos.Y + height / 2, pos.Z),
		blockpos_max, relpos);

	for (s16 i = blockpos_max.Y; i > blockpos_min.Y - 1; i--) {
		std::map<v3s16, MinimapMapblock *>::iterator it =
			m_blocks_cache.find(v3s16(blockpos_max.X, i, blockpos_max.Z));
		if (it != m_blocks_cache.end()) {
			MinimapMapblock *mmblock = it->second;
			MinimapPixel *pixel = &mmblock->data[relpos.Z * MAP_BLOCKSIZE + relpos.X];
			air_count += pixel->air_count;
		}
	}

	return air_count;
}

void MinimapUpdateThread::getMap(v3s16 pos, s16 size, s16 height, bool is_radar)
{
	v3s16 p = v3s16(pos.X - size / 2, pos.Y, pos.Z - size / 2);

	for (s16 x = 0; x < size; x++)
	for (s16 z = 0; z < size; z++) {
		u16 id = CONTENT_AIR;
		MinimapPixel *mmpixel = &m_data->minimap_scan[x + z * size];

		if (!is_radar) {
			s16 pixel_height = 0;
			MinimapPixel *cached_pixel =
				getMinimapPixel(v3s16(p.X + x, p.Y, p.Z + z), height, &pixel_height);
			if (cached_pixel) {
				id = cached_pixel->id;
				mmpixel->height = pixel_height;
			}
		} else {
			mmpixel->air_count = getAirCount(v3s16(p.X + x, p.Y, p.Z + z), height);
		}

		mmpixel->id = id;
	}
}

////
//// Mapper
////

Mapper::Mapper(IrrlichtDevice *device, Client *client)
{
	this->client    = client;
	this->driver    = device->getVideoDriver();
	this->m_tsrc    = client->getTextureSource();
	this->m_shdrsrc = client->getShaderSource();
	this->m_ndef    = client->getNodeDefManager();

	m_angle = 0.f;

	// Initialize static settings
	m_enable_shaders = g_settings->getBool("enable_shaders");
	m_surface_mode_scan_height =
		g_settings->getBool("minimap_double_scan_height") ? 256 : 128;

	// Initialize minimap data
	m_data = new MinimapData;
	MutexAutoLock lock(m_data->mutex);
	m_data->map_invalidated   = true;
	m_data->texture           = NULL;
	m_data->heightmap_texture = NULL;
	m_data->minimap_image     = NULL;
	m_data->heightmap_image   = NULL;

	// TODO create default minimap mode machine
	m_machine = NULL;

	// Create mesh buffer for minimap
	m_meshbuffer = getMinimapMeshBuffer();

	// Initialize and start thread
	m_minimap_update_thread = new MinimapUpdateThread(m_data);
	m_minimap_update_thread->start();
}

Mapper::~Mapper()
{
	m_minimap_update_thread->stop();
	m_minimap_update_thread->wait();

	m_meshbuffer->drop();

	m_data->minimap_image->drop();
	m_data->heightmap_image->drop();

	driver->removeTexture(m_data->texture);
	driver->removeTexture(m_data->heightmap_texture);

	delete m_machine;
	delete m_data;
	delete m_minimap_update_thread;
}

void Mapper::addBlock(v3s16 pos, MinimapMapblock *data)
{
	m_minimap_update_thread->enqueueBlock(pos, data);
}

void Mapper::setModeMachine(ClientMinimapModeMachine *mach)
{
	MutexAutoLock lock(m_mutex);
	delete m_machine;
	m_machine->initMasks(driver, m_tsrc,
		core::dimension2d<u32>(MINIMAP_MAX_SX, MINIMAP_MAX_SY));
	m_machine = mach;
	miniMapModeChanged();
}

void Mapper::setMinimapDisabled(bool disabled)
{
	// TODO call setModeMachine with an "empty only" machine we create.
}

void Mapper::changeMode(bool shift_pressed, bool *flag_minimap_shown,
	std::string *status_text)
{
	MutexAutoLock lock(m_mutex);

	if (m_machine->modeChange(shift_pressed)) {
		miniMapModeChanged();
	}
	if (status_text) {
		*status_text = m_machine->getCurrentMode()->status_text;
	}
	if (flag_minimap_shown) {
		*flag_minimap_shown = m_machine->getCurrentMode()->is_minimap_shown;
	}

}

void Mapper::miniMapModeChanged()
{
	MinimapModeDef *m = m_machine->getCurrentMode();

	MutexAutoLock lock(m_data->mutex);
	// TODO update other stuff too...

	m_data->mode = m;
	m_data->minimap_mask         = m_machine->getCurrentModeMask();
	m_data->minimap_overlay      = m_tsrc->getTexture(m->overlay_texture_name);
	m_data->player_marker        = m_tsrc->getTexture(m->player_marker_name);
	m_data->other_players_marker = m_tsrc->getTexture(m->other_players_marker_name);

	m_minimap_update_thread->deferUpdate();
}

void Mapper::setPos(v3s16 pos)
{
	bool do_update = false;

	{
		MutexAutoLock lock(m_data->mutex);

		if (pos != m_data->old_pos) {
			m_data->old_pos = m_data->pos;
			m_data->pos = pos;
			do_update = true;
		}
	}

	if (do_update)
		m_minimap_update_thread->deferUpdate();
}

void Mapper::setAngle(f32 angle)
{
	m_angle = angle;
}

void Mapper::blitMinimapPixelsToImageRadar(video::IImage *map_image)
{
	u16 siz = m_data->mode->map_size;
	for (s16 x = 0; x < siz; x++)
	for (s16 z = 0; z < siz; z++) {
		MinimapPixel *mmpixel = &m_data->minimap_scan[x + z * siz];

		video::SColor c(240, 0, 0, 0);
		if (mmpixel->air_count > 0)
			c.setGreen(core::clamp(core::round32(32 + mmpixel->air_count * 8), 0, 255));

		map_image->setPixel(x, siz - z - 1, c);
	}
}

void Mapper::blitMinimapPixelsToImageSurface(
	video::IImage *map_image, video::IImage *heightmap_image)
{
	u16 siz = m_data->mode->map_size;
	for (s16 x = 0; x < siz; x++)
	for (s16 z = 0; z < siz; z++) {
		MinimapPixel *mmpixel = &m_data->minimap_scan[x + z * siz];

		video::SColor c = m_ndef->get(mmpixel->id).minimap_color;
		c.setAlpha(240);

		map_image->setPixel(x, siz - z - 1, c);

		u32 h = mmpixel->height;
		heightmap_image->setPixel(x, siz - z - 1,
			video::SColor(255, h, h, h));
	}
}

video::ITexture *Mapper::getMinimapTexture()
{

	// update minimap textures when new scan is ready
	if (m_data->map_invalidated)
		return m_data->texture;

	// create minimap and heightmap images in memory
	core::dimension2d<u32> dim(m_data->mode->map_size, m_data->mode->map_size);
	video::IImage *map_image       = driver->createImage(video::ECF_A8R8G8B8, dim);
	video::IImage *heightmap_image = driver->createImage(video::ECF_A8R8G8B8, dim);
	video::IImage *minimap_image   = driver->createImage(video::ECF_A8R8G8B8,
		core::dimension2d<u32>(MINIMAP_MAX_SX, MINIMAP_MAX_SY));

	// Blit MinimapPixels to images
	if (m_data->mode->is_radar)
		blitMinimapPixelsToImageRadar(map_image);
	else
		blitMinimapPixelsToImageSurface(map_image, heightmap_image);

	map_image->copyToScaling(minimap_image);
	map_image->drop();

	video::IImage *minimap_mask = m_data->minimap_mask;

	if (minimap_mask) {
		for (s16 y = 0; y < MINIMAP_MAX_SY; y++)
		for (s16 x = 0; x < MINIMAP_MAX_SX; x++) {
			video::SColor mask_col = minimap_mask->getPixel(x, y);
			if (!mask_col.getAlpha())
				minimap_image->setPixel(x, y, video::SColor(0,0,0,0));
		}
	}

	if (m_data->texture)
		driver->removeTexture(m_data->texture);
	if (m_data->heightmap_texture)
		driver->removeTexture(m_data->heightmap_texture);

	m_data->texture = driver->addTexture("minimap__", minimap_image);
	m_data->heightmap_texture =
		driver->addTexture("minimap_heightmap__", heightmap_image);
	minimap_image->drop();
	heightmap_image->drop();

	m_data->map_invalidated = true;

	return m_data->texture;
}

v3f Mapper::getYawVec()
{
	MutexAutoLock(m_data->mutex);
	if (m_data->mode->rotate_with_yaw) {
		return v3f(
			cos(m_angle * core::DEGTORAD),
			sin(m_angle * core::DEGTORAD),
			1.0);
	} else {
		return v3f(1.0, 0.0, 1.0);
	}
}

scene::SMeshBuffer *Mapper::getMinimapMeshBuffer()
{
	scene::SMeshBuffer *buf = new scene::SMeshBuffer();
	buf->Vertices.set_used(4);
	buf->Indices.set_used(6);
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
	video::ITexture *minimap_texture = getMinimapTexture();

	if (!minimap_texture)
		return;

	MinimapModeDef *mode = m_data->mode;

	updateActiveMarkers();
	v2u32 screensize = porting::getWindowSize();
	const u32 size = 0.25 * screensize.Y;

	core::rect<s32> oldViewPort = driver->getViewPort();
	core::matrix4 oldProjMat = driver->getTransform(video::ETS_PROJECTION);
	core::matrix4 oldViewMat = driver->getTransform(video::ETS_VIEW);

	driver->setViewPort(core::rect<s32>(
		screensize.X - size - 10, 10,
		screensize.X - 10, size + 10));
	driver->setTransform(video::ETS_PROJECTION, core::matrix4());
	driver->setTransform(video::ETS_VIEW, core::matrix4());

	core::matrix4 matrix;
	matrix.makeIdentity();

	video::SMaterial &material = m_meshbuffer->getMaterial();
	material.setFlag(video::EMF_TRILINEAR_FILTER, true);
	material.Lighting = false;
	material.TextureLayer[0].Texture = minimap_texture;
	material.TextureLayer[1].Texture = m_data->heightmap_texture;

	if (m_enable_shaders && !mode->is_radar) {
		u16 sid = m_shdrsrc->getShader("minimap_shader", 1, 1);
		material.MaterialType = m_shdrsrc->getShaderInfo(sid).material;
	} else {
		material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	}

	if (mode->rotate_with_yaw)
		matrix.setRotationDegrees(core::vector3df(0, 0, 360 - m_angle));

	// Draw minimap
	driver->setTransform(video::ETS_WORLD, matrix);
	driver->setMaterial(material);
	driver->drawMeshBuffer(m_meshbuffer);

	// Draw overlay
	video::ITexture *minimap_overlay = m_data->minimap_overlay;
	material.TextureLayer[0].Texture = minimap_overlay;
	material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	driver->setMaterial(material);
	driver->drawMeshBuffer(m_meshbuffer);

	// Draw player marker if requested
	if (mode->show_player_marker) {
		if (!mode->rotate_with_yaw)
			matrix.setRotationDegrees(core::vector3df(0, 0, m_angle));
		material.TextureLayer[0].Texture = m_data->player_marker;

		driver->setTransform(video::ETS_WORLD, matrix);
		driver->setMaterial(material);
		driver->drawMeshBuffer(m_meshbuffer);
	}

	// Reset transformations
	driver->setTransform(video::ETS_VIEW, oldViewMat);
	driver->setTransform(video::ETS_PROJECTION, oldProjMat);
	driver->setViewPort(oldViewPort);

	// Draw player markers
	if (mode->show_other_player_markers) {
		v2s32 s_pos(screensize.X - size - 10, 10);
		core::dimension2di imgsize(m_data->other_players_marker->getOriginalSize());
		core::rect<s32> img_rect(0, 0, imgsize.Width, imgsize.Height);
		static const video::SColor col(255, 255, 255, 255);
		static const video::SColor c[4] = {col, col, col, col};
		f32 sin_angle = sin(m_angle * core::DEGTORAD);
		f32 cos_angle = cos(m_angle * core::DEGTORAD);
		s32 marker_size2 =  0.025 * (float)size;
		for (std::list<v2f>::const_iterator
				i = m_active_markers.begin();
				i != m_active_markers.end(); ++i) {
			v2f posf = *i;
			if (m_data->mode->rotate_with_yaw) {
				f32 t1 = posf.X * cos_angle - posf.Y * sin_angle;
				f32 t2 = posf.X * sin_angle + posf.Y * cos_angle;
				posf.X = t1;
				posf.Y = t2;
			}
			posf.X = (posf.X + 0.5) * (float)size;
			posf.Y = (posf.Y + 0.5) * (float)size;
			core::rect<s32> dest_rect(
				s_pos.X + posf.X - marker_size2,
				s_pos.Y + posf.Y - marker_size2,
				s_pos.X + posf.X + marker_size2,
				s_pos.Y + posf.Y + marker_size2);
			driver->draw2DImage(m_data->other_players_marker, dest_rect,
				img_rect, &dest_rect, &c[0], true);
		}
	}
}

void Mapper::updateActiveMarkers()
{
	MutexAutoLock(m_data->mutex);
	u16 map_size = m_data->mode->map_size;
	u16 scan_height = m_data->mode->scan_height;
	video::IImage *minimap_mask = m_data->minimap_mask;

	std::list<Nametag *> *nametags = client->getCamera()->getNametags();

	m_active_markers.clear();

	for (std::list<Nametag *>::const_iterator
			i = nametags->begin();
			i != nametags->end(); ++i) {
		Nametag *nametag = *i;
		v3s16 pos = floatToInt(nametag->parent_node->getPosition() +
			intToFloat(client->getCamera()->getOffset(), BS), BS);
		pos -= m_data->pos - v3s16(map_size / 2,
				scan_height / 2,
				map_size / 2);
		if (pos.X < 0 || pos.X > map_size ||
				pos.Y < 0 || pos.Y > scan_height ||
				pos.Z < 0 || pos.Z > map_size) {
			continue;
		}
		pos.X = ((float)pos.X / map_size) * MINIMAP_MAX_SX;
		pos.Z = ((float)pos.Z / map_size) * MINIMAP_MAX_SY;
		video::SColor mask_col = minimap_mask->getPixel(pos.X, pos.Z);
		if (!mask_col.getAlpha()) {
			continue;
		}
		m_active_markers.push_back(v2f(((float)pos.X / (float)MINIMAP_MAX_SX) - 0.5,
			(1.0 - (float)pos.Z / (float)MINIMAP_MAX_SY) - 0.5));
	}
}

////
//// MinimapMapblock
////

void MinimapMapblock::getMinimapNodes(VoxelManipulator *vmanip, v3s16 pos)
{

	for (s16 x = 0; x < MAP_BLOCKSIZE; x++)
	for (s16 z = 0; z < MAP_BLOCKSIZE; z++) {
		s16 air_count = 0;
		bool surface_found = false;
		MinimapPixel *mmpixel = &data[z * MAP_BLOCKSIZE + x];

		for (s16 y = MAP_BLOCKSIZE -1; y >= 0; y--) {
			v3s16 p(x, y, z);
			MapNode n = vmanip->getNodeNoEx(pos + p);
			if (!surface_found && n.getContent() != CONTENT_AIR) {
				mmpixel->height = y;
				mmpixel->id = n.getContent();
				surface_found = true;
			} else if (n.getContent() == CONTENT_AIR) {
				air_count++;
			}
		}

		if (!surface_found)
			mmpixel->id = CONTENT_AIR;

		mmpixel->air_count = air_count;
	}
}

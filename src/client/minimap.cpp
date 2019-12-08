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
#include <cmath>
#include "client.h"
#include "clientmap.h"
#include "settings.h"
#include "shader.h"
#include "mapblock.h"
#include "client/renderingengine.h"


////
//// MinimapUpdateThread
////

MinimapUpdateThread::~MinimapUpdateThread()
{
	for (auto &it : m_blocks_cache) {
		delete it.second;
	}

	for (auto &q : m_update_queue) {
		delete q.data;
	}
}

bool MinimapUpdateThread::pushBlockUpdate(v3s16 pos, MinimapMapblock *data)
{
	MutexAutoLock lock(m_queue_mutex);

	// Find if block is already in queue.
	// If it is, update the data and quit.
	for (QueuedMinimapUpdate &q : m_update_queue) {
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
			if (!result.second) {
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

	if (data->map_invalidated && data->mode != MINIMAP_MODE_OFF) {
		getMap(data->pos, data->map_size, data->scan_height);
		data->map_invalidated = false;
	}
}

void MinimapUpdateThread::getMap(v3s16 pos, s16 size, s16 height)
{
	v3s16 pos_min(pos.X - size / 2, pos.Y - height / 2, pos.Z - size / 2);
	v3s16 pos_max(pos_min.X + size - 1, pos.Y + height / 2, pos_min.Z + size - 1);
	v3s16 blockpos_min = getNodeBlockPos(pos_min);
	v3s16 blockpos_max = getNodeBlockPos(pos_max);

// clear the map
	for (int z = 0; z < size; z++)
	for (int x = 0; x < size; x++) {
		MinimapPixel &mmpixel = data->minimap_scan[x + z * size];
		mmpixel.air_count = 0;
		mmpixel.height = 0;
		mmpixel.n = MapNode(CONTENT_AIR);
	}

// draw the map
	v3s16 blockpos;
	for (blockpos.Z = blockpos_min.Z; blockpos.Z <= blockpos_max.Z; ++blockpos.Z)
	for (blockpos.Y = blockpos_min.Y; blockpos.Y <= blockpos_max.Y; ++blockpos.Y)
	for (blockpos.X = blockpos_min.X; blockpos.X <= blockpos_max.X; ++blockpos.X) {
		std::map<v3s16, MinimapMapblock *>::const_iterator pblock =
			m_blocks_cache.find(blockpos);
		if (pblock == m_blocks_cache.end())
			continue;
		const MinimapMapblock &block = *pblock->second;

		v3s16 block_node_min(blockpos * MAP_BLOCKSIZE);
		v3s16 block_node_max(block_node_min + MAP_BLOCKSIZE - 1);
		// clip
		v3s16 range_min = componentwise_max(block_node_min, pos_min);
		v3s16 range_max = componentwise_min(block_node_max, pos_max);

		v3s16 pos;
		pos.Y = range_min.Y;
		for (pos.Z = range_min.Z; pos.Z <= range_max.Z; ++pos.Z)
		for (pos.X = range_min.X; pos.X <= range_max.X; ++pos.X) {
			v3s16 inblock_pos = pos - block_node_min;
			const MinimapPixel &in_pixel =
				block.data[inblock_pos.Z * MAP_BLOCKSIZE + inblock_pos.X];

			v3s16 inmap_pos = pos - pos_min;
			MinimapPixel &out_pixel =
				data->minimap_scan[inmap_pos.X + inmap_pos.Z * size];

			out_pixel.air_count += in_pixel.air_count;
			if (in_pixel.n.param0 != CONTENT_AIR) {
				out_pixel.n = in_pixel.n;
				out_pixel.height = inmap_pos.Y + in_pixel.height;
			}
		}
	}
}

////
//// Mapper
////

Minimap::Minimap(Client *client)
{
	this->client    = client;
	this->driver    = RenderingEngine::get_video_driver();
	this->m_tsrc    = client->getTextureSource();
	this->m_shdrsrc = client->getShaderSource();
	this->m_ndef    = client->getNodeDefManager();

	m_angle = 0.f;

	// Initialize static settings
	m_enable_shaders = g_settings->getBool("enable_shaders");
	m_surface_mode_scan_height =
		g_settings->getBool("minimap_double_scan_height") ? 256 : 128;

	// Initialize minimap data
	data = new MinimapData;
	data->mode              = MINIMAP_MODE_OFF;
	data->is_radar          = false;
	data->map_invalidated   = true;
	data->texture           = NULL;
	data->heightmap_texture = NULL;
	data->minimap_shape_round = g_settings->getBool("minimap_shape_round");

	// Get round minimap textures
	data->minimap_mask_round = driver->createImage(
		m_tsrc->getTexture("minimap_mask_round.png"),
		core::position2d<s32>(0, 0),
		core::dimension2d<u32>(MINIMAP_MAX_SX, MINIMAP_MAX_SY));
	data->minimap_overlay_round = m_tsrc->getTexture("minimap_overlay_round.png");

	// Get square minimap textures
	data->minimap_mask_square = driver->createImage(
		m_tsrc->getTexture("minimap_mask_square.png"),
		core::position2d<s32>(0, 0),
		core::dimension2d<u32>(MINIMAP_MAX_SX, MINIMAP_MAX_SY));
	data->minimap_overlay_square = m_tsrc->getTexture("minimap_overlay_square.png");

	// Create player marker texture
	data->player_marker = m_tsrc->getTexture("player_marker.png");
	// Create object marker texture
	data->object_marker_red = m_tsrc->getTexture("object_marker_red.png");

	// Create mesh buffer for minimap
	m_meshbuffer = getMinimapMeshBuffer();

	// Initialize and start thread
	m_minimap_update_thread = new MinimapUpdateThread();
	m_minimap_update_thread->data = data;
	m_minimap_update_thread->start();
}

Minimap::~Minimap()
{
	m_minimap_update_thread->stop();
	m_minimap_update_thread->wait();

	m_meshbuffer->drop();

	data->minimap_mask_round->drop();
	data->minimap_mask_square->drop();

	driver->removeTexture(data->texture);
	driver->removeTexture(data->heightmap_texture);
	driver->removeTexture(data->minimap_overlay_round);
	driver->removeTexture(data->minimap_overlay_square);
	driver->removeTexture(data->object_marker_red);

	delete data;
	delete m_minimap_update_thread;
}

void Minimap::addBlock(v3s16 pos, MinimapMapblock *data)
{
	m_minimap_update_thread->enqueueBlock(pos, data);
}

void Minimap::toggleMinimapShape()
{
	MutexAutoLock lock(m_mutex);

	data->minimap_shape_round = !data->minimap_shape_round;
	g_settings->setBool("minimap_shape_round", data->minimap_shape_round);
	m_minimap_update_thread->deferUpdate();
}

void Minimap::setMinimapShape(MinimapShape shape)
{
	MutexAutoLock lock(m_mutex);

	if (shape == MINIMAP_SHAPE_SQUARE)
		data->minimap_shape_round = false;
	else if (shape == MINIMAP_SHAPE_ROUND)
		data->minimap_shape_round = true;

	g_settings->setBool("minimap_shape_round", data->minimap_shape_round);
	m_minimap_update_thread->deferUpdate();
}

MinimapShape Minimap::getMinimapShape()
{
	if (data->minimap_shape_round) {
		return MINIMAP_SHAPE_ROUND;
	}

	return MINIMAP_SHAPE_SQUARE;
}

void Minimap::setMinimapMode(MinimapMode mode)
{
	static const MinimapModeDef modedefs[MINIMAP_MODE_COUNT] = {
		{false, 0, 0},
		{false, m_surface_mode_scan_height, 256},
		{false, m_surface_mode_scan_height, 128},
		{false, m_surface_mode_scan_height, 64},
		{true, 32, 128},
		{true, 32, 64},
		{true, 32, 32}
	};

	if (mode >= MINIMAP_MODE_COUNT)
		return;

	MutexAutoLock lock(m_mutex);

	data->is_radar    = modedefs[mode].is_radar;
	data->scan_height = modedefs[mode].scan_height;
	data->map_size    = modedefs[mode].map_size;
	data->mode        = mode;

	m_minimap_update_thread->deferUpdate();
}

void Minimap::setPos(v3s16 pos)
{
	bool do_update = false;

	{
		MutexAutoLock lock(m_mutex);

		if (pos != data->old_pos) {
			data->old_pos = data->pos;
			data->pos = pos;
			do_update = true;
		}
	}

	if (do_update)
		m_minimap_update_thread->deferUpdate();
}

void Minimap::setAngle(f32 angle)
{
	m_angle = angle;
}

void Minimap::blitMinimapPixelsToImageRadar(video::IImage *map_image)
{
	video::SColor c(240, 0, 0, 0);
	for (s16 x = 0; x < data->map_size; x++)
	for (s16 z = 0; z < data->map_size; z++) {
		MinimapPixel *mmpixel = &data->minimap_scan[x + z * data->map_size];

		if (mmpixel->air_count > 0)
			c.setGreen(core::clamp(core::round32(32 + mmpixel->air_count * 8), 0, 255));
		else
			c.setGreen(0);

		map_image->setPixel(x, data->map_size - z - 1, c);
	}
}

void Minimap::blitMinimapPixelsToImageSurface(
	video::IImage *map_image, video::IImage *heightmap_image)
{
	// This variable creation/destruction has a 1% cost on rendering minimap
	video::SColor tilecolor;
	for (s16 x = 0; x < data->map_size; x++)
	for (s16 z = 0; z < data->map_size; z++) {
		MinimapPixel *mmpixel = &data->minimap_scan[x + z * data->map_size];

		const ContentFeatures &f = m_ndef->get(mmpixel->n);
		const TileDef *tile = &f.tiledef[0];

		// Color of the 0th tile (mostly this is the topmost)
		if(tile->has_color)
			tilecolor = tile->color;
		else
			mmpixel->n.getColor(f, &tilecolor);

		tilecolor.setRed(tilecolor.getRed() * f.minimap_color.getRed() / 255);
		tilecolor.setGreen(tilecolor.getGreen() * f.minimap_color.getGreen() / 255);
		tilecolor.setBlue(tilecolor.getBlue() * f.minimap_color.getBlue() / 255);
		tilecolor.setAlpha(240);

		map_image->setPixel(x, data->map_size - z - 1, tilecolor);

		u32 h = mmpixel->height;
		heightmap_image->setPixel(x,data->map_size - z - 1,
			video::SColor(255, h, h, h));
	}
}

video::ITexture *Minimap::getMinimapTexture()
{
	// update minimap textures when new scan is ready
	if (data->map_invalidated)
		return data->texture;

	// create minimap and heightmap images in memory
	core::dimension2d<u32> dim(data->map_size, data->map_size);
	video::IImage *map_image       = driver->createImage(video::ECF_A8R8G8B8, dim);
	video::IImage *heightmap_image = driver->createImage(video::ECF_A8R8G8B8, dim);
	video::IImage *minimap_image   = driver->createImage(video::ECF_A8R8G8B8,
		core::dimension2d<u32>(MINIMAP_MAX_SX, MINIMAP_MAX_SY));

	// Blit MinimapPixels to images
	if (data->is_radar)
		blitMinimapPixelsToImageRadar(map_image);
	else
		blitMinimapPixelsToImageSurface(map_image, heightmap_image);

	map_image->copyToScaling(minimap_image);
	map_image->drop();

	video::IImage *minimap_mask = data->minimap_shape_round ?
		data->minimap_mask_round : data->minimap_mask_square;

	if (minimap_mask) {
		for (s16 y = 0; y < MINIMAP_MAX_SY; y++)
		for (s16 x = 0; x < MINIMAP_MAX_SX; x++) {
			const video::SColor &mask_col = minimap_mask->getPixel(x, y);
			if (!mask_col.getAlpha())
				minimap_image->setPixel(x, y, video::SColor(0,0,0,0));
		}
	}

	if (data->texture)
		driver->removeTexture(data->texture);
	if (data->heightmap_texture)
		driver->removeTexture(data->heightmap_texture);

	data->texture = driver->addTexture("minimap__", minimap_image);
	data->heightmap_texture =
		driver->addTexture("minimap_heightmap__", heightmap_image);
	minimap_image->drop();
	heightmap_image->drop();

	data->map_invalidated = true;

	return data->texture;
}

v3f Minimap::getYawVec()
{
	if (data->minimap_shape_round) {
		return v3f(
			std::cos(m_angle * core::DEGTORAD),
			std::sin(m_angle * core::DEGTORAD),
			1.0);
	}

	return v3f(1.0, 0.0, 1.0);
}

scene::SMeshBuffer *Minimap::getMinimapMeshBuffer()
{
	scene::SMeshBuffer *buf = new scene::SMeshBuffer();
	buf->Vertices.set_used(4);
	buf->Indices.set_used(6);
	static const video::SColor c(255, 255, 255, 255);

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

void Minimap::drawMinimap()
{
	video::ITexture *minimap_texture = getMinimapTexture();
	if (!minimap_texture)
		return;

	updateActiveMarkers();
	v2u32 screensize = RenderingEngine::get_instance()->getWindowSize();
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
	material.TextureLayer[1].Texture = data->heightmap_texture;

	if (m_enable_shaders && !data->is_radar) {
		u16 sid = m_shdrsrc->getShader("minimap_shader", 1, 1);
		material.MaterialType = m_shdrsrc->getShaderInfo(sid).material;
	} else {
		material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	}

	if (data->minimap_shape_round)
		matrix.setRotationDegrees(core::vector3df(0, 0, 360 - m_angle));

	// Draw minimap
	driver->setTransform(video::ETS_WORLD, matrix);
	driver->setMaterial(material);
	driver->drawMeshBuffer(m_meshbuffer);

	// Draw overlay
	video::ITexture *minimap_overlay = data->minimap_shape_round ?
		data->minimap_overlay_round : data->minimap_overlay_square;
	material.TextureLayer[0].Texture = minimap_overlay;
	material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	driver->setMaterial(material);
	driver->drawMeshBuffer(m_meshbuffer);

	// Draw player marker on minimap
	if (data->minimap_shape_round) {
		matrix.setRotationDegrees(core::vector3df(0, 0, 0));
	} else {
		matrix.setRotationDegrees(core::vector3df(0, 0, m_angle));
	}

	material.TextureLayer[0].Texture = data->player_marker;
	driver->setTransform(video::ETS_WORLD, matrix);
	driver->setMaterial(material);
	driver->drawMeshBuffer(m_meshbuffer);

	// Reset transformations
	driver->setTransform(video::ETS_VIEW, oldViewMat);
	driver->setTransform(video::ETS_PROJECTION, oldProjMat);
	driver->setViewPort(oldViewPort);

	// Draw player markers
	v2s32 s_pos(screensize.X - size - 10, 10);
	core::dimension2di imgsize(data->object_marker_red->getOriginalSize());
	core::rect<s32> img_rect(0, 0, imgsize.Width, imgsize.Height);
	static const video::SColor col(255, 255, 255, 255);
	static const video::SColor c[4] = {col, col, col, col};
	f32 sin_angle = std::sin(m_angle * core::DEGTORAD);
	f32 cos_angle = std::cos(m_angle * core::DEGTORAD);
	s32 marker_size2 =  0.025 * (float)size;
	for (std::list<v2f>::const_iterator
			i = m_active_markers.begin();
			i != m_active_markers.end(); ++i) {
		v2f posf = *i;
		if (data->minimap_shape_round) {
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
		driver->draw2DImage(data->object_marker_red, dest_rect,
			img_rect, &dest_rect, &c[0], true);
	}
}

void Minimap::updateActiveMarkers()
{
	video::IImage *minimap_mask = data->minimap_shape_round ?
		data->minimap_mask_round : data->minimap_mask_square;

	const std::list<Nametag *> &nametags = client->getCamera()->getNametags();

	m_active_markers.clear();

	for (Nametag *nametag : nametags) {
		v3s16 pos = floatToInt(nametag->parent_node->getAbsolutePosition() +
			intToFloat(client->getCamera()->getOffset(), BS), BS);
		pos -= data->pos - v3s16(data->map_size / 2,
				data->scan_height / 2,
				data->map_size / 2);
		if (pos.X < 0 || pos.X > data->map_size ||
				pos.Y < 0 || pos.Y > data->scan_height ||
				pos.Z < 0 || pos.Z > data->map_size) {
			continue;
		}
		pos.X = ((float)pos.X / data->map_size) * MINIMAP_MAX_SX;
		pos.Z = ((float)pos.Z / data->map_size) * MINIMAP_MAX_SY;
		const video::SColor &mask_col = minimap_mask->getPixel(pos.X, pos.Z);
		if (!mask_col.getAlpha()) {
			continue;
		}

		m_active_markers.emplace_back(((float)pos.X / (float)MINIMAP_MAX_SX) - 0.5,
			(1.0 - (float)pos.Z / (float)MINIMAP_MAX_SY) - 0.5);
	}
}

////
//// MinimapMapblock
////

void MinimapMapblock::getMinimapNodes(VoxelManipulator *vmanip, const v3s16 &pos)
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
				mmpixel->n = n;
				surface_found = true;
			} else if (n.getContent() == CONTENT_AIR) {
				air_count++;
			}
		}

		if (!surface_found)
			mmpixel->n = MapNode(CONTENT_AIR);

		mmpixel->air_count = air_count;
	}
}

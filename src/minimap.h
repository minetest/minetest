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

#ifndef MINIMAP_HEADER
#define MINIMAP_HEADER

#include "irrlichttypes_extrabloated.h"
#include "client.h"
#include "voxel.h"
#include "jthread/jmutex.h"
#include "jthread/jsemaphore.h"
#include <map>
#include <string>
#include <vector>

enum MinimapMode {
	MINIMAP_MODE_OFF,
	MINIMAP_MODE_SURFACEx1,
	MINIMAP_MODE_SURFACEx2,
	MINIMAP_MODE_SURFACEx4,
	MINIMAP_MODE_RADARx1,
	MINIMAP_MODE_RADARx2,
	MINIMAP_MODE_RADARx4
};

struct MinimapPixel
{
	u16 id;
	u16 height;
	u16 air_count;
	u16 light;
};

struct MinimapMapblock
{
	MinimapPixel data[MAP_BLOCKSIZE * MAP_BLOCKSIZE];
};

struct MinimapData
{
	bool radar;
	MinimapMode mode;
	v3s16 pos;
	v3s16 old_pos;
	u16 scan_height;
	u16 map_size;
	MinimapPixel minimap_scan[512 * 512];
	bool map_invalidated;
	bool minimap_shape_round;
	video::IImage *minimap_image;
	video::IImage *heightmap_image;
	video::IImage *minimap_mask_round;
	video::IImage *minimap_mask_square;
	video::ITexture *texture;
	video::ITexture *heightmap_texture;
	video::ITexture *minimap_overlay_round;
	video::ITexture *minimap_overlay_square;
	video::ITexture *player_marker;
};

struct QueuedMinimapUpdate
{
	v3s16 pos;
	MinimapMapblock *data;

	QueuedMinimapUpdate();
	~QueuedMinimapUpdate();
};

/*
	A thread-safe queue of minimap mapblocks update tasks
*/

class MinimapUpdateQueue
{
public:
	MinimapUpdateQueue();

	~MinimapUpdateQueue();

	bool addBlock(v3s16 pos, MinimapMapblock *data);

	// blocking!!
	QueuedMinimapUpdate *pop();

	u32 size()
	{
		JMutexAutoLock lock(m_mutex);
		return m_queue.size();
	}

private:
	std::list<QueuedMinimapUpdate*> m_queue;
	JMutex m_mutex;
};

class MinimapUpdateThread : public JThread
{
private:
	JSemaphore m_queue_sem;
	MinimapUpdateQueue m_queue;

public:
	MinimapUpdateThread(IrrlichtDevice *device, Client *client)
	{
		this->device = device;
		this->client = client;
		this->driver = device->getVideoDriver();
		this->tsrc = client->getTextureSource();
	}
	~MinimapUpdateThread();
	void getMap (v3s16 pos, s16 size, s16 height, bool radar);
	MinimapPixel *getMinimapPixel (v3s16 pos, s16 height, s16 &pixel_height);
	s16 getAirCount (v3s16 pos, s16 height);
	video::SColor getColorFromId(u16 id);

	void enqueue_Block(v3s16 pos, MinimapMapblock *data);

	IrrlichtDevice *device;
	Client *client;
	video::IVideoDriver *driver;
	ITextureSource *tsrc;
	void Stop();
	void *Thread();
	MinimapData *data;
	std::map<v3s16, MinimapMapblock *> m_blocks_cache;
};

class Mapper
{
private:
	MinimapUpdateThread *m_minimap_update_thread;
	video::ITexture *minimap_texture;
	scene::SMeshBuffer *m_meshbuffer;
	bool m_enable_shaders;
	u16 m_surface_mode_scan_height;
	JMutex m_mutex;

public:
	Mapper(IrrlichtDevice *device, Client *client);
	~Mapper();

	void addBlock(v3s16 pos, MinimapMapblock *data);
	void setPos(v3s16 pos);
	video::ITexture* getMinimapTexture();
	v3f getYawVec();
	MinimapMode getMinimapMode();
	void setMinimapMode(MinimapMode mode);
	void toggleMinimapShape();
	scene::SMeshBuffer *getMinimapMeshBuffer();
	void drawMinimap();
	IrrlichtDevice *device;
	Client *client;
	video::IVideoDriver *driver;
	LocalPlayer *player;
	ITextureSource *tsrc;
	IShaderSource *shdrsrc;
	MinimapData *data;
};

#endif

/*
Minetest
Copyright (C) 2013, 2017 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#pragma once

#include <ctime>
#include <mutex>
#include "mapblock_mesh.h"
#include "threading/mutex_auto_lock.h"
#include "util/thread.h"

struct CachedMapBlockData
{
	v3s16 p = v3s16(-1337, -1337, -1337);
	MapNode *data = nullptr; // A copy of the MapBlock's data member
	int refcount_from_queue = 0;
	std::time_t last_used_timestamp = std::time(0);

	CachedMapBlockData() = default;
	~CachedMapBlockData();
};

struct QueuedMeshUpdate
{
	v3s16 p = v3s16(-1337, -1337, -1337);
	bool ack_block_to_server = false;
	int crack_level = -1;
	v3s16 crack_pos;
	MeshMakeData *data = nullptr; // This is generated in MeshUpdateQueue::pop()

	QueuedMeshUpdate() = default;
	~QueuedMeshUpdate();
};

/*
	A thread-safe queue of mesh update tasks and a cache of MapBlock data
*/
class MeshUpdateQueue
{
	enum UpdateMode
	{
		FORCE_UPDATE,
		SKIP_UPDATE_IF_ALREADY_CACHED,
	};

public:
	MeshUpdateQueue(Client *client);

	~MeshUpdateQueue();

	// Caches the block at p and its neighbors (if needed) and queues a mesh
	// update for the block at p
	bool addBlock(Map *map, v3s16 p, bool ack_block_to_server, bool urgent);

	// Returned pointer must be deleted
	// Returns NULL if queue is empty
	QueuedMeshUpdate *pop();

	u32 size()
	{
		MutexAutoLock lock(m_mutex);
		return m_queue.size();
	}

private:
	Client *m_client;
	std::vector<QueuedMeshUpdate *> m_queue;
	std::set<v3s16> m_urgents;
	std::map<v3s16, CachedMapBlockData *> m_cache;
	std::mutex m_mutex;

	// TODO: Add callback to update these when g_settings changes
	bool m_cache_enable_shaders;
	bool m_cache_smooth_lighting;
	int m_meshgen_block_cache_size;

	CachedMapBlockData *cacheBlock(Map *map, v3s16 p, UpdateMode mode,
			size_t *cache_hit_counter = NULL);
	CachedMapBlockData *getCachedBlock(const v3s16 &p);
	void fillDataFromMapBlockCache(QueuedMeshUpdate *q);
	void cleanupCache();
};

struct MeshUpdateResult
{
	v3s16 p = v3s16(-1338, -1338, -1338);
	MapBlockMesh *mesh = nullptr;
	bool ack_block_to_server = false;

	MeshUpdateResult() = default;
};

class MeshUpdateThread : public UpdateThread
{
public:
	MeshUpdateThread(Client *client);

	// Caches the block at p and its neighbors (if needed) and queues a mesh
	// update for the block at p
	void updateBlock(Map *map, v3s16 p, bool ack_block_to_server, bool urgent,
			bool update_neighbors = false);

	v3s16 m_camera_offset;
	MutexedQueue<MeshUpdateResult> m_queue_out;

private:
	MeshUpdateQueue m_queue_in;

	// TODO: Add callback to update these when g_settings changes
	int m_generation_interval;

protected:
	virtual void doUpdate();
};

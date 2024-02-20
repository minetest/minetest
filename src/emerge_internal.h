/*
Minetest
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

/******************************************************************/
/* may only be included by emerge.cpp or emerge scripting related */
/******************************************************************/

#include "emerge.h"

#include <queue>

#include "util/thread.h"
#include "threading/event.h"

class Server;
class ServerMap;
class Mapgen;

class EmergeManager;
class EmergeScripting;

class EmergeThread : public Thread {
public:
	bool enable_mapgen_debug_info;
	int id;

	EmergeThread(Server *server, int ethreadid);
	~EmergeThread() = default;

	void *run();
	void signal();

	// Requires queue mutex held
	bool pushBlock(const v3s16 &pos);

	void cancelPendingItems();

	EmergeManager *getEmergeManager() { return m_emerge; }
	Mapgen *getMapgen() { return m_mapgen; }

protected:

	void runCompletionCallbacks(
		const v3s16 &pos, EmergeAction action,
		const EmergeCallbackList &callbacks);

private:
	Server *m_server;
	ServerMap *m_map;
	EmergeManager *m_emerge;
	Mapgen *m_mapgen;

	std::unique_ptr<EmergeScripting> m_script;
	// read from scripting:
	UniqueQueue<v3s16> *m_trans_liquid; //< non-null only when generating a mapblock

	Event m_queue_event;
	std::queue<v3s16> m_block_queue;

	bool initScripting();

	bool popBlockEmerge(v3s16 *pos, BlockEmergeData *bedata);

	EmergeAction getBlockOrStartGen(
		const v3s16 &pos, bool allow_gen, MapBlock **block, BlockMakeData *data);
	MapBlock *finishGen(v3s16 pos, BlockMakeData *bmdata,
		std::map<v3s16, MapBlock *> *modified_blocks);

	friend class EmergeManager;
	friend class EmergeScripting;
	friend class ModApiMapgen;
};

// Scoped helper to set Server::m_ignore_map_edit_events_area
class MapEditEventAreaIgnorer
{
public:
	MapEditEventAreaIgnorer(VoxelArea *ignorevariable, const VoxelArea &a):
		m_ignorevariable(ignorevariable)
	{
		if (m_ignorevariable->getVolume() == 0)
			*m_ignorevariable = a;
		else
			m_ignorevariable = nullptr;
	}

	~MapEditEventAreaIgnorer()
	{
		if (m_ignorevariable) {
			assert(m_ignorevariable->getVolume() != 0);
			*m_ignorevariable = VoxelArea();
		}
	}

private:
	VoxelArea *m_ignorevariable;
};

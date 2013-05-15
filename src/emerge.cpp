/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
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


#include "server.h"
#include <iostream>
#include <queue>
#include "clientserver.h"
#include "map.h"
#include "jmutexautolock.h"
#include "main.h"
#include "constants.h"
#include "voxel.h"
#include "config.h"
#include "mapblock.h"
#include "serverobject.h"
#include "settings.h"
#include "cpp_api/scriptapi.h"
#include "profiler.h"
#include "log.h"
#include "nodedef.h"
#include "biome.h"
#include "emerge.h"
#include "mapgen_v6.h"
#include "mapgen_v7.h"
#include "mapgen_indev.h"
#include "mapgen_singlenode.h"


/////////////////////////////// Emerge Manager ////////////////////////////////

EmergeManager::EmergeManager(IGameDef *gamedef) {
	//register built-in mapgens
	registerMapgen("v6", new MapgenFactoryV6());
	registerMapgen("v7", new MapgenFactoryV7());
	registerMapgen("indev", new MapgenFactoryIndev());
	registerMapgen("singlenode", new MapgenFactorySinglenode());

	this->ndef     = gamedef->getNodeDefManager();
	this->biomedef = new BiomeDefManager();
	this->params   = NULL;
	
	mapgen_debug_info = g_settings->getBool("enable_mapgen_debug_info");

	queuemutex.Init();
	
	int nthreads;
	if (g_settings->get("num_emerge_threads").empty()) {
		int nprocs = porting::getNumberOfProcessors();
		// leave a proc for the main thread and one for some other misc threads
		nthreads = (nprocs > 2) ? nprocs - 2 : 1;
	} else {
		nthreads = g_settings->getU16("num_emerge_threads");
	}
	if (nthreads < 1)
		nthreads = 1;
	
	qlimit_total    = g_settings->getU16("emergequeue_limit_total");
	qlimit_diskonly = g_settings->get("emergequeue_limit_diskonly").empty() ?
		nthreads * 5 + 1 :
		g_settings->getU16("emergequeue_limit_diskonly");
	qlimit_generate = g_settings->get("emergequeue_limit_generate").empty() ?
		nthreads + 1 :
		g_settings->getU16("emergequeue_limit_generate");
	
	for (int i = 0; i != nthreads; i++)
		emergethread.push_back(new EmergeThread((Server *)gamedef, i));
		
	infostream << "EmergeManager: using " << nthreads << " threads" << std::endl;
}


EmergeManager::~EmergeManager() {
	for (unsigned int i = 0; i != emergethread.size(); i++) {
		emergethread[i]->setRun(false);
		emergethread[i]->qevent.signal();
		emergethread[i]->stop();
		delete emergethread[i];
		delete mapgen[i];
	}
	emergethread.clear();
	mapgen.clear();

	for (unsigned int i = 0; i < ores.size(); i++)
		delete ores[i];
	ores.clear();
	
	for (std::map<std::string, MapgenFactory *>::iterator iter = mglist.begin();
			iter != mglist.end(); iter ++) {
		delete iter->second;
	}
	mglist.clear();

	delete biomedef;
}


void EmergeManager::initMapgens(MapgenParams *mgparams) {
	Mapgen *mg;
	
	if (mapgen.size())
		return;
	
	biomedef->resolveNodeNames(ndef);
	
	this->params = mgparams;
	for (unsigned int i = 0; i != emergethread.size(); i++) {
		mg = createMapgen(params->mg_name, 0, params);
		if (!mg) {
			infostream << "EmergeManager: falling back to mapgen v6" << std::endl;
			delete params;
			params = createMapgenParams("v6");
			mg = createMapgen("v6", 0, params);
		}
		mapgen.push_back(mg);
	}
}


bool EmergeManager::enqueueBlockEmerge(u16 peer_id, v3s16 p, bool allow_generate) {
	std::map<v3s16, BlockEmergeData *>::const_iterator iter;
	BlockEmergeData *bedata;
	u16 count;
	u8 flags = 0;
	int idx = 0;
	
	if (allow_generate)
		flags |= BLOCK_EMERGE_ALLOWGEN;

	{
		JMutexAutoLock queuelock(queuemutex);
		
		count = blocks_enqueued.size();
		if (count >= qlimit_total)
			return false;

		count = peer_queue_count[peer_id];
		u16 qlimit_peer = allow_generate ? qlimit_generate : qlimit_diskonly;
		if (count >= qlimit_peer)
			return false;
		
		iter = blocks_enqueued.find(p);
		if (iter != blocks_enqueued.end()) {
			bedata = iter->second;
			bedata->flags |= flags;
			return true;
		}

		bedata = new BlockEmergeData;
		bedata->flags = flags;
		bedata->peer_requested = peer_id;
		blocks_enqueued.insert(std::make_pair(p, bedata));
		
		peer_queue_count[peer_id] = count + 1;
		
		// insert into the EmergeThread queue with the least items
		int lowestitems = emergethread[0]->blockqueue.size();
		for (unsigned int i = 1; i != emergethread.size(); i++) {
			int nitems = emergethread[i]->blockqueue.size();
			if (nitems < lowestitems) {
				idx = i;
				lowestitems = nitems;
			}
		}
		
		emergethread[idx]->blockqueue.push(p);
	}
	emergethread[idx]->qevent.signal();
	
	return true;
}


int EmergeManager::getGroundLevelAtPoint(v2s16 p) {
	if (mapgen.size() == 0 || !mapgen[0]) {
		errorstream << "EmergeManager: getGroundLevelAtPoint() called"
		" before mapgen initialized" << std::endl;
		return 0;
	}
	
	return mapgen[0]->getGroundLevelAtPoint(p);
}


bool EmergeManager::isBlockUnderground(v3s16 blockpos) {
	/*
	v2s16 p = v2s16((blockpos.X * MAP_BLOCKSIZE) + MAP_BLOCKSIZE / 2,
					(blockpos.Y * MAP_BLOCKSIZE) + MAP_BLOCKSIZE / 2);
	int ground_level = getGroundLevelAtPoint(p);
	return blockpos.Y * (MAP_BLOCKSIZE + 1) <= min(water_level, ground_level);
	*/

	//yuck, but then again, should i bother being accurate?
	//the height of the nodes in a single block is quite variable
	return blockpos.Y * (MAP_BLOCKSIZE + 1) <= params->water_level;
}


u32 EmergeManager::getBlockSeed(v3s16 p) {
	return (u32)(params->seed & 0xFFFFFFFF) +
		p.Z * 38134234 +
		p.Y * 42123 +
		p.X * 23;
}


Mapgen *EmergeManager::createMapgen(std::string mgname, int mgid,
									 MapgenParams *mgparams) {
	std::map<std::string, MapgenFactory *>::const_iterator iter;
	iter = mglist.find(mgname);
	if (iter == mglist.end()) {
		errorstream << "EmergeManager; mapgen " << mgname <<
		 " not registered" << std::endl;
		return NULL;
	}
	
	MapgenFactory *mgfactory = iter->second;
	return mgfactory->createMapgen(mgid, mgparams, this);
}


MapgenParams *EmergeManager::createMapgenParams(std::string mgname) {
	std::map<std::string, MapgenFactory *>::const_iterator iter;
	iter = mglist.find(mgname);
	if (iter == mglist.end()) {
		errorstream << "EmergeManager: mapgen " << mgname <<
		 " not registered" << std::endl;
		return NULL;
	}
	
	MapgenFactory *mgfactory = iter->second;
	return mgfactory->createMapgenParams();
}


MapgenParams *EmergeManager::getParamsFromSettings(Settings *settings) {
	std::string mg_name = settings->get("mg_name");
	MapgenParams *mgparams = createMapgenParams(mg_name);
	if (!mgparams)
		return NULL;
	
	mgparams->mg_name     = mg_name;
	mgparams->seed        = settings->getU64(settings == g_settings ? "fixed_map_seed" : "seed");
	mgparams->water_level = settings->getS16("water_level");
	mgparams->chunksize   = settings->getS16("chunksize");
	mgparams->flags       = settings->getFlagStr("mg_flags", flagdesc_mapgen);

	if (!mgparams->readParams(settings)) {
		delete mgparams;
		return NULL;
	}
	return mgparams;
}


void EmergeManager::setParamsToSettings(Settings *settings) {
	settings->set("mg_name",         params->mg_name);
	settings->setU64("seed",         params->seed);
	settings->setS16("water_level",  params->water_level);
	settings->setS16("chunksize",    params->chunksize);
	settings->setFlagStr("mg_flags", params->flags, flagdesc_mapgen);

	params->writeParams(settings);
}


void EmergeManager::registerMapgen(std::string mgname, MapgenFactory *mgfactory) {
	mglist.insert(std::make_pair(mgname, mgfactory));
	infostream << "EmergeManager: registered mapgen " << mgname << std::endl;
}


////////////////////////////// Emerge Thread ////////////////////////////////// 

bool EmergeThread::popBlockEmerge(v3s16 *pos, u8 *flags) {
	std::map<v3s16, BlockEmergeData *>::iterator iter;
	JMutexAutoLock queuelock(emerge->queuemutex);

	if (blockqueue.empty())
		return false;
	v3s16 p = blockqueue.front();
	blockqueue.pop();
	
	*pos = p;
	
	iter = emerge->blocks_enqueued.find(p);
	if (iter == emerge->blocks_enqueued.end()) 
		return false; //uh oh, queue and map out of sync!!

	BlockEmergeData *bedata = iter->second;
	*flags = bedata->flags;
	
	emerge->peer_queue_count[bedata->peer_requested]--;

	delete bedata;
	emerge->blocks_enqueued.erase(iter);
	
	return true;
}


bool EmergeThread::getBlockOrStartGen(v3s16 p, MapBlock **b, 
									BlockMakeData *data, bool allow_gen) {
	v2s16 p2d(p.X, p.Z);
	//envlock: usually takes <=1ms, sometimes 90ms or ~400ms to acquire
	JMutexAutoLock envlock(m_server->m_env_mutex); 
	
	// Load sector if it isn't loaded
	if (map->getSectorNoGenerateNoEx(p2d) == NULL)
		map->loadSectorMeta(p2d);

	// Attempt to load block
	MapBlock *block = map->getBlockNoCreateNoEx(p);
	if (!block || block->isDummy() || !block->isGenerated()) {
		EMERGE_DBG_OUT("not in memory, attempting to load from disk");
		block = map->loadBlock(p);
	}

	// If could not load and allowed to generate,
	// start generation inside this same envlock
	if (allow_gen && (block == NULL || !block->isGenerated())) {
		EMERGE_DBG_OUT("generating");
		*b = block;
		return map->initBlockMake(data, p);
	}
	
	*b = block;
	return false;
}


void *EmergeThread::Thread() {
	ThreadStarted();
	log_register_thread("EmergeThread" + id);
	DSTACK(__FUNCTION_NAME);
	BEGIN_DEBUG_EXCEPTION_HANDLER

	v3s16 last_tried_pos(-32768,-32768,-32768); // For error output
	v3s16 p;
	u8 flags;
	
	map    = (ServerMap *)&(m_server->m_env->getMap());
	emerge = m_server->m_emerge;
	mapgen = emerge->mapgen[id];
	enable_mapgen_debug_info = emerge->mapgen_debug_info;
	
	while (getRun())
	try {
		if (!popBlockEmerge(&p, &flags)) {
			qevent.wait();
			continue;
		}

		last_tried_pos = p;
		if (blockpos_over_limit(p))
			continue;

		bool allow_generate = flags & BLOCK_EMERGE_ALLOWGEN;
		EMERGE_DBG_OUT("p=" PP(p) " allow_generate=" << allow_generate);
		
		/*
			Try to fetch block from memory or disk.
			If not found and asked to generate, initialize generator.
		*/
		BlockMakeData data;
		MapBlock *block = NULL;
		std::map<v3s16, MapBlock *> modified_blocks;
		
		if (getBlockOrStartGen(p, &block, &data, allow_generate)) {
			{
				ScopeProfiler sp(g_profiler, "EmergeThread: Mapgen::makeChunk", SPT_AVG);
				TimeTaker t("mapgen::make_block()");

				mapgen->makeChunk(&data);

				if (enable_mapgen_debug_info == false)
					t.stop(true); // Hide output
			}

			{
				//envlock: usually 0ms, but can take either 30 or 400ms to acquire
				JMutexAutoLock envlock(m_server->m_env_mutex); 
				ScopeProfiler sp(g_profiler, "EmergeThread: after "
						"Mapgen::makeChunk (envlock)", SPT_AVG);

				map->finishBlockMake(&data, modified_blocks);
				
				block = map->getBlockNoCreateNoEx(p);
				if (block) {
					/*
						Do some post-generate stuff
					*/
					v3s16 minp = data.blockpos_min * MAP_BLOCKSIZE;
					v3s16 maxp = data.blockpos_max * MAP_BLOCKSIZE +
								 v3s16(1,1,1) * (MAP_BLOCKSIZE - 1);

					// Ignore map edit events, they will not need to be sent
					// to anybody because the block hasn't been sent to anybody
					MapEditEventAreaIgnorer 
						ign(&m_server->m_ignore_map_edit_events_area,
						VoxelArea(minp, maxp));
					{  // takes about 90ms with -O1 on an e3-1230v2
						SERVER_TO_SA(m_server)->environment_OnGenerated(
								minp, maxp, emerge->getBlockSeed(minp));
					}

					EMERGE_DBG_OUT("ended up with: " << analyze_block(block));
					
					m_server->m_env->activateBlock(block, 0);
				}
			}
		}

		/*
			Set sent status of modified blocks on clients
		*/

		// NOTE: Server's clients are also behind the connection mutex
		//conlock: consistently takes 30-40ms to acquire
		JMutexAutoLock lock(m_server->m_con_mutex);
		// Add the originally fetched block to the modified list
		if (block)
			modified_blocks[p] = block;

		// Set the modified blocks unsent for all the clients
		for (std::map<u16, RemoteClient*>::iterator
			 i = m_server->m_clients.begin();
			 i != m_server->m_clients.end(); ++i) {
			RemoteClient *client = i->second;
			if (modified_blocks.size() > 0) {
				// Remove block from sent history
				client->SetBlocksNotSent(modified_blocks);
			}
		}
	}
	catch (VersionMismatchException &e) {
		std::ostringstream err;
		err << "World data version mismatch in MapBlock "<<PP(last_tried_pos)<<std::endl;
		err << "----"<<std::endl;
		err << "\""<<e.what()<<"\""<<std::endl;
		err << "See debug.txt."<<std::endl;
		err << "World probably saved by a newer version of Minetest."<<std::endl;
		m_server->setAsyncFatalError(err.str());
	}
	catch (SerializationError &e) {
		std::ostringstream err;
		err << "Invalid data in MapBlock "<<PP(last_tried_pos)<<std::endl;
		err << "----"<<std::endl;
		err << "\""<<e.what()<<"\""<<std::endl;
		err << "See debug.txt."<<std::endl;
		err << "You can ignore this using [ignore_world_load_errors = true]."<<std::endl;
		m_server->setAsyncFatalError(err.str());
	}
	
	END_DEBUG_EXCEPTION_HANDLER(errorstream)
	log_deregister_thread();
	return NULL;
}

/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <iostream>
#include <algorithm>
#include <sstream>
#include <IFileSystem.h>
#include "jthread/jmutexautolock.h"
#include "util/directiontables.h"
#include "util/pointedthing.h"
#include "util/serialize.h"
#include "util/string.h"
#include "strfnd.h"
#include "client.h"
#include "clientserver.h"
#include "main.h"
#include "filesys.h"
#include "porting.h"
#include "mapsector.h"
#include "mapblock_mesh.h"
#include "mapblock.h"
#include "settings.h"
#include "profiler.h"
#include "gettext.h"
#include "log.h"
#include "nodemetadata.h"
#include "nodedef.h"
#include "itemdef.h"
#include "shader.h"
#include "base64.h"
#include "clientmap.h"
#include "clientmedia.h"
#include "sound.h"
#include "IMeshCache.h"
#include "serialization.h"
#include "config.h"
#include "version.h"
#include "drawscene.h"

extern gui::IGUIEnvironment* guienv;

/*
	QueuedMeshUpdate
*/

QueuedMeshUpdate::QueuedMeshUpdate():
	p(-1337,-1337,-1337),
	data(NULL),
	ack_block_to_server(false)
{
}

QueuedMeshUpdate::~QueuedMeshUpdate()
{
	if(data)
		delete data;
}

/*
	MeshUpdateQueue
*/
	
MeshUpdateQueue::MeshUpdateQueue()
{
}

MeshUpdateQueue::~MeshUpdateQueue()
{
	JMutexAutoLock lock(m_mutex);

	for(std::vector<QueuedMeshUpdate*>::iterator
			i = m_queue.begin();
			i != m_queue.end(); i++)
	{
		QueuedMeshUpdate *q = *i;
		delete q;
	}
}

/*
	peer_id=0 adds with nobody to send to
*/
void MeshUpdateQueue::addBlock(v3s16 p, MeshMakeData *data, bool ack_block_to_server, bool urgent)
{
	DSTACK(__FUNCTION_NAME);

	assert(data);

	JMutexAutoLock lock(m_mutex);

	if(urgent)
		m_urgents.insert(p);

	/*
		Find if block is already in queue.
		If it is, update the data and quit.
	*/
	for(std::vector<QueuedMeshUpdate*>::iterator
			i = m_queue.begin();
			i != m_queue.end(); i++)
	{
		QueuedMeshUpdate *q = *i;
		if(q->p == p)
		{
			if(q->data)
				delete q->data;
			q->data = data;
			if(ack_block_to_server)
				q->ack_block_to_server = true;
			return;
		}
	}
	
	/*
		Add the block
	*/
	QueuedMeshUpdate *q = new QueuedMeshUpdate;
	q->p = p;
	q->data = data;
	q->ack_block_to_server = ack_block_to_server;
	m_queue.push_back(q);
}

// Returned pointer must be deleted
// Returns NULL if queue is empty
QueuedMeshUpdate * MeshUpdateQueue::pop()
{
	JMutexAutoLock lock(m_mutex);

	bool must_be_urgent = !m_urgents.empty();
	for(std::vector<QueuedMeshUpdate*>::iterator
			i = m_queue.begin();
			i != m_queue.end(); i++)
	{
		QueuedMeshUpdate *q = *i;
		if(must_be_urgent && m_urgents.count(q->p) == 0)
			continue;
		m_queue.erase(i);
		m_urgents.erase(q->p);
		return q;
	}
	return NULL;
}

/*
	MeshUpdateThread
*/

void * MeshUpdateThread::Thread()
{
	ThreadStarted();

	log_register_thread("MeshUpdateThread");

	DSTACK(__FUNCTION_NAME);
	
	BEGIN_DEBUG_EXCEPTION_HANDLER

	porting::setThreadName("MeshUpdateThread");

	while(!StopRequested())
	{
		QueuedMeshUpdate *q = m_queue_in.pop();
		if(q == NULL)
		{
			sleep_ms(3);
			continue;
		}

		ScopeProfiler sp(g_profiler, "Client: Mesh making");

		MapBlockMesh *mesh_new = new MapBlockMesh(q->data, m_camera_offset);
		if(mesh_new->getMesh()->getMeshBufferCount() == 0)
		{
			delete mesh_new;
			mesh_new = NULL;
		}

		MeshUpdateResult r;
		r.p = q->p;
		r.mesh = mesh_new;
		r.ack_block_to_server = q->ack_block_to_server;

		m_queue_out.push_back(r);

		delete q;
	}

	END_DEBUG_EXCEPTION_HANDLER(errorstream)

	return NULL;
}

/*
	Client
*/

Client::Client(
		IrrlichtDevice *device,
		const char *playername,
		std::string password,
		MapDrawControl &control,
		IWritableTextureSource *tsrc,
		IWritableShaderSource *shsrc,
		IWritableItemDefManager *itemdef,
		IWritableNodeDefManager *nodedef,
		ISoundManager *sound,
		MtEventManager *event,
		bool ipv6
):
	m_packetcounter_timer(0.0),
	m_connection_reinit_timer(0.1),
	m_avg_rtt_timer(0.0),
	m_playerpos_send_timer(0.0),
	m_ignore_damage_timer(0.0),
	m_tsrc(tsrc),
	m_shsrc(shsrc),
	m_itemdef(itemdef),
	m_nodedef(nodedef),
	m_sound(sound),
	m_event(event),
	m_mesh_update_thread(this),
	m_env(
		new ClientMap(this, this, control,
			device->getSceneManager()->getRootSceneNode(),
			device->getSceneManager(), 666),
		device->getSceneManager(),
		tsrc, this, device
	),
	m_con(PROTOCOL_ID, 512, CONNECTION_TIMEOUT, ipv6, this),
	m_device(device),
	m_server_ser_ver(SER_FMT_VER_INVALID),
	m_playeritem(0),
	m_inventory_updated(false),
	m_inventory_from_server(NULL),
	m_inventory_from_server_age(0.0),
	m_show_hud(true),
	m_animation_time(0),
	m_crack_level(-1),
	m_crack_pos(0,0,0),
	m_map_seed(0),
	m_password(password),
	m_access_denied(false),
	m_itemdef_received(false),
	m_nodedef_received(false),
	m_media_downloader(new ClientMediaDownloader()),
	m_time_of_day_set(false),
	m_last_time_of_day_f(-1),
	m_time_of_day_update_timer(0),
	m_recommended_send_interval(0.1),
	m_removed_sounds_check_timer(0),
	m_state(LC_Created)
{
	/*
		Add local player
	*/
	{
		Player *player = new LocalPlayer(this, playername);

		m_env.addPlayer(player);
	}
}

void Client::Stop()
{
	//request all client managed threads to stop
	m_mesh_update_thread.Stop();
}

bool Client::isShutdown()
{

	if (!m_mesh_update_thread.IsRunning()) return true;

	return false;
}

Client::~Client()
{
	m_con.Disconnect();

	m_mesh_update_thread.Stop();
	m_mesh_update_thread.Wait();
	while(!m_mesh_update_thread.m_queue_out.empty()) {
		MeshUpdateResult r = m_mesh_update_thread.m_queue_out.pop_frontNoEx();
		delete r.mesh;
	}


	delete m_inventory_from_server;

	// Delete detached inventories
	for(std::map<std::string, Inventory*>::iterator
			i = m_detached_inventories.begin();
			i != m_detached_inventories.end(); i++){
		delete i->second;
	}

	// cleanup 3d model meshes on client shutdown
	while (m_device->getSceneManager()->getMeshCache()->getMeshCount() != 0) {
		scene::IAnimatedMesh * mesh =
			m_device->getSceneManager()->getMeshCache()->getMeshByIndex(0);

		if (mesh != NULL)
			m_device->getSceneManager()->getMeshCache()->removeMesh(mesh);
	}
}

void Client::connect(Address address)
{
	DSTACK(__FUNCTION_NAME);
	m_con.SetTimeoutMs(0);
	m_con.Connect(address);
}

void Client::step(float dtime)
{
	DSTACK(__FUNCTION_NAME);

	// Limit a bit
	if(dtime > 2.0)
		dtime = 2.0;

	if(m_ignore_damage_timer > dtime)
		m_ignore_damage_timer -= dtime;
	else
		m_ignore_damage_timer = 0.0;
	
	m_animation_time += dtime;
	if(m_animation_time > 60.0)
		m_animation_time -= 60.0;

	m_time_of_day_update_timer += dtime;

	ReceiveAll();

	/*
		Packet counter
	*/
	{
		float &counter = m_packetcounter_timer;
		counter -= dtime;
		if(counter <= 0.0)
		{
			counter = 20.0;
			
			infostream << "Client packetcounter (" << m_packetcounter_timer
					<< "):"<<std::endl;
			m_packetcounter.print(infostream);
			m_packetcounter.clear();
		}
	}

#if 0
	{
		/*
			Delete unused sectors

			NOTE: This jams the game for a while because deleting sectors
			      clear caches
		*/
		
		float &counter = m_delete_unused_sectors_timer;
		counter -= dtime;
		if(counter <= 0.0)
		{
			// 3 minute interval
			//counter = 180.0;
			counter = 60.0;

			//JMutexAutoLock lock(m_env_mutex); //bulk comment-out

			core::list<v3s16> deleted_blocks;

			float delete_unused_sectors_timeout =
				g_settings->getFloat("client_delete_unused_sectors_timeout");
	
			// Delete sector blocks
			/*u32 num = m_env.getMap().unloadUnusedData
					(delete_unused_sectors_timeout,
					true, &deleted_blocks);*/
			
			// Delete whole sectors
			m_env.getMap().unloadUnusedData
					(delete_unused_sectors_timeout,
					&deleted_blocks);

			if(deleted_blocks.size() > 0)
			{
				/*infostream<<"Client: Deleted blocks of "<<num
						<<" unused sectors"<<std::endl;*/
				/*infostream<<"Client: Deleted "<<num
						<<" unused sectors"<<std::endl;*/
				
				/*
					Send info to server
				*/

				// Env is locked so con can be locked.
				//JMutexAutoLock lock(m_con_mutex); //bulk comment-out
				
				core::list<v3s16>::Iterator i = deleted_blocks.begin();
				core::list<v3s16> sendlist;
				for(;;)
				{
					if(sendlist.size() == 255 || i == deleted_blocks.end())
					{
						if(sendlist.size() == 0)
							break;
						/*
							[0] u16 command
							[2] u8 count
							[3] v3s16 pos_0
							[3+6] v3s16 pos_1
							...
						*/
						u32 replysize = 2+1+6*sendlist.size();
						SharedBuffer<u8> reply(replysize);
						writeU16(&reply[0], TOSERVER_DELETEDBLOCKS);
						reply[2] = sendlist.size();
						u32 k = 0;
						for(core::list<v3s16>::Iterator
								j = sendlist.begin();
								j != sendlist.end(); j++)
						{
							writeV3S16(&reply[2+1+6*k], *j);
							k++;
						}
						m_con.Send(PEER_ID_SERVER, 1, reply, true);

						if(i == deleted_blocks.end())
							break;

						sendlist.clear();
					}

					sendlist.push_back(*i);
					i++;
				}
			}
		}
	}
#endif
	// UGLY hack to fix 2 second startup delay caused by non existent
	// server client startup synchronization in local server or singleplayer mode
	static bool initial_step = true;
	if (initial_step) {
		initial_step = false;
	}
	else if(m_state == LC_Created)
	{
		float &counter = m_connection_reinit_timer;
		counter -= dtime;
		if(counter <= 0.0)
		{
			counter = 2.0;

			//JMutexAutoLock envlock(m_env_mutex); //bulk comment-out
			
			Player *myplayer = m_env.getLocalPlayer();
			assert(myplayer != NULL);
			// Send TOSERVER_INIT
			// [0] u16 TOSERVER_INIT
			// [2] u8 SER_FMT_VER_HIGHEST_READ
			// [3] u8[20] player_name
			// [23] u8[28] password (new in some version)
			// [51] u16 minimum supported network protocol version (added sometime)
			// [53] u16 maximum supported network protocol version (added later than the previous one)
			SharedBuffer<u8> data(2+1+PLAYERNAME_SIZE+PASSWORD_SIZE+2+2);
			writeU16(&data[0], TOSERVER_INIT);
			writeU8(&data[2], SER_FMT_VER_HIGHEST_READ);

			memset((char*)&data[3], 0, PLAYERNAME_SIZE);
			snprintf((char*)&data[3], PLAYERNAME_SIZE, "%s", myplayer->getName());

			/*infostream<<"Client: sending initial password hash: \""<<m_password<<"\""
					<<std::endl;*/

			memset((char*)&data[23], 0, PASSWORD_SIZE);
			snprintf((char*)&data[23], PASSWORD_SIZE, "%s", m_password.c_str());
			
			writeU16(&data[51], CLIENT_PROTOCOL_VERSION_MIN);
			writeU16(&data[53], CLIENT_PROTOCOL_VERSION_MAX);

			// Send as unreliable
			Send(1, data, false);
		}

		// Not connected, return
		return;
	}

	/*
		Do stuff if connected
	*/
	
	/*
		Run Map's timers and unload unused data
	*/
	const float map_timer_and_unload_dtime = 5.25;
	if(m_map_timer_and_unload_interval.step(dtime, map_timer_and_unload_dtime))
	{
		ScopeProfiler sp(g_profiler, "Client: map timer and unload");
		std::list<v3s16> deleted_blocks;
		m_env.getMap().timerUpdate(map_timer_and_unload_dtime,
				g_settings->getFloat("client_unload_unused_data_timeout"),
				&deleted_blocks);
				
		/*if(deleted_blocks.size() > 0)
			infostream<<"Client: Unloaded "<<deleted_blocks.size()
					<<" unused blocks"<<std::endl;*/
			
		/*
			Send info to server
			NOTE: This loop is intentionally iterated the way it is.
		*/

		std::list<v3s16>::iterator i = deleted_blocks.begin();
		std::list<v3s16> sendlist;
		for(;;)
		{
			if(sendlist.size() == 255 || i == deleted_blocks.end())
			{
				if(sendlist.size() == 0)
					break;
				/*
					[0] u16 command
					[2] u8 count
					[3] v3s16 pos_0
					[3+6] v3s16 pos_1
					...
				*/
				u32 replysize = 2+1+6*sendlist.size();
				SharedBuffer<u8> reply(replysize);
				writeU16(&reply[0], TOSERVER_DELETEDBLOCKS);
				reply[2] = sendlist.size();
				u32 k = 0;
				for(std::list<v3s16>::iterator
						j = sendlist.begin();
						j != sendlist.end(); ++j)
				{
					writeV3S16(&reply[2+1+6*k], *j);
					k++;
				}
				m_con.Send(PEER_ID_SERVER, 2, reply, true);

				if(i == deleted_blocks.end())
					break;

				sendlist.clear();
			}

			sendlist.push_back(*i);
			++i;
		}
	}

	/*
		Handle environment
	*/
	{
		// Control local player (0ms)
		LocalPlayer *player = m_env.getLocalPlayer();
		assert(player != NULL);
		player->applyControl(dtime);

		// Step environment
		m_env.step(dtime);
		
		/*
			Get events
		*/
		for(;;)
		{
			ClientEnvEvent event = m_env.getClientEvent();
			if(event.type == CEE_NONE)
			{
				break;
			}
			else if(event.type == CEE_PLAYER_DAMAGE)
			{
				if(m_ignore_damage_timer <= 0)
				{
					u8 damage = event.player_damage.amount;
					
					if(event.player_damage.send_to_server)
						sendDamage(damage);

					// Add to ClientEvent queue
					ClientEvent event;
					event.type = CE_PLAYER_DAMAGE;
					event.player_damage.amount = damage;
					m_client_event_queue.push_back(event);
				}
			}
			else if(event.type == CEE_PLAYER_BREATH)
			{
					u16 breath = event.player_breath.amount;
					sendBreath(breath);
			}
		}
	}

	/*
		Print some info
	*/
	{
		float &counter = m_avg_rtt_timer;
		counter += dtime;
		if(counter >= 10)
		{
			counter = 0.0;
			// connectedAndInitialized() is true, peer exists.
			float avg_rtt = getRTT();
			infostream<<"Client: avg_rtt="<<avg_rtt<<std::endl;
		}
	}

	/*
		Send player position to server
	*/
	{
		float &counter = m_playerpos_send_timer;
		counter += dtime;
		if((m_state == LC_Ready) && (counter >= m_recommended_send_interval))
		{
			counter = 0.0;
			sendPlayerPos();
		}
	}

	/*
		Replace updated meshes
	*/
	{
		int num_processed_meshes = 0;
		while(!m_mesh_update_thread.m_queue_out.empty())
		{
			num_processed_meshes++;
			MeshUpdateResult r = m_mesh_update_thread.m_queue_out.pop_frontNoEx();
			MapBlock *block = m_env.getMap().getBlockNoCreateNoEx(r.p);
			if(block)
			{
				// Delete the old mesh
				if(block->mesh != NULL)
				{
					// TODO: Remove hardware buffers of meshbuffers of block->mesh
					delete block->mesh;
					block->mesh = NULL;
				}

				// Replace with the new mesh
				block->mesh = r.mesh;
			} else {
				delete r.mesh;
			}
			if(r.ack_block_to_server)
			{
				/*
					Acknowledge block
				*/
				/*
					[0] u16 command
					[2] u8 count
					[3] v3s16 pos_0
					[3+6] v3s16 pos_1
					...
				*/
				u32 replysize = 2+1+6;
				SharedBuffer<u8> reply(replysize);
				writeU16(&reply[0], TOSERVER_GOTBLOCKS);
				reply[2] = 1;
				writeV3S16(&reply[3], r.p);
				// Send as reliable
				m_con.Send(PEER_ID_SERVER, 2, reply, true);
			}
		}
		if(num_processed_meshes > 0)
			g_profiler->graphAdd("num_processed_meshes", num_processed_meshes);
	}

	/*
		Load fetched media
	*/
	if (m_media_downloader && m_media_downloader->isStarted()) {
		m_media_downloader->step(this);
		if (m_media_downloader->isDone()) {
			received_media();
			delete m_media_downloader;
			m_media_downloader = NULL;
		}
	}

	/*
		If the server didn't update the inventory in a while, revert
		the local inventory (so the player notices the lag problem
		and knows something is wrong).
	*/
	if(m_inventory_from_server)
	{
		float interval = 10.0;
		float count_before = floor(m_inventory_from_server_age / interval);

		m_inventory_from_server_age += dtime;

		float count_after = floor(m_inventory_from_server_age / interval);

		if(count_after != count_before)
		{
			// Do this every <interval> seconds after TOCLIENT_INVENTORY
			// Reset the locally changed inventory to the authoritative inventory
			Player *player = m_env.getLocalPlayer();
			player->inventory = *m_inventory_from_server;
			m_inventory_updated = true;
		}
	}

	/*
		Update positions of sounds attached to objects
	*/
	{
		for(std::map<int, u16>::iterator
				i = m_sounds_to_objects.begin();
				i != m_sounds_to_objects.end(); i++)
		{
			int client_id = i->first;
			u16 object_id = i->second;
			ClientActiveObject *cao = m_env.getActiveObject(object_id);
			if(!cao)
				continue;
			v3f pos = cao->getPosition();
			m_sound->updateSoundPosition(client_id, pos);
		}
	}
	
	/*
		Handle removed remotely initiated sounds
	*/
	m_removed_sounds_check_timer += dtime;
	if(m_removed_sounds_check_timer >= 2.32)
	{
		m_removed_sounds_check_timer = 0;
		// Find removed sounds and clear references to them
		std::set<s32> removed_server_ids;
		for(std::map<s32, int>::iterator
				i = m_sounds_server_to_client.begin();
				i != m_sounds_server_to_client.end();)
		{
			s32 server_id = i->first;
			int client_id = i->second;
			i++;
			if(!m_sound->soundExists(client_id)){
				m_sounds_server_to_client.erase(server_id);
				m_sounds_client_to_server.erase(client_id);
				m_sounds_to_objects.erase(client_id);
				removed_server_ids.insert(server_id);
			}
		}
		// Sync to server
		if(removed_server_ids.size() != 0)
		{
			std::ostringstream os(std::ios_base::binary);
			writeU16(os, TOSERVER_REMOVED_SOUNDS);
			size_t server_ids = removed_server_ids.size();
			assert(server_ids <= 0xFFFF);
			writeU16(os, (u16) (server_ids & 0xFFFF));
			for(std::set<s32>::iterator i = removed_server_ids.begin();
					i != removed_server_ids.end(); i++)
				writeS32(os, *i);
			std::string s = os.str();
			SharedBuffer<u8> data((u8*)s.c_str(), s.size());
			// Send as reliable
			Send(1, data, true);
		}
	}
}

bool Client::loadMedia(const std::string &data, const std::string &filename)
{
	// Silly irrlicht's const-incorrectness
	Buffer<char> data_rw(data.c_str(), data.size());
	
	std::string name;

	const char *image_ext[] = {
		".png", ".jpg", ".bmp", ".tga",
		".pcx", ".ppm", ".psd", ".wal", ".rgb",
		NULL
	};
	name = removeStringEnd(filename, image_ext);
	if(name != "")
	{
		verbosestream<<"Client: Attempting to load image "
		<<"file \""<<filename<<"\""<<std::endl;

		io::IFileSystem *irrfs = m_device->getFileSystem();
		video::IVideoDriver *vdrv = m_device->getVideoDriver();

		// Create an irrlicht memory file
		io::IReadFile *rfile = irrfs->createMemoryReadFile(
				*data_rw, data_rw.getSize(), "_tempreadfile");
		assert(rfile);
		// Read image
		video::IImage *img = vdrv->createImageFromFile(rfile);
		if(!img){
			errorstream<<"Client: Cannot create image from data of "
					<<"file \""<<filename<<"\""<<std::endl;
			rfile->drop();
			return false;
		}
		else {
			m_tsrc->insertSourceImage(filename, img);
			img->drop();
			rfile->drop();
			return true;
		}
	}

	const char *sound_ext[] = {
		".0.ogg", ".1.ogg", ".2.ogg", ".3.ogg", ".4.ogg",
		".5.ogg", ".6.ogg", ".7.ogg", ".8.ogg", ".9.ogg",
		".ogg", NULL
	};
	name = removeStringEnd(filename, sound_ext);
	if(name != "")
	{
		verbosestream<<"Client: Attempting to load sound "
		<<"file \""<<filename<<"\""<<std::endl;
		m_sound->loadSoundData(name, data);
		return true;
	}

	const char *model_ext[] = {
		".x", ".b3d", ".md2", ".obj",
		NULL
	};
	name = removeStringEnd(filename, model_ext);
	if(name != "")
	{
		verbosestream<<"Client: Storing model into memory: "
				<<"\""<<filename<<"\""<<std::endl;
		if(m_mesh_data.count(filename))
			errorstream<<"Multiple models with name \""<<filename.c_str()
					<<"\" found; replacing previous model"<<std::endl;
		m_mesh_data[filename] = data;
		return true;
	}

	errorstream<<"Client: Don't know how to load file \""
			<<filename<<"\""<<std::endl;
	return false;
}

// Virtual methods from con::PeerHandler
void Client::peerAdded(con::Peer *peer)
{
	infostream<<"Client::peerAdded(): peer->id="
			<<peer->id<<std::endl;
}
void Client::deletingPeer(con::Peer *peer, bool timeout)
{
	infostream<<"Client::deletingPeer(): "
			"Server Peer is getting deleted "
			<<"(timeout="<<timeout<<")"<<std::endl;
}

/*
	u16 command
	u16 number of files requested
	for each file {
		u16 length of name
		string name
	}
*/
void Client::request_media(const std::list<std::string> &file_requests)
{
	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOSERVER_REQUEST_MEDIA);
	size_t file_requests_size = file_requests.size();
	assert(file_requests_size <= 0xFFFF);
	writeU16(os, (u16) (file_requests_size & 0xFFFF));

	for(std::list<std::string>::const_iterator i = file_requests.begin();
			i != file_requests.end(); ++i) {
		os<<serializeString(*i);
	}

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(1, data, true);
	infostream<<"Client: Sending media request list to server ("
			<<file_requests.size()<<" files)"<<std::endl;
}

void Client::received_media()
{
	// notify server we received everything
	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOSERVER_RECEIVED_MEDIA);
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(1, data, true);
	infostream<<"Client: Notifying server that we received all media"
			<<std::endl;
}

void Client::ReceiveAll()
{
	DSTACK(__FUNCTION_NAME);
	u32 start_ms = porting::getTimeMs();
	for(;;)
	{
		// Limit time even if there would be huge amounts of data to
		// process
		if(porting::getTimeMs() > start_ms + 100)
			break;
		
		try{
			Receive();
			g_profiler->graphAdd("client_received_packets", 1);
		}
		catch(con::NoIncomingDataException &e)
		{
			break;
		}
		catch(con::InvalidIncomingDataException &e)
		{
			infostream<<"Client::ReceiveAll(): "
					"InvalidIncomingDataException: what()="
					<<e.what()<<std::endl;
		}
	}
}

void Client::Receive()
{
	DSTACK(__FUNCTION_NAME);
	SharedBuffer<u8> data;
	u16 sender_peer_id;
	u32 datasize = m_con.Receive(sender_peer_id, data);
	ProcessData(*data, datasize, sender_peer_id);
}

/*
	sender_peer_id given to this shall be quaranteed to be a valid peer
*/
void Client::ProcessData(u8 *data, u32 datasize, u16 sender_peer_id)
{
	DSTACK(__FUNCTION_NAME);

	// Ignore packets that don't even fit a command
	if(datasize < 2)
	{
		m_packetcounter.add(60000);
		return;
	}

	ToClientCommand command = (ToClientCommand)readU16(&data[0]);

	//infostream<<"Client: received command="<<command<<std::endl;
	m_packetcounter.add((u16)command);
	
	/*
		If this check is removed, be sure to change the queue
		system to know the ids
	*/
	if(sender_peer_id != PEER_ID_SERVER)
	{
		infostream<<"Client::ProcessData(): Discarding data not "
				"coming from server: peer_id="<<sender_peer_id
				<<std::endl;
		return;
	}

	u8 ser_version = m_server_ser_ver;

	if(command == TOCLIENT_INIT)
	{
		if(datasize < 3)
			return;

		u8 deployed = data[2];

		infostream<<"Client: TOCLIENT_INIT received with "
				"deployed="<<((int)deployed&0xff)<<std::endl;

		if(!ser_ver_supported(deployed))
		{
			infostream<<"Client: TOCLIENT_INIT: Server sent "
					<<"unsupported ser_fmt_ver"<<std::endl;
			return;
		}
		
		m_server_ser_ver = deployed;

		// Get player position
		v3s16 playerpos_s16(0, BS*2+BS*20, 0);
		if(datasize >= 2+1+6)
			playerpos_s16 = readV3S16(&data[2+1]);
		v3f playerpos_f = intToFloat(playerpos_s16, BS) - v3f(0, BS/2, 0);

			
		// Set player position
		Player *player = m_env.getLocalPlayer();
		assert(player != NULL);
		player->setPosition(playerpos_f);
		
		if(datasize >= 2+1+6+8)
		{
			// Get map seed
			m_map_seed = readU64(&data[2+1+6]);
			infostream<<"Client: received map seed: "<<m_map_seed<<std::endl;
		}

		if(datasize >= 2+1+6+8+4)
		{
			// Get map seed
			m_recommended_send_interval = readF1000(&data[2+1+6+8]);
			infostream<<"Client: received recommended send interval "
					<<m_recommended_send_interval<<std::endl;
		}
		
		// Reply to server
		u32 replysize = 2;
		SharedBuffer<u8> reply(replysize);
		writeU16(&reply[0], TOSERVER_INIT2);
		// Send as reliable
		m_con.Send(PEER_ID_SERVER, 1, reply, true);

		m_state = LC_Init;

		return;
	}

	if(command == TOCLIENT_ACCESS_DENIED)
	{
		// The server didn't like our password. Note, this needs
		// to be processed even if the serialisation format has
		// not been agreed yet, the same as TOCLIENT_INIT.
		m_access_denied = true;
		m_access_denied_reason = L"Unknown";
		if(datasize >= 4)
		{
			std::string datastring((char*)&data[2], datasize-2);
			std::istringstream is(datastring, std::ios_base::binary);
			m_access_denied_reason = deSerializeWideString(is);
		}
		return;
	}

	if(ser_version == SER_FMT_VER_INVALID)
	{
		infostream<<"Client: Server serialization"
				" format invalid or not initialized."
				" Skipping incoming command="<<command<<std::endl;
		return;
	}
	
	/*
	  Handle runtime commands
	*/
	// there's no sane reason why we shouldn't have a player and
	// almost everyone needs a player reference
	Player *player = m_env.getLocalPlayer();
	assert(player != NULL);

	if(command == TOCLIENT_REMOVENODE)
	{
		if(datasize < 8)
			return;
		v3s16 p;
		p.X = readS16(&data[2]);
		p.Y = readS16(&data[4]);
		p.Z = readS16(&data[6]);
		removeNode(p);
	}
	else if(command == TOCLIENT_ADDNODE)
	{
		if(datasize < 8 + MapNode::serializedLength(ser_version))
			return;

		v3s16 p;
		p.X = readS16(&data[2]);
		p.Y = readS16(&data[4]);
		p.Z = readS16(&data[6]);

		MapNode n;
		n.deSerialize(&data[8], ser_version);
		
		bool remove_metadata = true;
		u32 index = 8 + MapNode::serializedLength(ser_version);
		if ((datasize >= index+1) && data[index]){
			remove_metadata = false;
		}
		
		addNode(p, n, remove_metadata);
	}
	else if(command == TOCLIENT_BLOCKDATA)
	{
		// Ignore too small packet
		if(datasize < 8)
			return;
			
		v3s16 p;
		p.X = readS16(&data[2]);
		p.Y = readS16(&data[4]);
		p.Z = readS16(&data[6]);
		
		std::string datastring((char*)&data[8], datasize-8);
		std::istringstream istr(datastring, std::ios_base::binary);
		
		MapSector *sector;
		MapBlock *block;
		
		v2s16 p2d(p.X, p.Z);
		sector = m_env.getMap().emergeSector(p2d);
		
		assert(sector->getPos() == p2d);
		
		block = sector->getBlockNoCreateNoEx(p.Y);
		if(block)
		{
			/*
				Update an existing block
			*/
			block->deSerialize(istr, ser_version, false);
			block->deSerializeNetworkSpecific(istr);
		}
		else
		{
			/*
				Create a new block
			*/
			block = new MapBlock(&m_env.getMap(), p, this);
			block->deSerialize(istr, ser_version, false);
			block->deSerializeNetworkSpecific(istr);
			sector->insertBlock(block);
		}

		/*
			Add it to mesh update queue and set it to be acknowledged after update.
		*/
		addUpdateMeshTaskWithEdge(p, true);
	}
	else if(command == TOCLIENT_INVENTORY)
	{
		if(datasize < 3)
			return;

		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		player->inventory.deSerialize(is);

		m_inventory_updated = true;

		delete m_inventory_from_server;
		m_inventory_from_server = new Inventory(player->inventory);
		m_inventory_from_server_age = 0.0;

	}
	else if(command == TOCLIENT_TIME_OF_DAY)
	{
		if(datasize < 4)
			return;
		
		u16 time_of_day  = readU16(&data[2]);
		time_of_day      = time_of_day % 24000;
		float time_speed = 0;

		if(datasize >= 2 + 2 + 4)
		{
			time_speed = readF1000(&data[4]);
		}
		else {
			// Old message; try to approximate speed of time by ourselves
			float time_of_day_f = (float)time_of_day / 24000.0;
			float tod_diff_f = 0;

			if(time_of_day_f < 0.2 && m_last_time_of_day_f > 0.8)
				tod_diff_f = time_of_day_f - m_last_time_of_day_f + 1.0;
			else
				tod_diff_f = time_of_day_f - m_last_time_of_day_f;

			m_last_time_of_day_f         = time_of_day_f;
			float time_diff            = m_time_of_day_update_timer;
			m_time_of_day_update_timer = 0;

			if(m_time_of_day_set){
				time_speed = (3600.0*24.0) * tod_diff_f / time_diff;
				infostream<<"Client: Measured time_of_day speed (old format): "
						<<time_speed<<" tod_diff_f="<<tod_diff_f
						<<" time_diff="<<time_diff<<std::endl;
			}
		}
		
		// Update environment
		m_env.setTimeOfDay(time_of_day);
		m_env.setTimeOfDaySpeed(time_speed);
		m_time_of_day_set = true;

		u32 dr = m_env.getDayNightRatio();
		infostream<<"Client: time_of_day="<<time_of_day
				<<" time_speed="<<time_speed
				<<" dr="<<dr<<std::endl;
	}
	else if(command == TOCLIENT_CHAT_MESSAGE)
	{
		/*
			u16 command
			u16 length
			wstring message
		*/
		u8 buf[6];
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		
		// Read stuff
		is.read((char*) buf, 2);
		u16 len = readU16(buf);
		
		std::wstring message;
		for(unsigned int i=0; i<len; i++)
		{
			is.read((char*)buf, 2);
			message += (wchar_t)readU16(buf);
		}
		
		m_chat_queue.push_back(message);
	}
	else if(command == TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD)
	{
		/*
			u16 command
			u16 count of removed objects
			for all removed objects {
				u16 id
			}
			u16 count of added objects
			for all added objects {
				u16 id
				u8 type
				u32 initialization data length
				string initialization data
			}
		*/

		char buf[6];
		// Get all data except the command number
		std::string datastring((char*)&data[2], datasize-2);
		// Throw them in an istringstream
		std::istringstream is(datastring, std::ios_base::binary);

		// Read removed objects
		is.read(buf, 2);
		u16 removed_count = readU16((u8*)buf);
		for(unsigned int i=0; i<removed_count; i++)
		{
			is.read(buf, 2);
			u16 id = readU16((u8*)buf);
			m_env.removeActiveObject(id);
		}

		// Read added objects
		is.read(buf, 2);
		u16 added_count = readU16((u8*)buf);
		for(unsigned int i=0; i<added_count; i++)
		{
			is.read(buf, 2);
			u16 id = readU16((u8*)buf);
			is.read(buf, 1);
			u8 type = readU8((u8*)buf);
			std::string data = deSerializeLongString(is);
			// Add it
			m_env.addActiveObject(id, type, data);
		}
	}
	else if(command == TOCLIENT_ACTIVE_OBJECT_MESSAGES)
	{
		/*
			u16 command
			for all objects
			{
				u16 id
				u16 message length
				string message
			}
		*/
		char buf[6];
		// Get all data except the command number
		std::string datastring((char*)&data[2], datasize-2);
		// Throw them in an istringstream
		std::istringstream is(datastring, std::ios_base::binary);

		while(is.eof() == false)
		{
			is.read(buf, 2);
			u16 id = readU16((u8*)buf);
			if(is.eof())
				break;
			is.read(buf, 2);
			size_t message_size = readU16((u8*)buf);
			std::string message;
			message.reserve(message_size);
			for(unsigned int i=0; i<message_size; i++)
			{
				is.read(buf, 1);
				message.append(buf, 1);
			}
			// Pass on to the environment
			m_env.processActiveObjectMessage(id, message);
		}
	}
	else if(command == TOCLIENT_MOVEMENT)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		player->movement_acceleration_default   = readF1000(is) * BS;
		player->movement_acceleration_air       = readF1000(is) * BS;
		player->movement_acceleration_fast      = readF1000(is) * BS;
		player->movement_speed_walk             = readF1000(is) * BS;
		player->movement_speed_crouch           = readF1000(is) * BS;
		player->movement_speed_fast             = readF1000(is) * BS;
		player->movement_speed_climb            = readF1000(is) * BS;
		player->movement_speed_jump             = readF1000(is) * BS;
		player->movement_liquid_fluidity        = readF1000(is) * BS;
		player->movement_liquid_fluidity_smooth = readF1000(is) * BS;
		player->movement_liquid_sink            = readF1000(is) * BS;
		player->movement_gravity                = readF1000(is) * BS;
	}
	else if(command == TOCLIENT_HP)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		u8 oldhp   = player->hp;
		u8 hp      = readU8(is);
		player->hp = hp;

		if(hp < oldhp)
		{
			// Add to ClientEvent queue
			ClientEvent event;
			event.type = CE_PLAYER_DAMAGE;
			event.player_damage.amount = oldhp - hp;
			m_client_event_queue.push_back(event);
		}
	}
	else if(command == TOCLIENT_BREATH)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		player->setBreath(readU16(is));
	}
	else if(command == TOCLIENT_MOVE_PLAYER)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		v3f pos = readV3F1000(is);
		f32 pitch = readF1000(is);
		f32 yaw = readF1000(is);
		player->setPosition(pos);

		infostream<<"Client got TOCLIENT_MOVE_PLAYER"
				<<" pos=("<<pos.X<<","<<pos.Y<<","<<pos.Z<<")"
				<<" pitch="<<pitch
				<<" yaw="<<yaw
				<<std::endl;

		/*
			Add to ClientEvent queue.
			This has to be sent to the main program because otherwise
			it would just force the pitch and yaw values to whatever
			the camera points to.
		*/
		ClientEvent event;
		event.type = CE_PLAYER_FORCE_MOVE;
		event.player_force_move.pitch = pitch;
		event.player_force_move.yaw = yaw;
		m_client_event_queue.push_back(event);

		// Ignore damage for a few seconds, so that the player doesn't
		// get damage from falling on ground
		m_ignore_damage_timer = 3.0;
	}
	else if(command == TOCLIENT_PLAYERITEM)
	{
		infostream<<"Client: WARNING: Ignoring TOCLIENT_PLAYERITEM"<<std::endl;
	}
	else if(command == TOCLIENT_DEATHSCREEN)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		
		bool set_camera_point_target = readU8(is);
		v3f camera_point_target = readV3F1000(is);
		
		ClientEvent event;
		event.type                                = CE_DEATHSCREEN;
		event.deathscreen.set_camera_point_target = set_camera_point_target;
		event.deathscreen.camera_point_target_x   = camera_point_target.X;
		event.deathscreen.camera_point_target_y   = camera_point_target.Y;
		event.deathscreen.camera_point_target_z   = camera_point_target.Z;
		m_client_event_queue.push_back(event);
	}
	else if(command == TOCLIENT_ANNOUNCE_MEDIA)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		int num_files = readU16(is);
		
		infostream<<"Client: Received media announcement: packet size: "
				<<datasize<<std::endl;

		if (m_media_downloader == NULL ||
				m_media_downloader->isStarted()) {
			const char *problem = m_media_downloader ?
				"we already saw another announcement" :
				"all media has been received already";
			errorstream<<"Client: Received media announcement but "
				<<problem<<"! "
				<<" files="<<num_files
				<<" size="<<datasize<<std::endl;
			return;
		}

		// Mesh update thread must be stopped while
		// updating content definitions
		assert(!m_mesh_update_thread.IsRunning());

		for(int i=0; i<num_files; i++)
		{
			std::string name = deSerializeString(is);
			std::string sha1_base64 = deSerializeString(is);
			std::string sha1_raw = base64_decode(sha1_base64);
			m_media_downloader->addFile(name, sha1_raw);
		}

		std::vector<std::string> remote_media;
		try {
			Strfnd sf(deSerializeString(is));
			while(!sf.atend()) {
				std::string baseurl = trim(sf.next(","));
				if(baseurl != "")
					m_media_downloader->addRemoteServer(baseurl);
			}
		}
		catch(SerializationError& e) {
			// not supported by server or turned off
		}

		m_media_downloader->step(this);
	}
	else if(command == TOCLIENT_MEDIA)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		/*
			u16 command
			u16 total number of file bunches
			u16 index of this bunch
			u32 number of files in this bunch
			for each file {
				u16 length of name
				string name
				u32 length of data
				data
			}
		*/
		int num_bunches = readU16(is);
		int bunch_i = readU16(is);
		u32 num_files = readU32(is);
		infostream<<"Client: Received files: bunch "<<bunch_i<<"/"
				<<num_bunches<<" files="<<num_files
				<<" size="<<datasize<<std::endl;

		if (num_files == 0)
			return;

		if (m_media_downloader == NULL ||
				!m_media_downloader->isStarted()) {
			const char *problem = m_media_downloader ?
				"media has not been requested" :
				"all media has been received already";
			errorstream<<"Client: Received media but "
				<<problem<<"! "
				<<" bunch "<<bunch_i<<"/"<<num_bunches
				<<" files="<<num_files
				<<" size="<<datasize<<std::endl;
			return;
		}

		// Mesh update thread must be stopped while
		// updating content definitions
		assert(!m_mesh_update_thread.IsRunning());

		for(unsigned int i=0; i<num_files; i++){
			std::string name = deSerializeString(is);
			std::string data = deSerializeLongString(is);
			m_media_downloader->conventionalTransferDone(
					name, data, this);
		}
	}
	else if(command == TOCLIENT_TOOLDEF)
	{
		infostream<<"Client: WARNING: Ignoring TOCLIENT_TOOLDEF"<<std::endl;
	}
	else if(command == TOCLIENT_NODEDEF)
	{
		infostream<<"Client: Received node definitions: packet size: "
				<<datasize<<std::endl;

		// Mesh update thread must be stopped while
		// updating content definitions
		assert(!m_mesh_update_thread.IsRunning());

		// Decompress node definitions
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		std::istringstream tmp_is(deSerializeLongString(is), std::ios::binary);
		std::ostringstream tmp_os;
		decompressZlib(tmp_is, tmp_os);

		// Deserialize node definitions
		std::istringstream tmp_is2(tmp_os.str());
		m_nodedef->deSerialize(tmp_is2);
		m_nodedef_received = true;
	}
	else if(command == TOCLIENT_CRAFTITEMDEF)
	{
		infostream<<"Client: WARNING: Ignoring TOCLIENT_CRAFTITEMDEF"<<std::endl;
	}
	else if(command == TOCLIENT_ITEMDEF)
	{
		infostream<<"Client: Received item definitions: packet size: "
				<<datasize<<std::endl;

		// Mesh update thread must be stopped while
		// updating content definitions
		assert(!m_mesh_update_thread.IsRunning());

		// Decompress item definitions
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		std::istringstream tmp_is(deSerializeLongString(is), std::ios::binary);
		std::ostringstream tmp_os;
		decompressZlib(tmp_is, tmp_os);

		// Deserialize node definitions
		std::istringstream tmp_is2(tmp_os.str());
		m_itemdef->deSerialize(tmp_is2);
		m_itemdef_received = true;
	}
	else if(command == TOCLIENT_PLAY_SOUND)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		s32 server_id = readS32(is);
		std::string name = deSerializeString(is);
		float gain = readF1000(is);
		int type = readU8(is); // 0=local, 1=positional, 2=object
		v3f pos = readV3F1000(is);
		u16 object_id = readU16(is);
		bool loop = readU8(is);
		// Start playing
		int client_id = -1;
		switch(type){
		case 0: // local
			client_id = m_sound->playSound(name, loop, gain);
			break;
		case 1: // positional
			client_id = m_sound->playSoundAt(name, loop, gain, pos);
			break;
		case 2: { // object
			ClientActiveObject *cao = m_env.getActiveObject(object_id);
			if(cao)
				pos = cao->getPosition();
			client_id = m_sound->playSoundAt(name, loop, gain, pos);
			// TODO: Set up sound to move with object
			break; }
		default:
			break;
		}
		if(client_id != -1){
			m_sounds_server_to_client[server_id] = client_id;
			m_sounds_client_to_server[client_id] = server_id;
			if(object_id != 0)
				m_sounds_to_objects[client_id] = object_id;
		}
	}
	else if(command == TOCLIENT_STOP_SOUND)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		s32 server_id = readS32(is);
		std::map<s32, int>::iterator i =
				m_sounds_server_to_client.find(server_id);
		if(i != m_sounds_server_to_client.end()){
			int client_id = i->second;
			m_sound->stopSound(client_id);
		}
	}
	else if(command == TOCLIENT_PRIVILEGES)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);
		
		m_privileges.clear();
		infostream<<"Client: Privileges updated: ";
		u16 num_privileges = readU16(is);
		for(unsigned int i=0; i<num_privileges; i++){
			std::string priv = deSerializeString(is);
			m_privileges.insert(priv);
			infostream<<priv<<" ";
		}
		infostream<<std::endl;
	}
	else if(command == TOCLIENT_INVENTORY_FORMSPEC)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		// Store formspec in LocalPlayer
		player->inventory_formspec = deSerializeLongString(is);
	}
	else if(command == TOCLIENT_DETACHED_INVENTORY)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		std::string name = deSerializeString(is);
		
		infostream<<"Client: Detached inventory update: \""<<name<<"\""<<std::endl;

		Inventory *inv = NULL;
		if(m_detached_inventories.count(name) > 0)
			inv = m_detached_inventories[name];
		else{
			inv = new Inventory(m_itemdef);
			m_detached_inventories[name] = inv;
		}
		inv->deSerialize(is);
	}
	else if(command == TOCLIENT_SHOW_FORMSPEC)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		std::string formspec = deSerializeLongString(is);
		std::string formname = deSerializeString(is);

		ClientEvent event;
		event.type = CE_SHOW_FORMSPEC;
		// pointer is required as event is a struct only!
		// adding a std:string to a struct isn't possible
		event.show_formspec.formspec = new std::string(formspec);
		event.show_formspec.formname = new std::string(formname);
		m_client_event_queue.push_back(event);
	}
	else if(command == TOCLIENT_SPAWN_PARTICLE)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		v3f pos                 = readV3F1000(is);
		v3f vel                 = readV3F1000(is);
		v3f acc                 = readV3F1000(is);
		float expirationtime    = readF1000(is);
		float size              = readF1000(is);
		bool collisiondetection = readU8(is);
		std::string texture     = deSerializeLongString(is);
		bool vertical           = false;
		try {
			vertical = readU8(is);
		} catch (...) {}

		ClientEvent event;
		event.type                              = CE_SPAWN_PARTICLE;
		event.spawn_particle.pos                = new v3f (pos);
		event.spawn_particle.vel                = new v3f (vel);
		event.spawn_particle.acc                = new v3f (acc);
		event.spawn_particle.expirationtime     = expirationtime;
		event.spawn_particle.size               = size;
		event.spawn_particle.collisiondetection = collisiondetection;
		event.spawn_particle.vertical           = vertical;
		event.spawn_particle.texture            = new std::string(texture);

		m_client_event_queue.push_back(event);
	}
	else if(command == TOCLIENT_ADD_PARTICLESPAWNER)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		u16 amount              = readU16(is);
		float spawntime         = readF1000(is);
		v3f minpos              = readV3F1000(is);
		v3f maxpos              = readV3F1000(is);
		v3f minvel              = readV3F1000(is);
		v3f maxvel              = readV3F1000(is);
		v3f minacc              = readV3F1000(is);
		v3f maxacc              = readV3F1000(is);
		float minexptime        = readF1000(is);
		float maxexptime        = readF1000(is);
		float minsize           = readF1000(is);
		float maxsize           = readF1000(is);
		bool collisiondetection = readU8(is);
		std::string texture     = deSerializeLongString(is);
		u32 id                  = readU32(is);
		bool vertical = false;
		try {
			vertical = readU8(is);
		} catch (...) {}

		ClientEvent event;
		event.type                                   = CE_ADD_PARTICLESPAWNER;
		event.add_particlespawner.amount             = amount;
		event.add_particlespawner.spawntime          = spawntime;
		event.add_particlespawner.minpos             = new v3f (minpos);
		event.add_particlespawner.maxpos             = new v3f (maxpos);
		event.add_particlespawner.minvel             = new v3f (minvel);
		event.add_particlespawner.maxvel             = new v3f (maxvel);
		event.add_particlespawner.minacc             = new v3f (minacc);
		event.add_particlespawner.maxacc             = new v3f (maxacc);
		event.add_particlespawner.minexptime         = minexptime;
		event.add_particlespawner.maxexptime         = maxexptime;
		event.add_particlespawner.minsize            = minsize;
		event.add_particlespawner.maxsize            = maxsize;
		event.add_particlespawner.collisiondetection = collisiondetection;
		event.add_particlespawner.vertical           = vertical;
		event.add_particlespawner.texture            = new std::string(texture);
		event.add_particlespawner.id                 = id;

		m_client_event_queue.push_back(event);
	}
	else if(command == TOCLIENT_DELETE_PARTICLESPAWNER)
	{
		std::string datastring((char*)&data[2], datasize-2);
		std::istringstream is(datastring, std::ios_base::binary);

		u32 id = readU16(is);

		ClientEvent event;
		event.type                      = CE_DELETE_PARTICLESPAWNER;
		event.delete_particlespawner.id = id;

		m_client_event_queue.push_back(event);
	}
	else if(command == TOCLIENT_HUDADD)
	{
		std::string datastring((char *)&data[2], datasize - 2);
		std::istringstream is(datastring, std::ios_base::binary);

		u32 id           = readU32(is);
		u8 type          = readU8(is);
		v2f pos          = readV2F1000(is);
		std::string name = deSerializeString(is);
		v2f scale        = readV2F1000(is);
		std::string text = deSerializeString(is);
		u32 number       = readU32(is);
		u32 item         = readU32(is);
		u32 dir          = readU32(is);
		v2f align        = readV2F1000(is);
		v2f offset       = readV2F1000(is);
		v3f world_pos;
		v2s32 size;
		try{
			world_pos    = readV3F1000(is);
		}catch(SerializationError &e) {};
		try{
			size = readV2S32(is);
		} catch(SerializationError &e) {};

		ClientEvent event;
		event.type             = CE_HUDADD;
		event.hudadd.id        = id;
		event.hudadd.type      = type;
		event.hudadd.pos       = new v2f(pos);
		event.hudadd.name      = new std::string(name);
		event.hudadd.scale     = new v2f(scale);
		event.hudadd.text      = new std::string(text);
		event.hudadd.number    = number;
		event.hudadd.item      = item;
		event.hudadd.dir       = dir;
		event.hudadd.align     = new v2f(align);
		event.hudadd.offset    = new v2f(offset);
		event.hudadd.world_pos = new v3f(world_pos);
		event.hudadd.size      = new v2s32(size);
		m_client_event_queue.push_back(event);
	}
	else if(command == TOCLIENT_HUDRM)
	{
		std::string datastring((char *)&data[2], datasize - 2);
		std::istringstream is(datastring, std::ios_base::binary);

		u32 id = readU32(is);

		ClientEvent event;
		event.type     = CE_HUDRM;
		event.hudrm.id = id;
		m_client_event_queue.push_back(event);
	}
	else if(command == TOCLIENT_HUDCHANGE)
	{
		std::string sdata;
		v2f v2fdata;
		v3f v3fdata;
		u32 intdata = 0;
		v2s32 v2s32data;
		
		std::string datastring((char *)&data[2], datasize - 2);
		std::istringstream is(datastring, std::ios_base::binary);

		u32 id  = readU32(is);
		u8 stat = (HudElementStat)readU8(is);
		
		if (stat == HUD_STAT_POS || stat == HUD_STAT_SCALE ||
			stat == HUD_STAT_ALIGN || stat == HUD_STAT_OFFSET)
			v2fdata = readV2F1000(is);
		else if (stat == HUD_STAT_NAME || stat == HUD_STAT_TEXT)
			sdata = deSerializeString(is);
		else if (stat == HUD_STAT_WORLD_POS)
			v3fdata = readV3F1000(is);
		else if (stat == HUD_STAT_SIZE )
			v2s32data = readV2S32(is);
		else
			intdata = readU32(is);
		
		ClientEvent event;
		event.type              = CE_HUDCHANGE;
		event.hudchange.id      = id;
		event.hudchange.stat    = (HudElementStat)stat;
		event.hudchange.v2fdata = new v2f(v2fdata);
		event.hudchange.v3fdata = new v3f(v3fdata);
		event.hudchange.sdata   = new std::string(sdata);
		event.hudchange.data    = intdata;
		event.hudchange.v2s32data = new v2s32(v2s32data);
		m_client_event_queue.push_back(event);
	}
	else if(command == TOCLIENT_HUD_SET_FLAGS)
	{
		std::string datastring((char *)&data[2], datasize - 2);
		std::istringstream is(datastring, std::ios_base::binary);

		u32 flags = readU32(is);
		u32 mask  = readU32(is);
		
		player->hud_flags &= ~mask;
		player->hud_flags |= flags;
	}
	else if(command == TOCLIENT_HUD_SET_PARAM)
	{
		std::string datastring((char *)&data[2], datasize - 2);
		std::istringstream is(datastring, std::ios_base::binary);

		u16 param         = readU16(is);
		std::string value = deSerializeString(is);

		if(param == HUD_PARAM_HOTBAR_ITEMCOUNT && value.size() == 4) {
			s32 hotbar_itemcount = readS32((u8*) value.c_str());
			if(hotbar_itemcount > 0 && hotbar_itemcount <= HUD_HOTBAR_ITEMCOUNT_MAX)
				player->hud_hotbar_itemcount = hotbar_itemcount;
		}
		else if (param == HUD_PARAM_HOTBAR_IMAGE) {
			((LocalPlayer *) player)->hotbar_image = value;
		}
		else if (param == HUD_PARAM_HOTBAR_SELECTED_IMAGE) {
			((LocalPlayer *) player)->hotbar_selected_image = value;
		}
	}
	else if(command == TOCLIENT_SET_SKY)
	{
		std::string datastring((char *)&data[2], datasize - 2);
		std::istringstream is(datastring, std::ios_base::binary);

		video::SColor *bgcolor           = new video::SColor(readARGB8(is));
		std::string *type                = new std::string(deSerializeString(is));
		u16 count                        = readU16(is);
		std::vector<std::string> *params = new std::vector<std::string>;

		for(size_t i=0; i<count; i++)
			params->push_back(deSerializeString(is));

		ClientEvent event;
		event.type            = CE_SET_SKY;
		event.set_sky.bgcolor = bgcolor;
		event.set_sky.type    = type;
		event.set_sky.params  = params;
		m_client_event_queue.push_back(event);
	}
	else if(command == TOCLIENT_OVERRIDE_DAY_NIGHT_RATIO)
	{
		std::string datastring((char *)&data[2], datasize - 2);
		std::istringstream is(datastring, std::ios_base::binary);

		bool do_override        = readU8(is);
		float day_night_ratio_f = (float)readU16(is) / 65536;

		ClientEvent event;
		event.type                                 = CE_OVERRIDE_DAY_NIGHT_RATIO;
		event.override_day_night_ratio.do_override = do_override;
		event.override_day_night_ratio.ratio_f     = day_night_ratio_f;
		m_client_event_queue.push_back(event);
	}
	else if(command == TOCLIENT_LOCAL_PLAYER_ANIMATIONS)
	{
		std::string datastring((char *)&data[2], datasize - 2);
		std::istringstream is(datastring, std::ios_base::binary);

		LocalPlayer *player = m_env.getLocalPlayer();
		assert(player != NULL);

		player->local_animations[0] = readV2S32(is);
		player->local_animations[1] = readV2S32(is);
		player->local_animations[2] = readV2S32(is);
		player->local_animations[3] = readV2S32(is);
		player->local_animation_speed = readF1000(is);
	}
	else if(command == TOCLIENT_EYE_OFFSET)
	{
		std::string datastring((char *)&data[2], datasize - 2);
		std::istringstream is(datastring, std::ios_base::binary);

		LocalPlayer *player = m_env.getLocalPlayer();
		assert(player != NULL);

		player->eye_offset_first = readV3F1000(is);
		player->eye_offset_third = readV3F1000(is);
	}
	else
	{
		infostream<<"Client: Ignoring unknown command "
				<<command<<std::endl;
	}
}

void Client::Send(u16 channelnum, SharedBuffer<u8> data, bool reliable)
{
	//JMutexAutoLock lock(m_con_mutex); //bulk comment-out
	m_con.Send(PEER_ID_SERVER, channelnum, data, reliable);
}

void Client::interact(u8 action, const PointedThing& pointed)
{
	if(m_state != LC_Ready){
		infostream<<"Client::interact() "
				"cancelled (not connected)"
				<<std::endl;
		return;
	}

	std::ostringstream os(std::ios_base::binary);

	/*
		[0] u16 command
		[2] u8 action
		[3] u16 item
		[5] u32 length of the next item
		[9] serialized PointedThing
		actions:
		0: start digging (from undersurface) or use
		1: stop digging (all parameters ignored)
		2: digging completed
		3: place block or item (to abovesurface)
		4: use item
	*/
	writeU16(os, TOSERVER_INTERACT);
	writeU8(os, action);
	writeU16(os, getPlayerItem());
	std::ostringstream tmp_os(std::ios::binary);
	pointed.serialize(tmp_os);
	os<<serializeLongString(tmp_os.str());

	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());

	// Send as reliable
	Send(0, data, true);
}

void Client::sendNodemetaFields(v3s16 p, const std::string &formname,
		const std::map<std::string, std::string> &fields)
{
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOSERVER_NODEMETA_FIELDS);
	writeV3S16(os, p);
	os<<serializeString(formname);
	size_t fields_size = fields.size();
	assert(fields_size <= 0xFFFF);
	writeU16(os, (u16) (fields_size & 0xFFFF));
	for(std::map<std::string, std::string>::const_iterator
			i = fields.begin(); i != fields.end(); i++){
		const std::string &name = i->first;
		const std::string &value = i->second;
		os<<serializeString(name);
		os<<serializeLongString(value);
	}

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}
	
void Client::sendInventoryFields(const std::string &formname,
		const std::map<std::string, std::string> &fields)
{
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOSERVER_INVENTORY_FIELDS);
	os<<serializeString(formname);
	size_t fields_size = fields.size();
	assert(fields_size <= 0xFFFF);
	writeU16(os, (u16) (fields_size & 0xFFFF));
	for(std::map<std::string, std::string>::const_iterator
			i = fields.begin(); i != fields.end(); i++){
		const std::string &name  = i->first;
		const std::string &value = i->second;
		os<<serializeString(name);
		os<<serializeLongString(value);
	}

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}

void Client::sendInventoryAction(InventoryAction *a)
{
	std::ostringstream os(std::ios_base::binary);
	u8 buf[12];
	
	// Write command
	writeU16(buf, TOSERVER_INVENTORY_ACTION);
	os.write((char*)buf, 2);

	a->serialize(os);
	
	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}

void Client::sendChatMessage(const std::wstring &message)
{
	std::ostringstream os(std::ios_base::binary);
	u8 buf[12];
	
	// Write command
	writeU16(buf, TOSERVER_CHAT_MESSAGE);
	os.write((char*)buf, 2);
	
	// Write length
	size_t messagesize = message.size();
	if (messagesize > 0xFFFF) {
		messagesize = 0xFFFF;
	}
	writeU16(buf, (u16) messagesize);
	os.write((char*)buf, 2);
	
	// Write string
	for(unsigned int i=0; i<message.size(); i++)
	{
		u16 w = message[i];
		writeU16(buf, w);
		os.write((char*)buf, 2);
	}
	
	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}

void Client::sendChangePassword(const std::wstring &oldpassword,
                                const std::wstring &newpassword)
{
	Player *player = m_env.getLocalPlayer();
	if(player == NULL)
		return;

	std::string playername = player->getName();
	std::string oldpwd = translatePassword(playername, oldpassword);
	std::string newpwd = translatePassword(playername, newpassword);

	std::ostringstream os(std::ios_base::binary);
	u8 buf[2+PASSWORD_SIZE*2];
	/*
		[0] u16 TOSERVER_PASSWORD
		[2] u8[28] old password
		[30] u8[28] new password
	*/

	writeU16(buf, TOSERVER_PASSWORD);
	for(unsigned int i=0;i<PASSWORD_SIZE-1;i++)
	{
		buf[2+i] = i<oldpwd.length()?oldpwd[i]:0;
		buf[30+i] = i<newpwd.length()?newpwd[i]:0;
	}
	buf[2+PASSWORD_SIZE-1] = 0;
	buf[30+PASSWORD_SIZE-1] = 0;
	os.write((char*)buf, 2+PASSWORD_SIZE*2);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}


void Client::sendDamage(u8 damage)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOSERVER_DAMAGE);
	writeU8(os, damage);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}

void Client::sendBreath(u16 breath)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOSERVER_BREATH);
	writeU16(os, breath);
	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}

void Client::sendRespawn()
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOSERVER_RESPAWN);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}

void Client::sendReady()
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOSERVER_CLIENT_READY);
	writeU8(os,VERSION_MAJOR);
	writeU8(os,VERSION_MINOR);
	writeU8(os,VERSION_PATCH_ORIG);
	writeU8(os,0);

	writeU16(os,strlen(minetest_version_hash));
	os.write(minetest_version_hash,strlen(minetest_version_hash));

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	Send(0, data, true);
}

void Client::sendPlayerPos()
{
	LocalPlayer *myplayer = m_env.getLocalPlayer();
	if(myplayer == NULL)
		return;

	// Save bandwidth by only updating position when something changed
	if(myplayer->last_position        == myplayer->getPosition() &&
			myplayer->last_speed      == myplayer->getSpeed()    &&
			myplayer->last_pitch      == myplayer->getPitch()    &&
			myplayer->last_yaw        == myplayer->getYaw()      &&
			myplayer->last_keyPressed == myplayer->keyPressed)
		return;

	myplayer->last_position   = myplayer->getPosition();
	myplayer->last_speed      = myplayer->getSpeed();
	myplayer->last_pitch      = myplayer->getPitch();
	myplayer->last_yaw        = myplayer->getYaw();
	myplayer->last_keyPressed = myplayer->keyPressed;

	u16 our_peer_id;
	{
		//JMutexAutoLock lock(m_con_mutex); //bulk comment-out
		our_peer_id = m_con.GetPeerID();
	}
	
	// Set peer id if not set already
	if(myplayer->peer_id == PEER_ID_INEXISTENT)
		myplayer->peer_id = our_peer_id;
	// Check that an existing peer_id is the same as the connection's
	assert(myplayer->peer_id == our_peer_id);
	
	v3f pf         = myplayer->getPosition();
	v3f sf         = myplayer->getSpeed();
	s32 pitch      = myplayer->getPitch() * 100;
	s32 yaw        = myplayer->getYaw() * 100;
	u32 keyPressed = myplayer->keyPressed;

	v3s32 position(pf.X*100, pf.Y*100, pf.Z*100);
	v3s32 speed(sf.X*100, sf.Y*100, sf.Z*100);
	/*
		Format:
		[0] u16 command
		[2] v3s32 position*100
		[2+12] v3s32 speed*100
		[2+12+12] s32 pitch*100
		[2+12+12+4] s32 yaw*100
		[2+12+12+4+4] u32 keyPressed
	*/
	SharedBuffer<u8> data(2+12+12+4+4+4);
	writeU16(&data[0], TOSERVER_PLAYERPOS);
	writeV3S32(&data[2], position);
	writeV3S32(&data[2+12], speed);
	writeS32(&data[2+12+12], pitch);
	writeS32(&data[2+12+12+4], yaw);
	writeU32(&data[2+12+12+4+4], keyPressed);
	// Send as unreliable
	Send(0, data, false);
}

void Client::sendPlayerItem(u16 item)
{
	Player *myplayer = m_env.getLocalPlayer();
	if(myplayer == NULL)
		return;

	u16 our_peer_id = m_con.GetPeerID();

	// Set peer id if not set already
	if(myplayer->peer_id == PEER_ID_INEXISTENT)
		myplayer->peer_id = our_peer_id;
	// Check that an existing peer_id is the same as the connection's
	assert(myplayer->peer_id == our_peer_id);

	SharedBuffer<u8> data(2+2);
	writeU16(&data[0], TOSERVER_PLAYERITEM);
	writeU16(&data[2], item);

	// Send as reliable
	Send(0, data, true);
}

void Client::removeNode(v3s16 p)
{
	std::map<v3s16, MapBlock*> modified_blocks;

	try
	{
		m_env.getMap().removeNodeAndUpdate(p, modified_blocks);
	}
	catch(InvalidPositionException &e)
	{
	}
	
	// add urgent task to update the modified node
	addUpdateMeshTaskForNode(p, false, true);

	for(std::map<v3s16, MapBlock * >::iterator
			i = modified_blocks.begin();
			i != modified_blocks.end(); ++i)
	{
		addUpdateMeshTaskWithEdge(i->first);
	}
}

void Client::addNode(v3s16 p, MapNode n, bool remove_metadata)
{
	TimeTaker timer1("Client::addNode()");

	std::map<v3s16, MapBlock*> modified_blocks;

	try
	{
		//TimeTaker timer3("Client::addNode(): addNodeAndUpdate");
		m_env.getMap().addNodeAndUpdate(p, n, modified_blocks, remove_metadata);
	}
	catch(InvalidPositionException &e)
	{}
	
	for(std::map<v3s16, MapBlock * >::iterator
			i = modified_blocks.begin();
			i != modified_blocks.end(); ++i)
	{
		addUpdateMeshTaskWithEdge(i->first);
	}
}
	
void Client::setPlayerControl(PlayerControl &control)
{
	LocalPlayer *player = m_env.getLocalPlayer();
	assert(player != NULL);
	player->control = control;
}

void Client::selectPlayerItem(u16 item)
{
	m_playeritem = item;
	m_inventory_updated = true;
	sendPlayerItem(item);
}

// Returns true if the inventory of the local player has been
// updated from the server. If it is true, it is set to false.
bool Client::getLocalInventoryUpdated()
{
	bool updated = m_inventory_updated;
	m_inventory_updated = false;
	return updated;
}

// Copies the inventory of the local player to parameter
void Client::getLocalInventory(Inventory &dst)
{
	Player *player = m_env.getLocalPlayer();
	assert(player != NULL);
	dst = player->inventory;
}

Inventory* Client::getInventory(const InventoryLocation &loc)
{
	switch(loc.type){
	case InventoryLocation::UNDEFINED:
	{}
	break;
	case InventoryLocation::CURRENT_PLAYER:
	{
		Player *player = m_env.getLocalPlayer();
		assert(player != NULL);
		return &player->inventory;
	}
	break;
	case InventoryLocation::PLAYER:
	{
		Player *player = m_env.getPlayer(loc.name.c_str());
		if(!player)
			return NULL;
		return &player->inventory;
	}
	break;
	case InventoryLocation::NODEMETA:
	{
		NodeMetadata *meta = m_env.getMap().getNodeMetadata(loc.p);
		if(!meta)
			return NULL;
		return meta->getInventory();
	}
	break;
	case InventoryLocation::DETACHED:
	{
		if(m_detached_inventories.count(loc.name) == 0)
			return NULL;
		return m_detached_inventories[loc.name];
	}
	break;
	default:
		assert(0);
	}
	return NULL;
}

void Client::inventoryAction(InventoryAction *a)
{
	/*
		Send it to the server
	*/
	sendInventoryAction(a);

	/*
		Predict some local inventory changes
	*/
	a->clientApply(this, this);

	// Remove it
	delete a;
}

ClientActiveObject * Client::getSelectedActiveObject(
		f32 max_d,
		v3f from_pos_f_on_map,
		core::line3d<f32> shootline_on_map
	)
{
	std::vector<DistanceSortedActiveObject> objects;

	m_env.getActiveObjects(from_pos_f_on_map, max_d, objects);
	
	// Sort them.
	// After this, the closest object is the first in the array.
	std::sort(objects.begin(), objects.end());

	for(unsigned int i=0; i<objects.size(); i++)
	{
		ClientActiveObject *obj = objects[i].obj;
		
		core::aabbox3d<f32> *selection_box = obj->getSelectionBox();
		if(selection_box == NULL)
			continue;

		v3f pos = obj->getPosition();

		core::aabbox3d<f32> offsetted_box(
				selection_box->MinEdge + pos,
				selection_box->MaxEdge + pos
		);

		if(offsetted_box.intersectsWithLine(shootline_on_map))
		{
			return obj;
		}
	}

	return NULL;
}

std::list<std::string> Client::getConnectedPlayerNames()
{
	return m_env.getPlayerNames();
}

float Client::getAnimationTime()
{
	return m_animation_time;
}

int Client::getCrackLevel()
{
	return m_crack_level;
}

void Client::setHighlighted(v3s16 pos, bool show_hud)
{
	m_show_hud = show_hud;
	v3s16 old_highlighted_pos = m_highlighted_pos;
	m_highlighted_pos = pos;
	addUpdateMeshTaskForNode(old_highlighted_pos, false, true);
	addUpdateMeshTaskForNode(m_highlighted_pos, false, true);
}

void Client::setCrack(int level, v3s16 pos)
{
	int old_crack_level = m_crack_level;
	v3s16 old_crack_pos = m_crack_pos;

	m_crack_level = level;
	m_crack_pos = pos;

	if(old_crack_level >= 0 && (level < 0 || pos != old_crack_pos))
	{
		// remove old crack
		addUpdateMeshTaskForNode(old_crack_pos, false, true);
	}
	if(level >= 0 && (old_crack_level < 0 || pos != old_crack_pos))
	{
		// add new crack
		addUpdateMeshTaskForNode(pos, false, true);
	}
}

u16 Client::getHP()
{
	Player *player = m_env.getLocalPlayer();
	assert(player != NULL);
	return player->hp;
}

u16 Client::getBreath()
{
	Player *player = m_env.getLocalPlayer();
	assert(player != NULL);
	return player->getBreath();
}

bool Client::getChatMessage(std::wstring &message)
{
	if(m_chat_queue.size() == 0)
		return false;
	message = m_chat_queue.pop_front();
	return true;
}

void Client::typeChatMessage(const std::wstring &message)
{
	// Discard empty line
	if(message == L"")
		return;

	// Send to others
	sendChatMessage(message);

	// Show locally
	if (message[0] == L'/')
	{
		m_chat_queue.push_back((std::wstring)L"issued command: " + message);
	}
	else
	{
		LocalPlayer *player = m_env.getLocalPlayer();
		assert(player != NULL);
		std::wstring name = narrow_to_wide(player->getName());
		m_chat_queue.push_back((std::wstring)L"<" + name + L"> " + message);
	}
}

void Client::addUpdateMeshTask(v3s16 p, bool ack_to_server, bool urgent)
{
	MapBlock *b = m_env.getMap().getBlockNoCreateNoEx(p);
	if(b == NULL)
		return;

	/*
		Create a task to update the mesh of the block
	*/

	MeshMakeData *data = new MeshMakeData(this);

	{
		//TimeTaker timer("data fill");
		// Release: ~0ms
		// Debug: 1-6ms, avg=2ms
		data->fill(b);
		data->setCrack(m_crack_level, m_crack_pos);
		data->setHighlighted(m_highlighted_pos, m_show_hud);
		data->setSmoothLighting(g_settings->getBool("smooth_lighting"));
	}

	// Add task to queue
	m_mesh_update_thread.m_queue_in.addBlock(p, data, ack_to_server, urgent);
}

void Client::addUpdateMeshTaskWithEdge(v3s16 blockpos, bool ack_to_server, bool urgent)
{
	try{
		v3s16 p = blockpos + v3s16(0,0,0);
		//MapBlock *b = m_env.getMap().getBlockNoCreate(p);
		addUpdateMeshTask(p, ack_to_server, urgent);
	}
	catch(InvalidPositionException &e){}

	// Leading edge
	for (int i=0;i<6;i++)
	{
		try{
			v3s16 p = blockpos + g_6dirs[i];
			addUpdateMeshTask(p, false, urgent);
		}
		catch(InvalidPositionException &e){}
	}
}

void Client::addUpdateMeshTaskForNode(v3s16 nodepos, bool ack_to_server, bool urgent)
{
	{
		v3s16 p = nodepos;
		infostream<<"Client::addUpdateMeshTaskForNode(): "
				<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
				<<std::endl;
	}

	v3s16 blockpos          = getNodeBlockPos(nodepos);
	v3s16 blockpos_relative = blockpos * MAP_BLOCKSIZE;

	try{
		v3s16 p = blockpos + v3s16(0,0,0);
		addUpdateMeshTask(p, ack_to_server, urgent);
	}
	catch(InvalidPositionException &e){}

	// Leading edge
	if(nodepos.X == blockpos_relative.X){
		try{
			v3s16 p = blockpos + v3s16(-1,0,0);
			addUpdateMeshTask(p, false, urgent);
		}
		catch(InvalidPositionException &e){}
	}

	if(nodepos.Y == blockpos_relative.Y){
		try{
			v3s16 p = blockpos + v3s16(0,-1,0);
			addUpdateMeshTask(p, false, urgent);
		}
		catch(InvalidPositionException &e){}
	}

	if(nodepos.Z == blockpos_relative.Z){
		try{
			v3s16 p = blockpos + v3s16(0,0,-1);
			addUpdateMeshTask(p, false, urgent);
		}
		catch(InvalidPositionException &e){}
	}
}

ClientEvent Client::getClientEvent()
{
	if(m_client_event_queue.size() == 0)
	{
		ClientEvent event;
		event.type = CE_NONE;
		return event;
	}
	return m_client_event_queue.pop_front();
}

float Client::mediaReceiveProgress()
{
	if (m_media_downloader)
		return m_media_downloader->getProgress();
	else
		return 1.0; // downloader only exists when not yet done
}

void Client::afterContentReceived(IrrlichtDevice *device, gui::IGUIFont* font)
{
	infostream<<"Client::afterContentReceived() started"<<std::endl;
	assert(m_itemdef_received);
	assert(m_nodedef_received);
	assert(mediaReceived());
	
	// Rebuild inherited images and recreate textures
	infostream<<"- Rebuilding images and textures"<<std::endl;
	m_tsrc->rebuildImagesAndTextures();

	// Rebuild shaders
	infostream<<"- Rebuilding shaders"<<std::endl;
	m_shsrc->rebuildShaders();

	// Update node aliases
	infostream<<"- Updating node aliases"<<std::endl;
	m_nodedef->updateAliases(m_itemdef);

	// Update node textures and assign shaders to each tile
	infostream<<"- Updating node textures"<<std::endl;
	m_nodedef->updateTextures(this);

	// Preload item textures and meshes if configured to
	if(g_settings->getBool("preload_item_visuals"))
	{
		verbosestream<<"Updating item textures and meshes"<<std::endl;
		wchar_t* text = wgettext("Item textures...");
		draw_load_screen(text, device, guienv, font, 0, 0);
		std::set<std::string> names = m_itemdef->getAll();
		size_t size = names.size();
		size_t count = 0;
		int percent = 0;
		for(std::set<std::string>::const_iterator
				i = names.begin(); i != names.end(); ++i){
			// Asking for these caches the result
			m_itemdef->getInventoryTexture(*i, this);
			m_itemdef->getWieldMesh(*i, this);
			count++;
			percent = count*100/size;
			if (count%50 == 0) // only update every 50 item
				draw_load_screen(text, device, guienv, font, 0, percent);
		}
		delete[] text;
	}

	// Start mesh update thread after setting up content definitions
	infostream<<"- Starting mesh update thread"<<std::endl;
	m_mesh_update_thread.Start();
	
	m_state = LC_Ready;
	sendReady();
	infostream<<"Client::afterContentReceived() done"<<std::endl;
}

float Client::getRTT(void)
{
	return m_con.getPeerStat(PEER_ID_SERVER,con::AVG_RTT);
}

float Client::getCurRate(void)
{
	return ( m_con.getLocalStat(con::CUR_INC_RATE) +
			m_con.getLocalStat(con::CUR_DL_RATE));
}

float Client::getAvgRate(void)
{
	return ( m_con.getLocalStat(con::AVG_INC_RATE) +
			m_con.getLocalStat(con::AVG_DL_RATE));
}

void Client::makeScreenshot(IrrlichtDevice *device)
{
	irr::video::IVideoDriver *driver = device->getVideoDriver();
	irr::video::IImage* const raw_image = driver->createScreenShot();
	if (raw_image) {
		irr::video::IImage* const image = driver->createImage(video::ECF_R8G8B8, 
			raw_image->getDimension());

		if (image) {
			raw_image->copyTo(image);
			irr::c8 filename[256];
			snprintf(filename, sizeof(filename), "%s" DIR_DELIM "screenshot_%u.png",
				 g_settings->get("screenshot_path").c_str(),
				 device->getTimer()->getRealTime());
			std::stringstream sstr;
			if (driver->writeImageToFile(image, filename)) {
				sstr << "Saved screenshot to '" << filename << "'";
			} else {
				sstr << "Failed to save screenshot '" << filename << "'";
			}
			m_chat_queue.push_back(narrow_to_wide(sstr.str()));
			infostream << sstr << std::endl;
			image->drop();
		}
		raw_image->drop();
	}
}

// IGameDef interface
// Under envlock
IItemDefManager* Client::getItemDefManager()
{
	return m_itemdef;
}
INodeDefManager* Client::getNodeDefManager()
{
	return m_nodedef;
}
ICraftDefManager* Client::getCraftDefManager()
{
	return NULL;
	//return m_craftdef;
}
ITextureSource* Client::getTextureSource()
{
	return m_tsrc;
}
IShaderSource* Client::getShaderSource()
{
	return m_shsrc;
}
scene::ISceneManager* Client::getSceneManager()
{
	return m_device->getSceneManager();
}
u16 Client::allocateUnknownNodeId(const std::string &name)
{
	errorstream<<"Client::allocateUnknownNodeId(): "
			<<"Client cannot allocate node IDs"<<std::endl;
	assert(0);
	return CONTENT_IGNORE;
}
ISoundManager* Client::getSoundManager()
{
	return m_sound;
}
MtEventManager* Client::getEventManager()
{
	return m_event;
}

scene::IAnimatedMesh* Client::getMesh(const std::string &filename)
{
	std::map<std::string, std::string>::const_iterator i =
			m_mesh_data.find(filename);
	if(i == m_mesh_data.end()){
		errorstream<<"Client::getMesh(): Mesh not found: \""<<filename<<"\""
				<<std::endl;
		return NULL;
	}
	const std::string &data    = i->second;
	scene::ISceneManager *smgr = m_device->getSceneManager();

	// Create the mesh, remove it from cache and return it
	// This allows unique vertex colors and other properties for each instance
	Buffer<char> data_rw(data.c_str(), data.size()); // Const-incorrect Irrlicht
	io::IFileSystem *irrfs = m_device->getFileSystem();
	io::IReadFile *rfile   = irrfs->createMemoryReadFile(
			*data_rw, data_rw.getSize(), filename.c_str());
	assert(rfile);

	scene::IAnimatedMesh *mesh = smgr->getMesh(rfile);
	rfile->drop();
	// NOTE: By playing with Irrlicht refcounts, maybe we could cache a bunch
	// of uniquely named instances and re-use them
	mesh->grab();
	smgr->getMeshCache()->removeMesh(mesh);
	return mesh;
}


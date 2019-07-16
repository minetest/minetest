/*
Minetest
Copyright (C) 2019 BuckarooBanzai/naturefreshmilk, Thomas Rudin <thomas@rudin.io>

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

#include <string>
#include "database/database.h"
#include "threading/thread.h"
#include <libpq-fe.h>
#include "remoteplayer.h"

class SerializedPlayer {
public:
	SerializedPlayer(RemotePlayer *player);
	void persist(PGconn *m_conn, int pgversion);
	std::string getName();
private:
	std::string name;
	std::string pitch;
	std::string yaw;
	std::string posx;
	std::string posy;
	std::string posz;
	std::string hp;
	std::string breath;
};

class SerializedInventory {
public:
	SerializedInventory(std::string player, std::string inv_id, const InventoryList* list);
	void persist(PGconn *m_conn);
private:
	std::string player;
	std::string inv_id;
	std::string width;
	std::string name;
	std::string lsize;
};

class SerializedInventoryItem {
public:
	SerializedInventoryItem(std::string player, std::string inv_id, std::string slotId, std::string itemStr);
	void persist(PGconn *m_conn);
private:
	std::string player;
	std::string inv_id;
	std::string slotId;
	std::string itemStr;
};

class SerializedPlayerMetadata {
public:
	SerializedPlayerMetadata(std::string player, std::string attr, std::string value);
	void persist(PGconn *m_conn);
private:
	std::string player;
	std::string attr;
	std::string value;
	
};

struct QueuedPlayerData {
	SerializedPlayer *serialized_player;
	std::vector<SerializedInventory*> inventories;
	std::vector<SerializedInventoryItem*> inventory_items;
	std::vector<SerializedPlayerMetadata*> metadata;
};


class PlayerSaveQueue : public Thread {
public:
	PlayerSaveQueue(PGconn *m_conn);
	~PlayerSaveQueue();
	void enqueue(RemotePlayer *player);

protected:
	void *run();

private:
	std::exception_ptr m_async_exception;

	int m_pgversion;

	void save(std::vector<QueuedPlayerData*> *queue);
	void savePlayer(QueuedPlayerData *playerdata);

	PGconn *m_conn = nullptr;

	std::mutex m_mutex;
	std::mutex m_exception_mutex;

	std::vector<QueuedPlayerData*> queue;
};

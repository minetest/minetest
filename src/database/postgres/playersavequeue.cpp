#include "playersavequeue.h"

#include "log.h"
#include "threading/mutex_auto_lock.h"
#include "exceptions.h"
#include "pgutil.h"
#include "content_sao.h"

#include <thread>
#include <chrono>
#include <list>


/*
Serialized player data
*/

SerializedPlayer::SerializedPlayer(RemotePlayer *player){
	PlayerSAO* sao = player->getPlayerSAO();
	v3f pos = sao->getBasePosition();

	name = player->getName();
	pitch = ftos(sao->getLookPitch());
	yaw = ftos(sao->getRotation().Y);
	posx = ftos(pos.X);
	posy = ftos(pos.Y);
	posz = ftos(pos.Z);
	hp = itos(sao->getHP());
	breath = itos(sao->getBreath());
}

void SerializedPlayer::persist(PGconn *m_conn, int pgversion){
	const char *values[] = {
		name.c_str(),
		pitch.c_str(),
		yaw.c_str(),
		posx.c_str(), posy.c_str(), posz.c_str(),
		hp.c_str(),
		breath.c_str()
	};

	if (pgversion < 90500) {

		const char *exists_values[] = { name.c_str() };
		PGresult *exists_results = PGUtil::execPrepared(m_conn, "load_player", 1, exists_values, false);

		bool player_exists = (PQntuples(exists_results) > 0);
		PQclear(exists_results);

		if (!player_exists)
			PGUtil::execPrepared(m_conn, "create_player", 8, values, true, false);
		else
			PGUtil::execPrepared(m_conn, "update_player", 8, values, true, false);
	} else {
		PGUtil::execPrepared(m_conn, "save_player", 8, values, true, false);
	}

}

std::string SerializedPlayer::getName(){
	return name;
}

/*
Serialized inventory data
*/

SerializedInventory::SerializedInventory(std::string player, std::string inv_id, const InventoryList* list){
	this->player = player;
	this->inv_id = inv_id;

	name = list->getName();
	width = itos(list->getWidth());
	lsize = itos(list->getSize());
}

void SerializedInventory::persist(PGconn *m_conn){
	const char* inv_values[] = {
		player.c_str(),
		inv_id.c_str(),
		width.c_str(),
		name.c_str(),
		lsize.c_str()
	};

	PGUtil::execPrepared(m_conn, "add_player_inventory", 5, inv_values);
}

/*
Serialized inventory item data
*/

SerializedInventoryItem::SerializedInventoryItem(std::string player, std::string inv_id, std::string slotId, std::string itemStr):
	player(player), inv_id(inv_id), slotId(slotId), itemStr(itemStr)
{}

void SerializedInventoryItem::persist(PGconn *m_conn){
	const char* invitem_values[] = {
		player.c_str(),
		inv_id.c_str(),
		slotId.c_str(),
		itemStr.c_str()
	};
	PGUtil::execPrepared(m_conn, "add_player_inventory_item", 4, invitem_values);
}

/*
Serializes player metadata
*/

SerializedPlayerMetadata::SerializedPlayerMetadata(std::string player, std::string attr, std::string value):
	player(player), attr(attr), value(value)
{}

void SerializedPlayerMetadata::persist(PGconn *m_conn){
	const char *meta_values[] = {
		player.c_str(),
		attr.c_str(),
		value.c_str()
	};
	PGUtil::execPrepared(m_conn, "save_player_metadata", 3, meta_values);
}

/*
Save queue
*/

PlayerSaveQueue::PlayerSaveQueue(PGconn *m_conn) :
	Thread("player_save_queue"),
	m_conn(m_conn) {

	m_pgversion = PQserverVersion(m_conn);

	if (m_pgversion < 90500) {
		PGUtil::prepareStatement(m_conn, "create_player",
			"INSERT INTO player(name, pitch, yaw, posX, posY, posZ, hp, breath) VALUES "
				"($1, $2, $3, $4, $5, $6, $7::int, $8::int)");

		PGUtil::prepareStatement(m_conn, "update_player",
			"UPDATE SET pitch = $2, yaw = $3, posX = $4, posY = $5, posZ = $6, hp = $7::int, "
				"breath = $8::int, modification_date = NOW() WHERE name = $1");
	} else {
		PGUtil::prepareStatement(m_conn, "save_player",
			"INSERT INTO player(name, pitch, yaw, posX, posY, posZ, hp, breath) VALUES "
				"($1, $2, $3, $4, $5, $6, $7::int, $8::int)"
				"ON CONFLICT ON CONSTRAINT player_pkey DO UPDATE SET pitch = $2, yaw = $3, "
				"posX = $4, posY = $5, posZ = $6, hp = $7::int, breath = $8::int, "
				"modification_date = NOW()");
	}

	PGUtil::prepareStatement(m_conn, "remove_player_inventories",
		"DELETE FROM player_inventories WHERE player = $1");

	PGUtil::prepareStatement(m_conn, "remove_player_inventory_items",
		"DELETE FROM player_inventory_items WHERE player = $1");

	PGUtil::prepareStatement(m_conn, "add_player_inventory",
		"INSERT INTO player_inventories (player, inv_id, inv_width, inv_name, inv_size) VALUES "
			"($1, $2::int, $3::int, $4, $5::int)");

	PGUtil::prepareStatement(m_conn, "add_player_inventory_item",
		"INSERT INTO player_inventory_items (player, inv_id, slot_id, item) VALUES "
			"($1, $2::int, $3::int, $4)");

	PGUtil::prepareStatement(m_conn, "remove_player_metadata",
		"DELETE FROM player_metadata WHERE player = $1");

	PGUtil::prepareStatement(m_conn, "save_player_metadata",
		"INSERT INTO player_metadata (player, attr, value) VALUES ($1, $2, $3)");

}

PlayerSaveQueue::~PlayerSaveQueue(){
}


void PlayerSaveQueue::enqueue(RemotePlayer *player){
	{
		MutexAutoLock lock(m_exception_mutex);
		if (m_async_exception) {
			std::rethrow_exception(m_async_exception);
		}
	}

	MutexAutoLock lock(m_mutex);

	QueuedPlayerData *queued_data = new QueuedPlayerData;
	queued_data->serialized_player = new SerializedPlayer(player);

	std::string playername = player->getName();
	PlayerSAO* sao = player->getPlayerSAO();

	std::vector<const InventoryList*> inventory_lists = sao->getInventory()->getLists();
	for (u16 i = 0; i < inventory_lists.size(); i++) {
		const InventoryList* list = inventory_lists[i];
		std::string inv_id = itos(i);

		SerializedInventory* serialized_inv = new SerializedInventory(playername, inv_id, list);
		queued_data->inventories.push_back(serialized_inv);


		for (u32 j = 0; j < list->getSize(); j++) {
			std::ostringstream os;
			list->getItem(j).serialize(os);
			std::string itemStr = os.str();
			std::string slotId = itos(j);

			SerializedInventoryItem *invItem = new SerializedInventoryItem(
				playername,
				inv_id,
				slotId,
				itemStr
			);

			queued_data->inventory_items.push_back(invItem);
		}
	}

	const StringMap &attrs = sao->getMeta().getStrings();
	for (const auto &attr : attrs) {

		SerializedPlayerMetadata *metadata = new SerializedPlayerMetadata(
			playername,
			attr.first,
			attr.second
		);

		queued_data->metadata.push_back(metadata);
	}

	queue.push_back(queued_data);
}


void *PlayerSaveQueue::run(){
	std::vector<QueuedPlayerData*> save_items;

	while (!stopRequested()){
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		{
			// move items over here
			MutexAutoLock lock(m_mutex);
			if (!queue.empty()){
				save_items.swap(queue);
			}
		}

		this->save(&save_items);
		save_items.clear();
	}


	if (!queue.empty()){
		//flush at exit
		MutexAutoLock lock(m_mutex);
		this->save(&queue);
	}

	return nullptr;
}

void PlayerSaveQueue::save(std::vector<QueuedPlayerData*> *queue){
	try {

		for (QueuedPlayerData* item: *queue){
			PGUtil::checkResults(PQexec(m_conn, "BEGIN;"));
			savePlayer(item);
			PGUtil::checkResults(PQexec(m_conn, "COMMIT;"));
			delete item;
		}


	} catch (std::exception &e) {
		MutexAutoLock lock(m_exception_mutex);
		m_async_exception = std::current_exception();
		this->stop();

	}
}

void PlayerSaveQueue::savePlayer(QueuedPlayerData *item){

	const char* rmvalues[] = { item->serialized_player->getName().c_str() };

	item->serialized_player->persist(m_conn, m_pgversion);

	// Write player inventories
	PGUtil::execPrepared(m_conn, "remove_player_inventories", 1, rmvalues);
	PGUtil::execPrepared(m_conn, "remove_player_inventory_items", 1, rmvalues);
	PGUtil::execPrepared(m_conn, "remove_player_metadata", 1, rmvalues);

	for (SerializedInventory *inv: item->inventories){
		inv->persist(m_conn);
		delete inv;
	}

	for (SerializedInventoryItem *invItem: item->inventory_items){
		invItem->persist(m_conn);
		delete invItem;
	}

	for (SerializedPlayerMetadata *metadata: item->metadata){
		metadata->persist(m_conn);
		delete metadata;
	}

	delete item->serialized_player;
}


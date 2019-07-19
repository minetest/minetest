#ifdef USE_POSTGRESQL

#include "mapsavequeue.h"

#include "log.h"
#include "threading/mutex_auto_lock.h"
#include "exceptions.h"

#include <thread>
#include <chrono>
#include <list>
#include "pgutil.h"

MapSaveQueue::MapSaveQueue(PGconn *m_conn) :
	Thread("map_save_queue"),
	m_conn(m_conn) {

	m_pgversion = PQserverVersion(m_conn);

	if (m_pgversion < 90500) {
		PGUtil::prepareStatement(m_conn, "write_block_insert",
			"INSERT INTO blocks (posX, posY, posZ, data) SELECT "
				"$1::int4, $2::int4, $3::int4, $4::bytea "
				"WHERE NOT EXISTS (SELECT true FROM blocks "
				"WHERE posX = $1::int4 AND posY = $2::int4 AND "
				"posZ = $3::int4)");

		PGUtil::prepareStatement(m_conn, "write_block_update",
			"UPDATE blocks SET data = $4::bytea "
				"WHERE posX = $1::int4 AND posY = $2::int4 AND "
				"posZ = $3::int4");
	} else {
		PGUtil::prepareStatement(m_conn, "write_block",
			"INSERT INTO blocks (posX, posY, posZ, data) VALUES "
				"($1::int4, $2::int4, $3::int4, $4::bytea) "
				"ON CONFLICT ON CONSTRAINT blocks_pkey DO "
				"UPDATE SET data = $4::bytea");
	}

}

MapSaveQueue::~MapSaveQueue(){}

void *MapSaveQueue::run(){
	std::vector<QueuedItem*> save_items;

	while (!stopRequested()){
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		// move items over here
		MutexAutoLock lock(m_mutex);
		if (!queue.empty()){
			save_items.swap(queue);
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

void MapSaveQueue::saveBlock(QueuedItem *item){
	// Verify if we don't overflow the platform integer with the mapblock size
	if (item->data.size() > INT_MAX) {
		errorstream << "Database_PostgreSQL::saveBlock: Data truncation! "
		<< "data.size() over 0xFFFFFFFF (== " << item->data.size()
		<< ")" << std::endl;
		throw DatabaseException(std::string(
			"PostgreSQL database error: data truncation @ " +
			std::to_string(item->pos.X) + "/" +
			std::to_string(item->pos.Y) + "/" +
			std::to_string(item->pos.Z)
			)
		);
	}

	if (PQstatus(m_conn) != CONNECTION_OK){
		throw DatabaseException(std::string(
			"PostgreSQL database error: ") +
			PQerrorMessage(m_conn)
		);
	}

	s32 x, y, z;
	x = htonl(item->pos.X);
	y = htonl(item->pos.Y);
	z = htonl(item->pos.Z);

	const void *args[] = { &x, &y, &z, item->data.c_str() };
	const int argLen[] = {
		sizeof(x), sizeof(y), sizeof(z), (int)(item->data.size())
	};
	const int argFmt[] = { 1, 1, 1, 1 };

	if (m_pgversion < 90500) {
		PGUtil::execPrepared(m_conn, "write_block_update", ARRLEN(args), args, argLen, argFmt);
		PGUtil::execPrepared(m_conn, "write_block_insert", ARRLEN(args), args, argLen, argFmt);
	} else {
		PGUtil::execPrepared(m_conn, "write_block", ARRLEN(args), args, argLen, argFmt);
	}

}

void MapSaveQueue::save(std::vector<QueuedItem*> *items){

	try {
		PGUtil::checkResults(PQexec(m_conn, "BEGIN;"));

		for (QueuedItem* item: *items){
			saveBlock(item);
			delete item;
		}

		PGUtil::checkResults(PQexec(m_conn, "COMMIT;"));

	} catch (std::exception &e) {
		MutexAutoLock lock(m_exception_mutex);
		m_async_exception = std::current_exception();
		this->stop();

	}
}


void MapSaveQueue::enqueue(const v3s16 &pos, const std::string &data){
	{
		MutexAutoLock lock(m_exception_mutex);
		if (m_async_exception) {
			std::rethrow_exception(m_async_exception);
		}
	}

	MutexAutoLock lock(m_mutex);

	QueuedItem *item = new QueuedItem();
	item->pos = pos;
	item->data = data;

	queue.push_back(item);
}

#endif //USE_POSTGRESQL
/*
Minetest
Copyright (C) 2014 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "config.h"

#if USE_REDIS
/*
	Redis databases
*/


#include "database-redis.h"
#include <hiredis.h>

#include "map.h"
#include "mapsector.h"
#include "mapblock.h"
#include "serialization.h"
#include "main.h"
#include "settings.h"
#include "log.h"

Database_Redis::Database_Redis(ServerMap *map, std::string savedir)
{
	Settings conf;
	conf.readConfigFile((std::string(savedir) + DIR_DELIM + "world.mt").c_str());
	std::string tmp;
	try {
	tmp = conf.get("redis_address");
	hash = conf.get("redis_hash");
	} catch(SettingNotFoundException e) {
		throw SettingNotFoundException("Set redis_address and redis_hash in world.mt to use the redis backend");
	}
	const char *addr = tmp.c_str();
	int port = conf.exists("redis_port") ? conf.getU16("redis_port") : 6379;
	ctx = redisConnect(addr, port);
	if(!ctx)
		throw FileNotGoodException("Cannot allocate redis context");
	else if(ctx->err) {
		std::string err = std::string("Connection error: ") + ctx->errstr;
		redisFree(ctx);
		throw FileNotGoodException(err);
	}
	srvmap = map;
}

int Database_Redis::Initialized(void)
{
	return 1;
}

void Database_Redis::beginSave() {
	redisReply *reply;
	reply = (redisReply*) redisCommand(ctx, "MULTI");
	if(!reply)
		throw FileNotGoodException(std::string("redis command 'MULTI' failed: ") + ctx->errstr);
	freeReplyObject(reply);
}

void Database_Redis::endSave() {
	redisReply *reply;
	reply = (redisReply*) redisCommand(ctx, "EXEC");
	if(!reply)
		throw FileNotGoodException(std::string("redis command 'EXEC' failed: ") + ctx->errstr);
	freeReplyObject(reply);
}

void Database_Redis::saveBlock(MapBlock *block)
{
	DSTACK(__FUNCTION_NAME);
	/*
		Dummy blocks are not written
	*/
	if(block->isDummy())
	{
		return;
	}

	// Format used for writing
	u8 version = SER_FMT_VER_HIGHEST_WRITE;
	// Get destination
	v3s16 p3d = block->getPos();

	/*
		[0] u8 serialization version
		[1] data
	*/

	std::ostringstream o(std::ios_base::binary);
	o.write((char*)&version, 1);
	// Write basic data
	block->serialize(o, version, true);
	// Write block to database
	std::string tmp1 = o.str();
	std::string tmp2 = i64tos(getBlockAsInteger(p3d));

	redisReply *reply;
	reply = (redisReply*) redisCommand(ctx, "HSET %s %s %b", hash.c_str(), tmp2.c_str(), tmp1.c_str(), tmp1.size());
	if(!reply)
		throw FileNotGoodException(std::string("redis command 'HSET %s %s %b' failed: ") + ctx->errstr);
	if(reply->type == REDIS_REPLY_ERROR)
		throw FileNotGoodException("Failed to store block in Database");

	// We just wrote it to the disk so clear modified flag
	block->resetModified();
}

MapBlock* Database_Redis::loadBlock(v3s16 blockpos)
{
	v2s16 p2d(blockpos.X, blockpos.Z);

	std::string tmp = i64tos(getBlockAsInteger(blockpos));
	redisReply *reply;
	reply = (redisReply*) redisCommand(ctx, "HGET %s %s", hash.c_str(), tmp.c_str());
	if(!reply)
		throw FileNotGoodException(std::string("redis command 'HGET %s %s' failed: ") + ctx->errstr);

	if (reply->type == REDIS_REPLY_STRING && reply->len == 0) {
		freeReplyObject(reply);
		errorstream << "Blank block data in database (reply->len == 0) ("
			<< blockpos.X << "," << blockpos.Y << "," << blockpos.Z << ")" << std::endl;

		if (g_settings->getBool("ignore_world_load_errors")) {
			errorstream << "Ignoring block load error. Duck and cover! "
				<< "(ignore_world_load_errors)" << std::endl;
		} else {
			throw SerializationError("Blank block data in database");
		}
		return NULL;
	}

	if (reply->type == REDIS_REPLY_STRING) {
		/*
			Make sure sector is loaded
		*/
		MapSector *sector = srvmap->createSector(p2d);

		try {
			std::istringstream is(std::string(reply->str, reply->len), std::ios_base::binary);
			freeReplyObject(reply); // std::string copies the memory so we can already do this here
			u8 version = SER_FMT_VER_INVALID;
			is.read((char *)&version, 1);

			if (is.fail())
				throw SerializationError("ServerMap::loadBlock(): Failed"
					" to read MapBlock version");

			MapBlock *block = NULL;
			bool created_new = false;
			block = sector->getBlockNoCreateNoEx(blockpos.Y);
			if (block == NULL)
			{
				block = sector->createBlankBlockNoInsert(blockpos.Y);
				created_new = true;
			}

			// Read basic data
			block->deSerialize(is, version, true);

			// If it's a new block, insert it to the map
			if (created_new)
				sector->insertBlock(block);

			// We just loaded it from, so it's up-to-date.
			block->resetModified();
		}
		catch (SerializationError &e)
		{
			errorstream << "Invalid block data in database"
				<< " (" << blockpos.X << "," << blockpos.Y << "," << blockpos.Z
				<< ") (SerializationError): " << e.what() << std::endl;
			// TODO: Block should be marked as invalid in memory so that it is
			// not touched but the game can run

			if (g_settings->getBool("ignore_world_load_errors")) {
				errorstream << "Ignoring block load error. Duck and cover! "
					<< "(ignore_world_load_errors)" << std::endl;
			} else {
				throw SerializationError("Invalid block data in database");
			}
		}

		return srvmap->getBlockNoCreateNoEx(blockpos);  // should not be using this here
	}
	return NULL;
}

void Database_Redis::listAllLoadableBlocks(std::list<v3s16> &dst)
{
	redisReply *reply;
	reply = (redisReply*) redisCommand(ctx, "HKEYS %s", hash.c_str());
	if(!reply)
		throw FileNotGoodException(std::string("redis command 'HKEYS %s' failed: ") + ctx->errstr);
	if(reply->type != REDIS_REPLY_ARRAY)
		throw FileNotGoodException("Failed to get keys from database");
	for(size_t i = 0; i < reply->elements; i++)
	{
		assert(reply->element[i]->type == REDIS_REPLY_STRING);
		dst.push_back(getIntegerAsBlock(stoi64(reply->element[i]->str)));
	}
	freeReplyObject(reply);
}

Database_Redis::~Database_Redis()
{
	redisFree(ctx);
}
#endif

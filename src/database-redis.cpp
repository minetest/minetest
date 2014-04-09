/*
Minetest
Copyright (C) 2013 sfan5 <sfan5@live.de>

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
#include "hiredis.h"

#include "map.h"
#include "mapsector.h"
#include "mapblock.h"
#include "serialization.h"
#include "main.h"
#include "settings.h"
#include "log.h"

#define REDIS_CMD(_r , _ctx, _cmd, ...) \
	_r = (redisReply*) redisCommand(_ctx, _cmd, ##__VA_ARGS__); \
	if(_r == NULL) \
		throw FileNotGoodException("redis command '" _cmd "' failed");

Database_Redis::Database_Redis(ServerMap *map, std::string savedir)
{
	Settings conf;
	conf.readConfigFile((std::string(savedir) + DIR_DELIM + "world.mt").c_str());
	std::string tmp = conf.get("redis_address"); // This will raise an error if that setting does not exist
	const char *addr = tmp.c_str();
	int port = conf.exists("redis_port")? conf.getU16("redis_port") : 6379;
	ctx = redisConnect(addr, port);
	if(ctx == NULL || ctx->err) {
		if (ctx) {
			std::string err = std::string("Connection error: ") + ctx->errstr;
			redisFree(ctx);
			throw FileNotGoodException(err);
		} else
			throw FileNotGoodException("Cannot allocate redis context");
	}
	srvmap = map;
}

int Database_Redis::Initialized(void)
{
	return 1;
}

void Database_Redis::beginSave() {
	redisReply *reply;
	REDIS_CMD(reply, ctx, "MULTI");
	freeReplyObject(reply);
}

void Database_Redis::endSave() {
	redisReply *reply;
	REDIS_CMD(reply, ctx, "EXEC");
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
	REDIS_CMD(reply, ctx, "SET %s %b", tmp2.c_str(), tmp1.c_str(), tmp1.size());
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
	REDIS_CMD(reply, ctx, "GET %s", tmp.c_str());

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

			/*
				Save blocks loaded in old format in new format
			*/
			//if(version < SER_FMT_VER_HIGHEST || save_after_load)
			// Only save if asked to; no need to update version
			//if(save_after_load)
			//	saveBlock(block);
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
				//assert(0);
			}
		}

		return srvmap->getBlockNoCreateNoEx(blockpos);  // should not be using this here
	}
	return NULL;
}

void Database_Redis::listAllLoadableBlocks(std::list<v3s16> &dst)
{
	redisReply *reply;
	REDIS_CMD(reply, ctx, "KEYS *");
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

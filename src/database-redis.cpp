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
#include "filesys.h"


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

bool Database_Redis::saveBlock(v3s16 blockpos, std::string &data)
{
	std::string tmp = i64tos(getBlockAsInteger(blockpos));

	redisReply *reply = (redisReply *)redisCommand(ctx, "HSET %s %s %b",
			hash.c_str(), tmp.c_str(), data.c_str(), data.size());
	if (!reply) {
		errorstream << "WARNING: saveBlock: redis command 'HSET' failed on "
			"block " << PP(blockpos) << ": " << ctx->errstr << std::endl;
		freeReplyObject(reply);
		return false;
	}

	if (reply->type == REDIS_REPLY_ERROR) {
		errorstream << "WARNING: saveBlock: saving block " << PP(blockpos)
			<< "failed" << std::endl;
		freeReplyObject(reply);
		return false;
	}

	freeReplyObject(reply);
	return true;
}

std::string Database_Redis::loadBlock(v3s16 blockpos)
{
	std::string tmp = i64tos(getBlockAsInteger(blockpos));
	redisReply *reply;
	reply = (redisReply*) redisCommand(ctx, "HGET %s %s", hash.c_str(), tmp.c_str());

	if(!reply)
		throw FileNotGoodException(std::string("redis command 'HGET %s %s' failed: ") + ctx->errstr);
	if(reply->type != REDIS_REPLY_STRING)
		return "";

	std::string str(reply->str, reply->len);
	freeReplyObject(reply); // std::string copies the memory so this won't cause any problems
	return str;
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

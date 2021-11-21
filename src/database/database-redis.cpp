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

#include "database-redis.h"

#include "settings.h"
#include "log.h"
#include "exceptions.h"
#include "util/string.h"

#include <hiredis.h>
#include <cassert>


Database_Redis::Database_Redis(Settings &conf)
{
	std::string tmp;
	try {
		tmp = conf.get("redis_address");
		hash = conf.get("redis_hash");
	} catch (SettingNotFoundException &) {
		throw SettingNotFoundException("Set redis_address and "
			"redis_hash in world.mt to use the redis backend");
	}
	const char *addr = tmp.c_str();
	int port = conf.exists("redis_port") ? conf.getU16("redis_port") : 6379;
	// if redis_address contains '/' assume unix socket, else hostname/ip
	ctx = tmp.find('/') != std::string::npos ? redisConnectUnix(addr) : redisConnect(addr, port);
	if (!ctx) {
		throw DatabaseException("Cannot allocate redis context");
	} else if (ctx->err) {
		std::string err = std::string("Connection error: ") + ctx->errstr;
		redisFree(ctx);
		throw DatabaseException(err);
	}
	if (conf.exists("redis_password")) {
		tmp = conf.get("redis_password");
		redisReply *reply = static_cast<redisReply *>(redisCommand(ctx, "AUTH %s", tmp.c_str()));
		if (!reply)
			throw DatabaseException("Redis authentication failed");
		if (reply->type == REDIS_REPLY_ERROR) {
			std::string err = "Redis authentication failed: " + std::string(reply->str, reply->len);
			freeReplyObject(reply);
			throw DatabaseException(err);
		}
		freeReplyObject(reply);
	}
}

Database_Redis::~Database_Redis()
{
	redisFree(ctx);
}

void Database_Redis::beginSave() {
	redisReply *reply = static_cast<redisReply *>(redisCommand(ctx, "MULTI"));
	if (!reply) {
		throw DatabaseException(std::string(
			"Redis command 'MULTI' failed: ") + ctx->errstr);
	}
	freeReplyObject(reply);
}

void Database_Redis::endSave() {
	redisReply *reply = static_cast<redisReply *>(redisCommand(ctx, "EXEC"));
	if (!reply) {
		throw DatabaseException(std::string(
			"Redis command 'EXEC' failed: ") + ctx->errstr);
	}
	freeReplyObject(reply);
}

bool Database_Redis::saveBlock(const v3s16 &pos, const std::string &data)
{
	std::string tmp = i64tos(getBlockAsInteger(pos));

	redisReply *reply = static_cast<redisReply *>(redisCommand(ctx, "HSET %s %s %b",
			hash.c_str(), tmp.c_str(), data.c_str(), data.size()));
	if (!reply) {
		warningstream << "saveBlock: redis command 'HSET' failed on "
			"block " << PP(pos) << ": " << ctx->errstr << std::endl;
		freeReplyObject(reply);
		return false;
	}

	if (reply->type == REDIS_REPLY_ERROR) {
		warningstream << "saveBlock: saving block " << PP(pos)
			<< " failed: " << std::string(reply->str, reply->len) << std::endl;
		freeReplyObject(reply);
		return false;
	}

	freeReplyObject(reply);
	return true;
}

void Database_Redis::loadBlock(const v3s16 &pos, std::string *block)
{
	std::string tmp = i64tos(getBlockAsInteger(pos));
	redisReply *reply = static_cast<redisReply *>(redisCommand(ctx,
			"HGET %s %s", hash.c_str(), tmp.c_str()));

	if (!reply) {
		throw DatabaseException(std::string(
			"Redis command 'HGET %s %s' failed: ") + ctx->errstr);
	}

	switch (reply->type) {
	case REDIS_REPLY_STRING: {
		block->assign(reply->str, reply->len);
		freeReplyObject(reply);
		return;
	}
	case REDIS_REPLY_ERROR: {
		std::string errstr(reply->str, reply->len);
		freeReplyObject(reply);
		errorstream << "loadBlock: loading block " << PP(pos)
			<< " failed: " << errstr << std::endl;
		throw DatabaseException(std::string(
			"Redis command 'HGET %s %s' errored: ") + errstr);
	}
	case REDIS_REPLY_NIL: {
		block->clear();
		freeReplyObject(reply);
		return;
	}
	}

	errorstream << "loadBlock: loading block " << PP(pos)
		<< " returned invalid reply type " << reply->type
		<< ": " << std::string(reply->str, reply->len) << std::endl;
	freeReplyObject(reply);
	throw DatabaseException(std::string(
		"Redis command 'HGET %s %s' gave invalid reply."));
}

bool Database_Redis::deleteBlock(const v3s16 &pos)
{
	std::string tmp = i64tos(getBlockAsInteger(pos));

	redisReply *reply = static_cast<redisReply *>(redisCommand(ctx,
		"HDEL %s %s", hash.c_str(), tmp.c_str()));
	if (!reply) {
		throw DatabaseException(std::string(
			"Redis command 'HDEL %s %s' failed: ") + ctx->errstr);
	} else if (reply->type == REDIS_REPLY_ERROR) {
		warningstream << "deleteBlock: deleting block " << PP(pos)
			<< " failed: " << std::string(reply->str, reply->len) << std::endl;
		freeReplyObject(reply);
		return false;
	}

	freeReplyObject(reply);
	return true;
}

void Database_Redis::listAllLoadableBlocks(std::vector<v3s16> &dst)
{
	redisReply *reply = static_cast<redisReply *>(redisCommand(ctx, "HKEYS %s", hash.c_str()));
	if (!reply) {
		throw DatabaseException(std::string(
			"Redis command 'HKEYS %s' failed: ") + ctx->errstr);
	}
	switch (reply->type) {
	case REDIS_REPLY_ARRAY:
		dst.reserve(reply->elements);
		for (size_t i = 0; i < reply->elements; i++) {
			assert(reply->element[i]->type == REDIS_REPLY_STRING);
			dst.push_back(getIntegerAsBlock(stoi64(reply->element[i]->str)));
		}
		break;
	case REDIS_REPLY_ERROR:
		throw DatabaseException(std::string(
			"Failed to get keys from database: ") +
			std::string(reply->str, reply->len));
	}
	freeReplyObject(reply);
}

#endif // USE_REDIS


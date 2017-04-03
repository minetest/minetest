/*
Copyright (C) 2016 Loic Blot <loic.blot@unix-experience.fr>

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

#if USE_POSTGRESQL

#include "database-postgresql.h"

#ifdef _WIN32
        #ifndef WIN32_LEAN_AND_MEAN
                #define WIN32_LEAN_AND_MEAN
        #endif
        // Without this some of the network functions are not found on mingw
        #ifndef _WIN32_WINNT
                #define _WIN32_WINNT 0x0501
        #endif
        #include <windows.h>
        #include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include "log.h"
#include "exceptions.h"
#include "settings.h"

Database_PostgreSQL::Database_PostgreSQL(const Settings &conf) :
	m_connect_string(""),
	m_conn(NULL),
	m_pgversion(0)
{
	if (!conf.getNoEx("pgsql_connection", m_connect_string)) {
		throw SettingNotFoundException(
			"Set pgsql_connection string in world.mt to "
			"use the postgresql backend\n"
			"Notes:\n"
			"pgsql_connection has the following form: \n"
			"\tpgsql_connection = host=127.0.0.1 port=5432 user=mt_user "
			"password=mt_password dbname=minetest_world\n"
			"mt_user should have CREATE TABLE, INSERT, SELECT, UPDATE and "
			"DELETE rights on the database.\n"
			"Don't create mt_user as a SUPERUSER!");
	}

	connectToDatabase();
}

Database_PostgreSQL::~Database_PostgreSQL()
{
	PQfinish(m_conn);
}

void Database_PostgreSQL::connectToDatabase()
{
	m_conn = PQconnectdb(m_connect_string.c_str());

	if (PQstatus(m_conn) != CONNECTION_OK) {
		throw DatabaseException(std::string(
			"PostgreSQL database error: ") +
			PQerrorMessage(m_conn));
	}

	m_pgversion = PQserverVersion(m_conn);

	/*
	* We are using UPSERT feature from PostgreSQL 9.5
	* to have the better performance where possible.
	*/
	if (m_pgversion < 90500) {
		warningstream << "Your PostgreSQL server lacks UPSERT "
			<< "support. Use version 9.5 or better if possible."
			<< std::endl;
	}

	infostream << "PostgreSQL Database: Version " << m_pgversion
			<< " Connection made." << std::endl;

	createDatabase();
	initStatements();
}

void Database_PostgreSQL::verifyDatabase()
{
	if (PQstatus(m_conn) == CONNECTION_OK)
		return;

	PQreset(m_conn);
	ping();
}

void Database_PostgreSQL::ping()
{
	if (PQping(m_connect_string.c_str()) != PQPING_OK) {
		throw DatabaseException(std::string(
			"PostgreSQL database error: ") +
			PQerrorMessage(m_conn));
	}
}

bool Database_PostgreSQL::initialized() const
{
	return (PQstatus(m_conn) == CONNECTION_OK);
}

void Database_PostgreSQL::initStatements()
{
	prepareStatement("read_block",
			"SELECT data FROM blocks "
			"WHERE posX = $1::int4 AND posY = $2::int4 AND "
			"posZ = $3::int4");

	if (m_pgversion < 90500) {
		prepareStatement("write_block_insert",
			"INSERT INTO blocks (posX, posY, posZ, data) SELECT "
			"$1::int4, $2::int4, $3::int4, $4::bytea "
			"WHERE NOT EXISTS (SELECT true FROM blocks "
			"WHERE posX = $1::int4 AND posY = $2::int4 AND "
			"posZ = $3::int4)");

		prepareStatement("write_block_update",
			"UPDATE blocks SET data = $4::bytea "
			"WHERE posX = $1::int4 AND posY = $2::int4 AND "
			"posZ = $3::int4");
	} else {
		prepareStatement("write_block",
			"INSERT INTO blocks (posX, posY, posZ, data) VALUES "
			"($1::int4, $2::int4, $3::int4, $4::bytea) "
			"ON CONFLICT ON CONSTRAINT blocks_pkey DO "
			"UPDATE SET data = $4::bytea");
	}

	prepareStatement("delete_block", "DELETE FROM blocks WHERE "
			"posX = $1::int4 AND posY = $2::int4 AND posZ = $3::int4");

	prepareStatement("list_all_loadable_blocks",
			"SELECT posX, posY, posZ FROM blocks");
}

PGresult *Database_PostgreSQL::checkResults(PGresult *result, bool clear)
{
	ExecStatusType statusType = PQresultStatus(result);

	switch (statusType) {
	case PGRES_COMMAND_OK:
	case PGRES_TUPLES_OK:
		break;
	case PGRES_FATAL_ERROR:
	default:
		throw DatabaseException(
			std::string("PostgreSQL database error: ") +
			PQresultErrorMessage(result));
	}

	if (clear)
		PQclear(result);

	return result;
}

void Database_PostgreSQL::createDatabase()
{
	PGresult *result = checkResults(PQexec(m_conn,
		"SELECT relname FROM pg_class WHERE relname='blocks';"),
		false);

	// If table doesn't exist, create it
	if (!PQntuples(result)) {
		static const char* dbcreate_sql = "CREATE TABLE blocks ("
			"posX INT NOT NULL,"
			"posY INT NOT NULL,"
			"posZ INT NOT NULL,"
			"data BYTEA,"
			"PRIMARY KEY (posX,posY,posZ)"
		");";
		checkResults(PQexec(m_conn, dbcreate_sql));
	}

	PQclear(result);

	infostream << "PostgreSQL: Game Database was inited." << std::endl;
}


void Database_PostgreSQL::beginSave()
{
	verifyDatabase();
	checkResults(PQexec(m_conn, "BEGIN;"));
}

void Database_PostgreSQL::endSave()
{
	checkResults(PQexec(m_conn, "COMMIT;"));
}

bool Database_PostgreSQL::saveBlock(const v3s16 &pos,
		const std::string &data)
{
	// Verify if we don't overflow the platform integer with the mapblock size
	if (data.size() > INT_MAX) {
		errorstream << "Database_PostgreSQL::saveBlock: Data truncation! "
				<< "data.size() over 0xFFFF (== " << data.size()
				<< ")" << std::endl;
		return false;
	}

	verifyDatabase();

	s32 x, y, z;
	x = htonl(pos.X);
	y = htonl(pos.Y);
	z = htonl(pos.Z);

	const void *args[] = { &x, &y, &z, data.c_str() };
	const int argLen[] = {
		sizeof(x), sizeof(y), sizeof(z), (int)data.size()
	};
	const int argFmt[] = { 1, 1, 1, 1 };

	if (m_pgversion < 90500) {
		execPrepared("write_block_update", ARRLEN(args), args, argLen, argFmt);
		execPrepared("write_block_insert", ARRLEN(args), args, argLen, argFmt);
	} else {
		execPrepared("write_block", ARRLEN(args), args, argLen, argFmt);
	}
	return true;
}

void Database_PostgreSQL::loadBlock(const v3s16 &pos,
		std::string *block)
{
	verifyDatabase();

	s32 x, y, z;
	x = htonl(pos.X);
	y = htonl(pos.Y);
	z = htonl(pos.Z);

	const void *args[] = { &x, &y, &z };
	const int argLen[] = { sizeof(x), sizeof(y), sizeof(z) };
	const int argFmt[] = { 1, 1, 1 };

	PGresult *results = execPrepared("read_block", ARRLEN(args), args,
			argLen, argFmt, false);

	*block = "";

	if (PQntuples(results)) {
		*block = std::string(PQgetvalue(results, 0, 0),
				PQgetlength(results, 0, 0));
	}

	PQclear(results);
}

bool Database_PostgreSQL::deleteBlock(const v3s16 &pos)
{
	verifyDatabase();

	s32 x, y, z;
	x = htonl(pos.X);
	y = htonl(pos.Y);
	z = htonl(pos.Z);

	const void *args[] = { &x, &y, &z };
	const int argLen[] = { sizeof(x), sizeof(y), sizeof(z) };
	const int argFmt[] = { 1, 1, 1 };

	execPrepared("delete_block", ARRLEN(args), args, argLen, argFmt);

	return true;
}

void Database_PostgreSQL::listAllLoadableBlocks(std::vector<v3s16> &dst)
{
	verifyDatabase();

	PGresult *results = execPrepared("list_all_loadable_blocks", 0,
			NULL, NULL, NULL, false, false);

	int numrows = PQntuples(results);

	for (int row = 0; row < numrows; ++row) {
		dst.push_back(pg_to_v3s16(results, 0, 0));
	}

	PQclear(results);
}

#endif // USE_POSTGRESQL

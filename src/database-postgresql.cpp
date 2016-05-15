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

#include "log.h"
#include "exceptions.h"
#include "settings.h"

/*
 * IMPORTANT NOTICE
 * Every conversion from non string to const char* values
 * should be done by using an intermediate std::string
 * buffer.
 * If you don't use this intermediate, the conversion
 * to std::string followed by conversion to const char*
 * could have bad issues, refering to bad memory areas.
 */

Database_PostgreSQL::Database_PostgreSQL(const Settings &conf):
	m_connect_string(""), m_conn(NULL), m_pgversion(0)
{
	try {
		m_connect_string = conf.get("pgsql_connection");
	} catch (SettingNotFoundException) {
		throw SettingNotFoundException("Set pgsql_connection string in world.mt to "
			"use the postgresql backend\n"
			"Notes:\n"
			"pgsql_connection has the following form: \n"
			"\tpgsql_connection = host=127.0.0.1 port=5432 user=mt_user password=mt_password dbname=minetest_amazingworld\n"
			"mt_user should have CREATE TABLE, INSERT, SELECT, UPDATE and DELETE rights on the database.\n"
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

	if (m_pgversion < 90500) {
		throw DatabaseException("PostgreSQL database error: "
			"Server version 9.5 or greater required.");
	}

	infostream << "PostgreSQL Database: Version " << m_pgversion << " Connection made." << std::endl;
}

void Database_PostgreSQL::verifyDatabase()
{
	if (PQstatus(m_conn) == CONNECTION_OK)
		return;

	PQreset(m_conn);
	ping();
}

bool Database_PostgreSQL::ping()
{
	if (PQping(m_connect_string.c_str()) != PQPING_OK) {
		throw DatabaseException(std::string(
			"PostgreSQL database error: ") +
			PQerrorMessage(m_conn));
	}
	return true;
}

bool Database_PostgreSQL::initialized() const
{
	return (PQstatus(m_conn) == CONNECTION_OK);
}

void Database_PostgreSQL::initStatements()
{
	prepareStatement("read_block",
			"SELECT block_data FROM blocks "
					"WHERE posX = $1::int AND posY = $2::int AND posZ = $3::int");

	prepareStatement("write_block",
			"INSERT INTO blocks (posX, posY, posZ, block_data) VALUES "
					"($1::int, $2::int, $3::int, $4::bytea) "
					"ON CONFLICT ON CONSTRAINT blocks_pkey DO UPDATE SET block_data = $4::bytea");

	prepareStatement("delete_block", "DELETE FROM blocks WHERE posX = $1::int, posY = $2::int, posZ = $3::int");

	prepareStatement("list_all_loadable_blocks", "SELECT posX, posY, posZ FROM blocks");
}

void Database_PostgreSQL::prepareStatement(const std::string &name, const std::string &sql)
{
	resultsCheck(PQprepare(m_conn, name.c_str(), sql.c_str(), 0, NULL));
}

PGresult* Database_PostgreSQL::resultsCheck(PGresult* result, bool clear /*= true*/)
{
	ExecStatusType statusType = PQresultStatus(result);

	if ((statusType != PGRES_COMMAND_OK && statusType != PGRES_TUPLES_OK) ||
		statusType == PGRES_FATAL_ERROR) {
		throw DatabaseException(std::string(
			"PostgreSQL database error: ") +
			PQresultErrorMessage(result));
	}

	if (clear)
		PQclear(result);

	return result;
}

void Database_PostgreSQL::createDatabase()
{
	PGresult *result = resultsCheck(PQexec(m_conn,
		"SELECT relname FROM pg_class WHERE relname='blocks';"),
		false);

	// If table doesn't exist, create it
	if (!PQntuples(result)) {
		const std::string dbcreate_sql = "CREATE TABLE blocks ("
			"posX INT NOT NULL,"
			"posY INT NOT NULL,"
			"posZ INT NOT NULL,"
			"block_data BYTEA,"
			"PRIMARY KEY (posX,posY,posZ)"
		");";
		resultsCheck(PQexec(m_conn, dbcreate_sql.c_str()));
	}

	PQclear(result);

	infostream << "PostgreSQL: Game Database was inited." << std::endl;
}


/**
 * @brief Database_PostgreSQL::beginSave
 *
 * Start a write transaction
 */
void Database_PostgreSQL::beginSave()
{
	verifyDatabase();
	resultsCheck(PQexec(m_conn, "BEGIN;"));
}

/**
 * @brief Database_PostgreSQL::endSave
 *
 * Commit current transaction to DB
 */
void Database_PostgreSQL::endSave()
{
	resultsCheck(PQexec(m_conn, "COMMIT;"));
}

/**
 * @brief Database_PostgreSQL::saveBlock
 * @param pos
 * @param data
 * @return always true
 *
 * Save a block to the database
 */
bool Database_PostgreSQL::saveBlock(const v3s16 &pos, const std::string &data)
{
	verifyDatabase();

	// This case is manually handled because we manipulate blob datas
	std::string x = itos(pos.X), y = itos(pos.Y), z = itos(pos.Z);
	const char *values[] = {x.c_str(), y.c_str(), z.c_str(), data.c_str()};
	const int valLen[] = {sizeof(int), sizeof(int), sizeof(int), (int)data.size()};
	const int valFormats[] = { 0, 0, 0, 1 };

	resultsCheck(PQexecPrepared(m_conn, "write_block", 4,
				values, valLen, valFormats, 1), true);
	return true;
}

/**
 * @brief Database_PostgreSQL::loadBlock
 * @param pos
 * @return block datas
 */
std::string Database_PostgreSQL::loadBlock(const v3s16 &pos)
{
	verifyDatabase();

	std::string x = itos(pos.X), y = itos(pos.Y), z = itos(pos.Z);
	const char *values[] = {x.c_str(), y.c_str(), z.c_str()};

	PGresult *results = execPrepared("read_block", 3, values, false, false);

	std::string data = "";

	if (PQntuples(results)) {
		size_t rd_s;
		unsigned char* rd = PQunescapeBytea((unsigned char*)PQgetvalue(results, 0, 0), &rd_s);
		data = std::string((char*)rd, rd_s);
		PQfreemem(rd);
	}

	PQclear(results);

	return data;
}

bool Database_PostgreSQL::deleteBlock(const v3s16 &pos)
{
	verifyDatabase();

	std::string x = itos(pos.X), y = itos(pos.Y), z = itos(pos.Z);
	const char *values[] = {x.c_str(), y.c_str(), z.c_str()};

	execPrepared("read_block", 3, values);

	return true;
}

void Database_PostgreSQL::listAllLoadableBlocks(std::vector<v3s16> &dst)
{
	verifyDatabase();

	PGresult *results = execPrepared("list_all_loadable_blocks", 0, NULL, false, false);
	int numrows = PQntuples(results);

	for (int row = 0; row < numrows; ++row) {
		dst.push_back(pg_to_v3s16(results, 0, 0));
	}

	PQclear(results);
}

#endif // USE_POSTGRESQL

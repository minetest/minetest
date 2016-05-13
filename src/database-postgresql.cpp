/*
Copyright (C) 2014 jjb, James Dornan <james at catch22 dot com>

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
#include "filesys.h"


Database_PostgreSQL::Database_PostgreSQL(Settings &conf)
{
	connectString(conf);
	dbconnect();
	createDatabase();
	prepare();
}


void Database_PostgreSQL::beginSave()
{
	verifyDatabase();
	resultsCheck(PQexec(m_conn, "BEGIN;"));
}


void Database_PostgreSQL::endSave()
{
	resultsCheck(PQexec(m_conn, "COMMIT;"));
}


bool Database_PostgreSQL::deleteBlock(const v3s16 &pos)
{
	execPrepared("delete", 3, pos);
	return true;
}

bool Database_PostgreSQL::saveBlock(const v3s16 &pos, const std::string &data)
{
	execPrepared("save", 4, pos, true, data);
	return true;
}


std::string Database_PostgreSQL::loadBlock(const v3s16 &pos)
{
	std::string data;
	PGresult *results = execPrepared("load", 3, pos, false, data);

	if (PQntuples(results)) {
		data = std::string(PQgetvalue(results, 0,0),
			PQgetlength(results, 0, 0));
	}

	PQclear(results);
	return data;
}


void Database_PostgreSQL::listAllLoadableBlocks(std::vector<v3s16> &dst)
{
	PGresult *results = resultsCheck(PQexec(m_conn,
		"SELECT x, y, z FROM blocks;"), false);

	int numrows = PQntuples(results);

	if (!numrows) {
		PQclear(results);
		return;
	}

	dst.reserve(numrows);

	for (int row = 0; row < numrows; row++) {
		dst.push_back(v3s16(
			atoi(PQgetvalue(results, row, 0)),
			atoi(PQgetvalue(results, row, 1)),
			atoi(PQgetvalue(results, row, 2))));
	}

	PQclear(results);
}


void Database_PostgreSQL::connectString(Settings &conf)
{
	if (conf.exists("pg_connection_info")) {
		conn_info = conf.get("pg_connection_info");
	} else {
		infostream << "PostgreSQL: pg_connection_info not found in"
			<< "world.mt file. Using default database name minetest."
			<< std::endl;
		conn_info = std::string("dbname=minetest");
	}

	if (conn_info.find("application_name") == std::string::npos)
		conn_info.append(" application_name=minetest");

	verbosestream << "PostgreSQL: connection info: " << conn_info
		<< std::endl;
}


void Database_PostgreSQL::dbconnect()
{
	m_conn = PQconnectdb(conn_info.c_str());

	if (PQstatus(m_conn) != CONNECTION_OK) {
		throw FileNotGoodException(std::string(
			"PostgreSQL database error: ") +
			PQerrorMessage(m_conn));
	}

	pg_version = PQserverVersion(m_conn);

	if (pg_version < 90300) {
		throw FileNotGoodException("PostgreSQL database error: "
			"Server version 9.3 or greater required.");
	}

	verbosestream << "PostgreSQL Database: Version " << pg_version
		<< " Connection made." << std::endl;
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
	if (PQping(conn_info.c_str()) != PQPING_OK) {
		throw FileNotGoodException(std::string(
			"PostgreSQL database error: ") +
			PQerrorMessage(m_conn));
	}
	return true;
}


bool Database_PostgreSQL::initialized()
{
	return (PQstatus(m_conn) == CONNECTION_OK);
}


void Database_PostgreSQL::prepare()
{
	if (pg_version < 90500) {
		prepareStatement("save", "WITH upsert AS "
			"(UPDATE blocks SET data=$4::bytea WHERE "
			"x = $1::int AND y = $2::int AND z = $3::int "
			" RETURNING *) "
			"INSERT INTO blocks (x, y, z, data) "
			"SELECT $1::int, $2::int, $3::int, $4::bytea "
			"WHERE NOT EXISTS (SELECT * FROM upsert)");
	} else {
		prepareStatement("save", "INSERT INTO blocks (x, y, z, data) "
			"VALUES($1::int, $2::int, $3::int, $4::bytea) ON "
			"CONFLICT UPDATE SET data = $4::bytea;");
	}
	prepareStatement("load",
		"SELECT data FROM blocks WHERE x = $1::int AND y = $2::int AND "
		"z = $3::int");
	prepareStatement("delete",
		"DELETE FROM blocks WHERE x = $1::int AND y = $2::int AND "
		"z = $3::int");

	verbosestream << "PostgreSQL: SQL statements prepared." << std::endl;
}


void Database_PostgreSQL::prepareStatement(std::string name, std::string sql)
{
	resultsCheck(PQprepare(m_conn, name.c_str(), sql.c_str(), 0, NULL));
}


PGresult*  Database_PostgreSQL::resultsCheck(PGresult* result, bool clear /*= true*/)
{
	ExecStatusType statusType = PQresultStatus(result);

	if ((statusType != PGRES_COMMAND_OK && statusType != PGRES_TUPLES_OK) ||
		statusType == PGRES_FATAL_ERROR) {
		throw FileNotGoodException(std::string(
			"PostgreSQL database error: ") +
			PQresultErrorMessage(result));
	}

	if (clear)
		PQclear(result);

	return result;
}


PGresult* Database_PostgreSQL::execPrepared(std::string name, int nParams,
	const v3s16 &pos, bool clear /*= true*/,
	const std::string &data /*= ""*/)
{
	std::string x = itos(pos.X), y = itos(pos.Y), z = itos(pos.Z);
	const char *values[]  = {x.c_str(), y.c_str(), z.c_str(), data.c_str()};
	const int   lengths[] = {0, 0, 0, (int)data.size()};
	const int   formats[] = {0, 0, 0, 1};

	return resultsCheck(PQexecPrepared(m_conn, name.c_str(), nParams,
		values, lengths, formats, 1), clear);
}


void Database_PostgreSQL::createDatabase()
{
	PGresult *result = resultsCheck(PQexec(m_conn,
		"SELECT relname FROM pg_class WHERE relname='blocks';"),
		false);

	if (PQntuples(result)) {
		PQclear(result);
		return;
	}

	resultsCheck(PQexec(m_conn,
		"CREATE TABLE IF NOT EXISTS blocks (\n"
		"	x	INT NOT NULL,\n"
		"	y	INT NOT NULL,\n"
		"	z	INT NOT NULL,\n"
		"	data	BYTEA NOT NULL,\n"
		"	ctime	TIMESTAMP without time zone DEFAULT now(),\n"
		"	mtime	TIMESTAMP without time zone DEFAULT now(),\n"
		"	PRIMARY	KEY(x, y, z)\n"
		");"
	));

	resultsCheck(PQexec(m_conn,
		"CREATE OR REPLACE FUNCTION blocks_mtime() RETURNS trigger\n"
		"	LANGUAGE plpgsql\n"
		"	AS $$\n"
		"BEGIN\n"
		"	NEW.mtime = now();\n"
		"	RETURN NEW;\n"
		"END;\n"
		"$$;\n"
	));

	resultsCheck(PQexec(m_conn,
		"DROP TRIGGER IF EXISTS blocks_mtime ON blocks;\n"
		"CREATE TRIGGER blocks_mtime BEFORE UPDATE ON blocks "
		"FOR EACH ROW EXECUTE PROCEDURE blocks_mtime();\n"
	));

	verbosestream << "PostgreSQL: Database table, blocks, was created"
		<< std::endl;
}


Database_PostgreSQL::~Database_PostgreSQL()
{
	PQfinish(m_conn);
}

#endif

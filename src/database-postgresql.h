/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef DATABASE_POSTGRESQL_HEADER
#define DATABASE_POSTGRESQL_HEADER

#include <string>
#include <libpq-fe.h>
#include "database.h"
#include "util/basic_macros.h"

class Settings;

class Database_PostgreSQL : public Database
{
public:
	Database_PostgreSQL(const Settings &conf);
	~Database_PostgreSQL();

	void beginSave();
	void endSave();

	bool saveBlock(const v3s16 &pos, const std::string &data);
	void loadBlock(const v3s16 &pos, std::string *block);
	bool deleteBlock(const v3s16 &pos);
	void listAllLoadableBlocks(std::vector<v3s16> &dst);
	bool initialized() const;

private:
	// Database initialization
	void connectToDatabase();
	void initStatements();
	void createDatabase();

	inline void prepareStatement(const std::string &name, const std::string &sql)
	{
		checkResults(PQprepare(m_conn, name.c_str(), sql.c_str(), 0, NULL));
	}

	// Database connectivity checks
	void ping();
	void verifyDatabase();

	// Database usage
	PGresult *checkResults(PGresult *res, bool clear = true);

	inline PGresult *execPrepared(const char *stmtName, const int paramsNumber,
			const void **params,
			const int *paramsLengths = NULL, const int *paramsFormats = NULL,
			bool clear = true, bool nobinary = true)
	{
		return checkResults(PQexecPrepared(m_conn, stmtName, paramsNumber,
			(const char* const*) params, paramsLengths, paramsFormats,
			nobinary ? 1 : 0), clear);
	}

	// Conversion helpers
	inline int pg_to_int(PGresult *res, int row, int col)
	{
		return atoi(PQgetvalue(res, row, col));
	}

	inline v3s16 pg_to_v3s16(PGresult *res, int row, int col)
	{
		return v3s16(
			pg_to_int(res, row, col),
			pg_to_int(res, row, col + 1),
			pg_to_int(res, row, col + 2)
		);
	}

	// Attributes
	std::string m_connect_string;
	PGconn *m_conn;
	int m_pgversion;
};

#endif


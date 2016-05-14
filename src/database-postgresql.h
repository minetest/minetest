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

#include "database.h"
#include <string>
#include <libpq-fe.h>

class Settings;

class Database_PostgreSQL : public Database
{
public:
	Database_PostgreSQL(const Settings &conf);
	~Database_PostgreSQL();

	void beginSave();
	void endSave();

	bool saveBlock(const v3s16 &pos, const std::string &data);
	std::string loadBlock(const v3s16 &pos);
	bool deleteBlock(const v3s16 &pos);
	void listAllLoadableBlocks(std::vector<v3s16> &dst);
	bool initialized() const;

private:
	// Database initialization
	void connectToDatabase();
	void initStatements();
	void createDatabase();
	void prepareStatement(const std::string &name, const std::string &sql);

	// Database connectivity checks
	bool ping();
	void verifyDatabase();

	// Database usage
	PGresult* resultsCheck(PGresult* res, bool clear = true);

	inline PGresult* execPrepared(const char* stmtName, const int nbParams,
			const char** params, bool clear = true, bool nobinary = true)
	{
		return resultsCheck(PQexecPrepared(m_conn, stmtName, nbParams,
			params, NULL, NULL, nobinary ? 1 : 0), clear);
	}

	// Conversion helpers
	inline const int pg_to_int(PGresult *res, int row, int col) {
		return atoi(PQgetvalue(res, row, col));
	}

	inline const v3s16 pg_to_v3s16(PGresult *res, int row, int col) {
		return v3s16(
			pg_to_int(res, row, col),
			pg_to_int(res, row, col + 1),
			pg_to_int(res, row, col + 2)
		);
	}

	// Attributes
	std::string m_connect_string;
	PGconn* m_conn;
	int m_pgversion;
};

#endif


/*
Minetest
Copyright (C) 2019 BuckarooBanzai/naturefreshmilk, Thomas Rudin <thomas@rudin.io>

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
#pragma once

#include <libpq-fe.h>
#include "irr_v3d.h"
#include "exceptions.h"

namespace PGUtil {

	inline PGresult *checkResults(PGresult *result, bool clear = true)
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

	inline void prepareStatement(PGconn *m_conn, const std::string &name, const std::string &sql)
	{
		checkResults(PQprepare(m_conn, name.c_str(), sql.c_str(), 0, NULL));
	}

	inline PGresult *execPrepared(PGconn *m_conn, const char *stmtName, const int paramsNumber,
		const void **params,
		const int *paramsLengths = NULL, const int *paramsFormats = NULL,
		bool clear = true, bool nobinary = true) {

		return checkResults(PQexecPrepared(m_conn, stmtName, paramsNumber,
			(const char* const*) params, paramsLengths, paramsFormats,
			nobinary ? 1 : 0), clear);
	}

	inline PGresult *execPrepared(PGconn *m_conn, const char *stmtName, const int paramsNumber,
		const char **params, bool clear = true, bool nobinary = true) {
		return execPrepared(m_conn, stmtName, paramsNumber,
			(const void **)params, NULL, NULL, clear, nobinary);
	}

	// Conversion helpers
	inline int pg_to_int(PGresult *res, int row, int col)
	{
		return atoi(PQgetvalue(res, row, col));
	}

	inline u32 pg_to_uint(PGresult *res, int row, int col)
	{
		return (u32) atoi(PQgetvalue(res, row, col));
	}

	inline float pg_to_float(PGresult *res, int row, int col)
	{
		return (float) atof(PQgetvalue(res, row, col));
	}

	inline v3s16 pg_to_v3s16(PGresult *res, int row, int col)
	{
		return v3s16(
			pg_to_int(res, row, col),
			pg_to_int(res, row, col + 1),
			pg_to_int(res, row, col + 2)
		);
	}

}
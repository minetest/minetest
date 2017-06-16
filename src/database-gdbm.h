/*
 * GDBM database interface for Minetest
 *
Copyright (C) 2017 Maciej 'agaran' Pijanka <agaran@pld-linux.org>

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

#ifndef DATABASE_GDBM_HEADER
#define DATABASE_GDBM_HEADER

#include "database.h"
#include <string>
#include "util/string.h"
#include "util/serialize.h"

extern "C" {
	#include "gdbm.h"
}

class Database_Gdbm : public Database
{
public:
	Database_Gdbm(const std::string &savedir);
	~Database_Gdbm();

	bool saveBlock(const v3s16 &pos, const std::string &data);
	void loadBlock(const v3s16 &pos, std::string *block);
	bool deleteBlock(const v3s16 &pos);
	void listAllLoadableBlocks(std::vector<v3s16> &dst);

private:

	GDBM_FILE m_database;
};

#endif


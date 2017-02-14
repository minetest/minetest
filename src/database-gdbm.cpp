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

#include "config.h"

#if USE_GDBM
#include "database-gdbm.h"

#include "log.h"
#include "filesys.h"
#include "exceptions.h"
#include "settings.h"
#include "porting.h"
#include "util/string.h"

Database_Gdbm::Database_Gdbm(const std::string &savedir) :
	m_database(NULL)
{
	char buffer[512]; // or MAXPATH?
	std::string dbfile = savedir + DIR_DELIM + "map.gdbm";

	if (!fs::CreateAllDirs(savedir)) {
		infostream << "Database_GDBM: Failed to create directory \""
			<< savedir << "\"" << std::endl;
		throw FileNotGoodException("Failed to create database save "
				"directory \"" + savedir + "\"");
	}

	char *name = &buffer[0];
	memcpy(name, dbfile.c_str(), dbfile.length());
	name[dbfile.length()] = '\0';

	m_database = gdbm_open(name, 4096, GDBM_WRCREAT, 0644, NULL); // add fatal func someday?

	if (m_database == NULL) {
		throw DatabaseException(std::string("Failed to open GDBM database file ") + dbfile + ": " ); // use gdbm errno?
	}
}

bool Database_Gdbm::deleteBlock(const v3s16 &pos)
{
	datum key;
	char buffer[6];

	key.dsize = 6;
	key.dptr = &buffer[0];
	writeV3S16(reinterpret_cast<unsigned char*>(key.dptr),pos);

	if (gdbm_delete(m_database, key) != 0) {
		warningstream << "Database_GDBM: deleteBlock failed for " << PP(pos) << std::endl;
	}
	
	return true;
}

bool Database_Gdbm::saveBlock(const v3s16 &pos, const std::string &data)
{
	datum key, value;
	char buffer[6];

	key.dsize = 6;
	key.dptr = &buffer[0];
	writeV3S16(reinterpret_cast<unsigned char*>(key.dptr),pos);

	value.dptr = const_cast<char*>(data.c_str());
	value.dsize = data.length();

	if (gdbm_store(m_database, key, value, GDBM_REPLACE) != 0) {
		warningstream << "Database_GDBM: saveBlock failed for " << PP(pos) << std::endl;
		throw DatabaseException(std::string("Failed to save record in GDBM database.")); 
	}

	return true;
}

void Database_Gdbm::loadBlock(const v3s16 &pos, std::string *block)
{
	datum key,value;
	char buffer[6];

	key.dsize = 6;
	key.dptr = &buffer[0];
	writeV3S16(reinterpret_cast<unsigned char*>(key.dptr),pos);
	value = gdbm_fetch(m_database, key);
	*block = (value.dptr) ? std::string(value.dptr, value.dsize) : "";
	delete key.dptr;
}

// I am not entirelly sure about this one, but backend migrations do work well.
void Database_Gdbm::listAllLoadableBlocks(std::vector<v3s16> &dst)
{
	datum key;
	
	key = gdbm_firstkey (m_database);
	while (key.dptr) {
		datum value;

		if (key.dsize != 6)
			throw DatabaseException(std::string("Unexpected key size, database corrupted?"));

		dst.push_back(readV3S16(reinterpret_cast<unsigned char*>(key.dptr)));
		value = gdbm_nextkey (m_database, key);
		free (key.dptr);
		key = value;
	}
	free(key.dptr);
}

Database_Gdbm::~Database_Gdbm()
{
	gdbm_close(m_database);
	m_database = NULL;
}

#endif

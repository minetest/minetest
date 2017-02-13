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

#include <netinet/in.h>

#include <cassert>

Database_Gdbm::Database_Gdbm(const std::string &savedir) :
	m_savedir(savedir),
	m_database(NULL)
{
	std::string dbfile = m_savedir + DIR_DELIM + "map.gdbm";

	if (!fs::CreateAllDirs(m_savedir)) {
		infostream << "Database_GDBM: Failed to create directory \""
			<< m_savedir << "\"" << std::endl;
		throw FileNotGoodException("Failed to create database "
				"save directory");
	}

	char *name = (char *)malloc(dbfile.length()+1);
	memcpy(name, dbfile.c_str(), dbfile.length());
	name[dbfile.length()] = '\0';

	m_database = gdbm_open(name, 4096, GDBM_WRCREAT, 0644, NULL); // add fatal func someday?

	free(name);
	name = NULL;

	if (m_database == NULL) {
		throw DatabaseException(std::string("Failed to open GDBM database file ") + dbfile + ": " ); // use gdbm errno?
	}
}

// XXX Candidate for inlining?
void Database_Gdbm::v3s16_to_key(const v3s16 &pos, gdbm_entry &entry)
{
	assert(entry != NULL);

	entry.dkey.dpos.x = htons(pos.X);
	entry.dkey.dpos.y = htons(pos.Y);
	entry.dkey.dpos.z = htons(pos.Z);
	entry.key.dptr = entry.dkey.dblob;
	entry.key.dsize = 6;
	entry.value.dptr = NULL;
	entry.value.dsize = 0;
}

bool Database_Gdbm::deleteBlock(const v3s16 &pos)
{
	gdbm_entry entry;

	v3s16_to_key(pos, entry);

	if (gdbm_delete(m_database, entry.key) != 0) {
		warningstream << "Database_GDBM: Delete failed (" << pos.X << "," << pos.Y << "," << pos.Z << ")\"" << std::endl;
	}
	
	return true;
}

bool Database_Gdbm::saveBlock(const v3s16 &pos, const std::string &data)
{
	gdbm_entry entry;

	v3s16_to_key(pos, entry);

	entry.value.dptr = const_cast<char*>(data.c_str());
	entry.value.dsize = data.length();

	if (gdbm_store(m_database, entry.key, entry.value, GDBM_REPLACE) != 0) {
		// FIXME Do we need to log POS too?
		throw DatabaseException(std::string("Failed to save record in GDBM database.")); 
	}

	return true;
}

void Database_Gdbm::loadBlock(const v3s16 &pos, std::string *block)
{
	gdbm_entry entry;

	v3s16_to_key(pos, entry);

	entry.value = gdbm_fetch(m_database, entry.key);

	*block = (entry.value.dptr) ? std::string(entry.value.dptr, entry.value.dsize) : "";
}

// I am not entirelly sure about this one, but backend migrations do work well.
void Database_Gdbm::listAllLoadableBlocks(std::vector<v3s16> &dst)
{
	gdbm_entry entry;
	
	entry.key = gdbm_firstkey (m_database);

	while (entry.key.dptr) {
		if (entry.key.dsize != 6) {
			throw DatabaseException(std::string("Unexpected key size, database corrupted?");
		}

		memcpy(entry.dkey.dblob,entry.key.dptr,6);

		dst.push_back(v3s16(ntohs(entry.dkey.dpos.x),ntohs(entry.dkey.dpos.y),ntohs(entry.dkey.dpos.z)));

		entry.value = gdbm_nextkey (m_database, entry.key);

		free (entry.key.dptr);

		entry.key = entry.value;
	}
	free(entry.key.dptr);
}

Database_Gdbm::~Database_Gdbm()
{
	gdbm_close(m_database);
	m_database = false;
}

#endif

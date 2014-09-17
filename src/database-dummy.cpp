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

/*
Dummy "database" class
*/


#include "database-dummy.h"

#include "map.h"
#include "mapsector.h"
#include "mapblock.h"
#include "serialization.h"
#include "main.h"
#include "settings.h"
#include "log.h"

Database_Dummy::Database_Dummy(ServerMap *map)
{
	srvmap = map;
}

int Database_Dummy::Initialized(void)
{
	return 1;
}

void Database_Dummy::beginSave() {}
void Database_Dummy::endSave() {}

bool Database_Dummy::saveBlock(v3s16 blockpos, std::string &data)
{
	m_database[getBlockAsInteger(blockpos)] = data;
	return true;
}

std::string Database_Dummy::loadBlock(v3s16 blockpos)
{
	if (m_database.count(getBlockAsInteger(blockpos)))
		return m_database[getBlockAsInteger(blockpos)];
	else
		return "";
}

void Database_Dummy::listAllLoadableBlocks(std::list<v3s16> &dst)
{
	for(std::map<u64, std::string>::iterator x = m_database.begin(); x != m_database.end(); ++x)
	{
		v3s16 p = getIntegerAsBlock(x->first);
		//dstream<<"block_i="<<block_i<<" p="<<PP(p)<<std::endl;
		dst.push_back(p);
	}
}

Database_Dummy::~Database_Dummy()
{
	m_database.clear();
}

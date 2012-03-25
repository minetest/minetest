/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2012 Jonathan Neuschäfer <j.neuschaefer@gmx.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "filecache.h"
#include "clientserver.h"
#include "log.h"
#include "filesys.h"
#include "utility.h"
#include "hex.h"

#include <string>
#include <iostream>

bool FileCache::loadByPath(const std::string &path, std::ostream &os)
{
	std::ifstream fis(path.c_str(), std::ios_base::binary);

	if(!fis.good()){
		verbosestream<<"FileCache: File not found in cache: "
    			<<path<<std::endl;
		return false;
	}

	bool bad = false;
	for(;;){
		char buf[1024];
		fis.read(buf, 1024);
		std::streamsize len = fis.gcount();
		os.write(buf, len);
		if(fis.eof())
			break;
		if(!fis.good()){
			bad = true;
			break;
		}
	}
	if(bad){
		errorstream<<"FileCache: Failed to read file from cache: \""
		    	<<path<<"\""<<std::endl;
	}

	return !bad;
}

bool FileCache::updateByPath(const std::string &path, const std::string &data)
{
	std::ofstream file(path.c_str(), std::ios_base::binary |
			std::ios_base::trunc);

	if(!file.good())
	{
		errorstream<<"FileCache: Can't write to file at "
    			<<path<<std::endl;
		return false;
	}

	file.write(data.c_str(), data.length());
	file.close();

	return !file.fail();
}

bool FileCache::loadByName(const std::string &name, std::ostream &os)
{
	std::string path = m_dir + DIR_DELIM + name;
	return loadByPath(path, os);
}


bool FileCache::updateByName(const std::string &name, const std::string &data)
{
	std::string path = m_dir + DIR_DELIM + name;
	return updateByPath(path, data);
}

std::string FileCache::getPathFromChecksum(const std::string &checksum)
{
	std::string checksum_hex = hex_encode(checksum.c_str(), checksum.length());
	return m_dir + DIR_DELIM + checksum_hex;
}

bool FileCache::loadByChecksum(const std::string &checksum, std::ostream &os)
{
	std::string path = getPathFromChecksum(checksum);
	return loadByPath(path, os);
}

bool FileCache::updateByChecksum(const std::string &checksum,
		const std::string &data)
{
	std::string path = getPathFromChecksum(checksum);
	return updateByPath(path, data);
}

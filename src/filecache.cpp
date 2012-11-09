/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2012 Jonathan Neuschäfer <j.neuschaefer@gmx.net>

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

#include "filecache.h"

#include "clientserver.h"
#include "log.h"
#include "filesys.h"
#include "hex.h"
#include "sha1.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

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

bool FileCache::update(const std::string &name, const std::string &data)
{
	std::string path = m_dir + DIR_DELIM + name;
	return updateByPath(path, data);
}
bool FileCache::update_sha1(const std::string &data)
{
	SHA1 sha1;
	sha1.addBytes(data.c_str(), data.size());
	unsigned char *digest = sha1.getDigest();
	std::string sha1_raw((char*)digest, 20);
	free(digest);
	std::string sha1_hex = hex_encode(sha1_raw);
	return update(sha1_hex, data);
}
bool FileCache::load(const std::string &name, std::ostream &os)
{
	std::string path = m_dir + DIR_DELIM + name;
	return loadByPath(path, os);
}
bool FileCache::load_sha1(const std::string &sha1_raw, std::ostream &os)
{
	std::ostringstream tmp_os(std::ios_base::binary);
	if(!load(hex_encode(sha1_raw), tmp_os))
		return false;
	SHA1 sha1;
	sha1.addBytes(tmp_os.str().c_str(), tmp_os.str().length());
	unsigned char *digest = sha1.getDigest();
	std::string sha1_real_raw((char*)digest, 20);
	free(digest);
	if(sha1_real_raw != sha1_raw){
		verbosestream<<"FileCache["<<m_dir<<"]: filename "<<sha1_real_raw
				<<" mismatches actual checksum"<<std::endl;
		return false;
	}
	os<<tmp_os.str();
	return true;
}

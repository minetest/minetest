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

#ifndef FILECACHE_HEADER
#define FILECACHE_HEADER

#include <string>
#include <iostream>

class FileCache
{
public:
	/*
		'dir' is the file cache directory to use.
	*/
	FileCache(std::string dir):
		m_dir(dir)
	{
	}

	/*
		Searches the cache for a file with a given name.
		If the file is found, lookup copies it into 'os' and
		returns true. Otherwise false is returned.
	*/
	bool loadByName(const std::string &name, std::ostream &os);

	/*
		Stores a file in the cache based on its name.
		Returns true on success, false otherwise.
	*/
	bool updateByName(const std::string &name, const std::string &data);

	/*
		Loads a file based on a check sum, which may be any kind of
		rather unique byte sequence. Returns true, if the file could
		be written into os, false otherwise.
		A file name is required to give the disk file a name that
		has the right file name extension (e.g. ".png").
	*/
	bool loadByChecksum(const std::string &name, std::ostream &os,
			const std::string &checksum);

	/*
		Stores a file in the cache based on its checksum.
		Returns true on success, false otherwise.
	*/
	bool updateByChecksum(const std::string &name, const std::string &data,
			const std::string &checksum);

private:
	std::string m_dir;

	bool loadByPath(const std::string &name, std::ostream &os,
			const std::string &path);
	bool updateByPath(const std::string &name, const std::string &data,
			const std::string &path);
	std::string getPathFromChecksum(const std::string &name,
			const std::string &checksum);
};

#endif

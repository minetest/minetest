/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013 Jonathan Neuschäfer <j.neuschaefer@gmx.net>

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

#include <iostream>
#include <string>

class FileCache
{
public:
	/*
		'dir' is the file cache directory to use.
	*/
	FileCache(const std::string &dir) : m_dir(dir) {}

	bool update(const std::string &name, const std::string &data);
	bool load(const std::string &name, std::ostream &os);
	bool exists(const std::string &name);

private:
	std::string m_dir;

	bool loadByPath(const std::string &path, std::ostream &os);
	bool updateByPath(const std::string &path, const std::string &data);
};

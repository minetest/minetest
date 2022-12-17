/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2022 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

#include "mod_vfs.h"
#include "filesys.h"
#include "log.h"
#include <algorithm>

void ModVFS::scanModSubfolder(const std::string &mod_name, const std::string &mod_path,
		std::string mod_subpath)
{
	std::string full_path = mod_path + DIR_DELIM + mod_subpath;
	std::vector<fs::DirListNode> mod = fs::GetDirListing(full_path);
	for (const fs::DirListNode &j : mod) {
		if (j.name[0] == '.')
			continue;

		if (j.dir) {
			scanModSubfolder(mod_name, mod_path, mod_subpath + j.name + DIR_DELIM);
			continue;
		}
		std::replace(mod_subpath.begin(), mod_subpath.end(), DIR_DELIM_CHAR, '/');

		std::string real_path = full_path + j.name;
		std::string vfs_path = mod_name + ":" + mod_subpath + j.name;
		infostream << "ModVFS::scanModSubfolder(): Loading \"" << real_path
				<< "\" as \"" << vfs_path << "\"." << std::endl;

		std::string contents;
		if (!fs::ReadFile(real_path, contents)) {
			errorstream << "ModVFS::scanModSubfolder(): Can't read file \""
					<< real_path << "\"." << std::endl;
			continue;
		}

		m_vfs.emplace(vfs_path, contents);
	}
}

const std::string *ModVFS::getModFile(std::string filename)
{
	// strip dir delimiter from beginning of path
	auto pos = filename.find_first_of(':');
	if (pos == std::string::npos)
		return nullptr;
	pos++;
	auto pos2 = filename.find_first_not_of('/', pos);
	if (pos2 > pos)
		filename.erase(pos, pos2 - pos);

	StringMap::const_iterator it = m_vfs.find(filename);
	if (it == m_vfs.end())
		return nullptr;
	return &it->second;
}

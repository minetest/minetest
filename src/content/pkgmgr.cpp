/*
Minetest
Copyright (C) 2024 Hyland B. (swagtoy) <me@swag.toys>

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

#include <regex>
#include "content.h"
#include "filesys.h"
// TODO move the non-lua related method elsewhere?
#include "script/lua_api/l_mainmenu.h"
#include "../util/string.h"
#include "pkgmgr.h"

using namespace content;

bool content::PkgMgr::isValidModname(const std::string& str)
{
	std::smatch m;
	return ! std::regex_match(str, m, std::regex{"[^a-z0-9_]"});
}

std::string content::PkgMgr::getContentdbId(const ContentSpec& content)
{
	if (content.author != "" and content.release > 0)
	{
		if (content.type == "game")
			return lowercase(content.author) + DIR_DELIM + content.id;
		
		return lowercase(content.author) + DIR_DELIM + content.name;
	}
	
	/* Until Minetest 5.8.0, Minetest Game was bundled with Minetest.
	 * Unfortunately, the bundled MTG was not versioned (missing "release"
	 * field in game.conf).
	 * Therefore, we consider any installation of MTG that is not versioned,
	 * has not been cloned from Git, and is not system-wide to be updatable.
	 */
	if (content.type == "game" &&
	    content.id == "minetest" && 
	    content.release == 0 &&
		! fs::IsDir(content.path + DIR_DELIM + ".git") &&
		ModApiMainMenu::mayModifyPath(
	       fs::RemoveRelativePathComponents(content.path)))
	{
		return "minetest/minetest";
	}
	
	return {};
		
}

/*
Minetest
Copyright (C) 2018 rubenwardy <rw@rubenwardy.com>

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

#include <fstream>
#include "content/content.h"
#include "content/subgames.h"
#include "content/mods.h"
#include "filesys.h"
#include "settings.h"

ContentType getContentType(const std::string &path)
{
	if (fs::IsFile(path + DIR_DELIM "modpack.txt") || fs::IsFile(path + DIR_DELIM "modpack.conf"))
		return ContentType::MODPACK;

	if (fs::IsFile(path + DIR_DELIM "init.lua"))
		return ContentType::MOD;

	if (fs::IsFile(path + DIR_DELIM "game.conf"))
		return ContentType::GAME;

	if (fs::IsFile(path + DIR_DELIM "texture_pack.conf"))
		return ContentType::TXP;

	return ContentType::UNKNOWN;
}

void parseContentInfo(ContentSpec &spec)
{
	std::string conf_path;

	switch (getContentType(spec.path)) {
	case ContentType::MOD:
		spec.type = "mod";
		conf_path = spec.path + DIR_DELIM "mod.conf";
		break;
	case ContentType::MODPACK:
		spec.type = "modpack";
		conf_path = spec.path + DIR_DELIM "modpack.conf";
		break;
	case ContentType::GAME:
		spec.type = "game";
		conf_path = spec.path + DIR_DELIM "game.conf";
		break;
	case ContentType::TXP:
		spec.type = "txp";
		conf_path = spec.path + DIR_DELIM "texture_pack.conf";
		break;
	default:
		spec.type = "unknown";
		break;
	}

	Settings conf;
	if (!conf_path.empty() && conf.readConfigFile(conf_path.c_str())) {
		if (conf.exists("title"))
			spec.title = conf.get("title");
		else if (spec.type == "game" && conf.exists("name"))
			spec.title = conf.get("name");

		if (spec.type != "game" && conf.exists("name"))
			spec.name = conf.get("name");

		if (conf.exists("title"))
			spec.title = conf.get("title");

		if (spec.type == "game") {
			if (spec.title.empty())
				spec.title = spec.name;
			spec.name = "";
		}

		if (conf.exists("description"))
			spec.desc = conf.get("description");

		if (conf.exists("author"))
			spec.author = conf.get("author");

		if (conf.exists("release"))
			spec.release = conf.getS32("release");

		if (conf.exists("textdomain"))
			spec.textdomain = conf.get("textdomain");
	}

	if (spec.name.empty())
		spec.name = fs::GetFilenameFromPath(spec.path.c_str());

	if (spec.textdomain.empty())
		spec.textdomain = spec.name;

	if (spec.desc.empty()) {
		fs::ReadFile(spec.path + DIR_DELIM + "description.txt", spec.desc);
	}
}

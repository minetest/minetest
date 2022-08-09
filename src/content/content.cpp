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

enum ContentType
{
	ECT_UNKNOWN,
	ECT_MOD,
	ECT_MODPACK,
	ECT_GAME,
	ECT_TXP
};

ContentType getContentType(const ContentSpec &spec)
{
	std::ifstream modpack_is((spec.path + DIR_DELIM + "modpack.txt").c_str());
	if (modpack_is.good()) {
		modpack_is.close();
		return ECT_MODPACK;
	}

	std::ifstream modpack2_is((spec.path + DIR_DELIM + "modpack.conf").c_str());
	if (modpack2_is.good()) {
		modpack2_is.close();
		return ECT_MODPACK;
	}

	std::ifstream init_is((spec.path + DIR_DELIM + "init.lua").c_str());
	if (init_is.good()) {
		init_is.close();
		return ECT_MOD;
	}

	std::ifstream game_is((spec.path + DIR_DELIM + "game.conf").c_str());
	if (game_is.good()) {
		game_is.close();
		return ECT_GAME;
	}

	std::ifstream txp_is((spec.path + DIR_DELIM + "texture_pack.conf").c_str());
	if (txp_is.good()) {
		txp_is.close();
		return ECT_TXP;
	}

	return ECT_UNKNOWN;
}

void parseContentInfo(ContentSpec &spec)
{
	std::string conf_path;

	switch (getContentType(spec)) {
	case ECT_MOD:
		spec.type = "mod";
		conf_path = spec.path + DIR_DELIM + "mod.conf";
		break;
	case ECT_MODPACK:
		spec.type = "modpack";
		conf_path = spec.path + DIR_DELIM + "modpack.conf";
		break;
	case ECT_GAME:
		spec.type = "game";
		conf_path = spec.path + DIR_DELIM + "game.conf";
		break;
	case ECT_TXP:
		spec.type = "txp";
		conf_path = spec.path + DIR_DELIM + "texture_pack.conf";
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

		if (conf.exists("description"))
			spec.desc = conf.get("description");

		if (conf.exists("author"))
			spec.author = conf.get("author");

		if (conf.exists("release"))
			spec.release = conf.getS32("release");
	}

	if (spec.desc.empty()) {
		std::ifstream is((spec.path + DIR_DELIM + "description.txt").c_str());
		spec.desc = std::string((std::istreambuf_iterator<char>(is)),
				std::istreambuf_iterator<char>());
	}
}

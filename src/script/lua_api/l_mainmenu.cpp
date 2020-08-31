/*
Minetest
Copyright (C) 2013 sapier

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

#include "lua_api/l_mainmenu.h"
#include "lua_api/l_internal.h"
#include "common/c_content.h"
#include "cpp_api/s_async.h"
#include "gui/guiEngine.h"
#include "gui/guiMainMenu.h"
#include "gui/guiKeyChangeMenu.h"
#include "gui/guiPathSelectMenu.h"
#include "version.h"
#include "porting.h"
#include "filesys.h"
#include "convert_json.h"
#include "content/packages.h"
#include "content/content.h"
#include "content/subgames.h"
#include "serverlist.h"
#include "mapgen/mapgen.h"
#include "settings.h"

#include <IFileArchive.h>
#include <IFileSystem.h>
#include "client/renderingengine.h"
#include "network/networkprotocol.h"


/******************************************************************************/
std::string ModApiMainMenu::getTextData(lua_State *L, std::string name)
{
	lua_getglobal(L, "gamedata");

	lua_getfield(L, -1, name.c_str());

	if(lua_isnil(L, -1))
		return "";

	return luaL_checkstring(L, -1);
}

/******************************************************************************/
int ModApiMainMenu::getIntegerData(lua_State *L, std::string name,bool& valid)
{
	lua_getglobal(L, "gamedata");

	lua_getfield(L, -1, name.c_str());

	if(lua_isnil(L, -1)) {
		valid = false;
		return -1;
		}

	valid = true;
	return luaL_checkinteger(L, -1);
}

/******************************************************************************/
int ModApiMainMenu::getBoolData(lua_State *L, std::string name,bool& valid)
{
	lua_getglobal(L, "gamedata");

	lua_getfield(L, -1, name.c_str());

	if(lua_isnil(L, -1)) {
		valid = false;
		return false;
		}

	valid = true;
	return readParam<bool>(L, -1);
}

/******************************************************************************/
int ModApiMainMenu::l_update_formspec(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	sanity_check(engine != NULL);

	if (engine->m_startgame)
		return 0;

	//read formspec
	std::string formspec(luaL_checkstring(L, 1));

	if (engine->m_formspecgui != 0) {
		engine->m_formspecgui->setForm(formspec);
	}

	return 0;
}

/******************************************************************************/
int ModApiMainMenu::l_set_formspec_prepend(lua_State *L)
{
	GUIEngine *engine = getGuiEngine(L);
	sanity_check(engine != NULL);

	if (engine->m_startgame)
		return 0;

	std::string formspec(luaL_checkstring(L, 1));
	engine->setFormspecPrepend(formspec);

	return 0;
}

/******************************************************************************/
int ModApiMainMenu::l_start(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	sanity_check(engine != NULL);

	//update c++ gamedata from lua table

	bool valid = false;

	MainMenuData *data = engine->m_data;

	data->selected_world = getIntegerData(L, "selected_world",valid) -1;
	data->simple_singleplayer_mode = getBoolData(L,"singleplayer",valid);
	data->do_reconnect = getBoolData(L, "do_reconnect", valid);
	if (!data->do_reconnect) {
		data->name     = getTextData(L,"playername");
		data->password = getTextData(L,"password");
		data->address  = getTextData(L,"address");
		data->port     = getTextData(L,"port");
	}
	data->serverdescription = getTextData(L,"serverdescription");
	data->servername        = getTextData(L,"servername");

	//close menu next time
	engine->m_startgame = true;
	return 0;
}

/******************************************************************************/
int ModApiMainMenu::l_close(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	sanity_check(engine != NULL);

	engine->m_kill = true;
	return 0;
}

/******************************************************************************/
int ModApiMainMenu::l_set_background(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	sanity_check(engine != NULL);

	std::string backgroundlevel(luaL_checkstring(L, 1));
	std::string texturename(luaL_checkstring(L, 2));

	bool tile_image = false;
	bool retval     = false;
	unsigned int minsize = 16;

	if (!lua_isnone(L, 3)) {
		tile_image = readParam<bool>(L, 3);
	}

	if (!lua_isnone(L, 4)) {
		minsize = lua_tonumber(L, 4);
	}

	if (backgroundlevel == "background") {
		retval |= engine->setTexture(TEX_LAYER_BACKGROUND, texturename,
				tile_image, minsize);
	}

	if (backgroundlevel == "overlay") {
		retval |= engine->setTexture(TEX_LAYER_OVERLAY, texturename,
				tile_image, minsize);
	}

	if (backgroundlevel == "header") {
		retval |= engine->setTexture(TEX_LAYER_HEADER,  texturename,
				tile_image, minsize);
	}

	if (backgroundlevel == "footer") {
		retval |= engine->setTexture(TEX_LAYER_FOOTER, texturename,
				tile_image, minsize);
	}

	lua_pushboolean(L,retval);
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_set_clouds(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	sanity_check(engine != NULL);

	bool value = readParam<bool>(L,1);

	engine->m_clouds_enabled = value;

	return 0;
}

/******************************************************************************/
int ModApiMainMenu::l_get_textlist_index(lua_State *L)
{
	// get_table_index accepts both tables and textlists
	return l_get_table_index(L);
}

/******************************************************************************/
int ModApiMainMenu::l_get_table_index(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	sanity_check(engine != NULL);

	std::string tablename(luaL_checkstring(L, 1));
	GUITable *table = engine->m_menu->getTable(tablename);
	s32 selection = table ? table->getSelected() : 0;

	if (selection >= 1)
		lua_pushinteger(L, selection);
	else
		lua_pushnil(L);
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_get_worlds(lua_State *L)
{
	std::vector<WorldSpec> worlds = getAvailableWorlds();

	lua_newtable(L);
	int top = lua_gettop(L);
	unsigned int index = 1;

	for (const WorldSpec &world : worlds) {
		lua_pushnumber(L,index);

		lua_newtable(L);
		int top_lvl2 = lua_gettop(L);

		lua_pushstring(L,"path");
		lua_pushstring(L, world.path.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L,"name");
		lua_pushstring(L, world.name.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L,"gameid");
		lua_pushstring(L, world.gameid.c_str());
		lua_settable(L, top_lvl2);

		lua_settable(L, top);
		index++;
	}
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_get_favorites(lua_State *L)
{
	std::string listtype = "local";

	if (!lua_isnone(L, 1)) {
		listtype = luaL_checkstring(L, 1);
	}

	std::vector<ServerListSpec> servers;

	if(listtype == "online") {
		servers = ServerList::getOnline();
	} else {
		servers = ServerList::getLocal();
	}

	lua_newtable(L);
	int top = lua_gettop(L);
	unsigned int index = 1;

	for (const Json::Value &server : servers) {

		lua_pushnumber(L, index);

		lua_newtable(L);
		int top_lvl2 = lua_gettop(L);

		if (!server["clients"].asString().empty()) {
			std::string clients_raw = server["clients"].asString();
			char* endptr = 0;
			int numbervalue = strtol(clients_raw.c_str(), &endptr,10);

			if ((!clients_raw.empty()) && (*endptr == 0)) {
				lua_pushstring(L, "clients");
				lua_pushnumber(L, numbervalue);
				lua_settable(L, top_lvl2);
			}
		}

		if (!server["clients_max"].asString().empty()) {

			std::string clients_max_raw = server["clients_max"].asString();
			char* endptr = 0;
			int numbervalue = strtol(clients_max_raw.c_str(), &endptr,10);

			if ((!clients_max_raw.empty()) && (*endptr == 0)) {
				lua_pushstring(L, "clients_max");
				lua_pushnumber(L, numbervalue);
				lua_settable(L, top_lvl2);
			}
		}

		if (!server["version"].asString().empty()) {
			lua_pushstring(L, "version");
			std::string topush = server["version"].asString();
			lua_pushstring(L, topush.c_str());
			lua_settable(L, top_lvl2);
		}

		if (!server["proto_min"].asString().empty()) {
			lua_pushstring(L, "proto_min");
			lua_pushinteger(L, server["proto_min"].asInt());
			lua_settable(L, top_lvl2);
		}

		if (!server["proto_max"].asString().empty()) {
			lua_pushstring(L, "proto_max");
			lua_pushinteger(L, server["proto_max"].asInt());
			lua_settable(L, top_lvl2);
		}

		if (!server["password"].asString().empty()) {
			lua_pushstring(L, "password");
			lua_pushboolean(L, server["password"].asBool());
			lua_settable(L, top_lvl2);
		}

		if (!server["creative"].asString().empty()) {
			lua_pushstring(L, "creative");
			lua_pushboolean(L, server["creative"].asBool());
			lua_settable(L, top_lvl2);
		}

		if (!server["damage"].asString().empty()) {
			lua_pushstring(L, "damage");
			lua_pushboolean(L, server["damage"].asBool());
			lua_settable(L, top_lvl2);
		}

		if (!server["pvp"].asString().empty()) {
			lua_pushstring(L, "pvp");
			lua_pushboolean(L, server["pvp"].asBool());
			lua_settable(L, top_lvl2);
		}

		if (!server["description"].asString().empty()) {
			lua_pushstring(L, "description");
			std::string topush = server["description"].asString();
			lua_pushstring(L, topush.c_str());
			lua_settable(L, top_lvl2);
		}

		if (!server["name"].asString().empty()) {
			lua_pushstring(L, "name");
			std::string topush = server["name"].asString();
			lua_pushstring(L, topush.c_str());
			lua_settable(L, top_lvl2);
		}

		if (!server["address"].asString().empty()) {
			lua_pushstring(L, "address");
			std::string topush = server["address"].asString();
			lua_pushstring(L, topush.c_str());
			lua_settable(L, top_lvl2);
		}

		if (!server["port"].asString().empty()) {
			lua_pushstring(L, "port");
			std::string topush = server["port"].asString();
			lua_pushstring(L, topush.c_str());
			lua_settable(L, top_lvl2);
		}

		if (server.isMember("ping")) {
			float ping = server["ping"].asFloat();
			lua_pushstring(L, "ping");
			lua_pushnumber(L, ping);
			lua_settable(L, top_lvl2);
		}

		if (server["clients_list"].isArray()) {
			unsigned int index_lvl2 = 1;
			lua_pushstring(L, "clients_list");
			lua_newtable(L);
			int top_lvl3 = lua_gettop(L);
			for (const Json::Value &client : server["clients_list"]) {
				lua_pushnumber(L, index_lvl2);
				std::string topush = client.asString();
				lua_pushstring(L, topush.c_str());
				lua_settable(L, top_lvl3);
				index_lvl2++;
			}
			lua_settable(L, top_lvl2);
		}

		if (server["mods"].isArray()) {
			unsigned int index_lvl2 = 1;
			lua_pushstring(L, "mods");
			lua_newtable(L);
			int top_lvl3 = lua_gettop(L);
			for (const Json::Value &mod : server["mods"]) {

				lua_pushnumber(L, index_lvl2);
				std::string topush = mod.asString();
				lua_pushstring(L, topush.c_str());
				lua_settable(L, top_lvl3);
				index_lvl2++;
			}
			lua_settable(L, top_lvl2);
		}

		lua_settable(L, top);
		index++;
	}
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_delete_favorite(lua_State *L)
{
	std::vector<ServerListSpec> servers;

	std::string listtype = "local";

	if (!lua_isnone(L,2)) {
		listtype = luaL_checkstring(L,2);
	}

	if ((listtype != "local") &&
		(listtype != "online"))
		return 0;


	if(listtype == "online") {
		servers = ServerList::getOnline();
	} else {
		servers = ServerList::getLocal();
	}

	int fav_idx	= luaL_checkinteger(L,1) -1;

	if ((fav_idx >= 0) &&
			(fav_idx < (int) servers.size())) {

		ServerList::deleteEntry(servers[fav_idx]);
	}

	return 0;
}

/******************************************************************************/
int ModApiMainMenu::l_get_games(lua_State *L)
{
	std::vector<SubgameSpec> games = getAvailableGames();

	lua_newtable(L);
	int top = lua_gettop(L);
	unsigned int index = 1;

	for (const SubgameSpec &game : games) {
		lua_pushnumber(L, index);
		lua_newtable(L);
		int top_lvl2 = lua_gettop(L);

		lua_pushstring(L,  "id");
		lua_pushstring(L,  game.id.c_str());
		lua_settable(L,    top_lvl2);

		lua_pushstring(L,  "path");
		lua_pushstring(L,  game.path.c_str());
		lua_settable(L,    top_lvl2);

		lua_pushstring(L,  "type");
		lua_pushstring(L,  "game");
		lua_settable(L,    top_lvl2);

		lua_pushstring(L,  "gamemods_path");
		lua_pushstring(L,  game.gamemods_path.c_str());
		lua_settable(L,    top_lvl2);

		lua_pushstring(L,  "name");
		lua_pushstring(L,  game.name.c_str());
		lua_settable(L,    top_lvl2);

		lua_pushstring(L,  "author");
		lua_pushstring(L,  game.author.c_str());
		lua_settable(L,    top_lvl2);

		lua_pushstring(L,  "release");
		lua_pushinteger(L, game.release);
		lua_settable(L,    top_lvl2);

		lua_pushstring(L,  "menuicon_path");
		lua_pushstring(L,  game.menuicon_path.c_str());
		lua_settable(L,    top_lvl2);

		lua_pushstring(L, "addon_mods_paths");
		lua_newtable(L);
		int table2 = lua_gettop(L);
		int internal_index = 1;
		for (const std::string &addon_mods_path : game.addon_mods_paths) {
			lua_pushnumber(L, internal_index);
			lua_pushstring(L, addon_mods_path.c_str());
			lua_settable(L,   table2);
			internal_index++;
		}
		lua_settable(L, top_lvl2);
		lua_settable(L, top);
		index++;
	}
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_get_content_info(lua_State *L)
{
	std::string path = luaL_checkstring(L, 1);

	ContentSpec spec;
	spec.path = path;
	parseContentInfo(spec);

	lua_newtable(L);

	lua_pushstring(L, spec.name.c_str());
	lua_setfield(L, -2, "name");

	lua_pushstring(L, spec.type.c_str());
	lua_setfield(L, -2, "type");

	lua_pushstring(L, spec.author.c_str());
	lua_setfield(L, -2, "author");

	lua_pushinteger(L, spec.release);
	lua_setfield(L, -2, "release");

	lua_pushstring(L, spec.desc.c_str());
	lua_setfield(L, -2, "description");

	lua_pushstring(L, spec.path.c_str());
	lua_setfield(L, -2, "path");

	if (spec.type == "mod") {
		ModSpec spec;
		spec.path = path;
		parseModContents(spec);

		// Dependencies
		lua_newtable(L);
		int i = 1;
		for (const auto &dep : spec.depends) {
			lua_pushstring(L, dep.c_str());
			lua_rawseti(L, -2, i++);
		}
		lua_setfield(L, -2, "depends");

		// Optional Dependencies
		lua_newtable(L);
		i = 1;
		for (const auto &dep : spec.optdepends) {
			lua_pushstring(L, dep.c_str());
			lua_rawseti(L, -2, i++);
		}
		lua_setfield(L, -2, "optional_depends");
	}

	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_show_keys_menu(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	sanity_check(engine != NULL);

	GUIKeyChangeMenu *kmenu = new GUIKeyChangeMenu(RenderingEngine::get_gui_env(),
			engine->m_parent,
			-1,
			engine->m_menumanager,
			engine->m_texture_source);
	kmenu->drop();
	return 0;
}

/******************************************************************************/
int ModApiMainMenu::l_create_world(lua_State *L)
{
	const char *name	= luaL_checkstring(L, 1);
	int gameidx			= luaL_checkinteger(L,2) -1;

	std::string path = porting::path_user + DIR_DELIM
			"worlds" + DIR_DELIM
			+ sanitizeDirName(name, "world_");

	std::vector<SubgameSpec> games = getAvailableGames();

	if ((gameidx >= 0) &&
			(gameidx < (int) games.size())) {

		// Create world if it doesn't exist
		try {
			loadGameConfAndInitWorld(path, name, games[gameidx], true);
			lua_pushnil(L);
		} catch (const BaseException &e) {
			lua_pushstring(L, (std::string("Failed to initialize world: ") + e.what()).c_str());
		}
	} else {
		lua_pushstring(L, "Invalid game index");
	}
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_delete_world(lua_State *L)
{
	int world_id = luaL_checkinteger(L, 1) - 1;
	std::vector<WorldSpec> worlds = getAvailableWorlds();
	if (world_id < 0 || world_id >= (int) worlds.size()) {
		lua_pushstring(L, "Invalid world index");
		return 1;
	}
	const WorldSpec &spec = worlds[world_id];
	if (!fs::RecursiveDelete(spec.path)) {
		lua_pushstring(L, "Failed to delete world");
		return 1;
	}
	return 0;
}

/******************************************************************************/
int ModApiMainMenu::l_set_topleft_text(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	sanity_check(engine != NULL);

	std::string text;

	if (!lua_isnone(L,1) &&	!lua_isnil(L,1))
		text = luaL_checkstring(L, 1);

	engine->setTopleftText(text);
	return 0;
}

/******************************************************************************/
int ModApiMainMenu::l_get_mapgen_names(lua_State *L)
{
	std::vector<const char *> names;
	bool include_hidden = lua_isboolean(L, 1) && readParam<bool>(L, 1);
	Mapgen::getMapgenNames(&names, include_hidden);

	lua_newtable(L);
	for (size_t i = 0; i != names.size(); i++) {
		lua_pushstring(L, names[i]);
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}


/******************************************************************************/
int ModApiMainMenu::l_get_modpath(lua_State *L)
{
	std::string modpath = fs::RemoveRelativePathComponents(
		porting::path_user + DIR_DELIM + "mods" + DIR_DELIM);
	lua_pushstring(L, modpath.c_str());
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_get_clientmodpath(lua_State *L)
{
	std::string modpath = fs::RemoveRelativePathComponents(
		porting::path_user + DIR_DELIM + "clientmods" + DIR_DELIM);
	lua_pushstring(L, modpath.c_str());
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_get_gamepath(lua_State *L)
{
	std::string gamepath = fs::RemoveRelativePathComponents(
		porting::path_user + DIR_DELIM + "games" + DIR_DELIM);
	lua_pushstring(L, gamepath.c_str());
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_get_texturepath(lua_State *L)
{
	std::string gamepath = fs::RemoveRelativePathComponents(
		porting::path_user + DIR_DELIM + "textures");
	lua_pushstring(L, gamepath.c_str());
	return 1;
}

int ModApiMainMenu::l_get_texturepath_share(lua_State *L)
{
	std::string gamepath = fs::RemoveRelativePathComponents(
		porting::path_share + DIR_DELIM + "textures");
	lua_pushstring(L, gamepath.c_str());
	return 1;
}

int ModApiMainMenu::l_get_cache_path(lua_State *L)
{
	lua_pushstring(L, fs::RemoveRelativePathComponents(porting::path_cache).c_str());
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_create_dir(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);

	if (ModApiMainMenu::mayModifyPath(path)) {
		lua_pushboolean(L, fs::CreateAllDirs(path));
		return 1;
	}

	lua_pushboolean(L, false);
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_delete_dir(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);

	std::string absolute_path = fs::RemoveRelativePathComponents(path);

	if (ModApiMainMenu::mayModifyPath(absolute_path)) {
		lua_pushboolean(L, fs::RecursiveDelete(absolute_path));
		return 1;
	}

	lua_pushboolean(L, false);
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_copy_dir(lua_State *L)
{
	const char *source	= luaL_checkstring(L, 1);
	const char *destination	= luaL_checkstring(L, 2);

	bool keep_source = true;

	if ((!lua_isnone(L,3)) &&
			(!lua_isnil(L,3))) {
		keep_source = readParam<bool>(L,3);
	}

	std::string absolute_destination = fs::RemoveRelativePathComponents(destination);
	std::string absolute_source = fs::RemoveRelativePathComponents(source);

	if ((ModApiMainMenu::mayModifyPath(absolute_destination))) {
		bool retval = fs::CopyDir(absolute_source,absolute_destination);

		if (retval && (!keep_source)) {

			retval &= fs::RecursiveDelete(absolute_source);
		}
		lua_pushboolean(L,retval);
		return 1;
	}
	lua_pushboolean(L,false);
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_extract_zip(lua_State *L)
{
	const char *zipfile	= luaL_checkstring(L, 1);
	const char *destination	= luaL_checkstring(L, 2);

	std::string absolute_destination = fs::RemoveRelativePathComponents(destination);

	if (ModApiMainMenu::mayModifyPath(absolute_destination)) {
		fs::CreateAllDirs(absolute_destination);

		io::IFileSystem *fs = RenderingEngine::get_filesystem();

		if (!fs->addFileArchive(zipfile, false, false, io::EFAT_ZIP)) {
			lua_pushboolean(L,false);
			return 1;
		}

		sanity_check(fs->getFileArchiveCount() > 0);

		/**********************************************************************/
		/* WARNING this is not threadsafe!!                                   */
		/**********************************************************************/
		io::IFileArchive* opened_zip =
			fs->getFileArchive(fs->getFileArchiveCount()-1);

		const io::IFileList* files_in_zip = opened_zip->getFileList();

		unsigned int number_of_files = files_in_zip->getFileCount();

		for (unsigned int i=0; i < number_of_files; i++) {
			std::string fullpath = destination;
			fullpath += DIR_DELIM;
			fullpath += files_in_zip->getFullFileName(i).c_str();
			std::string fullpath_dir = fs::RemoveLastPathComponent(fullpath);

			if (!files_in_zip->isDirectory(i)) {
				if (!fs::PathExists(fullpath_dir) && !fs::CreateAllDirs(fullpath_dir)) {
					fs->removeFileArchive(fs->getFileArchiveCount()-1);
					lua_pushboolean(L,false);
					return 1;
				}

				io::IReadFile* toread = opened_zip->createAndOpenFile(i);

				FILE *targetfile = fopen(fullpath.c_str(),"wb");

				if (targetfile == NULL) {
					fs->removeFileArchive(fs->getFileArchiveCount()-1);
					lua_pushboolean(L,false);
					return 1;
				}

				char read_buffer[1024];
				long total_read = 0;

				while (total_read < toread->getSize()) {

					unsigned int bytes_read =
							toread->read(read_buffer,sizeof(read_buffer));
					if ((bytes_read == 0 ) ||
						(fwrite(read_buffer, 1, bytes_read, targetfile) != bytes_read))
					{
						fclose(targetfile);
						fs->removeFileArchive(fs->getFileArchiveCount()-1);
						lua_pushboolean(L,false);
						return 1;
					}
					total_read += bytes_read;
				}

				fclose(targetfile);
			}

		}

		fs->removeFileArchive(fs->getFileArchiveCount()-1);
		lua_pushboolean(L,true);
		return 1;
	}

	lua_pushboolean(L,false);
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_get_mainmenu_path(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	sanity_check(engine != NULL);

	lua_pushstring(L,engine->getScriptDir().c_str());
	return 1;
}

/******************************************************************************/
bool ModApiMainMenu::mayModifyPath(const std::string &path)
{
	if (fs::PathStartsWith(path, fs::TempPath()))
		return true;

	if (fs::PathStartsWith(path, fs::RemoveRelativePathComponents(porting::path_user + DIR_DELIM "games")))
		return true;

	if (fs::PathStartsWith(path, fs::RemoveRelativePathComponents(porting::path_user + DIR_DELIM "mods")))
		return true;

	if (fs::PathStartsWith(path, fs::RemoveRelativePathComponents(porting::path_user + DIR_DELIM "textures")))
		return true;

	if (fs::PathStartsWith(path, fs::RemoveRelativePathComponents(porting::path_user + DIR_DELIM "worlds")))
		return true;

	if (fs::PathStartsWith(path, fs::RemoveRelativePathComponents(porting::path_cache)))
		return true;

	return false;
}


/******************************************************************************/
int ModApiMainMenu::l_may_modify_path(lua_State *L)
{
	const char *target = luaL_checkstring(L, 1);
	std::string absolute_destination = fs::RemoveRelativePathComponents(target);
	lua_pushboolean(L, ModApiMainMenu::mayModifyPath(absolute_destination));
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_show_path_select_dialog(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	sanity_check(engine != NULL);

	const char *formname= luaL_checkstring(L, 1);
	const char *title	= luaL_checkstring(L, 2);
	bool is_file_select = readParam<bool>(L, 3);

	GUIFileSelectMenu* fileOpenMenu =
		new GUIFileSelectMenu(RenderingEngine::get_gui_env(),
				engine->m_parent,
				-1,
				engine->m_menumanager,
				title,
				formname,
				is_file_select);
	fileOpenMenu->setTextDest(engine->m_buttonhandler);
	fileOpenMenu->drop();
	return 0;
}

/******************************************************************************/
int ModApiMainMenu::l_download_file(lua_State *L)
{
	const char *url    = luaL_checkstring(L, 1);
	const char *target = luaL_checkstring(L, 2);

	//check path
	std::string absolute_destination = fs::RemoveRelativePathComponents(target);

	if (ModApiMainMenu::mayModifyPath(absolute_destination)) {
		if (GUIEngine::downloadFile(url,absolute_destination)) {
			lua_pushboolean(L,true);
			return 1;
		}
	} else {
		errorstream << "DOWNLOAD denied: " << absolute_destination
				<< " isn't a allowed path" << std::endl;
	}
	lua_pushboolean(L,false);
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_get_video_drivers(lua_State *L)
{
	std::vector<irr::video::E_DRIVER_TYPE> drivers = RenderingEngine::getSupportedVideoDrivers();

	lua_newtable(L);
	for (u32 i = 0; i != drivers.size(); i++) {
		const char *name  = RenderingEngine::getVideoDriverName(drivers[i]);
		const char *fname = RenderingEngine::getVideoDriverFriendlyName(drivers[i]);

		lua_newtable(L);
		lua_pushstring(L, name);
		lua_setfield(L, -2, "name");
		lua_pushstring(L, fname);
		lua_setfield(L, -2, "friendly_name");

		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_get_video_modes(lua_State *L)
{
	std::vector<core::vector3d<u32> > videomodes
		= RenderingEngine::getSupportedVideoModes();

	lua_newtable(L);
	for (u32 i = 0; i != videomodes.size(); i++) {
		lua_newtable(L);
		lua_pushnumber(L, videomodes[i].X);
		lua_setfield(L, -2, "w");
		lua_pushnumber(L, videomodes[i].Y);
		lua_setfield(L, -2, "h");
		lua_pushnumber(L, videomodes[i].Z);
		lua_setfield(L, -2, "depth");

		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_gettext(lua_State *L)
{
	std::string text = strgettext(std::string(luaL_checkstring(L, 1)));
	lua_pushstring(L, text.c_str());

	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_get_screen_info(lua_State *L)
{
	lua_newtable(L);
	int top = lua_gettop(L);
	lua_pushstring(L,"density");
	lua_pushnumber(L,RenderingEngine::getDisplayDensity());
	lua_settable(L, top);

	lua_pushstring(L,"display_width");
	lua_pushnumber(L,RenderingEngine::getDisplaySize().X);
	lua_settable(L, top);

	lua_pushstring(L,"display_height");
	lua_pushnumber(L,RenderingEngine::getDisplaySize().Y);
	lua_settable(L, top);

	const v2u32 &window_size = RenderingEngine::get_instance()->getWindowSize();
	lua_pushstring(L,"window_width");
	lua_pushnumber(L, window_size.X);
	lua_settable(L, top);

	lua_pushstring(L,"window_height");
	lua_pushnumber(L, window_size.Y);
	lua_settable(L, top);
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_get_min_supp_proto(lua_State *L)
{
	lua_pushinteger(L, CLIENT_PROTOCOL_VERSION_MIN);
	return 1;
}

int ModApiMainMenu::l_get_max_supp_proto(lua_State *L)
{
	lua_pushinteger(L, CLIENT_PROTOCOL_VERSION_MAX);
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_open_url(lua_State *L)
{
	std::string url = luaL_checkstring(L, 1);
	lua_pushboolean(L, porting::openURL(url));
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_do_async_callback(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);

	size_t func_length, param_length;
	const char* serialized_func_raw = luaL_checklstring(L, 1, &func_length);

	const char* serialized_param_raw = luaL_checklstring(L, 2, &param_length);

	sanity_check(serialized_func_raw != NULL);
	sanity_check(serialized_param_raw != NULL);

	std::string serialized_func = std::string(serialized_func_raw, func_length);
	std::string serialized_param = std::string(serialized_param_raw, param_length);

	lua_pushinteger(L, engine->queueAsync(serialized_func, serialized_param));

	return 1;
}

/******************************************************************************/
void ModApiMainMenu::Initialize(lua_State *L, int top)
{
	API_FCT(update_formspec);
	API_FCT(set_formspec_prepend);
	API_FCT(set_clouds);
	API_FCT(get_textlist_index);
	API_FCT(get_table_index);
	API_FCT(get_worlds);
	API_FCT(get_games);
	API_FCT(get_content_info);
	API_FCT(start);
	API_FCT(close);
	API_FCT(get_favorites);
	API_FCT(show_keys_menu);
	API_FCT(create_world);
	API_FCT(delete_world);
	API_FCT(delete_favorite);
	API_FCT(set_background);
	API_FCT(set_topleft_text);
	API_FCT(get_mapgen_names);
	API_FCT(get_modpath);
	API_FCT(get_clientmodpath);
	API_FCT(get_gamepath);
	API_FCT(get_texturepath);
	API_FCT(get_texturepath_share);
	API_FCT(get_cache_path);
	API_FCT(create_dir);
	API_FCT(delete_dir);
	API_FCT(copy_dir);
	API_FCT(extract_zip);
	API_FCT(may_modify_path);
	API_FCT(get_mainmenu_path);
	API_FCT(show_path_select_dialog);
	API_FCT(download_file);
	API_FCT(gettext);
	API_FCT(get_video_drivers);
	API_FCT(get_video_modes);
	API_FCT(get_screen_info);
	API_FCT(get_min_supp_proto);
	API_FCT(get_max_supp_proto);
	API_FCT(open_url);
	API_FCT(do_async_callback);
}

/******************************************************************************/
void ModApiMainMenu::InitializeAsync(lua_State *L, int top)
{
	API_FCT(get_worlds);
	API_FCT(get_games);
	API_FCT(get_favorites);
	API_FCT(get_mapgen_names);
	API_FCT(get_modpath);
	API_FCT(get_clientmodpath);
	API_FCT(get_gamepath);
	API_FCT(get_texturepath);
	API_FCT(get_texturepath_share);
	API_FCT(get_cache_path);
	API_FCT(create_dir);
	API_FCT(delete_dir);
	API_FCT(copy_dir);
	//API_FCT(extract_zip); //TODO remove dependency to GuiEngine
	API_FCT(may_modify_path);
	API_FCT(download_file);
	//API_FCT(gettext); (gettext lib isn't threadsafe)
}

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
#include "guiEngine.h"
#include "guiMainMenu.h"
#include "guiKeyChangeMenu.h"
#include "guiFileSelectMenu.h"
#include "subgame.h"
#include "version.h"
#include "porting.h"
#include "filesys.h"
#include "convert_json.h"
#include "serverlist.h"
#include "mapgen.h"
#include "settings.h"
#include "EDriverTypes.h"

#include <IFileArchive.h>
#include <IFileSystem.h>

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
	return lua_toboolean(L, -1);
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
		tile_image = lua_toboolean(L, 3);
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

	bool value = lua_toboolean(L,1);

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

	for (unsigned int i = 0; i < worlds.size(); i++)
	{
		lua_pushnumber(L,index);

		lua_newtable(L);
		int top_lvl2 = lua_gettop(L);

		lua_pushstring(L,"path");
		lua_pushstring(L,worlds[i].path.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L,"name");
		lua_pushstring(L,worlds[i].name.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L,"gameid");
		lua_pushstring(L,worlds[i].gameid.c_str());
		lua_settable(L, top_lvl2);

		lua_settable(L, top);
		index++;
	}
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_get_games(lua_State *L)
{
	std::vector<SubgameSpec> games = getAvailableGames();

	lua_newtable(L);
	int top = lua_gettop(L);
	unsigned int index = 1;

	for (unsigned int i = 0; i < games.size(); i++)
	{
		lua_pushnumber(L,index);
		lua_newtable(L);
		int top_lvl2 = lua_gettop(L);

		lua_pushstring(L,"id");
		lua_pushstring(L,games[i].id.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L,"path");
		lua_pushstring(L,games[i].path.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L,"gamemods_path");
		lua_pushstring(L,games[i].gamemods_path.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L,"name");
		lua_pushstring(L,games[i].name.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L,"menuicon_path");
		lua_pushstring(L,games[i].menuicon_path.c_str());
		lua_settable(L, top_lvl2);

		lua_pushstring(L,"addon_mods_paths");
		lua_newtable(L);
		int table2 = lua_gettop(L);
		int internal_index=1;
		for (std::set<std::string>::iterator iter = games[i].addon_mods_paths.begin();
				iter != games[i].addon_mods_paths.end(); ++iter) {
			lua_pushnumber(L,internal_index);
			lua_pushstring(L,(*iter).c_str());
			lua_settable(L, table2);
			internal_index++;
		}
		lua_settable(L, top_lvl2);
		lua_settable(L, top);
		index++;
	}
	return 1;
}
/******************************************************************************/
int ModApiMainMenu::l_get_modstore_details(lua_State *L)
{
	const char *modid	= luaL_checkstring(L, 1);

	if (modid != 0) {
		Json::Value details;
		std::string url = "";
		try{
			url = g_settings->get("modstore_details_url");
		}
		catch(SettingNotFoundException &e) {
			lua_pushnil(L);
			return 1;
		}

		size_t idpos = url.find("*");
		url.erase(idpos,1);
		url.insert(idpos,modid);

		details = getModstoreUrl(url);

		ModStoreModDetails current_mod = readModStoreModDetails(details);

		if ( current_mod.valid) {
			lua_newtable(L);
			int top = lua_gettop(L);

			lua_pushstring(L,"id");
			lua_pushnumber(L,current_mod.id);
			lua_settable(L, top);

			lua_pushstring(L,"title");
			lua_pushstring(L,current_mod.title.c_str());
			lua_settable(L, top);

			lua_pushstring(L,"basename");
			lua_pushstring(L,current_mod.basename.c_str());
			lua_settable(L, top);

			lua_pushstring(L,"description");
			lua_pushstring(L,current_mod.description.c_str());
			lua_settable(L, top);

			lua_pushstring(L,"author");
			lua_pushstring(L,current_mod.author.username.c_str());
			lua_settable(L, top);

			lua_pushstring(L,"download_url");
			lua_pushstring(L,current_mod.versions[0].file.c_str());
			lua_settable(L, top);

			lua_pushstring(L,"versions");
			lua_newtable(L);
			int versionstop = lua_gettop(L);
			for (unsigned int i=0;i < current_mod.versions.size(); i++) {
				lua_pushnumber(L,i+1);
				lua_newtable(L);
				int current_element = lua_gettop(L);

				lua_pushstring(L,"date");
				lua_pushstring(L,current_mod.versions[i].date.c_str());
				lua_settable(L,current_element);

				lua_pushstring(L,"download_url");
				lua_pushstring(L,current_mod.versions[i].file.c_str());
				lua_settable(L,current_element);

				lua_settable(L,versionstop);
			}
			lua_settable(L, top);

			lua_pushstring(L,"screenshot_url");
			lua_pushstring(L,current_mod.titlepic.file.c_str());
			lua_settable(L, top);

			lua_pushstring(L,"license");
			lua_pushstring(L,current_mod.license.shortinfo.c_str());
			lua_settable(L, top);

			lua_pushstring(L,"rating");
			lua_pushnumber(L,current_mod.rating);
			lua_settable(L, top);

			//TODO depends

			//TODO softdepends
			return 1;
		}
	}
	return 0;
}

/******************************************************************************/
int ModApiMainMenu::l_get_modstore_list(lua_State *L)
{
	Json::Value mods;
	std::string url = "";
	try{
		url = g_settings->get("modstore_listmods_url");
	}
	catch(SettingNotFoundException &e) {
		lua_pushnil(L);
		return 1;
	}

	mods = getModstoreUrl(url);

	std::vector<ModStoreMod> moddata = readModStoreList(mods);

	lua_newtable(L);
	int top = lua_gettop(L);
	unsigned int index = 1;

	for (unsigned int i = 0; i < moddata.size(); i++)
	{
		if (moddata[i].valid) {
			lua_pushnumber(L,index);
			lua_newtable(L);

			int top_lvl2 = lua_gettop(L);

			lua_pushstring(L,"id");
			lua_pushnumber(L,moddata[i].id);
			lua_settable(L, top_lvl2);

			lua_pushstring(L,"title");
			lua_pushstring(L,moddata[i].title.c_str());
			lua_settable(L, top_lvl2);

			lua_pushstring(L,"basename");
			lua_pushstring(L,moddata[i].basename.c_str());
			lua_settable(L, top_lvl2);

			lua_settable(L, top);
			index++;
		}
	}
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_get_favorites(lua_State *L)
{
	std::string listtype = "local";

	if (!lua_isnone(L,1)) {
		listtype = luaL_checkstring(L,1);
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

	for (unsigned int i = 0; i < servers.size(); i++)
	{

		lua_pushnumber(L,index);

		lua_newtable(L);
		int top_lvl2 = lua_gettop(L);

		if (servers[i]["clients"].asString().size()) {
			std::string clients_raw = servers[i]["clients"].asString();
			char* endptr = 0;
			int numbervalue = strtol(clients_raw.c_str(),&endptr,10);

			if ((clients_raw != "") && (*endptr == 0)) {
				lua_pushstring(L,"clients");
				lua_pushnumber(L,numbervalue);
				lua_settable(L, top_lvl2);
			}
		}

		if (servers[i]["clients_max"].asString().size()) {

			std::string clients_max_raw = servers[i]["clients_max"].asString();
			char* endptr = 0;
			int numbervalue = strtol(clients_max_raw.c_str(),&endptr,10);

			if ((clients_max_raw != "") && (*endptr == 0)) {
				lua_pushstring(L,"clients_max");
				lua_pushnumber(L,numbervalue);
				lua_settable(L, top_lvl2);
			}
		}

		if (servers[i]["version"].asString().size()) {
			lua_pushstring(L,"version");
			std::string topush = servers[i]["version"].asString();
			lua_pushstring(L,topush.c_str());
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["proto_min"].asString().size()) {
			lua_pushstring(L,"proto_min");
			lua_pushinteger(L,servers[i]["proto_min"].asInt());
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["proto_max"].asString().size()) {
			lua_pushstring(L,"proto_max");
			lua_pushinteger(L,servers[i]["proto_max"].asInt());
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["password"].asString().size()) {
			lua_pushstring(L,"password");
			lua_pushboolean(L,servers[i]["password"].asBool());
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["creative"].asString().size()) {
			lua_pushstring(L,"creative");
			lua_pushboolean(L,servers[i]["creative"].asBool());
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["damage"].asString().size()) {
			lua_pushstring(L,"damage");
			lua_pushboolean(L,servers[i]["damage"].asBool());
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["pvp"].asString().size()) {
			lua_pushstring(L,"pvp");
			lua_pushboolean(L,servers[i]["pvp"].asBool());
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["description"].asString().size()) {
			lua_pushstring(L,"description");
			std::string topush = servers[i]["description"].asString();
			lua_pushstring(L,topush.c_str());
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["name"].asString().size()) {
			lua_pushstring(L,"name");
			std::string topush = servers[i]["name"].asString();
			lua_pushstring(L,topush.c_str());
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["address"].asString().size()) {
			lua_pushstring(L,"address");
			std::string topush = servers[i]["address"].asString();
			lua_pushstring(L,topush.c_str());
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["port"].asString().size()) {
			lua_pushstring(L,"port");
			std::string topush = servers[i]["port"].asString();
			lua_pushstring(L,topush.c_str());
			lua_settable(L, top_lvl2);
		}

		if (servers[i].isMember("ping")) {
			float ping = servers[i]["ping"].asFloat();
			lua_pushstring(L, "ping");
			lua_pushnumber(L, ping);
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
int ModApiMainMenu::l_show_keys_menu(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	sanity_check(engine != NULL);

	GUIKeyChangeMenu *kmenu
		= new GUIKeyChangeMenu(	engine->m_device->getGUIEnvironment(),
								engine->m_parent,
								-1,
								engine->m_menumanager);
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
			+ name;

	std::vector<SubgameSpec> games = getAvailableGames();

	if ((gameidx >= 0) &&
			(gameidx < (int) games.size())) {

		// Create world if it doesn't exist
		if (!loadGameConfAndInitWorld(path, games[gameidx])) {
			lua_pushstring(L, "Failed to initialize world");
		} else {
			lua_pushnil(L);
		}
	} else {
		lua_pushstring(L, "Invalid game index");
	}
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_delete_world(lua_State *L)
{
	int worldidx	= luaL_checkinteger(L,1) -1;

	std::vector<WorldSpec> worlds = getAvailableWorlds();

	if ((worldidx >= 0) &&
		(worldidx < (int) worlds.size())) {

		WorldSpec spec = worlds[worldidx];

		std::vector<std::string> paths;
		paths.push_back(spec.path);
		fs::GetRecursiveSubPaths(spec.path, paths);

		// Delete files
		if (!fs::DeletePaths(paths)) {
			lua_pushstring(L, "Failed to delete world");
		}
		else {
			lua_pushnil(L);
		}
	}
	else {
		lua_pushstring(L, "Invalid world index");
	}
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_set_topleft_text(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	sanity_check(engine != NULL);

	std::string text = "";

	if (!lua_isnone(L,1) &&	!lua_isnil(L,1))
		text = luaL_checkstring(L, 1);

	engine->setTopleftText(text);
	return 0;
}

/******************************************************************************/
int ModApiMainMenu::l_get_mapgen_names(lua_State *L)
{
	std::vector<const char *> names;
	Mapgen::getMapgenNames(&names, lua_toboolean(L, 1));

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

/******************************************************************************/
int ModApiMainMenu::l_create_dir(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);

	if (ModApiMainMenu::isMinetestPath(path)) {
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

	if (ModApiMainMenu::isMinetestPath(absolute_path)) {
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
		keep_source = lua_toboolean(L,3);
	}

	std::string absolute_destination = fs::RemoveRelativePathComponents(destination);
	std::string absolute_source = fs::RemoveRelativePathComponents(source);

	if ((ModApiMainMenu::isMinetestPath(absolute_source)) &&
			(ModApiMainMenu::isMinetestPath(absolute_destination))) {
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
	GUIEngine* engine = getGuiEngine(L);
	sanity_check(engine);

	const char *zipfile	= luaL_checkstring(L, 1);
	const char *destination	= luaL_checkstring(L, 2);

	std::string absolute_destination = fs::RemoveRelativePathComponents(destination);

	if (ModApiMainMenu::isMinetestPath(absolute_destination)) {
		fs::CreateAllDirs(absolute_destination);

		io::IFileSystem* fs = engine->m_device->getFileSystem();

		if (!fs->addFileArchive(zipfile,true,false,io::EFAT_ZIP)) {
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
bool ModApiMainMenu::isMinetestPath(std::string path)
{
	if (fs::PathStartsWith(path,fs::TempPath()))
		return true;

	/* games */
	if (fs::PathStartsWith(path,fs::RemoveRelativePathComponents(porting::path_share + DIR_DELIM + "games")))
		return true;

	/* mods */
	if (fs::PathStartsWith(path,fs::RemoveRelativePathComponents(porting::path_user + DIR_DELIM + "mods")))
		return true;

	/* worlds */
	if (fs::PathStartsWith(path,fs::RemoveRelativePathComponents(porting::path_user + DIR_DELIM + "worlds")))
		return true;


	return false;
}

/******************************************************************************/
int ModApiMainMenu::l_show_file_open_dialog(lua_State *L)
{
	GUIEngine* engine = getGuiEngine(L);
	sanity_check(engine != NULL);

	const char *formname= luaL_checkstring(L, 1);
	const char *title	= luaL_checkstring(L, 2);

	GUIFileSelectMenu* fileOpenMenu =
		new GUIFileSelectMenu(engine->m_device->getGUIEnvironment(),
								engine->m_parent,
								-1,
								engine->m_menumanager,
								title,
								formname);
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

	if (ModApiMainMenu::isMinetestPath(absolute_destination)) {
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
	std::vector<irr::video::E_DRIVER_TYPE> drivers
		= porting::getSupportedVideoDrivers();

	lua_newtable(L);
	for (u32 i = 0; i != drivers.size(); i++) {
		const char *name  = porting::getVideoDriverName(drivers[i]);
		const char *fname = porting::getVideoDriverFriendlyName(drivers[i]);

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
		= porting::getSupportedVideoModes();

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
	lua_pushnumber(L,porting::getDisplayDensity());
	lua_settable(L, top);

	lua_pushstring(L,"display_width");
	lua_pushnumber(L,porting::getDisplaySize().X);
	lua_settable(L, top);

	lua_pushstring(L,"display_height");
	lua_pushnumber(L,porting::getDisplaySize().Y);
	lua_settable(L, top);

	lua_pushstring(L,"window_width");
	lua_pushnumber(L,porting::getWindowSize().X);
	lua_settable(L, top);

	lua_pushstring(L,"window_height");
	lua_pushnumber(L,porting::getWindowSize().Y);
	lua_settable(L, top);
	return 1;
}

/******************************************************************************/
int ModApiMainMenu::l_get_min_supp_proto(lua_State *L)
{
	u16 proto_version_min = g_settings->getFlag("send_pre_v25_init") ?
		CLIENT_PROTOCOL_VERSION_MIN_LEGACY : CLIENT_PROTOCOL_VERSION_MIN;
	lua_pushinteger(L, proto_version_min);
	return 1;
}

int ModApiMainMenu::l_get_max_supp_proto(lua_State *L)
{
	lua_pushinteger(L, CLIENT_PROTOCOL_VERSION_MAX);
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
	API_FCT(set_clouds);
	API_FCT(get_textlist_index);
	API_FCT(get_table_index);
	API_FCT(get_worlds);
	API_FCT(get_games);
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
	API_FCT(get_gamepath);
	API_FCT(get_texturepath);
	API_FCT(get_texturepath_share);
	API_FCT(create_dir);
	API_FCT(delete_dir);
	API_FCT(copy_dir);
	API_FCT(extract_zip);
	API_FCT(get_mainmenu_path);
	API_FCT(show_file_open_dialog);
	API_FCT(download_file);
	API_FCT(get_modstore_details);
	API_FCT(get_modstore_list);
	API_FCT(gettext);
	API_FCT(get_video_drivers);
	API_FCT(get_video_modes);
	API_FCT(get_screen_info);
	API_FCT(get_min_supp_proto);
	API_FCT(get_max_supp_proto);
	API_FCT(do_async_callback);
}

/******************************************************************************/
void ModApiMainMenu::InitializeAsync(AsyncEngine& engine)
{

	ASYNC_API_FCT(get_worlds);
	ASYNC_API_FCT(get_games);
	ASYNC_API_FCT(get_favorites);
	ASYNC_API_FCT(get_mapgen_names);
	ASYNC_API_FCT(get_modpath);
	ASYNC_API_FCT(get_gamepath);
	ASYNC_API_FCT(get_texturepath);
	ASYNC_API_FCT(get_texturepath_share);
	ASYNC_API_FCT(create_dir);
	ASYNC_API_FCT(delete_dir);
	ASYNC_API_FCT(copy_dir);
	//ASYNC_API_FCT(extract_zip); //TODO remove dependency to GuiEngine
	ASYNC_API_FCT(download_file);
	ASYNC_API_FCT(get_modstore_details);
	ASYNC_API_FCT(get_modstore_list);
	//ASYNC_API_FCT(gettext); (gettext lib isn't threadsafe)
}

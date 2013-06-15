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

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#include "porting.h"
#include "guiMainMenu.h"
#include "subgame.h"
#include "guiKeyChangeMenu.h"
#include "guiFileSelectMenu.h"
#include "main.h"
#include "settings.h"
#include "filesys.h"

#include "guiLuaApi.h"
#include "guiEngine.h"

#if USE_ARCHIVE
#include <archive.h>
#include <archive_entry.h>

#ifdef _WIN32
#include <direct.h>
#endif
#endif

#include "minizip/unzip.h"

#define MINIZIP_BUFFERSIZE 8192

#ifdef _WIN32
#define MINIZIP_PATHLEN _MAX_PATH
#else
#define MINIZIP_PATHLEN PATH_MAX
#endif

#define API_FCT(name) registerFunction(L,#name,l_##name,top)

void guiLuaApi::initialize(lua_State* L,GUIEngine* engine)
{
	lua_pushlightuserdata(L, engine);
	lua_setfield(L, LUA_REGISTRYINDEX, "engine");

	lua_pushstring(L, DIR_DELIM);
	lua_setglobal(L, "DIR_DELIM");

	lua_newtable(L);
	lua_setglobal(L, "gamedata");

	lua_newtable(L);
	lua_setglobal(L, "engine");

	lua_getglobal(L, "engine");
	int top = lua_gettop(L);

	bool retval = true;

	//add api functions
	retval &= API_FCT(update_formspec);
	retval &= API_FCT(set_clouds);
	retval &= API_FCT(get_textlist_index);
	retval &= API_FCT(get_worlds);
	retval &= API_FCT(get_games);
	retval &= API_FCT(start);
	retval &= API_FCT(close);
	retval &= API_FCT(get_favorites);
	retval &= API_FCT(show_keys_menu);
	retval &= API_FCT(setting_set);
	retval &= API_FCT(setting_get);
	retval &= API_FCT(setting_getbool);
	retval &= API_FCT(setting_setbool);
	retval &= API_FCT(create_world);
	retval &= API_FCT(delete_world);
	retval &= API_FCT(delete_favorite);
	retval &= API_FCT(set_background);
	retval &= API_FCT(set_topleft_text);
	retval &= API_FCT(get_modpath);
	retval &= API_FCT(get_gamepath);
	retval &= API_FCT(get_dirlist);
	retval &= API_FCT(create_dir);
	retval &= API_FCT(delete_dir);
	retval &= API_FCT(copy_dir);
	retval &= API_FCT(extract_zip);
	retval &= API_FCT(get_scriptdir);
	retval &= API_FCT(show_file_open_dialog);
	retval &= API_FCT(get_version);

#if USE_ARCHIVE
	retval &= API_FCT(extract_archive);
#endif

	if (!retval) {
		//TODO show error
	}
}

/******************************************************************************/
bool guiLuaApi::registerFunction(	lua_State* L,
									const char* name,
									lua_CFunction fct,
									int top
									)
{
	lua_pushstring(L,name);
	lua_pushcfunction(L,fct);
	lua_settable(L, top);

	return true;
}

/******************************************************************************/
GUIEngine* guiLuaApi::get_engine(lua_State *L)
{
	// Get server from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "engine");
	GUIEngine* sapi_ptr = (GUIEngine*) lua_touserdata(L, -1);
	lua_pop(L, 1);
	return sapi_ptr;
}

/******************************************************************************/
std::string guiLuaApi::getTextData(lua_State *L, std::string name)
{
	lua_getglobal(L, "gamedata");

	lua_getfield(L, -1, name.c_str());

	if(lua_isnil(L, -1))
		return "";

	return luaL_checkstring(L, -1);
}

/******************************************************************************/
int guiLuaApi::getIntegerData(lua_State *L, std::string name,bool& valid)
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
int guiLuaApi::getBoolData(lua_State *L, std::string name,bool& valid)
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
int guiLuaApi::l_update_formspec(lua_State *L)
{
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

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
int guiLuaApi::l_start(lua_State *L)
{
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

	//update c++ gamedata from lua table

	bool valid = false;


	engine->m_data->selected_world	= getIntegerData(L, "selected_world",valid) -1;
	engine->m_data->simple_singleplayer_mode = getBoolData(L,"singleplayer",valid);
	engine->m_data->name			= getTextData(L,"playername");
	engine->m_data->password		= getTextData(L,"password");
	engine->m_data->address			= getTextData(L,"address");
	engine->m_data->port			= getTextData(L,"port");

	//close menu next time
	engine->m_startgame = true;
	return 0;
}

/******************************************************************************/
int guiLuaApi::l_close(lua_State *L)
{
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

	engine->m_data->kill = true;

	//close menu next time
	engine->m_startgame = true;
	engine->m_menu->quitMenu();
	return 0;
}

/******************************************************************************/
int guiLuaApi::l_set_background(lua_State *L)
{
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

	std::string backgroundlevel(luaL_checkstring(L, 1));
	std::string texturename(luaL_checkstring(L, 2));

	bool retval = false;

	if (backgroundlevel == "background") {
		retval |= engine->setTexture(TEX_LAYER_BACKGROUND,texturename);
	}

	if (backgroundlevel == "overlay") {
		retval |= engine->setTexture(TEX_LAYER_OVERLAY,texturename);
	}

	if (backgroundlevel == "header") {
		retval |= engine->setTexture(TEX_LAYER_HEADER,texturename);
	}

	if (backgroundlevel == "footer") {
		retval |= engine->setTexture(TEX_LAYER_FOOTER,texturename);
	}

	lua_pushboolean(L,retval);
	return 1;
}

/******************************************************************************/
int guiLuaApi::l_set_clouds(lua_State *L)
{
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

	bool value = lua_toboolean(L,1);

	engine->m_clouds_enabled = value;

	return 0;
}

/******************************************************************************/
int guiLuaApi::l_get_textlist_index(lua_State *L)
{
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

	std::string listboxname(luaL_checkstring(L, 1));

	int selection = engine->m_menu->getListboxIndex(listboxname);

	if (selection >= 0)
		selection++;

	lua_pushinteger(L, selection);
	return 1;
}

/******************************************************************************/
int guiLuaApi::l_get_worlds(lua_State *L)
{
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

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
int guiLuaApi::l_get_games(lua_State *L)
{
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

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
				iter != games[i].addon_mods_paths.end(); iter++) {
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
int guiLuaApi::l_get_favorites(lua_State *L)
{
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

	std::string listtype = "local";

	if (!lua_isnone(L,1)) {
		listtype = luaL_checkstring(L,1);
	}

	std::vector<ServerListSpec> servers;
#if USE_CURL
	if(listtype == "online") {
		servers = ServerList::getOnline();
	} else {
		servers = ServerList::getLocal();
	}
#else
	servers = ServerList::getLocal();
#endif

	lua_newtable(L);
	int top = lua_gettop(L);
	unsigned int index = 1;

	for (unsigned int i = 0; i < servers.size(); i++)
	{
		lua_pushnumber(L,index);

		lua_newtable(L);
		int top_lvl2 = lua_gettop(L);

		if (servers[i]["clients"].asString().size()) {

			const char* clients_raw = servers[i]["clients"].asString().c_str();
			char* endptr = 0;
			int numbervalue = strtol(clients_raw,&endptr,10);

			if ((*clients_raw != 0) && (*endptr == 0)) {
				lua_pushstring(L,"clients");
				lua_pushnumber(L,numbervalue);
				lua_settable(L, top_lvl2);
			}
		}

		if (servers[i]["clients_max"].asString().size()) {

			const char* clients_max_raw = servers[i]["clients"].asString().c_str();
			char* endptr = 0;
			int numbervalue = strtol(clients_max_raw,&endptr,10);

			if ((*clients_max_raw != 0) && (*endptr == 0)) {
				lua_pushstring(L,"clients_max");
				lua_pushnumber(L,numbervalue);
				lua_settable(L, top_lvl2);
			}
		}

		if (servers[i]["version"].asString().size()) {
			lua_pushstring(L,"version");
			lua_pushstring(L,servers[i]["version"].asString().c_str());
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["password"].asString().size()) {
			lua_pushstring(L,"password");
			lua_pushboolean(L,true);
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["creative"].asString().size()) {
			lua_pushstring(L,"creative");
			lua_pushboolean(L,true);
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["damage"].asString().size()) {
			lua_pushstring(L,"damage");
			lua_pushboolean(L,true);
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["pvp"].asString().size()) {
			lua_pushstring(L,"pvp");
			lua_pushboolean(L,true);
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["description"].asString().size()) {
			lua_pushstring(L,"description");
			lua_pushstring(L,servers[i]["description"].asString().c_str());
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["name"].asString().size()) {
			lua_pushstring(L,"name");
			lua_pushstring(L,servers[i]["name"].asString().c_str());
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["address"].asString().size()) {
			lua_pushstring(L,"address");
			lua_pushstring(L,servers[i]["address"].asString().c_str());
			lua_settable(L, top_lvl2);
		}

		if (servers[i]["port"].asString().size()) {
			lua_pushstring(L,"port");
			lua_pushstring(L,servers[i]["port"].asString().c_str());
			lua_settable(L, top_lvl2);
		}

		lua_settable(L, top);
		index++;
	}
	return 1;
}

/******************************************************************************/
int guiLuaApi::l_delete_favorite(lua_State *L)
{
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

	std::vector<ServerListSpec> servers;

	std::string listtype = "local";

	if (!lua_isnone(L,2)) {
		listtype = luaL_checkstring(L,2);
	}

	if ((listtype != "local") &&
		(listtype != "online"))
		return 0;

#if USE_CURL
	if(listtype == "online") {
		servers = ServerList::getOnline();
	} else {
		servers = ServerList::getLocal();
	}
#else
	servers = ServerList::getLocal();
#endif

	int fav_idx	= luaL_checkinteger(L,1) -1;

	if ((fav_idx >= 0) &&
			(fav_idx < (int) servers.size())) {

		ServerList::deleteEntry(servers[fav_idx]);
	}

	return 0;
}

/******************************************************************************/
int guiLuaApi::l_show_keys_menu(lua_State *L)
{
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

	GUIKeyChangeMenu *kmenu
		= new GUIKeyChangeMenu(	engine->m_device->getGUIEnvironment(),
								engine->m_parent,
								-1,
								engine->m_menumanager);
	kmenu->drop();
	return 0;
}

/******************************************************************************/
int guiLuaApi::l_setting_set(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	const char *value = luaL_checkstring(L, 2);
	g_settings->set(name, value);
	return 0;
}

/******************************************************************************/
int guiLuaApi::l_setting_get(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	try{
		std::string value = g_settings->get(name);
		lua_pushstring(L, value.c_str());
	} catch(SettingNotFoundException &e){
		lua_pushnil(L);
	}
	return 1;
}

/******************************************************************************/
int guiLuaApi::l_setting_getbool(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	try{
		bool value = g_settings->getBool(name);
		lua_pushboolean(L, value);
	} catch(SettingNotFoundException &e){
		lua_pushnil(L);
	}
	return 1;
}

/******************************************************************************/
int guiLuaApi::l_setting_setbool(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	bool value = lua_toboolean(L,2);

	g_settings->setBool(name,value);

	return 0;
}

/******************************************************************************/
int guiLuaApi::l_create_world(lua_State *L)
{
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

	const char *name	= luaL_checkstring(L, 1);
	int gameidx			= luaL_checkinteger(L,2) -1;

	std::string path = porting::path_user + DIR_DELIM
			"worlds" + DIR_DELIM
			+ name;

	std::vector<SubgameSpec> games = getAvailableGames();

	if ((gameidx >= 0) &&
			(gameidx < (int) games.size())) {

		// Create world if it doesn't exist
		if(!initializeWorld(path, games[gameidx].id)){
			lua_pushstring(L, "Failed to initialize world");

		}
		else {
		lua_pushnil(L);
		}
	}
	else {
		lua_pushstring(L, "Invalid game index");
	}
	return 1;
}

/******************************************************************************/
int guiLuaApi::l_delete_world(lua_State *L)
{
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

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
int guiLuaApi::l_set_topleft_text(lua_State *L)
{
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

	std::string text = "";

	if (!lua_isnone(L,1) &&	!lua_isnil(L,1))
		text = luaL_checkstring(L, 1);

	engine->setTopleftText(text);
	return 0;
}

/******************************************************************************/
int guiLuaApi::l_get_modpath(lua_State *L)
{
	//TODO this path may be controversial!
	std::string modpath
			= fs::AbsolutePath(porting::path_share + DIR_DELIM + "mods" + DIR_DELIM);
	lua_pushstring(L, modpath.c_str());
	return 1;
}

/******************************************************************************/
int guiLuaApi::l_get_gamepath(lua_State *L)
{
	std::string gamepath
			= fs::AbsolutePath(porting::path_share + DIR_DELIM + "games"	+ DIR_DELIM);
	lua_pushstring(L, gamepath.c_str());
	return 1;
}

/******************************************************************************/
int guiLuaApi::l_get_dirlist(lua_State *L) {
	const char *path	= luaL_checkstring(L, 1);
	bool dironly		= lua_toboolean(L, 2);

	std::vector<fs::DirListNode> dirlist = fs::GetDirListing(path);

	unsigned int index = 1;
	lua_newtable(L);
	int table = lua_gettop(L);

	for (unsigned int i=0;i< dirlist.size(); i++) {
		if ((dirlist[i].dir) || (dironly == false)) {
			lua_pushnumber(L,index);
			lua_pushstring(L,dirlist[i].name.c_str());
			lua_settable(L, table);
			index++;
		}
	}

	return 1;
}

/******************************************************************************/
int guiLuaApi::l_create_dir(lua_State *L) {
	const char *path	= luaL_checkstring(L, 1);

	if (guiLuaApi::isMinetestPath(path)) {
		lua_pushboolean(L,fs::CreateAllDirs(path));
		return 1;
	}
	lua_pushboolean(L,false);
	return 1;
}

/******************************************************************************/
int guiLuaApi::l_delete_dir(lua_State *L) {
	const char *path	= luaL_checkstring(L, 1);

	std::string absolute_path = fs::AbsolutePath(path);

	if (guiLuaApi::isMinetestPath(absolute_path)) {
		lua_pushboolean(L,fs::RecursiveDelete(absolute_path));
		return 1;
	}
	lua_pushboolean(L,false);
	return 1;
}

/******************************************************************************/
int guiLuaApi::l_copy_dir(lua_State *L) {
	const char *source	= luaL_checkstring(L, 1);
	const char *destination	= luaL_checkstring(L, 2);

	bool keep_source = true;

	if ((!lua_isnone(L,3)) &&
			(!lua_isnil(L,3))) {
		keep_source = lua_toboolean(L,3);
	}

	//deny relative destination paths
	if (std::string(destination).find("..") != std::string::npos) {
		lua_pushboolean(L,false);
		return 1;
	}

	std::string absolute_source = fs::AbsolutePath(source);

	if ((guiLuaApi::isMinetestPath(absolute_source)) &&
			(guiLuaApi::isMinetestPath(destination))) {
		bool retval = fs::CopyDir(absolute_source,destination);

		if (retval && (!keep_source)) {

			retval &= fs::RecursiveDelete(absolute_source);
		}
		lua_pushboolean(L,retval);
		return 1;
	}
	lua_pushboolean(L,false);
	return 1;
}
#if USE_ARCHIVE
/******************************************************************************/
#define ERRORCLEANUP                       \
	archive_read_finish(a);                \
	archive_write_finish(t);               \
	chdir(origcwd);                        \
	lua_pushboolean(L,false);              \
	return 1;

int guiLuaApi::l_extract_archive(lua_State *L) {
	const char *archive = luaL_checkstring(L, 1);
	const char *destination = luaL_checkstring(L, 2);

	char origcwd[PATH_MAX];
#ifdef _WIN32
	_getcwd( origcwd, PATH_MAX );
#else
	getcwd( origcwd, PATH_MAX );
#endif

	fs::CreateAllDirs(destination);

	chdir(destination);

	std::string absolute_destination = fs::AbsolutePath(destination);

	if (guiLuaApi::isMinetestPath(absolute_destination)) {
		struct archive* a = archive_read_new();
		struct archive* t = archive_write_disk_new();
		struct archive_entry* entry;
		int retval;


		archive_read_support_compression_all(a);
		archive_read_support_format_all(a);

		archive_write_disk_set_options(t, ARCHIVE_EXTRACT_TIME);
		archive_write_disk_set_options(t,ARCHIVE_EXTRACT_SECURE_NODOTDOT);

		retval = archive_read_open_filename(a, archive, 16384);
		if (retval != ARCHIVE_OK) {
			ERRORCLEANUP
		}

		retval = archive_read_next_header(a, &entry);
		if (retval != ARCHIVE_OK) {
			ERRORCLEANUP
		}

		for (;;) {
			retval = archive_read_next_header(a, &entry);

			if (retval == ARCHIVE_EOF)
				break;

			retval = archive_write_header(t,entry);

			if (retval != ARCHIVE_OK) {
				fprintf(stderr, "%s\n", archive_error_string(t));
				ERRORCLEANUP
			}

			if (archive_entry_size(entry) > 0) {
				int lret;
				const void* buffer;
				size_t datasize;
				off_t offset;

				for (;;) {
					lret = archive_read_data_block(a,&buffer,&datasize, &offset);
					if (lret == ARCHIVE_EOF)
						break;
					if (lret != ARCHIVE_OK) {
						fprintf(stderr, "%s\n", archive_error_string(t));
						ERRORCLEANUP
					}

					lret = archive_write_data_block(t,buffer,datasize,offset);
					if (lret != ARCHIVE_OK) {
						fprintf(stderr, "%s\n", archive_error_string(t));
						ERRORCLEANUP
					}
				}
			}
			retval = archive_write_finish_entry(t);

			if (retval == ARCHIVE_OK) {
				continue;
			}
			ERRORCLEANUP
		}

		archive_read_finish(a);
		archive_write_finish(t);
		chdir(origcwd);
		lua_pushboolean(L,true);
		return 1;
	}

	chdir(origcwd);
	lua_pushboolean(L,false);
	return 1;
}
#undef ERRORCLEANUP
#endif

/******************************************************************************/
int guiLuaApi::l_extract_zip(lua_State *L) {
	const char *zipfile	= luaL_checkstring(L, 1);
	const char *destination	= luaL_checkstring(L, 2);

	fs::CreateAllDirs(destination);

	std::string absolute_destination = fs::AbsolutePath(destination);

	if (guiLuaApi::isMinetestPath(absolute_destination)) {
		unzFile ZipFile = unzOpen(zipfile);

		if (ZipFile != NULL) {

			unz_global_info global_info;

			if ( unzGetGlobalInfo( ZipFile, &global_info ) != UNZ_OK ) {
				unzClose(ZipFile);
				lua_pushboolean(L,false);
				return 1;
			}

			char read_buffer[MINIZIP_BUFFERSIZE];

			unsigned int i;
			for ( i = 0; i < global_info.number_entry; ++i ) {

				//read file info
				unz_file_info file_info;
				char filename[MINIZIP_PATHLEN];

				if ( unzGetCurrentFileInfo(
					ZipFile,
					&file_info,
					filename,
					MINIZIP_PATHLEN,
					NULL, 0, NULL, 0) != UNZ_OK ) {
					unzClose(ZipFile);
					lua_pushboolean(L,false);
					return 1;
				}

				const size_t fn_length = strlen(filename);

				//test for file or dir
				if (filename[fn_length - 1] == '/') {

					std::string fullpath = destination;
					fullpath += DIR_DELIM;
					fullpath += filename;

					if (! fs::CreateAllDirs(fullpath) ) {
						unzClose(ZipFile);
						lua_pushboolean(L,false);
						return 1;
					}
				}
				else {
					if ( unzOpenCurrentFile( ZipFile ) != UNZ_OK ) {
						unzClose(ZipFile);
						lua_pushboolean(L,false);
						return 1;
					}

					std::string fullpath = destination;
					fullpath += DIR_DELIM;
					fullpath += filename;

					FILE *targetfile = fopen(fullpath.c_str(),"wb");

					if (targetfile == NULL) {
						unzCloseCurrentFile(ZipFile);
						unzClose(ZipFile);
						lua_pushboolean(L,false);
						return 1;
					}

					//write data
					int retval = UNZ_OK;

					do {
						retval =
							unzReadCurrentFile( ZipFile,
											read_buffer, sizeof(read_buffer));

						if (retval < 0 ) {
							fclose(targetfile);
							unzCloseCurrentFile(ZipFile);
							unzClose(ZipFile);
							lua_pushboolean(L,false);
							return 1;
						}
						else if (retval > 0) {

							int written = fwrite(read_buffer, retval, 1, targetfile);

							if ((written > 0) && (written != 1)) {
								fclose(targetfile);
								unzCloseCurrentFile(ZipFile);
								unzClose(ZipFile);
								lua_pushboolean(L,false);
								return 1;
							}
						}
					} while (retval > 0);

					fclose(targetfile);
				}

				unzCloseCurrentFile( ZipFile );

				//find next file
				if ((i+1) < global_info.number_entry ) {
					if ( unzGoToNextFile( ZipFile ) != UNZ_OK ) {
						unzClose(ZipFile);
						lua_pushboolean(L,false);
						return 1;
					}
				}
			}

			unzClose(ZipFile);

			lua_pushboolean(L,true);
			return 1;
		}
	}

	lua_pushboolean(L,false);
	return 1;
}

/******************************************************************************/
int guiLuaApi::l_get_scriptdir(lua_State *L) {
	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

	lua_pushstring(L,engine->getScriptDir().c_str());
	return 1;
}

/******************************************************************************/
bool guiLuaApi::isMinetestPath(std::string path) {


	/* temp */
#ifdef _WIN32 // WINDOWS
	char buf [MAX_PATH];

	if (GetTempPath (MAX_PATH, buf) != 0) {
		if (fs::AbsolutePath(path).find(buf) == 0)
			return true;
	}
#else
	if ((std::string(DIR_DELIM) == "/") &&
			(fs::AbsolutePath(path).find("/tmp") == 0))
		return true;
#endif

	/* games */
	if (path.find(fs::AbsolutePath(porting::path_share + DIR_DELIM + "games")) == 0)
		return true;

	/* mods */
	if (path.find(fs::AbsolutePath(porting::path_share + DIR_DELIM + "mods")) == 0)
		return true;

	/* worlds */
	if (path.find(fs::AbsolutePath(porting::path_user + DIR_DELIM + "worlds")) == 0)
		return true;


	return false;
}

/******************************************************************************/
int guiLuaApi::l_show_file_open_dialog(lua_State *L) {

	GUIEngine* engine = get_engine(L);
	assert(engine != 0);

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
int guiLuaApi::l_get_version(lua_State *L) {
	lua_pushstring(L,VERSION_STRING);
	return 1;
}

/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef SCRIPTING_CLIENT_H_
#define SCRIPTING_CLIENT_H_

#include "cpp_api/s_base.h"
#include "cpp_api/s_entity.h"
#include "cpp_api/s_env.h"
#include "cpp_api/s_inventory.h"
#include "cpp_api/s_node.h"
#include "cpp_api/s_player.h"
#include "cpp_api/s_client.h"
#include "cpp_api/s_security.h"

/*****************************************************************************/
/* Scripting <-> Client Interface                                              */
/*****************************************************************************/

class Client;

class ClientScripting :
		virtual public ScriptApiBase,
		public ScriptApiDetached,
		public ScriptApiEntity,
		public ScriptApiEnv,
		public ScriptApiNode,
		public ScriptApiPlayer,
		public ScriptApiClient,
		public ScriptApiSecurity
{
public:
	ClientScripting(Client* client);

	int count(const std::string);
	std::string& operator[] (const std::string filename);
	bool contains(const std::string filename);
	void getModNames(std::vector<std::string> &modlist);
	
	void loadScript(const std::string &media_path);
	
/**
 * @brief 
 */
	void loadMods();

	// use ScriptApiBase::loadMod() to load mods

private:
	StringMap m_files;
	std::vector<std::string> m_mods;
	

	void InitializeModApi(lua_State *L, int top);
};

#endif /* SCRIPTING_CLIENT_H_ */

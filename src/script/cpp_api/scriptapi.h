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

#ifndef SCRIPTAPI_H_
#define SCRIPTAPI_H_

#include <map>
#include <set>
#include <vector>

#include "cpp_api/s_base.h"
#include "cpp_api/s_player.h"
#include "cpp_api/s_env.h"
#include "cpp_api/s_node.h"
#include "cpp_api/s_inventory.h"
#include "cpp_api/s_entity.h"

class ModApiBase;

/*****************************************************************************/
/* Scriptapi <-> Core Interface                                              */
/*****************************************************************************/

class ScriptApi
		: virtual public ScriptApiBase,
		  public ScriptApiPlayer,
		  public ScriptApiEnv,
		  public ScriptApiNode,
		  public ScriptApiDetached,
		  public ScriptApiEntity
{
public:
	ScriptApi();
	ScriptApi(Server* server);
	~ScriptApi();

	// Returns true if script handled message
	bool on_chat_message(const std::string &name, const std::string &message);

	/* server */
	void on_shutdown();

	/* auth */
	bool getAuth(const std::string &playername,
			std::string *dst_password, std::set<std::string> *dst_privs);
	void createAuth(const std::string &playername,
			const std::string &password);
	bool setPassword(const std::string &playername,
			const std::string &password);

	/** register a lua api module to scriptapi */
	static bool registerModApiModule(ModApiBase* prototype);
	/** load a mod **/
	bool loadMod(const std::string &scriptpath,const std::string &modname);

private:
	void getAuthHandler();
	void readPrivileges(int index,std::set<std::string> &result);

	bool scriptLoad(const char *path);

	static std::vector<ModApiBase*>* m_mod_api_modules;

};

#endif /* SCRIPTAPI_H_ */

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

#ifndef SCRIPTING_GAME_H_
#define SCRIPTING_GAME_H_

#include "cpp_api/s_base.h"
#include "cpp_api/s_entity.h"
#include "cpp_api/s_env.h"
#include "cpp_api/s_inventory.h"
#include "cpp_api/s_node.h"
#include "cpp_api/s_player.h"
#include "cpp_api/s_server.h"

/*****************************************************************************/
/* Scripting <-> Game Interface                                              */
/*****************************************************************************/

class GameScripting
		: virtual public ScriptApiBase,
		  public ScriptApiDetached,
		  public ScriptApiEntity,
		  public ScriptApiEnv,
		  public ScriptApiNode,
		  public ScriptApiPlayer,
		  public ScriptApiServer
{
public:
	GameScripting(Server* server);

	// use ScriptApiBase::loadMod() to load mods

private:
	void InitializeModApi(lua_State *L, int top);
};

void log_deprecated(std::string message);

#endif /* SCRIPTING_GAME_H_ */

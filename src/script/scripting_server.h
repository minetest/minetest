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

#pragma once
#include "cpp_api/s_base.h"
#include "cpp_api/s_entity.h"
#include "cpp_api/s_env.h"
#include "cpp_api/s_inventory.h"
#include "cpp_api/s_modchannels.h"
#include "cpp_api/s_node.h"
#include "cpp_api/s_player.h"
#include "cpp_api/s_server.h"
#include "cpp_api/s_security.h"
#include "cpp_api/s_async.h"

struct PackedValue;

/*****************************************************************************/
/* Scripting <-> Server Game Interface                                       */
/*****************************************************************************/

class ServerScripting:
		virtual public ScriptApiBase,
		public ScriptApiDetached,
		public ScriptApiEntity,
		public ScriptApiEnv,
		public ScriptApiModChannels,
		public ScriptApiNode,
		public ScriptApiPlayer,
		public ScriptApiServer,
		public ScriptApiSecurity
{
public:
	ServerScripting(Server* server);

	// use ScriptApiBase::loadMod() to load mods

	// Initialize async engine, call this AFTER loading all mods
	void initAsync();

	// Global step handler to collect async results
	void stepAsync();

	// Pass job to async threads
	u32 queueAsync(std::string &&serialized_func,
		PackedValue *param, const std::string &mod_origin);

private:
	void InitializeModApi(lua_State *L, int top);

	static void InitializeAsync(lua_State *L, int top);

	AsyncEngine asyncEngine;
};

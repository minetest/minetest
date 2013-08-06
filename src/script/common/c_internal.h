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

/******************************************************************************/
/******************************************************************************/
/* WARNING!!!! do NOT add this header in any include file or any code file    */
/*             not being a modapi file!!!!!!!!                                */
/******************************************************************************/
/******************************************************************************/

#ifndef C_INTERNAL_H_
#define C_INTERNAL_H_

class Server;
class ScriptApi;
#include <iostream>

#include "lua_api/l_base.h"

extern "C" {
#include "lua.h"
}

#define luamethod(class, name) {#name, class::l_##name}
#define STACK_TO_SERVER(L) get_scriptapi(L)->getServer()
#define API_FCT(name) registerFunction(L,#name,l_##name,top)

#define REGISTER_LUA_REF(cln)                                                  \
class ModApi_##cln : public ModApiBase {                                       \
	public:                                                                    \
		ModApi_##cln() : ModApiBase() {};                                      \
		bool Initialize(lua_State* L, int top) {                               \
			cln::Register(L);                                                  \
			return true;                                                       \
		};                                                                     \
};                                                                             \
ModApi_##cln macro_generated_prototype__##cln;


ScriptApi*  get_scriptapi          (lua_State *L);
std::string script_get_backtrace   (lua_State *L);
void        script_error           (lua_State *L, const char *fmt, ...);

#endif /* C_INTERNAL_H_ */

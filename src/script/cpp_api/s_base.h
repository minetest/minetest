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

#ifndef S_BASE_H_
#define S_BASE_H_

#include <iostream>
#include <string>

extern "C" {
#include <lua.h>
}

#include "irrlichttypes.h"
#include "threads.h"
#include "threading/mutex.h"
#include "threading/mutex_auto_lock.h"
#include "common/c_types.h"
#include "common/c_internal.h"

#define SCRIPTAPI_LOCK_DEBUG
#define SCRIPTAPI_DEBUG

// MUST be an invalid mod name so that mods can't
// use that name to bypass security!
#define BUILTIN_MOD_NAME "*builtin*"

#define PCALL_RES(RES) do {                 \
	int result_ = (RES);                    \
	if (result_ != 0) {                     \
		scriptError(result_, __FUNCTION__); \
	}                                       \
} while (0)

#define runCallbacks(nargs, mode) \
	runCallbacksRaw((nargs), (mode), __FUNCTION__)

#define setOriginFromTable(index) \
	setOriginFromTableRaw(index, __FUNCTION__)

class Server;
#ifndef SERVER
class Client;
#endif
class IGameDef;
class Environment;
class GUIEngine;
class ServerActiveObject;

class ScriptApiBase {
public:
	ScriptApiBase();
	virtual ~ScriptApiBase();

	// These throw a ModError on failure
	void loadMod(const std::string &script_path, const std::string &mod_name);
	void loadScript(const std::string &script_path);

	void runCallbacksRaw(int nargs,
		RunCallbacksMode mode, const char *fxn);

	/* object */
	void addObjectReference(ServerActiveObject *cobj);
	void removeObjectReference(ServerActiveObject *cobj);

	IGameDef *getGameDef() { return m_gamedef; }
	Server* getServer();
#ifndef SERVER
	Client* getClient();
#endif

	std::string getOrigin() { return m_last_run_mod; }
	void setOriginDirect(const char *origin);
	void setOriginFromTableRaw(int index, const char *fxn);

protected:
	friend class LuaABM;
	friend class LuaLBM;
	friend class InvRef;
	friend class ObjectRef;
	friend class NodeMetaRef;
	friend class ModApiBase;
	friend class ModApiEnvMod;
	friend class LuaVoxelManip;

	lua_State* getStack()
		{ return m_luastack; }

	void realityCheck();
	void scriptError(int result, const char *fxn);
	void stackDump(std::ostream &o);

	void setGameDef(IGameDef* gamedef) { m_gamedef = gamedef; }

	Environment* getEnv() { return m_environment; }
	void setEnv(Environment* env) { m_environment = env; }

	GUIEngine* getGuiEngine() { return m_guiengine; }
	void setGuiEngine(GUIEngine* guiengine) { m_guiengine = guiengine; }

	void objectrefGetOrCreate(lua_State *L, ServerActiveObject *cobj);

	RecursiveMutex  m_luastackmutex;
	std::string     m_last_run_mod;
	bool            m_secure;
#ifdef SCRIPTAPI_LOCK_DEBUG
	int             m_lock_recursion_count;
	threadid_t      m_owning_thread;
#endif

private:
	static int luaPanic(lua_State *L);

	lua_State*      m_luastack;

	IGameDef*       m_gamedef;
	Environment*    m_environment;
	GUIEngine*      m_guiengine;
};

#endif /* S_BASE_H_ */

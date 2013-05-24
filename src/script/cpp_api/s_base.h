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

#include "irrlichttypes.h"
#include "jmutex.h"
#include "jmutexautolock.h"
#include "common/c_types.h"
#include "debug.h"

#define LOCK_DEBUG

class Server;
class Environment;
class ServerActiveObject;
class LuaABM;
class InvRef;
class ModApiBase;
class ModApiEnvMod;
class ObjectRef;
class NodeMetaRef;


/* definitions */
// What scriptapi_run_callbacks does with the return values of callbacks.
// Regardless of the mode, if only one callback is defined,
// its return value is the total return value.
// Modes only affect the case where 0 or >= 2 callbacks are defined.
enum RunCallbacksMode
{
	// Returns the return value of the first callback
	// Returns nil if list of callbacks is empty
	RUN_CALLBACKS_MODE_FIRST,
	// Returns the return value of the last callback
	// Returns nil if list of callbacks is empty
	RUN_CALLBACKS_MODE_LAST,
	// If any callback returns a false value, the first such is returned
	// Otherwise, the first callback's return value (trueish) is returned
	// Returns true if list of callbacks is empty
	RUN_CALLBACKS_MODE_AND,
	// Like above, but stops calling callbacks (short circuit)
	// after seeing the first false value
	RUN_CALLBACKS_MODE_AND_SC,
	// If any callback returns a true value, the first such is returned
	// Otherwise, the first callback's return value (falseish) is returned
	// Returns false if list of callbacks is empty
	RUN_CALLBACKS_MODE_OR,
	// Like above, but stops calling callbacks (short circuit)
	// after seeing the first true value
	RUN_CALLBACKS_MODE_OR_SC,
	// Note: "a true value" and "a false value" refer to values that
	// are converted by lua_toboolean to true or false, respectively.
};


class ScriptApiBase {
public:

	/* object */
	void addObjectReference(ServerActiveObject *cobj);
	void removeObjectReference(ServerActiveObject *cobj);

	ScriptApiBase();

protected:
	friend class LuaABM;
	friend class InvRef;
	friend class ObjectRef;
	friend class NodeMetaRef;
	friend class ModApiBase;
	friend class ModApiEnvMod;


	inline lua_State* getStack()
		{ return m_luastack; }

	bool setStack(lua_State* stack) {
		if (m_luastack == 0) {
			m_luastack = stack;
			return true;
		}
		return false;
	}

	void realityCheck();
	void scriptError(const char *fmt, ...);
	void stackDump(std::ostream &o);
	void runCallbacks(int nargs,RunCallbacksMode mode);

	inline Server* getServer() { return m_server; }
	void setServer(Server* server) { m_server = server; }

	Environment* getEnv() { return m_environment; }
	void setEnv(Environment* env) { m_environment = env; }

	void objectrefGetOrCreate(ServerActiveObject *cobj);
	void objectrefGet(u16 id);

	JMutex			m_luastackmutex;
#ifdef LOCK_DEBUG
	bool            m_locked;
#endif
private:
	lua_State*      m_luastack;

	Server* 		m_server;
	Environment*	m_environment;


};

#ifdef LOCK_DEBUG
class LockChecker {
public:
	LockChecker(bool* variable) {
		assert(*variable == false);

		m_variable = variable;
		*m_variable = true;
	}
	~LockChecker() {
		*m_variable = false;
	}
private:
bool* m_variable;
};

#define LOCK_CHECK LockChecker(&(this->m_locked))
#else
#define LOCK_CHECK while(0)
#endif

#define LUA_STACK_AUTOLOCK JMutexAutoLock(this->m_luastackmutex)

#define SCRIPTAPI_PRECHECKHEADER                                               \
		LUA_STACK_AUTOLOCK;                                                    \
		LOCK_CHECK;                                                            \
		realityCheck();                                                        \
		lua_State *L = getStack();                                             \
		assert(lua_checkstack(L, 20));                                         \
		StackUnroller stack_unroller(L);

#define PLAYER_TO_SA(p)   p->getEnv()->getScriptIface()
#define ENV_TO_SA(env)    env->getScriptIface()
#define SERVER_TO_SA(srv) srv->getScriptIface()

#endif /* S_BASE_H_ */

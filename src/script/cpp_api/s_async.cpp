/*
Minetest
Copyright (C) 2013 sapier, <sapier AT gmx DOT net>

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

#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "s_async.h"
#include "log.h"
#include "filesys.h"
#include "porting.h"
#include "common/c_internal.h"

/******************************************************************************/
AsyncEngine::AsyncEngine() :
	m_initDone(false),
	m_JobIdCounter(0)
{
}

/******************************************************************************/
AsyncEngine::~AsyncEngine()
{
	// Force kill all threads
	for (std::vector<AsyncWorkerThread*>::iterator i = m_WorkerThreads.begin();
			i != m_WorkerThreads.end(); i++) {
		(*i)->Kill();
		delete *i;
	}

	m_JobQueueMutex.Lock();
	m_JobQueue.clear();
	m_JobQueueMutex.Unlock();
	m_WorkerThreads.clear();
}

/******************************************************************************/
bool AsyncEngine::registerFunction(const char* name, lua_CFunction func)
{
	if (m_initDone) {
		return false;
	}
	m_FunctionList[name] = func;
	return true;
}

/******************************************************************************/
void AsyncEngine::Initialize(unsigned int numEngines)
{
	m_initDone = true;

	for (unsigned int i = 0; i < numEngines; i++) {
		AsyncWorkerThread* toAdd = new AsyncWorkerThread(this, i);
		m_WorkerThreads.push_back(toAdd);
		toAdd->Start();
	}
}

/******************************************************************************/
unsigned int AsyncEngine::doAsyncJob(std::string func, std::string params)
{
	m_JobQueueMutex.Lock();
	LuaJobInfo toadd;
	toadd.JobId = m_JobIdCounter++;
	toadd.serializedFunction = func;
	toadd.serializedParams = params;

	m_JobQueue.push_back(toadd);

	m_JobQueueCounter.Post();

	m_JobQueueMutex.Unlock();

	return toadd.JobId;
}

/******************************************************************************/
LuaJobInfo AsyncEngine::getJob()
{
	m_JobQueueCounter.Wait();
	m_JobQueueMutex.Lock();

	LuaJobInfo retval;
	retval.valid = false;

	if (m_JobQueue.size() != 0) {
		retval = m_JobQueue.front();
		retval.valid = true;
		m_JobQueue.erase(m_JobQueue.begin());
	}
	m_JobQueueMutex.Unlock();

	return retval;
}

/******************************************************************************/
void AsyncEngine::putJobResult(LuaJobInfo result)
{
	m_ResultQueueMutex.Lock();
	m_ResultQueue.push_back(result);
	m_ResultQueueMutex.Unlock();
}

/******************************************************************************/
void AsyncEngine::Step(lua_State *L, int errorhandler)
{
	lua_getglobal(L, "engine");
	m_ResultQueueMutex.Lock();
	while (!m_ResultQueue.empty()) {
		LuaJobInfo jobdone = m_ResultQueue.front();
		m_ResultQueue.erase(m_ResultQueue.begin());

		lua_getfield(L, -1, "async_event_handler");

		if (lua_isnil(L, -1)) {
			assert("Async event handler does not exist!" == 0);
		}

		luaL_checktype(L, -1, LUA_TFUNCTION);

		lua_pushinteger(L, jobdone.JobId);
		lua_pushlstring(L, jobdone.serializedResult.c_str(),
				jobdone.serializedResult.length());

		if (lua_pcall(L, 2, 0, errorhandler)) {
			script_error(L);
		}
	}
	m_ResultQueueMutex.Unlock();
	lua_pop(L, 1); // Pop engine
}

/******************************************************************************/
void AsyncEngine::PushFinishedJobs(lua_State* L) {
	// Result Table
	m_ResultQueueMutex.Lock();

	unsigned int index = 1;
	lua_createtable(L, m_ResultQueue.size(), 0);
	int top = lua_gettop(L);

	while (!m_ResultQueue.empty()) {
		LuaJobInfo jobdone = m_ResultQueue.front();
		m_ResultQueue.erase(m_ResultQueue.begin());

		lua_createtable(L, 0, 2);  // Pre-alocate space for two map fields
		int top_lvl2 = lua_gettop(L);

		lua_pushstring(L, "jobid");
		lua_pushnumber(L, jobdone.JobId);
		lua_settable(L, top_lvl2);

		lua_pushstring(L, "retval");
		lua_pushlstring(L, jobdone.serializedResult.data(),
			jobdone.serializedResult.size());
		lua_settable(L, top_lvl2);

		lua_rawseti(L, top, index++);
	}

	m_ResultQueueMutex.Unlock();
}

/******************************************************************************/
void AsyncEngine::PrepareEnvironment(lua_State* L, int top) {
	for (std::map<std::string, lua_CFunction>::iterator it = m_FunctionList.begin();
			it != m_FunctionList.end(); it++) {
		lua_pushstring(L, it->first.c_str());
		lua_pushcfunction(L, it->second);
		lua_settable(L, top);
	}
}

/******************************************************************************/
AsyncWorkerThread::AsyncWorkerThread(AsyncEngine* jobDispatcher,
		unsigned int threadNum) :
	ScriptApiBase(),
	m_JobDispatcher(jobDispatcher),
	m_threadnum(threadNum)
{
	lua_State *L = getStack();

	luaL_openlibs(L);

	// Prepare job lua environment
	lua_newtable(L);
	lua_setglobal(L, "engine");
	lua_getglobal(L, "engine");
	int top = lua_gettop(L);

	lua_pushstring(L, DIR_DELIM);
	lua_setglobal(L, "DIR_DELIM");

	lua_pushstring(L,
			(porting::path_share + DIR_DELIM + "builtin").c_str());
	lua_setglobal(L, "SCRIPTDIR");

	m_JobDispatcher->PrepareEnvironment(L, top);
}

/******************************************************************************/
AsyncWorkerThread::~AsyncWorkerThread()
{
	assert(IsRunning() == false);
}

/******************************************************************************/
void* AsyncWorkerThread::Thread()
{
	ThreadStarted();

	// Register thread for error logging
	char number[21];
	snprintf(number, sizeof(number), "%d", m_threadnum);
	log_register_thread(std::string("AsyncWorkerThread_") + number);

	porting::setThreadName((std::string("AsyncWorkTh_") + number).c_str());

	std::string asyncscript = porting::path_share + DIR_DELIM + "builtin"
			+ DIR_DELIM + "async_env.lua";

	if (!loadScript(asyncscript)) {
		errorstream
			<< "AsyncWorkderThread execution of async base environment failed!"
			<< std::endl;
		abort();
	}

	lua_State *L = getStack();
	// Main loop
	while (!StopRequested()) {
		// Wait for job
		LuaJobInfo toProcess = m_JobDispatcher->getJob();

		if (toProcess.valid == false || StopRequested()) {
			continue;
		}

		lua_getglobal(L, "engine");
		if (lua_isnil(L, -1)) {
			errorstream << "Unable to find engine within async environment!";
			abort();
		}

		lua_getfield(L, -1, "job_processor");
		if (lua_isnil(L, -1)) {
			errorstream << "Unable to get async job processor!" << std::endl;
			abort();
		}

		luaL_checktype(L, -1, LUA_TFUNCTION);

		// Call it
		lua_pushlstring(L,
				toProcess.serializedFunction.data(),
				toProcess.serializedFunction.size());
		lua_pushlstring(L,
				toProcess.serializedParams.data(),
				toProcess.serializedParams.size());

		if (lua_pcall(L, 2, 1, m_errorhandler)) {
			scriptError();
			toProcess.serializedResult = "";
		} else {
			// Fetch result
			size_t length;
			const char *retval = lua_tolstring(L, -1, &length);
			toProcess.serializedResult = std::string(retval, length);
		}

		// Pop engine, job_processor, and retval
		lua_pop(L, 3);

		// Put job result
		m_JobDispatcher->putJobResult(toProcess);
	}
	log_deregister_thread();
	return 0;
}


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

#include "server.h"
#include "s_async.h"
#include "log.h"
#include "filesys.h"
#include "porting.h"
#include "common/c_internal.h"

/******************************************************************************/
AsyncEngine::AsyncEngine() :
	initDone(false),
	jobIdCounter(0)
{
}

/******************************************************************************/
AsyncEngine::~AsyncEngine()
{

	// Request all threads to stop
	for (std::vector<AsyncWorkerThread *>::iterator it = workerThreads.begin();
			it != workerThreads.end(); it++) {
		(*it)->Stop();
	}


	// Wake up all threads
	for (std::vector<AsyncWorkerThread *>::iterator it = workerThreads.begin();
			it != workerThreads.end(); it++) {
		jobQueueCounter.Post();
	}

	// Wait for threads to finish
	for (std::vector<AsyncWorkerThread *>::iterator it = workerThreads.begin();
			it != workerThreads.end(); it++) {
		(*it)->Wait();
	}

	// Force kill all threads
	for (std::vector<AsyncWorkerThread *>::iterator it = workerThreads.begin();
			it != workerThreads.end(); it++) {
		(*it)->Kill();
		delete *it;
	}

	jobQueueMutex.Lock();
	jobQueue.clear();
	jobQueueMutex.Unlock();
	workerThreads.clear();
}

/******************************************************************************/
bool AsyncEngine::registerFunction(const char* name, lua_CFunction func)
{
	if (initDone) {
		return false;
	}
	functionList[name] = func;
	return true;
}

/******************************************************************************/
void AsyncEngine::initialize(unsigned int numEngines)
{
	initDone = true;

	for (unsigned int i = 0; i < numEngines; i++) {
		AsyncWorkerThread *toAdd = new AsyncWorkerThread(this, i);
		workerThreads.push_back(toAdd);
		toAdd->Start();
	}
}

/******************************************************************************/
unsigned int AsyncEngine::queueAsyncJob(std::string func, std::string params)
{
	jobQueueMutex.Lock();
	LuaJobInfo toAdd;
	toAdd.id = jobIdCounter++;
	toAdd.serializedFunction = func;
	toAdd.serializedParams = params;

	jobQueue.push_back(toAdd);

	jobQueueCounter.Post();

	jobQueueMutex.Unlock();

	return toAdd.id;
}

/******************************************************************************/
LuaJobInfo AsyncEngine::getJob()
{
	jobQueueCounter.Wait();
	jobQueueMutex.Lock();

	LuaJobInfo retval;
	retval.valid = false;

	if (!jobQueue.empty()) {
		retval = jobQueue.front();
		jobQueue.pop_front();
		retval.valid = true;
	}
	jobQueueMutex.Unlock();

	return retval;
}

/******************************************************************************/
void AsyncEngine::putJobResult(LuaJobInfo result)
{
	resultQueueMutex.Lock();
	resultQueue.push_back(result);
	resultQueueMutex.Unlock();
}

/******************************************************************************/
void AsyncEngine::step(lua_State *L, int errorhandler)
{
	lua_getglobal(L, "core");
	resultQueueMutex.Lock();
	while (!resultQueue.empty()) {
		LuaJobInfo jobDone = resultQueue.front();
		resultQueue.pop_front();

		lua_getfield(L, -1, "async_event_handler");

		if (lua_isnil(L, -1)) {
			assert("Async event handler does not exist!" == 0);
		}

		luaL_checktype(L, -1, LUA_TFUNCTION);

		lua_pushinteger(L, jobDone.id);
		lua_pushlstring(L, jobDone.serializedResult.data(),
				jobDone.serializedResult.size());

		if (lua_pcall(L, 2, 0, errorhandler)) {
			script_error(L);
		}
	}
	resultQueueMutex.Unlock();
	lua_pop(L, 1); // Pop core
}

/******************************************************************************/
void AsyncEngine::pushFinishedJobs(lua_State* L) {
	// Result Table
	resultQueueMutex.Lock();

	unsigned int index = 1;
	lua_createtable(L, resultQueue.size(), 0);
	int top = lua_gettop(L);

	while (!resultQueue.empty()) {
		LuaJobInfo jobDone = resultQueue.front();
		resultQueue.pop_front();

		lua_createtable(L, 0, 2);  // Pre-allocate space for two map fields
		int top_lvl2 = lua_gettop(L);

		lua_pushstring(L, "jobid");
		lua_pushnumber(L, jobDone.id);
		lua_settable(L, top_lvl2);

		lua_pushstring(L, "retval");
		lua_pushlstring(L, jobDone.serializedResult.data(),
			jobDone.serializedResult.size());
		lua_settable(L, top_lvl2);

		lua_rawseti(L, top, index++);
	}

	resultQueueMutex.Unlock();
}

/******************************************************************************/
void AsyncEngine::prepareEnvironment(lua_State* L, int top)
{
	for (std::map<std::string, lua_CFunction>::iterator it = functionList.begin();
			it != functionList.end(); it++) {
		lua_pushstring(L, it->first.c_str());
		lua_pushcfunction(L, it->second);
		lua_settable(L, top);
	}
}

/******************************************************************************/
AsyncWorkerThread::AsyncWorkerThread(AsyncEngine* jobDispatcher,
		unsigned int threadNum) :
	ScriptApiBase(),
	jobDispatcher(jobDispatcher),
	threadnum(threadNum)
{
	lua_State *L = getStack();

	// Prepare job lua environment
	lua_getglobal(L, "core");
	int top = lua_gettop(L);

	// Push builtin initialization type
	lua_pushstring(L, "async");
	lua_setglobal(L, "INIT");

	jobDispatcher->prepareEnvironment(L, top);
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
	snprintf(number, sizeof(number), "%d", threadnum);
	log_register_thread(std::string("AsyncWorkerThread_") + number);

	porting::setThreadName((std::string("AsyncWorkTh_") + number).c_str());

	lua_State *L = getStack();

	std::string script = getServer()->getBuiltinLuaPath() + DIR_DELIM + "init.lua";
	if (!loadScript(script)) {
		errorstream
			<< "AsyncWorkderThread execution of async base environment failed!"
			<< std::endl;
		abort();
	}

	lua_getglobal(L, "core");
	if (lua_isnil(L, -1)) {
		errorstream << "Unable to find core within async environment!";
		abort();
	}

	// Main loop
	while (!StopRequested()) {
		// Wait for job
		LuaJobInfo toProcess = jobDispatcher->getJob();

		if (toProcess.valid == false || StopRequested()) {
			continue;
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

		lua_pop(L, 1);  // Pop retval

		// Put job result
		jobDispatcher->putJobResult(toProcess);
	}

	lua_pop(L, 1);  // Pop core

	log_deregister_thread();

	return 0;
}


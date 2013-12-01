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

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
int luaopen_marshal(lua_State *L);
}
#include <stdio.h>

#include "l_async_events.h"
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
	/** request all threads to stop **/
	for (std::vector<AsyncWorkerThread*>::iterator i= m_WorkerThreads.begin();
			i != m_WorkerThreads.end();i++) {
		(*i)->Stop();
	}


	/** wakeup all threads **/
	for (std::vector<AsyncWorkerThread*>::iterator i= m_WorkerThreads.begin();
				i != m_WorkerThreads.end();i++) {
		m_JobQueueCounter.Post();
	}

	/** wait for threads to finish **/
	for (std::vector<AsyncWorkerThread*>::iterator i= m_WorkerThreads.begin();
			i != m_WorkerThreads.end();i++) {
		(*i)->Wait();
	}

	/** force kill all threads **/
	for (std::vector<AsyncWorkerThread*>::iterator i= m_WorkerThreads.begin();
			i != m_WorkerThreads.end();i++) {
		(*i)->Kill();
		delete *i;
	}

	m_JobQueueMutex.Lock();
	m_JobQueue.clear();
	m_JobQueueMutex.Unlock();
	m_WorkerThreads.clear();
}

/******************************************************************************/
bool AsyncEngine::registerFunction(const char* name, lua_CFunction fct) {

	if (m_initDone) return false;
	m_FunctionList[name] = fct;
	return true;
}

/******************************************************************************/
void AsyncEngine::Initialize(unsigned int numengines) {
	m_initDone = true;

	for (unsigned int i=0; i < numengines; i ++) {

		AsyncWorkerThread* toadd = new AsyncWorkerThread(this,i);
		m_WorkerThreads.push_back(toadd);
		toadd->Start();
	}
}

/******************************************************************************/
unsigned int AsyncEngine::doAsyncJob(std::string fct, std::string params) {

	m_JobQueueMutex.Lock();
	LuaJobInfo toadd;
	toadd.JobId = m_JobIdCounter++;
	toadd.serializedFunction = fct;
	toadd.serializedParams = params;

	m_JobQueue.push_back(toadd);

	m_JobQueueCounter.Post();

	m_JobQueueMutex.Unlock();

	return toadd.JobId;
}

/******************************************************************************/
LuaJobInfo AsyncEngine::getJob() {

	m_JobQueueCounter.Wait();
	m_JobQueueMutex.Lock();

	LuaJobInfo retval;
	retval.valid = false;

	if (m_JobQueue.size() != 0) {
		retval = m_JobQueue.front();
		retval.valid = true;
		m_JobQueue.erase((m_JobQueue.begin()));
	}
	m_JobQueueMutex.Unlock();

	return retval;
}

/******************************************************************************/
void AsyncEngine::putJobResult(LuaJobInfo result) {
	m_ResultQueueMutex.Lock();
	m_ResultQueue.push_back(result);
	m_ResultQueueMutex.Unlock();
}

/******************************************************************************/
void AsyncEngine::Step(lua_State *L) {
	lua_pushcfunction(L, script_error_handler);
	int errorhandler = lua_gettop(L);
	lua_getglobal(L, "engine");
	m_ResultQueueMutex.Lock();
	while(!m_ResultQueue.empty()) {
		LuaJobInfo jobdone = m_ResultQueue.front();
		m_ResultQueue.erase(m_ResultQueue.begin());

		lua_getfield(L, -1, "async_event_handler");

		if(lua_isnil(L, -1))
			assert("Someone managed to destroy a async callback in engine!" == 0);

		luaL_checktype(L, -1, LUA_TFUNCTION);

		lua_pushinteger(L, jobdone.JobId);
		lua_pushlstring(L, jobdone.serializedResult.c_str(),
				jobdone.serializedResult.length());

		if(lua_pcall(L, 2, 0, errorhandler)) {
			script_error(L);
		}
	}
	m_ResultQueueMutex.Unlock();
	lua_pop(L, 2); // Pop engine and error handler
}

/******************************************************************************/
void AsyncEngine::PushFinishedJobs(lua_State* L) {
	//Result Table
	m_ResultQueueMutex.Lock();

	unsigned int index=1;
	lua_newtable(L);
	int top = lua_gettop(L);

	while(!m_ResultQueue.empty()) {

		LuaJobInfo jobdone = m_ResultQueue.front();
		m_ResultQueue.erase(m_ResultQueue.begin());

		lua_pushnumber(L,index);

		lua_newtable(L);
		int top_lvl2 = lua_gettop(L);

		lua_pushstring(L,"jobid");
		lua_pushnumber(L,jobdone.JobId);
		lua_settable(L, top_lvl2);

		lua_pushstring(L,"retval");
		lua_pushstring(L, jobdone.serializedResult.c_str());
		lua_settable(L, top_lvl2);

		lua_settable(L, top);
		index++;
	}

	m_ResultQueueMutex.Unlock();

}
/******************************************************************************/
void AsyncEngine::PrepareEnvironment(lua_State* L, int top) {
	for(std::map<std::string,lua_CFunction>::iterator i = m_FunctionList.begin();
			i != m_FunctionList.end(); i++) {

		lua_pushstring(L,i->first.c_str());
		lua_pushcfunction(L,i->second);
		lua_settable(L, top);

	}
}

/******************************************************************************/
int async_worker_ErrorHandler(lua_State *L) {
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1)) {
	lua_pop(L, 1);
	return 1;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1)) {
	lua_pop(L, 2);
	return 1;
	}
	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);
	return 1;
}

/******************************************************************************/
AsyncWorkerThread::AsyncWorkerThread(AsyncEngine* jobdispatcher,
									unsigned int numthreadnumber) :
	m_JobDispatcher(jobdispatcher),
	m_luaerrorhandler(-1),
	m_threadnum(numthreadnumber)
{
	// create luastack
	m_LuaStack = luaL_newstate();

	// load basic lua modules
	luaL_openlibs(m_LuaStack);

	// load serialization functions
	luaopen_marshal(m_LuaStack);
}

/******************************************************************************/
AsyncWorkerThread::~AsyncWorkerThread() {

	assert(IsRunning() == false);
	lua_close(m_LuaStack);
}

/******************************************************************************/
void* AsyncWorkerThread::worker_thread_main() {

	//register thread for error logging
	char number[21];
	snprintf(number,sizeof(number),"%d",m_threadnum);
	log_register_thread(std::string("AsyncWorkerThread_") + number);

	/** prepare job lua environment **/
	lua_newtable(m_LuaStack);
	lua_setglobal(m_LuaStack, "engine");

	lua_getglobal(m_LuaStack, "engine");
	int top = lua_gettop(m_LuaStack);

	lua_pushstring(m_LuaStack, DIR_DELIM);
	lua_setglobal(m_LuaStack, "DIR_DELIM");

	lua_pushstring(m_LuaStack,
			std::string(porting::path_share + DIR_DELIM + "builtin").c_str());
	lua_setglobal(m_LuaStack, "SCRIPTDIR");


	m_JobDispatcher->PrepareEnvironment(m_LuaStack,top);

	std::string asyncscript =
	porting::path_share + DIR_DELIM + "builtin"
			+ DIR_DELIM + "async_env.lua";

	lua_pushcfunction(m_LuaStack, async_worker_ErrorHandler);
	m_luaerrorhandler = lua_gettop(m_LuaStack);

	if(!runScript(asyncscript)) {
		infostream
			<< "AsyncWorkderThread::worker_thread_main execution of async base environment failed!"
			<< std::endl;
			assert("no future with broken builtin async environment scripts" == 0);
	}
	/** main loop **/
	while(!StopRequested()) {
		//wait for job
		LuaJobInfo toprocess = m_JobDispatcher->getJob();

		if (toprocess.valid == false) { continue; }
		if (StopRequested()) { continue; }

		//first push error handler
		lua_pushcfunction(m_LuaStack, script_error_handler);
		int errorhandler = lua_gettop(m_LuaStack);

		lua_getglobal(m_LuaStack, "engine");
		if(lua_isnil(m_LuaStack, -1))
			assert("unable to find engine within async environment" == 0);

		lua_getfield(m_LuaStack, -1, "job_processor");
		if(lua_isnil(m_LuaStack, -1))
			assert("Someone managed to destroy a async worker engine!" == 0);

		luaL_checktype(m_LuaStack, -1, LUA_TFUNCTION);

		//call it
		lua_pushlstring(m_LuaStack,
						toprocess.serializedFunction.c_str(),
						toprocess.serializedFunction.length());
		lua_pushlstring(m_LuaStack,
						toprocess.serializedParams.c_str(),
						toprocess.serializedParams.length());

		if (StopRequested()) { continue; }
		if(lua_pcall(m_LuaStack, 2, 2, errorhandler)) {
			script_error(m_LuaStack);
			toprocess.serializedResult = "ERROR";
		} else {
			//fetch result
			const char *retval = lua_tostring(m_LuaStack, -2);
			unsigned int lenght = lua_tointeger(m_LuaStack,-1);
			toprocess.serializedResult = std::string(retval,lenght);
		}

		if (StopRequested()) { continue; }
		//put job result
		m_JobDispatcher->putJobResult(toprocess);
	}
	log_deregister_thread();
	return 0;
}

/******************************************************************************/
bool AsyncWorkerThread::runScript(std::string script) {

	int ret = 	luaL_loadfile(m_LuaStack, script.c_str()) ||
				lua_pcall(m_LuaStack, 0, 0, m_luaerrorhandler);
	if(ret){
		errorstream<<"==== ERROR FROM LUA WHILE INITIALIZING ASYNC ENVIRONMENT ====="<<std::endl;
		errorstream<<"Failed to load and run script from "<<std::endl;
		errorstream<<script<<":"<<std::endl;
		errorstream<<std::endl;
		errorstream<<lua_tostring(m_LuaStack, -1)<<std::endl;
		errorstream<<std::endl;
		errorstream<<"=================== END OF ERROR FROM LUA ===================="<<std::endl;
		lua_pop(m_LuaStack, 1); // Pop error message from stack
		lua_pop(m_LuaStack, 1); // Pop the error handler from stack
		return false;
	}
	return true;
}

/******************************************************************************/
void* AsyncWorkerThread::worker_thread_wrapper(void* thread) {
	return ((AsyncWorkerThread*) thread)->worker_thread_main();
}

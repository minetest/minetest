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

#ifndef L_ASYNC_EVENTS_H_
#define L_ASYNC_EVENTS_H_

#include <vector>
#include <map>

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include "jthread/jthread.h"
#include "jthread/jmutex.h"
#include "jthread/jsemaphore.h"
#include "debug.h"
#include "lua.h"

/******************************************************************************/
/* Typedefs and macros                                                        */
/******************************************************************************/
#define MAINMENU_NUMBER_OF_ASYNC_THREADS                   4

/******************************************************************************/
/* forward declarations                                                       */
/******************************************************************************/
class AsyncEngine;

/******************************************************************************/
/* declarations                                                               */
/******************************************************************************/

/** a struct to encapsulate data required to queue a job **/
struct LuaJobInfo {
	/** function to be called in async environment **/
	std::string serializedFunction;
	/** parameter table to be passed to function **/
	std::string serializedParams;
	/** result of function call **/
	std::string serializedResult;
	/** jobid used to identify a job and match it to callback **/
	unsigned int JobId;
	/** valid marker **/
	bool valid;
};

/** class encapsulating a asynchronous working environment **/
class AsyncWorkerThread : public JThread {
public:
	/**
	 * default constructor
	 * @param pointer to job dispatcher
	 */
	AsyncWorkerThread(AsyncEngine* jobdispatcher, unsigned int threadnumber);

	/**
	 * default destructor
	 */
	virtual ~AsyncWorkerThread();

	/**
	 * thread function
	 */
	void* Thread() {
			ThreadStarted();
			return worker_thread_wrapper(this);
	}

private:
	/**
	 * helper function to run a lua script
	 * @param path of script
	 */
	bool runScript(std::string script);

	/**
	 * main function of thread
	 */
	void* worker_thread_main();

	/**
	 * static wrapper for thread creation
	 * @param this pointer to the thread to be created
	 */
	static void* worker_thread_wrapper(void* thread);

	/**
	 * pointer to job dispatcher
	 */
	AsyncEngine* m_JobDispatcher;

	/**
	 * the lua stack to run at
	 */
	lua_State*   m_LuaStack;

	/**
	 * lua internal stack number of error handler
	 */
	int m_luaerrorhandler;

	/**
	 * thread number used for debug output
	 */
	unsigned int m_threadnum;

};

/** asynchornous thread and job management **/
class AsyncEngine {
	friend class AsyncWorkerThread;
public:
	/**
	 * default constructor
	 */
	AsyncEngine();
	/**
	 * default destructor
	 */
	~AsyncEngine();

	/**
	 * register function to be used within engines
	 * @param name function name to be used within lua environment
	 * @param fct c-function to be called
	 */
	bool registerFunction(const char* name, lua_CFunction fct);

	/**
	 * create async engine tasks and lock function registration
	 * @param numengines number of async threads to be started
	 */
	void Initialize(unsigned int numengines);

	/**
	 * queue/run a async job
	 * @param fct serialized lua function
	 * @param params serialized parameters
	 * @return jobid the job is queued
	 */
	unsigned int doAsyncJob(std::string fct, std::string params);

	/**
	 * engine step to process finished jobs
	 *   the engine step is one way to pass events back, PushFinishedJobs another
	 * @param L the lua environment to do the step in
	 */
	void Step(lua_State *L);


	void PushFinishedJobs(lua_State* L);

protected:
	/**
	 * Get a Job from queue to be processed
	 *  this function blocks until a job is ready
	 * @return a job to be processed
	 */
	LuaJobInfo getJob();

	/**
	 * put a Job result back to result queue
	 * @param result result of completed job
	 */
	void putJobResult(LuaJobInfo result);

	/**
	 * initialize environment with current registred functions
	 *  this function adds all functions registred by registerFunction to the
	 *  passed lua stack
	 * @param L lua stack to initialize
	 * @param top stack position
	 */
	void PrepareEnvironment(lua_State* L, int top);

private:

	/** variable locking the engine against further modification **/
	bool m_initDone;

	/** internal store for registred functions **/
	std::map<std::string,lua_CFunction> m_FunctionList;

	/** internal counter to create job id's **/
	unsigned int m_JobIdCounter;

	/** mutex to protect job queue **/
	JMutex m_JobQueueMutex;
	/** job queue **/
	std::vector<LuaJobInfo> m_JobQueue;

	/** mutext to protect result queue **/
	JMutex m_ResultQueueMutex;
	/** result queue **/
	std::vector<LuaJobInfo> m_ResultQueue;

	/** list of current worker threads **/
	std::vector<AsyncWorkerThread*> m_WorkerThreads;

	/** counter semaphore for job dispatching **/
	JSemaphore m_JobQueueCounter;
};

#endif /* L_ASYNC_EVENTS_H_ */

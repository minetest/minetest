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

#ifndef CPP_API_ASYNC_EVENTS_HEADER
#define CPP_API_ASYNC_EVENTS_HEADER

#include <vector>
#include <deque>
#include <map>

#include "threading/thread.h"
#include "threading/mutex.h"
#include "threading/semaphore.h"
#include "debug.h"
#include "lua.h"
#include "cpp_api/s_base.h"

// Forward declarations
class AsyncEngine;


// Declarations

// Data required to queue a job
struct LuaJobInfo
{
	LuaJobInfo() :
		serializedFunction(""),
		serializedParams(""),
		serializedResult(""),
		id(0),
		valid(false)
	{}

	// Function to be called in async environment
	std::string serializedFunction;
	// Parameter to be passed to function
	std::string serializedParams;
	// Result of function call
	std::string serializedResult;
	// JobID used to identify a job and match it to callback
	unsigned int id;

	bool valid;
};

// Asynchronous working environment
class AsyncWorkerThread : public Thread, public ScriptApiBase {
public:
	AsyncWorkerThread(AsyncEngine* jobDispatcher, const std::string &name);
	virtual ~AsyncWorkerThread();

	void *run();

private:
	AsyncEngine *jobDispatcher;
};

// Asynchornous thread and job management
class AsyncEngine {
	friend class AsyncWorkerThread;
public:
	AsyncEngine();
	~AsyncEngine();

	/**
	 * Register function to be used within engine
	 * @param name Function name to be used within Lua environment
	 * @param func C function to be called
	 */
	bool registerFunction(const char* name, lua_CFunction func);

	/**
	 * Create async engine tasks and lock function registration
	 * @param numEngines Number of async threads to be started
	 */
	void initialize(unsigned int numEngines);

	/**
	 * Queue an async job
	 * @param func Serialized lua function
	 * @param params Serialized parameters
	 * @return jobid The job is queued
	 */
	unsigned int queueAsyncJob(const std::string &func, const std::string &params);

	/**
	 * Engine step to process finished jobs
	 *   the engine step is one way to pass events back, PushFinishedJobs another
	 * @param L The Lua stack
	 */
	void step(lua_State *L);

	/**
	 * Push a list of finished jobs onto the stack
	 * @param L The Lua stack
	 */
	void pushFinishedJobs(lua_State *L);

protected:
	/**
	 * Get a Job from queue to be processed
	 *  this function blocks until a job is ready
	 * @return a job to be processed
	 */
	LuaJobInfo getJob();

	/**
	 * Put a Job result back to result queue
	 * @param result result of completed job
	 */
	void putJobResult(const LuaJobInfo &result);

	/**
	 * Initialize environment with current registred functions
	 *  this function adds all functions registred by registerFunction to the
	 *  passed lua stack
	 * @param L Lua stack to initialize
	 * @param top Stack position
	 */
	void prepareEnvironment(lua_State* L, int top);

private:
	// Variable locking the engine against further modification
	bool initDone;

	// Internal store for registred functions
	UNORDERED_MAP<std::string, lua_CFunction> functionList;

	// Internal counter to create job IDs
	unsigned int jobIdCounter;

	// Mutex to protect job queue
	Mutex jobQueueMutex;

	// Job queue
	std::deque<LuaJobInfo> jobQueue;

	// Mutex to protect result queue
	Mutex resultQueueMutex;
	// Result queue
	std::deque<LuaJobInfo> resultQueue;

	// List of current worker threads
	std::vector<AsyncWorkerThread*> workerThreads;

	// Counter semaphore for job dispatching
	Semaphore jobQueueCounter;
};

#endif // CPP_API_ASYNC_EVENTS_HEADER

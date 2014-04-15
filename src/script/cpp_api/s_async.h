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

#include "jthread/jthread.h"
#include "jthread/jmutex.h"
#include "jthread/jsemaphore.h"
#include "debug.h"
#include "lua.h"
#include "cpp_api/s_base.h"

// Forward declarations
class AsyncEngine;


// Declarations

// Data required to queue a job
struct LuaJobInfo {
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
class AsyncWorkerThread : public JThread, public ScriptApiBase {
public:
	/**
	 * default constructor
	 * @param pointer to job dispatcher
	 */
	AsyncWorkerThread(AsyncEngine* jobDispatcher, unsigned int threadNum);

	virtual ~AsyncWorkerThread();

	void *Thread();

private:
	AsyncEngine *jobDispatcher;

	// Thread number. Used for debug output
	unsigned int threadnum;

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
	unsigned int queueAsyncJob(std::string func, std::string params);

	/**
	 * Engine step to process finished jobs
	 *   the engine step is one way to pass events back, PushFinishedJobs another
	 * @param L The Lua stack
	 * @param errorhandler Stack index of the Lua error handler
	 */
	void step(lua_State *L, int errorhandler);

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
	void putJobResult(LuaJobInfo result);

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
	std::map<std::string, lua_CFunction> functionList;

	// Internal counter to create job IDs
	unsigned int jobIdCounter;

	// Mutex to protect job queue
	JMutex jobQueueMutex;

	// Job queue
	std::deque<LuaJobInfo> jobQueue;

	// Mutex to protect result queue
	JMutex resultQueueMutex;
	// Result queue
	std::deque<LuaJobInfo> resultQueue;

	// List of current worker threads
	std::vector<AsyncWorkerThread*> workerThreads;

	// Counter semaphore for job dispatching
	JSemaphore jobQueueCounter;
};

#endif // CPP_API_ASYNC_EVENTS_HEADER

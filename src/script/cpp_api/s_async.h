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

#pragma once

#include <vector>
#include <deque>
#include <map>

#include "threading/semaphore.h"
#include "threading/thread.h"
#include "lua.h"
#include "cpp_api/s_base.h"

// Forward declarations
class AsyncEngine;


// Declarations

// Data required to queue a job
struct LuaJobInfo
{
	LuaJobInfo() = default;

	// Function to be called in async environment
	std::string serializedFunction = "";
	// Parameter to be passed to function
	std::string serializedParams = "";
	// Result of function call
	std::string serializedResult = "";
	// JobID used to identify a job and match it to callback
	unsigned int id = 0;

	bool valid = false;
};

// Asynchronous working environment
class AsyncWorkerThread : public Thread, public ScriptApiBase {
public:
	AsyncWorkerThread(AsyncEngine* jobDispatcher, const std::string &name);
	virtual ~AsyncWorkerThread();

	void *run();

private:
	AsyncEngine *jobDispatcher = nullptr;
};

// Asynchornous thread and job management
class AsyncEngine {
	friend class AsyncWorkerThread;
	typedef void (*StateInitializer)(lua_State *L, int top);
public:
	AsyncEngine() = default;
	~AsyncEngine();

	/**
	 * Register function to be called on new states
	 * @param func C function to be called
	 */
	void registerStateInitializer(StateInitializer func);

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
	bool initDone = false;

	// Internal store for registred state initializers
	std::vector<StateInitializer> stateInitializers;

	// Internal counter to create job IDs
	unsigned int jobIdCounter = 0;

	// Mutex to protect job queue
	std::mutex jobQueueMutex;

	// Job queue
	std::deque<LuaJobInfo> jobQueue;

	// Mutex to protect result queue
	std::mutex resultQueueMutex;
	// Result queue
	std::deque<LuaJobInfo> resultQueue;

	// List of current worker threads
	std::vector<AsyncWorkerThread*> workerThreads;

	// Counter semaphore for job dispatching
	Semaphore jobQueueCounter;
};

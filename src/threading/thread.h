/*
This file is a part of the JThread package, which contains some object-
oriented thread wrappers for different thread implementations.

Copyright (c) 2000-2006  Jori Liesenborgs (jori.liesenborgs@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef THREADING_THREAD_H
#define THREADING_THREAD_H

// Determine which threading API we will use
#if __cplusplus >= 201103L
	#define USE_CPP11_THREADS 1
#elif defined(_WIN32)
	#define USE_WIN_THREADS 1
#else
	#define USE_POSIX_THREADS 1
#endif


#if USE_CPP11_THREADS
	#include <thread>
#endif
#ifdef _AIX
	#include <sys/thread.h>
#endif

#include "threading/atomic.h"
#include "threading/mutex.h"

#include <string>


#ifndef _WIN32
	#define THREAD_PRIORITY_LOWEST 0
	#define THREAD_PRIORITY_BELOW_NORMAL 1
	#define THREAD_PRIORITY_NORMAL 2
	#define THREAD_PRIORITY_ABOVE_NORMAL 3
	#define THREAD_PRIORITY_HIGHEST 4
#endif


class Thread
{
public:
	Thread(const std::string &name);
	virtual ~Thread() { kill(); }

	// Id and Handle
#if USE_CPP11_THREADS
	typedef std::thread::id Id;
	typedef std::thread::native_handle_type Handle;
#elif USE_WIN_THREADS
	typedef DWORD Id;
	typedef HANDLE Handle;
#elif USE_POSIX_THREADS
	typedef pthread_t Id;
	typedef pthread_t Handle;
#endif

	/** Begins execution of a new thread at the pure virtual method Thread::run().
	 * Execution of the thread is guaranteed to have started after this function
	 * returns.
	 */
	bool start();

	/** Requests that the thread exit gracefully.
	 * Returns in constant time.  The thread should poll stopRequested()
	 * often, but this is not guaranteed.  If you need a thread to stop
	 * immediately consider kill().
	 */
	void stop() { m_request_stop = true; }

	/** Returns whether this thread has been requested
	 * to stop through a call to stop().
	 */
	bool stopRequested() { return m_request_stop; }

	/** Returns whether this thread is currently running.
	 */
	bool isRunning() { return m_running; }

	/** Immediately terminates the thread.
	 * This should be used with extreme caution, as the thread will not have
	 * any opportunity to release resources it may be holding (such as memory
	 * or locks).
	 */
	bool kill();

	/** Wait for thread to finish.
	 * Note: this does not stop the thread.
	 * Returns immediately if the thread is not running.
	 */
	void wait();

	/** Gets the thread return value.
	 * Returns true if the thread has exited and the return value is available,
	 * or false if the thread has yet to finish.
	 */
	bool getReturnValue(void **ret);

	/** Binds (if possible, otherwise sets the affinity of) the thread to the
	 * specific processor specified by proc_num.
	 */
	bool bindToProcessor(unsigned int proc_num);

	/** Sets the thread's priority to the specified priority.
	 *
	 * @p prio can be one of: THREAD_PRIORITY_{LOWEST,BELOW_NORMAL,NORMAL,
	 * ABOVE_NORMAL,HIGHEST}.
	 * On Windows, any of the other priorites as defined by SetThreadPriority
	 * are supported as well.
	 *
	 * Note that it may be necessary to first set the threading policy or
	 * scheduling algorithm to one that supports thread priorities if not
	 * supported by default, otherwise this call will have no effect.
	 */
	bool setPriority(int prio);

	/** Sets the currently executing thread's name to where supported;
	 * useful for debugging and auditing processor usage.
	 */
	static void setNativeThreadName(const std::string &name);

	/** Returns the number of processors/cores/threads
	 * available on this machine.
	 */
	static unsigned int getNumberOfProcessors();

	/** Returns whether the thread that this object represents
	 * is the currently running thread.
	 */
	bool isCurrentThread() const
		{ return isCurrentThread(m_thread_id); }

	/** Returns whether the thread identified by the passed
	 * identifier is the currently running thread.
	 */
	static bool isCurrentThread(Id id)
		{ return compareThreadIds(getCurrentThreadId(), id); }

	/** Returns the identifier of the currently running thread.
	 */
	static Id getCurrentThreadId()
	{
#if USE_CPP11_THREADS
		return std::this_thread::get_id();
#elif USE_WIN_THREADS
		return GetCurrentThreadId();
#elif USE_POSIX_THREADS
		return pthread_self();
#endif
	}

protected:
	virtual void *run() = 0;

private:
	static bool compareThreadIds(Id id1, Id id2)
	{
#if USE_POSIX_THREADS
		return pthread_equal(id1, id2);
#else
		return id1 == id2;
#endif
	}

	std::string m_name;
	void *m_retval;
	Atomic<bool> m_request_stop;
	Atomic<bool> m_running;
	Mutex m_continue_mutex;
	Handle m_handle;
	Id m_thread_id;

	void cleanup();

#if USE_WIN_THREADS
	static DWORD WINAPI threadFunc(void *param);
#else
	static void *threadFunc(void *param);
#endif

#if _AIX
	// For AIX, there does not exist any mapping from pthread_t to tid_t
	// available to us, so we maintain one ourselves.  This is set on thread start.
	tid_t m_kernel_thread_id;
#endif

#if USE_CPP11_THREADS
	std::thread *m_thread;
#endif
};

#endif


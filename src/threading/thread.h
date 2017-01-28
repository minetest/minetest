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

#include "util/basic_macros.h"
#include "threading/atomic.h"
#include "threading/mutex.h"
#include "threads.h"

#include <string>
#ifdef _AIX
	#include <sys/thread.h> // for tid_t
#endif

/*
 * On platforms using pthreads, these five priority classes correlate to
 * even divisions between the minimum and maximum reported thread priority.
 */
#if !defined(_WIN32)
	#define THREAD_PRIORITY_LOWEST       0
	#define THREAD_PRIORITY_BELOW_NORMAL 1
	#define THREAD_PRIORITY_NORMAL       2
	#define THREAD_PRIORITY_ABOVE_NORMAL 3
	#define THREAD_PRIORITY_HIGHEST      4
#endif


class Thread {
public:
	Thread(const std::string &name="");
	virtual ~Thread();

	/*
	 * Begins execution of a new thread at the pure virtual method Thread::run().
	 * Execution of the thread is guaranteed to have started after this function
	 * returns.
	 */
	bool start();

	/*
	 * Requests that the thread exit gracefully.
	 * Returns immediately; thread execution is guaranteed to be complete after
	 * a subsequent call to Thread::wait.
	 */
	bool stop();

	/*
	 * Immediately terminates the thread.
	 * This should be used with extreme caution, as the thread will not have
	 * any opportunity to release resources it may be holding (such as memory
	 * or locks).
	 */
	bool kill();

	/*
	 * Waits for thread to finish.
	 * Note:  This does not stop a thread, you have to do this on your own.
	 * Returns false immediately if the thread is not started or has been waited
	 * on before.
	 */
	bool wait();

	/*
	 * Returns true if the calling thread is this Thread object.
	 */
	bool isCurrentThread() { return thr_is_current_thread(getThreadId()); }

	inline bool isRunning() { return m_running; }
	inline bool stopRequested() { return m_request_stop; }

#if USE_CPP11_THREADS
	inline threadid_t getThreadId() { return m_thread_obj->get_id(); }
	inline threadhandle_t getThreadHandle() { return m_thread_obj->native_handle(); }
#else
#  if USE_WIN_THREADS
	inline threadid_t getThreadId() { return m_thread_id; }
#  else
	inline threadid_t getThreadId() { return m_thread_handle; }
#  endif
	inline threadhandle_t getThreadHandle() { return m_thread_handle; }
#endif

	/*
	 * Gets the thread return value.
	 * Returns true if the thread has exited and the return value was available,
	 * or false if the thread has yet to finish.
	 */
	bool getReturnValue(void **ret);

	/*
	 * Binds (if possible, otherwise sets the affinity of) the thread to the
	 * specific processor specified by proc_number.
	 */
	bool bindToProcessor(unsigned int proc_number);

	/*
	 * Sets the thread priority to the specified priority.
	 *
	 * prio can be one of: THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL,
	 * THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST.
	 * On Windows, any of the other priorites as defined by SetThreadPriority
	 * are supported as well.
	 *
	 * Note that it may be necessary to first set the threading policy or
	 * scheduling algorithm to one that supports thread priorities if not
	 * supported by default, otherwise this call will have no effect.
	 */
	bool setPriority(int prio);

	/*
	 * Sets the currently executing thread's name to where supported; useful
	 * for debugging.
	 */
	static void setName(const std::string &name);

	/*
	 * Returns the number of processors/cores configured and active on this machine.
	 */
	static unsigned int getNumberOfProcessors();

protected:
	std::string m_name;

	virtual void *run() = 0;

private:
	void *m_retval;
	bool m_joinable;
	Atomic<bool> m_request_stop;
	Atomic<bool> m_running;
	Mutex m_mutex;
	Mutex m_start_finished_mutex;

#if USE_CPP11_THREADS
	std::thread *m_thread_obj;
#else
	threadhandle_t m_thread_handle;
#   if USE_WIN_THREADS
        threadid_t m_thread_id;
#   endif
#endif

	static ThreadStartFunc threadProc;

#ifdef _AIX
	// For AIX, there does not exist any mapping from pthread_t to tid_t
	// available to us, so we maintain one ourselves.  This is set on thread start.
	tid_t m_kernel_thread_id;
#endif

	DISABLE_CLASS_COPY(Thread);
};

#endif


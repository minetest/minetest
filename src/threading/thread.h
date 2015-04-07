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

#include "threading/atomic.h"
#include "threading/mutex.h"

#include <string>
#if __cplusplus >= 201103L
	#include <thread>
#endif

#ifndef _WIN32
enum {
	THREAD_PRIORITY_LOWEST,
	THREAD_PRIORITY_BELOW_NORMAL,
	THREAD_PRIORITY_NORMAL,
	THREAD_PRIORITY_ABOVE_NORMAL,
	THREAD_PRIORITY_HIGHEST,
};
#endif


class Thread
{
public:
	Thread(const std::string &name="Unnamed");
	virtual ~Thread() { kill(); }

	bool start();
	inline void stop() { request_stop = true; }
	bool kill();

	inline bool isRunning() { return running; }
	inline bool stopRequested() { return request_stop; }
	void *getReturnValue() { return running ? NULL : retval; }
	bool isSameThread();

	static unsigned int getNumberOfProcessors();
	bool bindToProcessor(unsigned int);
	bool setPriority(int);

	/*
	 * Wait for thread to finish.
	 * Note: this does not stop a thread, you have to do this on your own.
	 * Returns immediately if the thread is not started.
	 */
	void wait();

	static void setName(const std::string &name);

protected:
	std::string name;

	virtual void *run() = 0;

private:
	void setName() { setName(name); }

	void *retval;
	Atomic<bool> request_stop;
	Atomic<bool> running;
	Mutex continue_mutex;

#if __cplusplus >= 201103L
	static void theThread(Thread *th);

	std::thread *thread;
	std::thread::native_handle_type getThreadHandle() const
		{ return thread->native_handle(); }
#else
# if defined(WIN32) || defined(_WIN32_WCE)
#  ifdef _WIN32_WCE
	DWORD thread_id;
	static DWORD WINAPI theThread(void *param);
#  else
	UINT thread_id;
	static UINT __stdcall theThread(void *param);
#  endif

	HANDLE thread;
	HANDLE getThreadHandle() const { return thread; }
# else // pthread
	static void *theThread(void *param);

	pthread_t thread;
	pthread_t getThreadHandle() const { return thread; }

	Atomic<bool> started;
# endif
#endif
};

#endif


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

#include "threading/thread.h"
#include "threading/mutex_auto_lock.h"
#include "log.h"

#if __cplusplus >= 201103L
	#include <chrono>
#else
	#define UNUSED(expr) do { (void)(expr); } while (0)
# ifdef _WIN32
#  ifndef _WIN32_WCE
	#include <process.h>
#  endif
# else
	#include <ctime>
	#include <cassert>
	#include <cstdlib>
	#include <sys/time.h>

	// For getNumberOfProcessors
	#include <unistd.h>
#  if defined(__FreeBSD__) || defined(__APPLE__)
	#include <sys/types.h>
	#include <sys/sysctl.h>
#  elif defined(_GNU_SOURCE)
	#include <sys/sysinfo.h>
#  endif
# endif
#endif


// For setName
#if defined(linux) || defined(__linux)
	#include <sys/prctl.h>
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
	#include <pthread_np.h>
#elif defined(_MSC_VER)
	struct THREADNAME_INFO {
		DWORD dwType; // Must be 0x1000
		LPCSTR szName; // Pointer to name (in user addr space)
		DWORD dwThreadID; // Thread ID (-1=caller thread)
		DWORD dwFlags; // Reserved for future use, must be zero
	};
#endif

// For bindToProcessor
#if __FreeBSD_version >= 702106
	typedef cpuset_t cpu_set_t;
#elif defined(__linux) || defined(linux)
	#include <sched.h>
#elif defined(__sun) || defined(sun)
	#include <sys/types.h>
	#include <sys/processor.h>
	#include <sys/procset.h>
#elif defined(_AIX)
	#include <sys/processor.h>
#elif defined(__APPLE__)
	#include <mach/mach_init.h>
	#include <mach/thread_act.h>
#endif


Thread::Thread(const std::string &name) :
	name(name),
	retval(NULL),
	request_stop(false),
	running(false)
#if __cplusplus >= 201103L
	, thread(NULL)
#elif !defined(_WIN32)
	, started(false)
#endif
{}


void Thread::wait()
{
#if __cplusplus >= 201103L
	if (!thread || !thread->joinable())
		return;
	thread->join();
#elif defined(_WIN32)
	if (!running)
		return;
	WaitForSingleObject(thread, INFINITE);
#else // pthread
	void *status;
	if (!started)
		return;
	int ret = pthread_join(thread, &status);
	assert(!ret);
	UNUSED(ret);
	started = false;
#endif
}


bool Thread::start()
{
	if (running)
		return false;
	request_stop = false;

#if __cplusplus >= 201103L
	MutexAutoLock l(continue_mutex);
	thread = new std::thread(theThread, this);
#elif defined(_WIN32)
	MutexAutoLock l(continue_mutex);
# ifdef _WIN32_WCE
	thread = CreateThread(NULL, 0, theThread, this, 0, &thread_id);
# else
	thread = (HANDLE)_beginthreadex(NULL, 0, theThread, this, 0, &thread_id);
# endif
	if (!thread)
		return false;
#else
	int status;

	MutexAutoLock l(continue_mutex);

	status = pthread_create(&thread, NULL, theThread, this);

	if (status)
		return false;
#endif

#if __cplusplus < 201103L
	// Wait until running
	while (!running) {
# ifdef _WIN32
		Sleep(1);
	}
# else
		struct timespec req, rem;
		req.tv_sec = 0;
		req.tv_nsec = 1000000;
		nanosleep(&req, &rem);
	}
	started = true;
# endif
#endif
	return true;
}


bool Thread::kill()
{
#ifdef _WIN32
	if (!running)
		return false;
	TerminateThread(getThreadHandle(), 0);
	CloseHandle(getThreadHandle());
#else
	if (!running) {
		wait();
		return false;
	}
# ifdef __ANDROID__
	pthread_kill(getThreadHandle(), SIGKILL);
# else
	pthread_cancel(getThreadHandle());
# endif
	wait();
#endif
#if __cplusplus >= 201103L
	delete thread;
#endif
	running = false;
	return true;
}


bool Thread::isSameThread()
{
#if __cplusplus >= 201103L
	return thread->get_id() == std::this_thread::get_id();
#elif defined(_WIN32)
	return GetCurrentThreadId() == thread_id;
#else
	return pthread_equal(pthread_self(), thread);
#endif
}


#if __cplusplus >= 201103L
void Thread::theThread(Thread *th)
#elif defined(_WIN32_WCE)
DWORD WINAPI Thread::theThread(void *param)
#elif defined(_WIN32)
UINT __stdcall Thread::theThread(void *param)
#else
void *Thread::theThread(void *param)
#endif
{
#if __cplusplus < 201103L
	Thread *th = static_cast<Thread *>(param);
#endif
	th->running = true;

	th->setName();
	log_register_thread(th->name);

	th->retval = th->run();

	log_deregister_thread();

	th->running = false;
#if __cplusplus < 201103L
# ifdef _WIN32
	CloseHandle(th->thread);
# endif
	return NULL;
#endif
}


void Thread::setName(const std::string &name)
{
#if defined(linux) || defined(__linux)
	/* It would be cleaner to do this with pthread_setname_np,
	 * which was added to glibc in version 2.12, but some major
	 * distributions are still runing 2.11 and previous versions.
	 */
	prctl(PR_SET_NAME, name.c_str());
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
	pthread_set_name_np(pthread_self(), name.c_str());
#elif defined(__NetBSD__)
	pthread_setname_np(pthread_self(), name.c_str());
#elif defined(__APPLE__)
	pthread_setname_np(name.c_str());
#elif defined(_MSC_VER)
	// Windows itself doesn't support thread names,
	// but the MSVC debugger does...
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name.c_str();
	info.dwThreadID = -1;
	info.dwFlags = 0;
	__try {
		RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (ULONG_PTR *)&info);
	} __except (EXCEPTION_CONTINUE_EXECUTION) {
	}
#elif defined(_WIN32) || defined(__GNU__)
	// These platforms are known to not support thread names.
	// Silently ignore the request.
#else
	#warning "Unrecognized platform, thread names will not be available."
#endif
}


unsigned int Thread::getNumberOfProcessors()
{
#if __cplusplus >= 201103L
	return std::thread::hardware_concurrency();
#elif defined(_SC_NPROCESSORS_ONLN)
	return sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(__FreeBSD__) || defined(__APPLE__)
	unsigned int len, count;
	len = sizeof(count);
	return sysctlbyname("hw.ncpu", &count, &len, NULL, 0);
#elif defined(_GNU_SOURCE)
	return get_nprocs();
#elif defined(_WIN32)
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
#elif defined(PTW32_VERSION) || defined(__hpux)
	return pthread_num_processors_np();
#else
	return 1;
#endif
}


bool Thread::bindToProcessor(unsigned int num)
{
#if defined(__ANDROID__)
	return false;
#elif defined(_WIN32)
	return SetThreadAffinityMask(getThreadHandle(), 1 << num);
#elif __FreeBSD_version >= 702106 || defined(__linux) || defined(linux)
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(num, &cpuset);
	return pthread_setaffinity_np(getThreadHandle(), sizeof(cpuset),
		&cpuset) == 0;
#elif defined(__sun) || defined(sun)
	return processor_bind(P_LWPID, MAKE_LWPID_PTHREAD(getThreadHandle()),
			num, NULL) == 0
#elif defined(_AIX)
	return bindprocessor(BINDTHREAD, (tid_t) getThreadHandle(), pnumber) == 0;
#elif defined(__hpux) || defined(hpux)
	pthread_spu_t answer;

	return pthread_processor_bind_np(PTHREAD_BIND_ADVISORY_NP,
			&answer, num, getThreadHandle()) == 0;
#elif defined(__APPLE__)
	struct thread_affinity_policy tapol;

	thread_port_t threadport = pthread_mach_thread_np(getThreadHandle());
	tapol.affinity_tag = num + 1;
	return thread_policy_set(threadport, THREAD_AFFINITY_POLICY,
			(thread_policy_t)&tapol,
			THREAD_AFFINITY_POLICY_COUNT) == KERN_SUCCESS;
#else
	return false;
#endif
}


bool Thread::setPriority(int prio)
{
#if defined(_WIN32)
	return SetThreadPriority(getThreadHandle(), prio);
#else
	struct sched_param sparam;
	int policy;

	if (pthread_getschedparam(getThreadHandle(), &policy, &sparam) != 0)
		return false;

	int min = sched_get_priority_min(policy);
	int max = sched_get_priority_max(policy);

	sparam.sched_priority = min + prio * (max - min) / THREAD_PRIORITY_HIGHEST;
	return pthread_setschedparam(getThreadHandle(), policy, &sparam) == 0;
#endif
}


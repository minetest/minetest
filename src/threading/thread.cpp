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
#include "porting.h"

#define UNUSED(expr) do { (void)(expr); } while (0)

#if USE_CPP11_THREADS
	#include <chrono>
	#include <system_error>
#elif USE_WIN_THREADS
	#ifndef _WIN32_WCE
		#include <process.h>
	#endif
#else
	#include <ctime>
	#include <cassert>
	#include <cstdlib>
	#include <sys/time.h>

	// For getNumberOfProcessors
	#include <unistd.h>
	#if defined(__FreeBSD__) || defined(__APPLE__)
		#include <sys/types.h>
		#include <sys/sysctl.h>
	#elif defined(_GNU_SOURCE)
		#include <sys/sysinfo.h>
	#endif
#endif


// For setName
#if defined(linux) || defined(__linux)
	#include <sys/prctl.h>
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
	#include <pthread_np.h>
#elif defined(_MSC_VER)
	struct THREADNAME_INFO {
		DWORD dwType;     // Must be 0x1000
		LPCSTR szName;    // Pointer to name (in user addr space)
		DWORD dwThreadID; // Thread ID (-1 means caller thread)
		DWORD dwFlags;    // Reserved for future use, must be zero
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
	#include <sys/thread.h>
#elif defined(__APPLE__)
	#include <mach/mach_init.h>
	#include <mach/thread_act.h>
#endif


Thread::Thread(const std::string &name) :
	m_name(name),
	m_retval(NULL),
	m_request_stop(false),
	m_running(false)
#if USE_CPP11_THREADS
	, m_thread(NULL)
#endif
#if _AIX
	, m_kernel_thread_id(-1)
#endif
{}


void Thread::wait()
{
	if (!m_running)
		return;
#if USE_CPP11_THREADS
	if (m_thread->joinable())
		m_thread->join();
#elif defined(_WIN32)
	int ret = WaitForSingleObject(m_handle, INFINITE);
	assert(ret == WAIT_OBJECT_O);
	UNUSED(ret);
#else  // pthread
	void *status;
	int ret = pthread_join(m_handle, &status);
	assert(!ret);
	UNUSED(ret);
#endif
}


bool Thread::start()
{
	if (m_running)
		return false;
	cleanup();
	m_retval = NULL;

	MutexAutoLock lock(m_continue_mutex);

#if USE_CPP11_THREADS
	try {
		m_thread    = new std::thread(threadFunc, this);
		m_thread_id = m_thread->get_id();
		m_handle    = m_thread->native_handle();
	} catch (const std::system_error &e) {
		return false;
	}
#elif defined(_WIN32)
	m_handle = CreateThread(NULL, 0, threadFunc, this, 0, &m_thread_id);
	if (!m_handle)
		return false;
#else
	if (pthread_create(&m_handle, NULL, threadFunc, this))
		return false;
	m_thread_id = m_handle;
#endif

#if !USE_CPP11_THREADS
	// Wait until running
	while (!m_running)
		sleep_ms(1);
#endif
	return true;
}


bool Thread::kill()
{
	if (!m_running)
		return false;
#ifdef _WIN32
	TerminateThread(m_handle, 0);
	CloseHandle(m_handle);
#else
# ifdef __ANDROID__
	pthread_kill(m_handle, SIGKILL);
# else
	pthread_cancel(m_handle);
# endif
	wait();
#endif
	cleanup();
	return true;
}


void Thread::cleanup()
{
#if USE_CPP11_THREADS
	delete m_thread;
	m_thread = NULL;
#elif USE_WIN_THREADS
	CloseHandle(m_handle);
	m_handle = NULL;
	m_thread_id = -1;
#endif
	m_running      = false;
	m_request_stop = false;
}


bool Thread::getReturnValue(void **ret)
{
	if (m_running)
		return false;
	*ret = m_retval;
	return true;
}


#if USE_CPP11_THREADS
void *Thread::threadFunc(Thread *param)
#elif USE_WIN_THREADS
DWORD WINAPI Thread::threadFunc(void *param)
#else  // pthreads
void *Thread::threadFunc(void *param)
#endif
{
	Thread *th = static_cast<Thread *>(param);
#ifdef _AIX
	th->m_kernel_thread_id = thread_self();
#endif
	th->m_running = true;

	setNativeThreadName(th->m_name);
	g_logger.registerThread(th->m_name);

	th->m_retval = th->run();

	g_logger.deregisterThread();

	th->cleanup();
	return NULL;
}


void Thread::setNativeThreadName(const std::string &name)
{
#if defined(linux) || defined(__linux)
	// It would be cleaner to do this with pthread_setname_np,
	// which was added to glibc in version 2.12, but some major
	// distributions are still runing 2.11 and previous versions.
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
		RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD),
				static_cast<ULONG_PTR *>(&info));
	} __except (EXCEPTION_CONTINUE_EXECUTION) {
	}
#elif USE_WIN_THREADS || defined(__GNU__)
	// These platforms are known to not support thread names.
	// Silently ignore the request.
#else
	#warning "Unrecognized platform, thread names will not be available."
#endif
}


unsigned int Thread::getNumberOfProcessors()
{
#if USE_CPP11_THREADS
	return std::thread::hardware_concurrency();
#elif USE_WIN_THREADS
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
#elif defined(_SC_NPROCESSORS_ONLN)
	return sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
	defined(__DragonFly__) || defined(__APPLE__)
	unsigned int len, count;
	len = sizeof(count);
	return sysctlbyname("hw.ncpu", &count, &len, NULL, 0);
#elif defined(_GNU_SOURCE)
	return get_nprocs();
#elif defined(PTW32_VERSION) || defined(__hpux)
	return pthread_num_processors_np();
#else
	return 1;
#endif
}


bool Thread::bindToProcessor(unsigned int proc_num)
{
#if defined(__ANDROID__)
	return false;
#elif USE_WIN_THREADS
	return SetThreadAffinityMask(m_handle, 1 << proc_num);
#elif __FreeBSD_version >= 702106 || defined(__linux) || defined(linux)
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(proc_num, &cpuset);
	return pthread_setaffinity_np(m_handle, sizeof(cpuset),
		&cpuset) == 0;
#elif defined(__sun) || defined(sun)
	return processor_bind(P_LWPID, P_MYID, proc_num, NULL) == 0
#elif defined(_AIX)
	return bindprocessor(BINDTHREAD, m_kernel_thread_id, proc_num) == 0;
#elif defined(__hpux) || defined(hpux)
	pthread_spu_t answer;

	return pthread_processor_bind_np(PTHREAD_BIND_ADVISORY_NP,
			&answer, proc_num, m_handle) == 0;
#elif defined(__APPLE__)
	struct thread_affinity_policy tapol;

	thread_port_t thread_port = pthread_mach_thread_np(m_handle);
	tapol.affinity_tag = proc_num + 1;
	return thread_policy_set(thread_port, THREAD_AFFINITY_POLICY,
			(thread_policy_t)&tapol,
			THREAD_AFFINITY_POLICY_COUNT) == KERN_SUCCESS;
#else
	return false;
#endif
}


bool Thread::setPriority(int prio)
{
#if USE_WIN_THREADS
	return SetThreadPriority(m_handle, prio);
#else
	struct sched_param sparam;
	int policy;

	if (pthread_getschedparam(m_handle, &policy, &sparam) != 0)
		return false;

	int min = sched_get_priority_min(policy);
	int max = sched_get_priority_max(policy);

	sparam.sched_priority = min + prio * (max - min) / THREAD_PRIORITY_HIGHEST;
	return pthread_setschedparam(m_handle, policy, &sparam) == 0;
#endif
}


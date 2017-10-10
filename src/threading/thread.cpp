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
#elif USE_POSIX_THREADS
	#include <time.h>
	#include <assert.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <sys/time.h>

	#if defined(__FreeBSD__) || defined(__APPLE__)
		#include <sys/types.h>
		#include <sys/sysctl.h>
	#elif defined(_GNU_SOURCE)
		#include <sys/sysinfo.h>
	#endif
#endif


// for setName
#if defined(__linux__)
	#include <sys/prctl.h>
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
	#include <pthread_np.h>
#elif defined(_MSC_VER)
	struct THREADNAME_INFO {
		DWORD dwType;     // Must be 0x1000
		LPCSTR szName;    // Pointer to name (in user addr space)
		DWORD dwThreadID; // Thread ID (-1=caller thread)
		DWORD dwFlags;    // Reserved for future use, must be zero
	};
#endif

// for bindToProcessor
#if __FreeBSD_version >= 702106
	typedef cpuset_t cpu_set_t;
#elif defined(__linux__)
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
	m_joinable(false),
	m_request_stop(false),
	m_running(false)
{
#ifdef _AIX
	m_kernel_thread_id = -1;
#endif
}


Thread::~Thread()
{
	kill();

	// Make sure start finished mutex is unlocked before it's destroyed
	if (m_start_finished_mutex.try_lock())
		m_start_finished_mutex.unlock();

}


bool Thread::start()
{
	MutexAutoLock lock(m_mutex);

	if (m_running)
		return false;

	m_request_stop = false;

	// The mutex may already be locked if the thread is being restarted
	m_start_finished_mutex.try_lock();

#if USE_CPP11_THREADS

	try {
		m_thread_obj = new std::thread(threadProc, this);
	} catch (const std::system_error &e) {
		return false;
	}

#elif USE_WIN_THREADS

	m_thread_handle = CreateThread(NULL, 0, threadProc, this, 0, &m_thread_id);
	if (!m_thread_handle)
		return false;

#elif USE_POSIX_THREADS

	int status = pthread_create(&m_thread_handle, NULL, threadProc, this);
	if (status)
		return false;

#endif

	// Allow spawned thread to continue
	m_start_finished_mutex.unlock();

	while (!m_running)
		sleep_ms(1);

	m_joinable = true;

	return true;
}


bool Thread::stop()
{
	m_request_stop = true;
	return true;
}


bool Thread::wait()
{
	MutexAutoLock lock(m_mutex);

	if (!m_joinable)
		return false;

#if USE_CPP11_THREADS

	m_thread_obj->join();

	delete m_thread_obj;
	m_thread_obj = NULL;

#elif USE_WIN_THREADS

	int ret = WaitForSingleObject(m_thread_handle, INFINITE);
	assert(ret == WAIT_OBJECT_0);
	UNUSED(ret);

	CloseHandle(m_thread_handle);
	m_thread_handle = NULL;
	m_thread_id = -1;

#elif USE_POSIX_THREADS

	int ret = pthread_join(m_thread_handle, NULL);
	assert(ret == 0);
	UNUSED(ret);

#endif

	assert(m_running == false);
	m_joinable = false;
	return true;
}


bool Thread::kill()
{
	if (!m_running) {
		wait();
		return false;
	}

	m_running = false;

#if USE_WIN_THREADS
	TerminateThread(m_thread_handle, 0);
	CloseHandle(m_thread_handle);
#else
	// We need to pthread_kill instead on Android since NDKv5's pthread
	// implementation is incomplete.
# ifdef __ANDROID__
	pthread_kill(getThreadHandle(), SIGKILL);
# else
	pthread_cancel(getThreadHandle());
# endif
	wait();
#endif

	m_retval       = NULL;
	m_joinable     = false;
	m_request_stop = false;

	return true;
}


bool Thread::getReturnValue(void **ret)
{
	if (m_running)
		return false;

	*ret = m_retval;
	return true;
}


#if USE_CPP11_THREADS || USE_POSIX_THREADS
void *Thread::threadProc(void *param)
#elif defined(_WIN32_WCE)
DWORD Thread::threadProc(LPVOID param)
#elif defined(_WIN32)
DWORD WINAPI Thread::threadProc(LPVOID param)
#endif
{
	Thread *thr = (Thread *)param;

#ifdef _AIX
	thr->m_kernel_thread_id = thread_self();
#endif

	thr->setName(thr->m_name);

	g_logger.registerThread(thr->m_name);
	thr->m_running = true;

	// Wait for the thread that started this one to finish initializing the
	// thread handle so that getThreadId/getThreadHandle will work.
	thr->m_start_finished_mutex.lock();

	thr->m_retval = thr->run();

	thr->m_running = false;
	// Unlock m_start_finished_mutex to prevent data race condition on Windows.
	// On Windows with VS2017 build TerminateThread is called and this mutex is not
	// released. We try to unlock it from caller thread and it's refused by system.
	thr->m_start_finished_mutex.unlock();
	g_logger.deregisterThread();

	// 0 is returned here to avoid an unnecessary ifdef clause
	return 0;
}


void Thread::setName(const std::string &name)
{
#if defined(__linux__)

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
		RaiseException(0x406D1388, 0,
			sizeof(info) / sizeof(DWORD), (ULONG_PTR *)&info);
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

	unsigned int num_cpus = 1;
	size_t len = sizeof(num_cpus);

	int mib[2];
	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;

	sysctl(mib, 2, &num_cpus, &len, NULL, 0);
	return num_cpus;

#elif defined(_GNU_SOURCE)

	return get_nprocs();

#elif defined(PTW32_VERSION) || defined(__hpux)

	return pthread_num_processors_np();

#else

	return 1;

#endif
}


bool Thread::bindToProcessor(unsigned int proc_number)
{
#if defined(__ANDROID__)

	return false;

#elif USE_WIN_THREADS

	return SetThreadAffinityMask(getThreadHandle(), 1 << proc_number);

#elif __FreeBSD_version >= 702106 || defined(__linux__)

	cpu_set_t cpuset;

	CPU_ZERO(&cpuset);
	CPU_SET(proc_number, &cpuset);

	return pthread_setaffinity_np(getThreadHandle(), sizeof(cpuset), &cpuset) == 0;

#elif defined(__sun) || defined(sun)

	return processor_bind(P_LWPID, P_MYID, proc_number, NULL) == 0

#elif defined(_AIX)

	return bindprocessor(BINDTHREAD, m_kernel_thread_id, proc_number) == 0;

#elif defined(__hpux) || defined(hpux)

	pthread_spu_t answer;

	return pthread_processor_bind_np(PTHREAD_BIND_ADVISORY_NP,
			&answer, proc_number, getThreadHandle()) == 0;

#elif defined(__APPLE__)

	struct thread_affinity_policy tapol;

	thread_port_t threadport = pthread_mach_thread_np(getThreadHandle());
	tapol.affinity_tag = proc_number + 1;
	return thread_policy_set(threadport, THREAD_AFFINITY_POLICY,
			(thread_policy_t)&tapol,
			THREAD_AFFINITY_POLICY_COUNT) == KERN_SUCCESS;

#else

	return false;

#endif
}


bool Thread::setPriority(int prio)
{
#if USE_WIN_THREADS

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


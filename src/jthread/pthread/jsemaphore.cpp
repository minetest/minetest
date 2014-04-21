/*
Minetest
Copyright (C) 2013 sapier, < sapier AT gmx DOT net >

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
#include <assert.h>
#include <errno.h>
#include <sys/time.h>
#include "jthread/jsemaphore.h"
#ifdef __MACH__
#include <unistd.h>
#endif

#define UNUSED(expr) do { (void)(expr); } while (0)

#ifdef __MACH__
#undef sem_t
#undef sem_init
#undef sem_wait
#undef sem_post
#undef sem_destroy
#define sem_t             semaphore_t
#define sem_init(s, p, c) semaphore_create(mach_task_self(), (s), 0, (c))
#define sem_wait(s)       semaphore_wait(*(s))
#define sem_post(s)       semaphore_signal(*(s))
#define sem_destroy(s)    semaphore_destroy(mach_task_self(), *(s))
pthread_mutex_t semcount_mutex;
#endif

JSemaphore::JSemaphore() {
	int sem_init_retval = sem_init(&m_semaphore,0,0);
	assert(sem_init_retval == 0);
	UNUSED(sem_init_retval);
#ifdef __MACH__
	semcount = 0;
#endif
}

JSemaphore::~JSemaphore() {
	int sem_destroy_retval = sem_destroy(&m_semaphore);
#ifdef __ANDROID__
// WORKAROUND for broken bionic semaphore implementation!
	assert(
			(sem_destroy_retval == 0) ||
			(errno == EBUSY)
		);
#else
	assert(sem_destroy_retval == 0);
#endif
	UNUSED(sem_destroy_retval);
}

JSemaphore::JSemaphore(int initval) {
	int sem_init_retval = sem_init(&m_semaphore,0,initval);
	assert(sem_init_retval == 0);
	UNUSED(sem_init_retval);
}

void JSemaphore::Post() {
	int sem_post_retval = sem_post(&m_semaphore);
	assert(sem_post_retval == 0);
	UNUSED(sem_post_retval);
#ifdef __MACH__
	pthread_mutex_lock(&semcount_mutex);
	semcount++;
	pthread_mutex_unlock(&semcount_mutex);
#endif
}

void JSemaphore::Wait() {
	int sem_wait_retval = sem_wait(&m_semaphore);
	assert(sem_wait_retval == 0);
	UNUSED(sem_wait_retval);
#ifdef __MACH__
	pthread_mutex_lock(&semcount_mutex);
	semcount--;
	pthread_mutex_unlock(&semcount_mutex);
#endif
}

bool JSemaphore::Wait(unsigned int time_ms) {
#ifdef __MACH__
	mach_timespec_t waittime;
	waittime.tv_sec = time_ms / 1000;
	waittime.tv_nsec = 1000000 * (time_ms % 1000);
#else
	struct timespec waittime;
#endif
	struct timeval now;

	if (gettimeofday(&now, NULL) == -1) {
		assert("Unable to get time by clock_gettime!" == 0);
		return false;
	}

#ifndef __MACH__
	waittime.tv_nsec = ((time_ms % 1000) * 1000 * 1000) + (now.tv_usec * 1000);
	waittime.tv_sec  = (time_ms / 1000) + (waittime.tv_nsec / (1000*1000*1000)) + now.tv_sec;
	waittime.tv_nsec %= 1000*1000*1000;
#endif

	errno = 0;
#ifdef __MACH__
	int sem_wait_retval = semaphore_timedwait(m_semaphore, waittime);
#else
	int sem_wait_retval = sem_timedwait(&m_semaphore, &waittime);
#endif

	if (sem_wait_retval == 0)
	{
#ifdef __MACH__
		pthread_mutex_lock(&semcount_mutex);
		semcount--;
		pthread_mutex_unlock(&semcount_mutex);
#endif
		return true;
	}
	else {
		assert((errno == ETIMEDOUT) || (errno == EINTR));
		return false;
	}
	return sem_wait_retval == 0 ? true : false;
}

int JSemaphore::GetValue() {
	int retval = 0;
#ifdef __MACH__
	pthread_mutex_lock(&semcount_mutex);
	retval = semcount;
	pthread_mutex_unlock(&semcount_mutex);
#else
	sem_getvalue(&m_semaphore, &retval);
#endif
	return retval;
}


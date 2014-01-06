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

#define UNUSED(expr) do { (void)(expr); } while (0)

JSemaphore::JSemaphore() {
	int sem_init_retval = sem_init(&m_semaphore,0,0);
	assert(sem_init_retval == 0);
	UNUSED(sem_init_retval);
}

JSemaphore::~JSemaphore() {
	int sem_destroy_retval = sem_destroy(&m_semaphore);
	assert(sem_destroy_retval == 0);
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
}

void JSemaphore::Wait() {
	int sem_wait_retval = sem_wait(&m_semaphore);
	assert(sem_wait_retval == 0);
	UNUSED(sem_wait_retval);
}

bool JSemaphore::Wait(unsigned int time_ms) {
	struct timespec waittime;
	struct timeval now;

	if (gettimeofday(&now, NULL) == -1) {
		assert("Unable to get time by clock_gettime!" == 0);
		return false;
	}

	waittime.tv_nsec = ((time_ms % 1000) * 1000 * 1000) + (now.tv_usec * 1000);
	waittime.tv_sec  = (time_ms / 1000) + (waittime.tv_nsec / (1000*1000*1000)) + now.tv_sec;
	waittime.tv_nsec %= 1000*1000*1000;

	errno = 0;
	int sem_wait_retval = sem_timedwait(&m_semaphore,&waittime);

	if (sem_wait_retval == 0)
	{
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
	sem_getvalue(&m_semaphore,&retval);

	return retval;
}


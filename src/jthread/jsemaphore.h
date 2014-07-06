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

#ifndef JSEMAPHORE_H_
#define JSEMAPHORE_H_

#if defined(WIN32)
#include <windows.h>
#include <assert.h>
#define MAX_SEMAPHORE_COUNT 1024
#elif __MACH__
#include <pthread.h>
#include <mach/mach.h>
#include <mach/task.h>
#include <mach/semaphore.h>
#include <sys/semaphore.h>
#include <errno.h>
#include <time.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif

class JSemaphore {
public:
	JSemaphore();
	~JSemaphore();
	JSemaphore(int initval);

	void Post();
	void Wait();
	bool Wait(unsigned int time_ms);

	int GetValue();

private:
#if defined(WIN32)
	HANDLE m_hSemaphore;
#elif __MACH__
	semaphore_t m_semaphore;
	int semcount;
#else
	sem_t m_semaphore;
#endif
};


#endif /* JSEMAPHORE_H_ */

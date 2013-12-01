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
#include "jthread/jsemaphore.h"

JSemaphore::JSemaphore() {
	assert(sem_init(&m_semaphore,0,0) == 0);
}

JSemaphore::~JSemaphore() {
	assert(sem_destroy(&m_semaphore) == 0);
}

JSemaphore::JSemaphore(int initval) {
	assert(sem_init(&m_semaphore,0,initval) == 0);
}

void JSemaphore::Post() {
	assert(sem_post(&m_semaphore) == 0);
}

void JSemaphore::Wait() {
	assert(sem_wait(&m_semaphore) == 0);
}

int JSemaphore::GetValue() {

	int retval = 0;
	sem_getvalue(&m_semaphore,&retval);

	return retval;
}


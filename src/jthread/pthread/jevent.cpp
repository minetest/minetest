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
#include <assert.h>
#include "jthread/jevent.h"

#define UNUSED(expr) do { (void)(expr); } while (0)

#ifdef __MACH__
#undef sem_t
#define sem_t semaphore_t
#undef sem_init
#define sem_init(s, p, c) semaphore_create(mach_task_self(), (s), 0, (c))
#undef sem_wait
#define sem_wait(s) semaphore_wait(*(s))
#undef sem_post
#define sem_post(s) semaphore_signal(*(s))
#undef sem_destroy
#define sem_destroy(s) semaphore_destroy(mach_task_self(), *(s))
#endif

Event::Event() {
	int sem_init_retval = sem_init(&sem, 0, 0);
	assert(sem_init_retval == 0);
	UNUSED(sem_init_retval);
}

Event::~Event() {
	int sem_destroy_retval = sem_destroy(&sem);
	assert(sem_destroy_retval == 0);
	UNUSED(sem_destroy_retval);
}

void Event::wait() {
	int sem_wait_retval = sem_wait(&sem);
	assert(sem_wait_retval == 0);
	UNUSED(sem_wait_retval);
}

void Event::signal() {
	int sem_post_retval = sem_post(&sem);
	assert(sem_post_retval == 0);
	UNUSED(sem_post_retval);
}

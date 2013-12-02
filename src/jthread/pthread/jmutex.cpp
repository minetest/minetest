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
#include "jthread/jmutex.h"
#define UNUSED(expr) do { (void)(expr); } while (0)
JMutex::JMutex()
{
	int mutex_init_retval = pthread_mutex_init(&mutex,NULL);
	assert( mutex_init_retval == 0 );
	UNUSED(mutex_init_retval);
}

JMutex::~JMutex()
{
	int mutex_dextroy_retval = pthread_mutex_destroy(&mutex);
	assert( mutex_dextroy_retval == 0 );
	UNUSED(mutex_dextroy_retval);
}

int JMutex::Lock()
{
	int mutex_lock_retval = pthread_mutex_lock(&mutex);
	assert( mutex_lock_retval == 0 );
	return mutex_lock_retval;
	UNUSED(mutex_lock_retval);
}

int JMutex::Unlock()
{
	int mutex_unlock_retval = pthread_mutex_unlock(&mutex);
	assert( mutex_unlock_retval == 0 );
	return mutex_unlock_retval;
	UNUSED(mutex_unlock_retval);
}

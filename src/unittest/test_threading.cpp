/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "test.h"

#include "threading/atomic.h"
#include "threading/semaphore.h"
#include "threading/thread.h"


class TestThreading : public TestBase {
public:
	TestThreading() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestThreading"; }
	void runTests(IGameDef *);
	void testAtomicSemaphoreThread();
};

static TestThreading g_test_instance;

void TestThreading::runTests(IGameDef *)
{
	TEST(testAtomicSemaphoreThread);
}


class AtomicTestThread : public Thread
{
public:
	AtomicTestThread(Atomic<u32> &v, Semaphore &trigger) :
		Thread("AtomicTest"),
		val(v),
		trigger(trigger)
	{}
private:
	void *run()
	{
		trigger.wait();
		for (u32 i = 0; i < 0x10000; ++i)
			++val;
		return NULL;
	}
	Atomic<u32> &val;
	Semaphore &trigger;
};


void TestThreading::testAtomicSemaphoreThread()
{
	Atomic<u32> val;
	Semaphore trigger;
	static const u8 num_threads = 4;

	AtomicTestThread *threads[num_threads];
	for (u8 i = 0; i < num_threads; ++i) {
		threads[i] = new AtomicTestThread(val, trigger);
		UASSERT(threads[i]->start());
	}

	trigger.post(num_threads);

	for (u8 i = 0; i < num_threads; ++i) {
		threads[i]->wait();
		delete threads[i];
	}

	UASSERT(val == num_threads * 0x10000);
}


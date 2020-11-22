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

#include <atomic>
#include "threading/semaphore.h"
#include "threading/thread.h"


class TestThreading : public TestBase {
public:
	TestThreading() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestThreading"; }
	void runTests(IGameDef *gamedef);

	void testStartStopWait();
	void testAtomicSemaphoreThread();
};

static TestThreading g_test_instance;

void TestThreading::runTests(IGameDef *gamedef)
{
	TEST(testStartStopWait);
	TEST(testAtomicSemaphoreThread);
}

class SimpleTestThread : public Thread {
public:
	SimpleTestThread(unsigned int interval) :
		Thread("SimpleTest"),
		m_interval(interval)
	{
	}

private:
	void *run()
	{
		void *retval = this;

		if (isCurrentThread() == false)
			retval = (void *)0xBAD;

		while (!stopRequested())
			sleep_ms(m_interval);

		return retval;
	}

	unsigned int m_interval;
};

void TestThreading::testStartStopWait()
{
	void *thread_retval;
	SimpleTestThread *thread = new SimpleTestThread(25);

	// Try this a couple times, since a Thread should be reusable after waiting
	for (size_t i = 0; i != 5; i++) {
		// Can't wait() on a joined, stopped thread
		UASSERT(thread->wait() == false);

		// start() should work the first time, but not the second.
		UASSERT(thread->start() == true);
		UASSERT(thread->start() == false);

		UASSERT(thread->isRunning() == true);
		UASSERT(thread->isCurrentThread() == false);

		// Let it loop a few times...
		sleep_ms(70);

		// It's still running, so the return value shouldn't be available to us.
		UASSERT(thread->getReturnValue(&thread_retval) == false);

		// stop() should always succeed
		UASSERT(thread->stop() == true);

		// wait() only needs to wait the first time - the other two are no-ops.
		UASSERT(thread->wait() == true);
		UASSERT(thread->wait() == false);
		UASSERT(thread->wait() == false);

		// Now that the thread is stopped, we should be able to get the
		// return value, and it should be the object itself.
		thread_retval = NULL;
		UASSERT(thread->getReturnValue(&thread_retval) == true);
		UASSERT(thread_retval == thread);
	}

	delete thread;
}



class AtomicTestThread : public Thread {
public:
	AtomicTestThread(std::atomic<u32> &v, Semaphore &trigger) :
		Thread("AtomicTest"),
		val(v),
		trigger(trigger)
	{
	}

private:
	void *run()
	{
		trigger.wait();
		for (u32 i = 0; i < 0x10000; ++i)
			++val;
		return NULL;
	}

	std::atomic<u32> &val;
	Semaphore &trigger;
};


void TestThreading::testAtomicSemaphoreThread()
{
	std::atomic<u32> val;
	val = 0;
	Semaphore trigger;
	static const u8 num_threads = 4;

	AtomicTestThread *threads[num_threads];
	for (auto &thread : threads) {
		thread = new AtomicTestThread(val, trigger);
		UASSERT(thread->start());
	}

	trigger.post(num_threads);

	for (AtomicTestThread *thread : threads) {
		thread->wait();
		delete thread;
	}

	UASSERT(val == num_threads * 0x10000);
}

